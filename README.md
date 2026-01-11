# Hệ Thống Theo Dõi Bóng Pickleball

Hệ thống tự động phát hiện, theo dõi và phân tích bóng Pickleball trong video, bao gồm phát hiện điểm nảy và xác định bóng IN/OUT dựa trên đường biên sân.

##  Mô Tả

Dự án này sử dụng các kỹ thuật Computer Vision và Deep Learning để:
- **Phát hiện bóng**: Sử dụng mô hình YOLO (ONNX) để phát hiện bóng trong từng frame
- **Theo dõi bóng**: Theo dõi đa đối tượng với thuật toán Hungarian và Kalman Filter
- **Phát hiện đường biên**: Tự động phát hiện đường kẻ trắng trên sân
- **Phát hiện điểm nảy**: Phân tích thay đổi góc quỹ đạo để phát hiện khi bóng chạm đất
- **Xác định IN/OUT**: Sử dụng geometry để xác định bóng nảy bên trong hay bên ngoài đường biên

##  Yêu Cầu Hệ Thống

### Phần Mềm
- **CMake** (version 3.10 trở lên)
- **C++ Compiler** hỗ trợ C++17 (GCC, Clang, hoặc MSVC)
- **OpenCV** (version 4.x được khuyến nghị)

### Phần Cứng
- CPU: Bất kỳ (GPU tùy chọn để tăng tốc YOLO)
- RAM: Tối thiểu 4GB
- Dung lượng: ~500MB cho model và dependencies

##  Cài Đặt

### 1. Cài Đặt OpenCV


#### Linux (Ubuntu/Debian)
```bash
cd ~
# Tải OpenCV Core
git clone https://github.com/opencv/opencv.git
cd opencv
git checkout 4.12.0
cd ..

# Tải OpenCV Contrib (Các module mở rộng)
git clone https://github.com/opencv/opencv_contrib.git
cd opencv_contrib
git checkout 4.12.0
cd ..
```



### 2. Clone Repository
```bash
git clone [<repository-url>](https://github.com/0xi8eth/pickleball.git)
cd pickleball
```

### 3. Build Project

```bash
# Tạo thư mục build
mkdir build
cd build

# Cấu hình với CMake
cmake ..

# Build
cmake --build . --config Release

# Hoặc trên Linux/macOS
make -j4
```

### 4. Chuẩn Bị Dữ Liệu

Đảm bảo bạn có các file sau trong thư mục `data/`:
- `In.mp4`: Video input cần xử lý
- `model_ver2.onnx`: Mô hình YOLO đã được huấn luyện

## 🚀 Sử Dụng

### Chạy Chương Trình

```bash
# Từ thư mục build
./Pickleball


```

### Quy Trình Xử Lý

1. **Khởi tạo**: Chương trình đọc video từ `data/In.mp4`
2. **Chọn đường biên**: Ở frame đầu tiên, hệ thống sẽ:
   - Phát hiện tất cả đường kẻ trắng
   - Tự động chọn đường dài nhất làm đường biên chính
   - Định nghĩa điểm OUT tham chiếu (góc phải màn hình)
3. **Xử lý video**: 
   - Phát hiện bóng trong từng frame
   - Theo dõi quỹ đạo bóng
   - Phát hiện điểm nảy
   - Xác định IN/OUT
4. **Kết quả**: Video output được lưu tại `Out.mp4`

### Cấu Hình

Bạn có thể chỉnh sửa các tham số trong `include/Config.h`:

```cpp
namespace Config {
    const std::string SOURCE_VIDEO_PATH = "data/In.mp4";
    const std::string TARGET_VIDEO_PATH = "Out.mp4";
    const std::string MODEL_PATH = "data/model_ver2.onnx";
    
    // YOLO Detection Thresholds
    const float CONF_THRESHOLD = 0.2f;      // Confidence threshold
    const float SCORE_THRESHOLD = 0.4f;     // Score threshold
    const float NMS_THRESHOLD = 0.4f;       // Non-Maximum Suppression
    
    // Kalman Filter
    const float PROCESS_NOISE = 0.5f;
    const float MEASUREMENT_NOISE = 5.0f;
}
```

## 📁 Cấu Trúc Dự Án

