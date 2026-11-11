
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_BMP085.h>
#include <math.h>

// ================= KHAI BAO CHAN KET NOI =================
#define BMP_SCL 32
#define BMP_SDA 33
#define LED_PIN 16
#define BUZZER_PIN 4

// ================= CAM BIEN BMP180 =================
Adafruit_BMP085 bmp;

// ================= CAU HINH WI-FI =================
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// ================= CAU HINH MQTT =================
const char* mqtt_server = "broker.emqx.io";
const int mqtt_port = 1883;

// Cac Topic MQTT
const char* topic_pub_temp =
    "lab_runner_esp32_231432324/BMP180/nhietdo";

const char* topic_pub_pressure =
    "lab_runner_esp32_231432324/BMP180/apsuat";

const char* topic_pub_altitude =
    "lab_runner_esp32_231432324/BMP180/doco";

const char* topic_sub_led =
    "lab_runner_esp32_231432324/device/led";

const char* topic_sub_buzzer =
    "lab_runner_esp32_231432324/device/buzzer";

// ================= KHOI TAO WI-FI VA MQTT =================
WiFiClient espClient;
PubSubClient client(espClient);

// ================= NGUONG CANH BAO =================
const float TEMP_MIN = 10.0;
const float TEMP_MAX = 35.0;
const float ALTITUDE_CHANGE_LIMIT = 5.0;

// Thoi gian duy tri canh bao do cao
const unsigned long ALTITUDE_ALERT_TIME = 3000;

// ================= CAU TRUC DU LIEU =================
struct SensorData {
    float temperature;
    float pressure;
    float altitude;
};

struct AlertData {
    float temperature;
    float altitude;
};

// ================= KHAI BAO QUEUE =================
QueueHandle_t dataQueue;
QueueHandle_t alertQueue;

// ================= TRANG THAI DIEU KHIEN =================
volatile bool manualLED = false;
volatile bool manualBuzzer = false;
volatile bool autoAlert = false;

// Bao ve cac bien dung chung giua cac Task
portMUX_TYPE stateMux = portMUX_INITIALIZER_UNLOCKED;

// ================= KHAI BAO TASK =================
TaskHandle_t TaskReadHandle = NULL;
TaskHandle_t TaskSendHandle = NULL;
TaskHandle_t TaskAlertHandle = NULL;


// =====================================================
// 1. XU LY LENH NHAN TU MQTT
// =====================================================
void callback(char* topic, byte* payload,
              unsigned int length) {

    String message = "";

    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }

    message.trim();
    message.toUpperCase();

    Serial.print("Nhan tin nhan tu topic [");
    Serial.print(topic);
    Serial.print("] - Noi dung: ");
    Serial.println(message);

    portENTER_CRITICAL(&stateMux);

    // Dieu khien LED
    if (String(topic) == topic_sub_led) {
        if (message == "ON") {
            manualLED = true;
            Serial.println("-> Da BAT LED");
        }
        else if (message == "OFF") {
            manualLED = false;
            Serial.println("-> Da TAT LED");
        }
    }

    // Dieu khien Buzzer
    if (String(topic) == topic_sub_buzzer) {
        if (message == "ON") {
            manualBuzzer = true;
            Serial.println("-> Da BAT Buzzer");
        }
        else if (message == "OFF") {
            manualBuzzer = false;
            Serial.println("-> Da TAT Buzzer");
        }
    }

    portEXIT_CRITICAL(&stateMux);
}


// =====================================================
// 2. KET NOI WI-FI
// =====================================================
void connectWiFi() {

    if (WiFi.status() == WL_CONNECTED) {
        return;
    }

    Serial.println("Dang ket noi Wi-Fi...");

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        vTaskDelay(pdMS_TO_TICKS(500));
        Serial.print(".");
    }

    Serial.println("\nWi-Fi da ket noi!");
    Serial.print("Dia chi IP: ");
    Serial.println(WiFi.localIP());
}


