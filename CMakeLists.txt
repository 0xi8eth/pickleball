cmake_minimum_required(VERSION 3.10)
project(Pickleball)

set(CMAKE_CXX_STANDARD 17)

# Tìm thư viện OpenCV
find_package(OpenCV REQUIRED)

include_directories(include)

file(GLOB SOURCES "src/*.cpp")

add_executable(TennisSystem ${SOURCES})

target_link_libraries(TennisSystem ${OpenCV_LIBS})