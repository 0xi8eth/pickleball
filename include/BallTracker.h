#ifndef BALL_TRACKER_H
#define BALL_TRACKER_H

#include <opencv2/opencv.hpp>
#include <vector>
#include <map>
#include "KalmanFilter.h"

struct TrackedObj {
    cv::Point2f pos;
    cv::Rect bbox;
    int miss_count;
    float total_dist;
    float avg_brightness; // Độ sáng trung bình để filter shadow
};

class BallTracker {
public:
    BallTracker();
    
    // Trả về tâm bóng chính (main ball) sau khi xử lý logic
    bool update(const std::vector<cv::Rect>& detections, cv::Mat& frame, cv::Point2f& outCenter);

    // Logic logic phát hiện bounce và IN/OUT
    void processBounce(cv::Mat& frame, cv::Point2f currentPos, 
                       cv::Point2f linePt1, cv::Point2f linePt2, cv::Point2f outRefPoint);

private:
    std::map<int, TrackedObj> tracking_objects;
    int next_id = 0;
    
    // History vị trí để tính góc nảy (tương tự ball_positions trong Python)
    std::vector<cv::Point2f> position_history;
    KalmanWrapper kalman;
    bool bounce_flag = false;
    cv::Size last_ball_size = cv::Size(0,0);
    cv::Point2f bounce_point = cv::Point2f(-1, -1); // Lưu điểm bounce để vẽ lại
    bool has_bounce_point = false;
    
    // Helper function: Tính độ sáng trung bình trong bbox để filter shadow
    float computeBrightness(const cv::Rect& bbox, const cv::Mat& frame);
};

#endif