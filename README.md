
# RUNNER MONITORING SYSTEM

## 1. Giới thiệu dự án
Dự án xây dựng hệ thống giám sát và cảnh báo người chạy sử dụng ESP32, cảm biến BMP180, giao thức MQTT và hệ điều hành thời gian thực FreeRTOS.

## 2. Chức năng chính
- Đo nhiệt độ môi trường.
- Đo áp suất khí quyển.
- Tính toán độ cao.
- Hiển thị dữ liệu trên Serial Monitor.
- Truyền dữ liệu lên MQTT Broker.
- Hiển thị dữ liệu trên giao diện Web.
- Cảnh báo khi nhiệt độ vượt ngưỡng.
- Cảnh báo khi độ cao thay đổi đột ngột.
- Điều khiển LED và Buzzer từ Web.

## 3. Linh kiện sử dụng
- ESP32
- Cảm biến BMP180
- LED
- Buzzer

## 4. Công nghệ sử dụng
- Arduino IDE
- FreeRTOS
- MQTT
- EMQX Broker
- HTML, CSS, JavaScript
- Wokwi Simulator

## 5. Phân chia nhiệm vụ FreeRTOS
- Task truyền data: Core 0.
- Task đọc data: Core 1.
- Task cảnh báo: Core 1.
- dataQueue: truyền dữ liệu cảm biến.
- alertQueue: truyền dữ liệu cảnh báo.

## 6. Sơ đồ mô phỏng
Dự án được thiết kế và mô phỏng trên Wokwi.

## 7. Tác giả
- Sinh viên: Đỗ Văn Quân
- Lớp: Kỹ sư Kỹ thuật thông tin và truyền thông
- Trường: Trường Đại học Giao thông Vận tải
