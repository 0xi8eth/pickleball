#ifndef BALL_TRACKER_H
#define BALL_TRACKER_H

#include <opencv2/opencv.hpp>
#include <vector>
#include <map>

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
    // Lưu centers của 2 frame trước để kiểm tra 3 frame trùng nhau
    std::vector<cv::Point2f> prev_frame_centers_1; // Frame n-1
    std::vector<cv::Point2f> prev_frame_centers_2; // Frame n-2
    bool bounce_flag = false;
    cv::Size last_ball_size = cv::Size(0,0);
    cv::Point2f bounce_point = cv::Point2f(-1, -1); // Lưu điểm bounce để vẽ lại
    bool has_bounce_point = false;
    
    // Helper function: Tính độ sáng trung bình trong bbox để filter shadow
    float computeBrightness(const cv::Rect& bbox, const cv::Mat& frame);
    
    // Helper function: Kiểm tra xem bbox có phải màu đen/xám không
    bool isBlackOrGray(const cv::Rect& bbox, const cv::Mat& frame);
};

#endif