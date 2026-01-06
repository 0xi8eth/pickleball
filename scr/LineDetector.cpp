#include "LineDetector.h"

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
    
    // Lấy line dài nhất
    auto max_it = std::max_element(lines.begin(), lines.end(), 
        [](const CourtLine& a, const CourtLine& b){ return a.length < b.length; });
    
    outLine = *max_it;
    return true;
}