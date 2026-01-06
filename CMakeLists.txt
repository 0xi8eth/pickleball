cmake_minimum_required(VERSION 3.10)
project(TennisSystem)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Tìm OpenCV
find_package(OpenCV REQUIRED)

# Include directories
# Thêm thư mục 'include' nội bộ của project bên cạnh OpenCV
include_directories(
    ${OpenCV_INCLUDE_DIRS}
    include
)

# Source files
# Liệt kê các file .cpp nằm trong thư mục src/
set(SOURCES
    src/main.cpp
    src/Utils.cpp
    src/KalmanFilter.cpp
    src/BallTracker.cpp
    src/YoloDetector.cpp
    src/LineDetector.cpp
)

# Header files
# Liệt kê các file .h nằm trong thư mục include/
set(HEADERS
    include/Config.h
    include/Utils.h
    include/KalmanFilter.h
    include/BallTracker.h
    include/YoloDetector.h
    include/LineDetector.h
)

# Tạo executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Link OpenCV libraries
target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS})

# Output message
message(STATUS "OpenCV version: ${OpenCV_VERSION}")
message(STATUS "OpenCV libraries: ${OpenCV_LIBS}")