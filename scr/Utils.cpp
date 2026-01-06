#include "Utils.h"

namespace Utils {

bool lineIntersection(cv::Point2f p0, cv::Point2f p1, cv::Point2f p2, cv::Point2f p3, cv::Point2f& r_intersection) {
    float s1_x, s1_y, s2_x, s2_y;
    s1_x = p1.x - p0.x;     s1_y = p1.y - p0.y;
    s2_x = p3.x - p2.x;     s2_y = p3.y - p2.y;

    float s, t;
    float denom = -s2_x * s1_y + s1_x * s2_y;
    if (fabs(denom) < 1e-6) return false; // Song song

    s = (-s1_y * (p0.x - p2.x) + s1_x * (p0.y - p2.y)) / denom;
    t = ( s2_x * (p0.y - p2.y) - s2_y * (p0.x - p2.x)) / denom;

    if (s >= 0 && s <= 1 && t >= 0 && t <= 1) {
        // Intersection detected within segments
        r_intersection.x = p0.x + (t * s1_x);
        r_intersection.y = p0.y + (t * s1_y);
        return true;
    }
    
    // Tính toán giao điểm của đường thẳng (line) chứ không chỉ đoạn thẳng (segment)
    // Logic Python dùng công thức tổng quát
    r_intersection.x = p0.x + (t * s1_x);
    r_intersection.y = p0.y + (t * s1_y);
    return true; 
}

float computeAngle(cv::Point2f p0, cv::Point2f p1, cv::Point2f p2) {
    // Vector p1->p0
    cv::Point2f v1 = p0 - p1;
    // Vector p1->p2
    cv::Point2f v2 = p2 - p1;
    
    float dot = v1.x * v2.x + v1.y * v2.y;
    float mag1 = sqrt(v1.x*v1.x + v1.y*v1.y);
    float mag2 = sqrt(v2.x*v2.x + v2.y*v2.y);
    
    if (mag1 == 0 || mag2 == 0) return 0.0f;
    
    float cosine = dot / (mag1 * mag2);
    // Clamp
    if(cosine < -1.0f) cosine = -1.0f;
    if(cosine > 1.0f) cosine = 1.0f;
    
    return acos(cosine) * 180.0f / CV_PI;
}

float sideValue(cv::Point2f linePt1, cv::Point2f linePt2, cv::Point2f pt) {
    return (pt.x - linePt1.x) * (linePt2.y - linePt1.y) - (pt.y - linePt1.y) * (linePt2.x - linePt1.x);
}

std::string checkOutIn(cv::Point2f linePt1, cv::Point2f linePt2, cv::Point2f outPoint, cv::Point2f checkPoint) {
    float v_out = sideValue(linePt1, linePt2, outPoint);
    float v_check = sideValue(linePt1, linePt2, checkPoint);

    if (abs(v_check) < 1e-6) return "ON LINE";
    if (v_out * v_check > 0) return "OUT";
    return "IN";
}

}