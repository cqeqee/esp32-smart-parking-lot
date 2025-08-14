# Project năm 3 hệ thống bãi giữ xe thông minh

## Mô tả
Hệ thống tự động hóa ra/vào bãi đỗ, kết hợp nhận diện biển số, thẻ RFID và cảm biến để quản lý vị trí đỗ và điều khiển cổng. Nhận diện biển số bàng cách: chụp ảnh bằng ESP32-CAM, phát hiện vị trí biển bằng YOLOv5, sau đó cắt ảnh biển, nhận dạng ký tự bằng YOLOv5 OCR, cuối cùng là đối chiếu kết quả với danh sách đăng ký. Dữ liệu đồng bộ lên nền tảng Blynk.

## Tính năng chính
1. Mở cổng tự động bằng nhận diện biển số hoặc thẻ RFID.
2. Quản lý đăng ký và xóa biển số, thẻ RFID qua giao diện.
3. Hiển thị trạng thái chỗ trống trên LCD và trên ứng dụng IoT.
4. Lưu dữ liệu đăng ký trên EEPROM để giữ khi mất điện.

## Phần cứng
1. ESP32 ở cổng gắn với LCD1602, RFID MFRC522 và 2 Servo SG90.
2. ESP32 ở bên trong gắn với 4 cặp cảm biến hồng ngoại, mỗi vị trí 1 cảm biến trước mặt, 1 cảm biến phía trên.
3. ESP32-CAM để lấy biển số xe.
