
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

## 5.Xây dụng cấu trúc FreeRTOS
+ “Task truyền data”: kết nối Wifi chạy Core 0, nếu thành công, đọc dữ liệu từ 
hàng đợi “data Queue”, và gửi lên MQTT broker.   
+ “Task đọc data”, chạy Core 1: đọc dữ liệu cảm biến BMP180, hiển thị dữ liệu 
qua giao tiếp serial, gửi dữ liệu đầy đủ vào hàng đợi “data Queue” và dữ liệu 
cảnh báo gồm nhiệt độ + độ cao vào hàng đợi “data Alert”   
+ “Tast cảnh báo”, chạy Core 1: đọc dữ liệu tử hàng đợi “data Alert” , và thực 
hiện Cảnh báo bằng Còi/Đèn khi nhiệt độ người chạy (runner) quá ngưỡng hoặc 
dưới ngưỡng & Cảnh báo khi có sự thay đổi độ cao đột ngột. 

## 6. Sơ đồ mô phỏng
- LinK Wokwi: https://wokwi.com/projects/477953395558713345
- Truy cập Web mô phỏng: https://dovanquan2537-bot.github.io/DoVanQuan_23143234_lab3_Runner-Monitoring-System/

## 7. Tác giả
- Sinh viên: Đỗ Văn Quân
- Lớp: Kỹ sư Kỹ thuật thông tin và truyền thông
- Trường: Trường Đại học Giao thông Vận tải