```
pickleball/
├── CMakeLists.txt          # File cấu hình CMake
├── data/                   # Thư mục chứa dữ liệu
│   ├── In.mp4             # Video input
│   └── model_ver2.onnx    # Mô hình YOLO
├── include/                # Header files
│   ├── BallTracker.h      # Theo dõi bóng đa đối tượng
│   ├── Config.h           # Cấu hình hệ thống
│   ├── KalmanFilter.h     # Bộ lọc Kalman
│   ├── LineDetector.h     # Phát hiện đường biên
│   ├── Utils.h            # Các hàm tiện ích
│   └── YoloDetector.h     # Phát hiện bóng bằng YOLO
└── scr/                    # Source files
    ├── main.cpp           # Entry point
    ├── BallTracker.cpp
    ├── KalmanFilter.cpp
    ├── LineDetector.cpp
    ├── Utils.cpp
    └── YoloDetector.cpp
```

## 🎯 Tính Năng Chính

### 1. Phát Hiện Bóng (YOLO)
- Sử dụng mô hình YOLO ONNX để phát hiện bóng
- Hỗ trợ phát hiện đa đối tượng
- Non-Maximum Suppression để loại bỏ detection trùng lặp

### 2. Theo Dõi Bóng (BallTracker)
- **Lọc nhiễu thông minh**:
  - Lọc bóng đen/xám (shadow)
  - Lọc đối tượng đứng yên
  - Lọc theo độ sáng
- **Hungarian Algorithm**: Ghép nối detection với tracking object
- **Quản lý vòng đời**: Tự động tạo/xóa tracking object
- **Xác định bóng chính**: Scoring system dựa trên quãng đường và độ sáng

### 3. Phát Hiện Đường Biên (LineDetector)
- Phát hiện đường kẻ trắng bằng HSV color space
- Morphological operations để làm sạch mask
- Tự động chọn đường dài nhất làm đường biên chính

### 4. Phát Hiện Điểm Nảy
- Phân tích thay đổi góc quỹ đạo (3 điểm liên tiếp)
- Tính toán giao điểm chính xác của quỹ đạo xuống và lên
- Hiển thị "BOUNCE DETECTED" khi phát hiện

### 5. Xác Định IN/OUT
- Sử dụng cross product để xác định vị trí điểm so với đường thẳng
- So sánh với điểm OUT tham chiếu
- Hiển thị kết quả "IN" hoặc "OUT" trên video

## 🔧 Công Nghệ Sử Dụng

- **C++17**: Ngôn ngữ lập trình chính
- **OpenCV**: Xử lý hình ảnh và video
- **ONNX Runtime**: Chạy mô hình YOLO
- **CMake**: Build system
- **YOLO**: Mô hình deep learning phát hiện đối tượng

## 📊 Thuật Toán

- **Hungarian Algorithm (Lite)**: Matching detection với tracking object
- **Kalman Filter**: Làm mượt quỹ đạo và dự đoán vị trí
- **Line Intersection**: Tính giao điểm 2 đường thẳng
- **Cross Product**: Xác định vị trí điểm so với đường thẳng
- **Angle Computation**: Tính góc giữa 3 điểm để phát hiện bounce

## 🎨 Visualization

Video output bao gồm:
- **Đường biên**: Màu xanh dương (blue)
- **Điểm OUT tham chiếu**: Màu đỏ (red)
- **Quỹ đạo bóng**: Các điểm xanh lá (green)
- **Bounding box bóng chính**: Màu trắng (white)
- **Điểm bounce**: Màu đỏ với viền rõ ràng
- **Text IN/OUT**: Màu vàng ở góc phải màn hình

## ⚠️ Lưu Ý

- Đảm bảo video input có chất lượng tốt và đường biên rõ ràng
- Mô hình YOLO cần được huấn luyện phù hợp với dữ liệu của bạn
- Điểm OUT tham chiếu mặc định là góc phải màn hình, có thể cần điều chỉnh tùy góc quay camera
- Hệ thống hoạt động tốt nhất với video có độ phân giải 720p trở lên

## 🐛 Xử Lý Lỗi

### Lỗi: "Cannot open video"
- Kiểm tra đường dẫn file video trong `Config.h`
- Đảm bảo file `data/In.mp4` tồn tại

### Lỗi: "No court line found!"
- Kiểm tra video có đường kẻ trắng rõ ràng không
- Điều chỉnh ngưỡng màu trong `LineDetector.cpp` nếu cần

### Lỗi: OpenCV not found
- Đảm bảo OpenCV đã được cài đặt đúng cách
- Kiểm tra biến môi trường và CMake paths



**Lưu ý**: Đây là phiên bản demo. Một số tính năng có thể cần tinh chỉnh cho môi trường sản xuất.

