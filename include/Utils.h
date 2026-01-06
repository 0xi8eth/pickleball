#ifndef UTILS_H
#define UTILS_H

#include <opencv2/opencv.hpp>
#include <vector>
#include <cmath>

namespace Utils {
    // Tính giao điểm 2 đoạn thẳng (p0-p1) và (p2-p3)
    bool lineIntersection(cv::Point2f p0, cv::Point2f p1, cv::Point2f p2, cv::Point2f p3, cv::Point2f& r_intersection);
    
    // Tính góc giữa 3 điểm (p0 -> p1 -> p2)
    float computeAngle(cv::Point2f p0, cv::Point2f p1, cv::Point2f p2);
    
    // Kiểm tra IN/OUT dựa trên tích có hướng (Cross Product)
    // side_value > 0 hoặc < 0 tùy phía
    float sideValue(cv::Point2f linePt1, cv::Point2f linePt2, cv::Point2f pt);
    
    std::string checkOutIn(cv::Point2f linePt1, cv::Point2f linePt2, cv::Point2f outPoint, cv::Point2f checkPoint);
}

#endif