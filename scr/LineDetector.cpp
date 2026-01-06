#include "LineDetector.h"
#include <iostream>
#include <cfloat>

// Global variables for mouse callback
static std::vector<CourtLine> g_lines;
static cv::Mat g_displayFrame;
static int g_selectedIndex = -1;
static bool g_lineSelected = false;
static double g_scaleFactor = 1.0; // Scale factor để convert từ resized frame về original frame

// Mouse callback function
static void onMouse(int event, int x, int y, int flags, void* userdata) {
    if (event == cv::EVENT_LBUTTONDOWN) {
        // Scale lại tọa độ click về frame gốc để so sánh với lines
        cv::Point2f pt(x / g_scaleFactor, y / g_scaleFactor);
        
        // Tìm line gần điểm click nhất
        float minDist = FLT_MAX;
        int closestIdx = -1;
        
        for (size_t i = 0; i < g_lines.size(); ++i) {
            const auto& line = g_lines[i];
            // Tính khoảng cách từ điểm đến line segment (với tọa độ gốc)
            cv::Point2f v = line.pt2 - line.pt1;
            cv::Point2f w = pt - line.pt1;
            
            float c1 = v.dot(w);
            if (c1 <= 0) {
                // Gần điểm pt1
                float dist = cv::norm(pt - line.pt1);
                if (dist < minDist) {
                    minDist = dist;
                    closestIdx = i;
                }
            } else {
                float c2 = v.dot(v);
                if (c2 <= c1) {
                    // Gần điểm pt2
                    float dist = cv::norm(pt - line.pt2);
                    if (dist < minDist) {
                        minDist = dist;
                        closestIdx = i;
                    }
                } else {
                    // Gần đoạn thẳng
                    float b = c1 / c2;
                    cv::Point2f pb = line.pt1 + b * v;
                    float dist = cv::norm(pt - pb);
                    if (dist < minDist) {
                        minDist = dist;
                        closestIdx = i;
                    }
                }
            }
        }
        
        // Nếu click gần một line (trong vòng 30 pixels của frame gốc)
        if (closestIdx >= 0 && minDist < 30.0f) {
            g_selectedIndex = closestIdx;
            g_lineSelected = true;
            
            // Vẽ lại frame với line được chọn highlight
            cv::Mat frameCopy = g_displayFrame.clone();
            for (size_t i = 0; i < g_lines.size(); ++i) {
                const auto& line = g_lines[i];
                // Scale tọa độ line về resized frame
                cv::Point2f pt1Scaled = line.pt1 * g_scaleFactor;
                cv::Point2f pt2Scaled = line.pt2 * g_scaleFactor;
                cv::Scalar color = (i == g_selectedIndex) ? cv::Scalar(0, 255, 0) : cv::Scalar(0, 0, 255);
                int thickness = (i == g_selectedIndex) ? 4 : 2;
                cv::line(frameCopy, pt1Scaled, pt2Scaled, color, thickness);
                cv::putText(frameCopy, std::to_string(i), 
                           (pt1Scaled + pt2Scaled) * 0.5f, 
                           cv::FONT_HERSHEY_SIMPLEX, 0.6, color, 2);
            }
            cv::putText(frameCopy, "Selected! Press SPACE to confirm or click another line", 
                       cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 0), 2);
            cv::imshow("Select Line", frameCopy);
        }
    }
}