// =====================================================
// 3. KET NOI LAI MQTT KHI MAT KET NOI
// =====================================================
void reconnect() {

    while (!client.connected()) {

        Serial.print("Dang ket noi lai MQTT...");

        String clientId = "ESP32Runner-";
        clientId += String((uint32_t)ESP.getEfuseMac(), HEX);

        if (client.connect(clientId.c_str())) {

            Serial.println("Thanh cong!");

            client.subscribe(topic_sub_led);
            client.subscribe(topic_sub_buzzer);

            Serial.println("Da dang ky topic LED va Buzzer");
        }
        else {

            Serial.print("Loi rc = ");
            Serial.print(client.state());
            Serial.println(" - Thu lai sau 5 giay");

            vTaskDelay(pdMS_TO_TICKS(5000));
        }
    }
}


// =====================================================
// 4. TASK DOC DATA - CORE 1
// =====================================================
void TaskReadData(void *parameter) {

    Serial.println("Task doc data dang chay tren Core 1");

    for (;;) {

        SensorData data;

        // Doc du lieu tu BMP180
        data.temperature = bmp.readTemperature();
        data.pressure = bmp.readPressure() / 100.0;
        data.altitude = bmp.readAltitude(101325);

        // Hien thi du lieu ra Serial Monitor
        Serial.println("\n========== RUNNER MONITOR ==========");

        Serial.print("Nhiet do: ");
        Serial.print(data.temperature);
        Serial.println(" *C");

        Serial.print("Ap suat: ");
        Serial.print(data.pressure);
        Serial.println(" hPa");

        Serial.print("Do cao: ");
        Serial.print(data.altitude);
        Serial.println(" m");

        // Gui du lieu vao data Queue
        if (xQueueSend(dataQueue, &data, 0) == pdPASS) {
            Serial.println("Da dua du lieu vao data Queue");
        }
        else {
            Serial.println("data Queue day - Bo qua mau du lieu");
        }

        // Gui du lieu canh bao vao alert Queue
        AlertData alert;
        alert.temperature = data.temperature;
        alert.altitude = data.altitude;

        // Queue co do dai 1, luu mau canh bao moi nhat
        xQueueOverwrite(alertQueue, &alert);

        Serial.println("Da cap nhat alert Queue");

        // Chu ky doc cam bien 2 giay
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}


// =====================================================
// 5. TASK TRUYEN DATA - CORE 0
// =====================================================
void TaskSendData(void *parameter) {

    Serial.println("Task truyen data dang chay tren Core 0");

    SensorData data;

    // Cau hinh MQTT
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);

    for (;;) {

        // Kiem tra va ket noi Wi-Fi
        if (WiFi.status() != WL_CONNECTED) {
            connectWiFi();
        }

        // Kiem tra ket noi MQTT
        if (!client.connected()) {
            reconnect();
        }

        // Xu ly cac ban tin MQTT
        client.loop();

        // Nhan du lieu tu data Queue
        if (xQueueReceive(dataQueue, &data,
                          pdMS_TO_TICKS(100)) == pdPASS) {

            if (client.connected()) {

                char tempStr[16];
                char pressureStr[16];
                char altitudeStr[16];

                dtostrf(data.temperature, 1, 2, tempStr);
                dtostrf(data.pressure, 1, 2, pressureStr);
                dtostrf(data.altitude, 1, 2, altitudeStr);

                // Gui tung gia tri len MQTT Broker
                bool ok1 = client.publish(
                    topic_pub_temp, tempStr);

                bool ok2 = client.publish(
                    topic_pub_pressure, pressureStr);

                bool ok3 = client.publish(
                    topic_pub_altitude, altitudeStr);

                if (ok1 && ok2 && ok3) {
                    Serial.println(
                        "Da gui du lieu len MQTT Broker!");
                }
                else {
                    Serial.println(
                        "Gui du lieu MQTT that bai!");
                }
            }
        }

        // Duy tri xu ly MQTT
        client.loop();

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}


// =====================================================
// 6. TASK CANH BAO - CORE 1
// =====================================================
void TaskAlert(void *parameter) {

    Serial.println("Task canh bao dang chay tren Core 1");

    AlertData alert;

    float previousAltitude = 0;
    bool firstReading = true;

    bool alertTemp = false;
    bool alertAltitude = false;

    TickType_t altitudeAlertUntil = 0;

    for (;;) {

        // Nhan du lieu tu alert Queue
        if (xQueueReceive(alertQueue, &alert,
                          pdMS_TO_TICKS(100)) == pdPASS) {

            // Kiem tra nguong nhiet do
            alertTemp =
                (alert.temperature < TEMP_MIN ||
                 alert.temperature > TEMP_MAX);

            // Kiem tra do cao thay doi dot ngot
            if (!firstReading) {

                float deltaAltitude =
                    fabsf(alert.altitude - previousAltitude);

                Serial.print("Do cao thay doi: ");
                Serial.print(deltaAltitude);
                Serial.println(" m");

                if (deltaAltitude >= ALTITUDE_CHANGE_LIMIT) {

                    alertAltitude = true;

                    // Duy tri canh bao trong 3 giay
                    altitudeAlertUntil =
                        xTaskGetTickCount() +
                        pdMS_TO_TICKS(ALTITUDE_ALERT_TIME);
                }
            }

            previousAltitude = alert.altitude;
            firstReading = false;

            if (alertTemp) {
                Serial.println(
                    "CANH BAO: NHIET DO BAT THUONG!");
            }
        }

        // Kiem tra thoi gian ket thuc canh bao do cao
        if (alertAltitude &&
            (int32_t)(altitudeAlertUntil -
                      xTaskGetTickCount()) <= 0) {

            alertAltitude = false;
        }

        // Tong hop trang thai canh bao tu dong
        bool currentAlert = alertTemp || alertAltitude;

        // Doc trang thai dieu khien thu cong
        bool ledOutput;
        bool buzzerOutput;

        portENTER_CRITICAL(&stateMux);

        autoAlert = currentAlert;

        // Canh bao tu dong duoc uu tien cao hon dieu khien thu cong
        ledOutput = manualLED || autoAlert;
        buzzerOutput = manualBuzzer || autoAlert;

        portEXIT_CRITICAL(&stateMux);

        // Dieu khien LED va Buzzer
        digitalWrite(LED_PIN, ledOutput ? HIGH : LOW);
        digitalWrite(BUZZER_PIN, buzzerOutput ? HIGH : LOW);

        if (alertAltitude) {
            Serial.println(
                "CANH BAO: DO CAO THAY DOI DOT NGOT!");
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}


// =====================================================
// 7. HAM SETUP
// =====================================================
void setup() {

    Serial.begin(115200);

    // Cai dat chan LED va Buzzer
    pinMode(LED_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);

    digitalWrite(LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);

    // Khoi tao giao tiep I2C
    Wire.begin(BMP_SDA, BMP_SCL);

    // Khoi tao BMP180
    if (!bmp.begin()) {

        Serial.println("Loi: Khong tim thay BMP180!");

        while (true) {
            digitalWrite(LED_PIN, HIGH);
            vTaskDelay(pdMS_TO_TICKS(200));

            digitalWrite(LED_PIN, LOW);
            vTaskDelay(pdMS_TO_TICKS(800));
        }
    }

    Serial.println("BMP180 da khoi tao thanh cong!");

    // Tao data Queue gom 5 phan tu
    dataQueue = xQueueCreate(5, sizeof(SensorData));

    // Tao alert Queue gom 1 phan tu
    alertQueue = xQueueCreate(1, sizeof(AlertData));

    if (dataQueue == NULL || alertQueue == NULL) {

        Serial.println("Loi: Khong the tao Queue!");

        while (true) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    Serial.println("Da tao thanh cong 2 Queue!");

    // Tao Task truyen data tren Core 0
    xTaskCreatePinnedToCore(
        TaskSendData,
        "Task truyen data",
        8192,
        NULL,
        2,
        &TaskSendHandle,
        0
    );

    // Tao Task doc data tren Core 1
    xTaskCreatePinnedToCore(
        TaskReadData,
        "Task doc data",
        4096,
        NULL,
        2,
        &TaskReadHandle,
        1
    );

    // Tao Task canh bao tren Core 1
    xTaskCreatePinnedToCore(
        TaskAlert,
        "Task canh bao",
        4096,
        NULL,
        3,
        &TaskAlertHandle,
        1
    );

    Serial.println("He thong giam sat Runner san sang!");
}


// =====================================================
// 8. HAM LOOP
// =====================================================
void loop() {

    // Cac cong viec chinh duoc thuc hien trong 3 Task
    vTaskDelay(pdMS_TO_TICKS(1000));
}
