#include "BallTracker.h"
#include "Utils.h"
#include <iostream>

BallTracker::BallTracker() {}

bool BallTracker::update(const std::vector<cv::Rect>& detections, cv::Mat& frame, cv::Point2f& outCenter) {
    // 1. Map detections với tracking objects hiện có (Hungarian lite)
    std::vector<int> used_ids;
    
    for (const auto& box : detections) {
        cv::Point2f center(box.x + box.width/2.0f, box.y + box.height/2.0f);
        
        int matched_id = -1;
        float min_dist = 10000.0f;
        float dist_threshold = 150.0f;
        
        for (auto& [id, obj] : tracking_objects) {
            float dist = cv::norm(center - obj.pos);
            if (dist < min_dist && dist < dist_threshold) {
                min_dist = dist;
                matched_id = id;
            }
        }
        
        if (matched_id == -1) {
            matched_id = next_id++;
            tracking_objects[matched_id] = {center, box, 0, 0.0f};
        } else {
            tracking_objects[matched_id].total_dist += cv::norm(center - tracking_objects[matched_id].pos);
            tracking_objects[matched_id].pos = center;
            tracking_objects[matched_id].bbox = box;
            tracking_objects[matched_id].miss_count = 0;
        }
        used_ids.push_back(matched_id);
    }
    
    // Remove lost objects
    for (auto it = tracking_objects.begin(); it != tracking_objects.end();) {
        bool found = false;
        for (int id : used_ids) if (id == it->first) found = true;
        
        if (!found) {
            it->second.miss_count++;
            if (it->second.miss_count > 10) {
                it = tracking_objects.erase(it);
            } else {
                ++it;
            }
        } else {
            ++it;
        }
    }
    
    // Tìm Main Ball (ball đi xa nhất)
    int main_id = -1;
    float max_dist = 0;
    for (auto& [id, obj] : tracking_objects) {
        if (obj.total_dist > max_dist) {
            max_dist = obj.total_dist;
            main_id = id;
        }
    }
    
    if (main_id != -1 && max_dist > 50) {
        TrackedObj& mainBall = tracking_objects[main_id];
        last_ball_size = mainBall.bbox.size();
        
        // Kalman
        cv::Point2f predicted = kalman.update(mainBall.pos.x, mainBall.pos.y);
        
        // Kiểm tra nhảy vị trí bất thường (validation logic)
        if (position_history.size() > 0) {
             float d = cv::norm(predicted - position_history.back());
             if (d > 400) { // New ball detected far away
                 kalman.reset();
                 position_history.clear();
             }
        }

        position_history.push_back(predicted);
        if (position_history.size() > 5) position_history.erase(position_history.begin());
        
        outCenter = predicted;
        
        // Vẽ ball history
        for(const auto& p : position_history) {
            cv::circle(frame, p, 4, cv::Scalar(0,255,0), -1);
        }
        cv::rectangle(frame, mainBall.bbox, cv::Scalar(255,255,255), 2);
        
        return true;
    } else {
        // No detection -> Try Predict
        if (kalman.isInitialized()) {
             cv::Point2f predicted = kalman.predict();
             outCenter = predicted;
             cv::putText(frame, "PREDICTING", cv::Point(50,50), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0,255,255), 2);
             
             // Có thể thêm predict vào history để tính bounce
             position_history.push_back(predicted);
             if (position_history.size() > 5) position_history.erase(position_history.begin());
             
             return true;
        }
    }
    
    return false;
}

void BallTracker::processBounce(cv::Mat& frame, cv::Point2f currentPos, cv::Point2f linePt1, cv::Point2f linePt2, cv::Point2f outRefPoint) {
    if (position_history.size() < 3) return;
    
    // Lấy 3 điểm cuối
    cv::Point2f p0 = position_history[position_history.size()-3];
    cv::Point2f p1 = position_history[position_history.size()-2];
    cv::Point2f p2 = position_history[position_history.size()-1]; // current
    
    float angle = Utils::computeAngle(p0, p1, p2);
    
    // Draw vectors for debug
    cv::arrowedLine(frame, p1, p0, cv::Scalar(200, 220, 100), 2);
    cv::arrowedLine(frame, p1, p2, cv::Scalar(120, 255, 160), 2);
    
    if (angle < 150 && !bounce_flag) {
        bounce_flag = true;
        cv::putText(frame, "BOUNCE DETECTED", cv::Point(50, 100), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0,0,255), 2);
        
        // Tính điểm chạm đất (intersection)
        // Dùng 2 vector p0-p1 và p2-p3 (nếu có p3) hoặc xấp xỉ tại p1
        // Để chính xác như Python:
        if (position_history.size() >= 4) {
             cv::Point2f p_prev = position_history[position_history.size()-4];
             cv::Point2f inter;
             // Giao điểm đường p_prev->p0 và p1->p2 (đại diện cho quỹ đạo xuống và lên)
             if (Utils::lineIntersection(p_prev, p1, p1, p2, inter)) {
                 // Dịch xuống đáy bóng
                 inter.y += last_ball_size.height / 2.0f;
                 
                 cv::circle(frame, inter, 8, cv::Scalar(0,0,255), -1);
                 
                 // CHECK IN/OUT
                 std::string result = Utils::checkOutIn(linePt1, linePt2, outRefPoint, inter);
                 cv::putText(frame, result, cv::Point(frame.cols - 200, 100), cv::FONT_HERSHEY_SIMPLEX, 2, cv::Scalar(0,255,255), 3);
             }
        }
    } else if (angle >= 150) {
        bounce_flag = false;
    }
}