std::vector<CourtLine> LineDetector::detect(const cv::Mat& frame) {
    std::vector<CourtLine> lines;
    cv::Mat hsv, mask;
    cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);
    
    // White color range
    cv::Scalar lower(0, 0, 130);
    cv::Scalar upper(180, 80, 255);
    cv::inRange(hsv, lower, upper, mask);
    
    // Morph ops
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(7,7));
    cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, kernel);
    cv::morphologyEx(mask, mask, cv::MORPH_OPEN, kernel);
    
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    
    for (const auto& cnt : contours) {
        double perimeter = cv::arcLength(cnt, true);
        std::vector<cv::Point> approx;
        cv::approxPolyDP(cnt, approx, 0.01 * perimeter, true);
        
        for (size_t i = 0; i < approx.size(); ++i) {
            cv::Point pt1 = approx[i];
            cv::Point pt2 = approx[(i + 1) % approx.size()];
            
            float length = (float)cv::norm(pt1 - pt2);
            if (length > 200) { // Min length filter
                lines.push_back({cv::Point2f(pt1), cv::Point2f(pt2), length});
            }
        }
    }
    return lines;
}

bool LineDetector::getMainLine(const cv::Mat& frame, CourtLine& outLine) {
    auto lines = detect(frame);
    if (lines.empty()) return false;
    
    // Hiển thị các line để user chọn
    g_lines = lines;
    g_selectedIndex = -1;
    g_lineSelected = false;
    
    // Tính toán kích thước window vừa vặn với màn hình (max 1280x720)
    int maxWidth = 1280;
    int maxHeight = 720;
    int frameWidth = frame.cols;
    int frameHeight = frame.rows;
    
    // Tính scale factor để fit vào max size
    double scaleW = (double)maxWidth / frameWidth;
    double scaleH = (double)maxHeight / frameHeight;
    double scale = std::min(scaleW, scaleH);
    
    // Nếu frame nhỏ hơn max size thì không cần scale
    if (scale > 1.0) scale = 1.0;
    
    int displayWidth = (int)(frameWidth * scale);
    int displayHeight = (int)(frameHeight * scale);
    
    // Resize frame nếu cần
    cv::Mat resizedFrame;
    if (scale < 1.0) {
        cv::resize(frame, resizedFrame, cv::Size(displayWidth, displayHeight));
    } else {
        resizedFrame = frame.clone();
    }
    
    // Lưu scale factor để dùng trong mouse callback
    g_scaleFactor = scale;
    
    // Vẽ tất cả các line lên resizedFrame với tọa độ đã scale
    for (size_t i = 0; i < lines.size(); ++i) {
        const auto& line = lines[i];
        cv::Point2f pt1Scaled = line.pt1 * scale;
        cv::Point2f pt2Scaled = line.pt2 * scale;
        cv::line(resizedFrame, pt1Scaled, pt2Scaled, cv::Scalar(0, 0, 255), 2);
        cv::putText(resizedFrame, std::to_string(i), 
                   (pt1Scaled + pt2Scaled) * 0.5f, 
                   cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 0, 255), 2);
    }
    cv::putText(resizedFrame, "Click on a line to select, then press SPACE to confirm", 
               cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);
    
    // Cập nhật g_displayFrame với frame đã resize để mouse callback hoạt động đúng
    g_displayFrame = resizedFrame.clone();
    
    // Tạo window và set mouse callback
    cv::namedWindow("Select Line", cv::WINDOW_NORMAL);
    cv::resizeWindow("Select Line", displayWidth, displayHeight);
    cv::setMouseCallback("Select Line", onMouse, nullptr);
    cv::imshow("Select Line", resizedFrame);
    
    // Chờ user chọn line
    std::cout << "Found " << lines.size() << " lines. Please select one by clicking on it." << std::endl;
    while (true) {
        int key = cv::waitKey(30) & 0xFF;
        
        if (key == 27) { // ESC - cancel
            cv::destroyWindow("Select Line");
            return false;
        }
        
        if (key == 32 && g_lineSelected) { // SPACE - confirm selection
            outLine = g_lines[g_selectedIndex];
            cv::destroyWindow("Select Line");
            std::cout << "Line " << g_selectedIndex << " selected: " 
                      << outLine.pt1 << " -> " << outLine.pt2 << std::endl;
            return true;
        }
        
        if (g_lineSelected && key == 32) {
            std::cout << "Please click on a line first!" << std::endl;
        }
    }
}