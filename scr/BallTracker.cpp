#include "BallTracker.h"
#include "Utils.h"
#include <iostream>

BallTracker::BallTracker() {}

// Helper function: Tính độ sáng trung bình trong bbox
float BallTracker::computeBrightness(const cv::Rect& bbox, const cv::Mat& frame) {
    // Đảm bảo bbox nằm trong frame
    cv::Rect valid_bbox = bbox & cv::Rect(0, 0, frame.cols, frame.rows);
    if (valid_bbox.width <= 0 || valid_bbox.height <= 0) {
        return 0.0f;
    }
    
    // Lấy ROI
    cv::Mat roi = frame(valid_bbox);
    
    // Convert sang grayscale nếu cần
    cv::Mat gray;
    if (roi.channels() == 3) {
        cv::cvtColor(roi, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = roi;
    }
    
    // Tính mean brightness
    cv::Scalar mean_val = cv::mean(gray);
    return static_cast<float>(mean_val[0]);
}

// Helper function: Kiểm tra xem bbox có phải màu đen/xám không
bool BallTracker::isBlackOrGray(const cv::Rect& bbox, const cv::Mat& frame) {
    // Đảm bảo bbox nằm trong frame
    cv::Rect valid_bbox = bbox & cv::Rect(0, 0, frame.cols, frame.rows);
    if (valid_bbox.width <= 0 || valid_bbox.height <= 0) {
        return true; // Coi như không hợp lệ
    }
    
    // Lấy ROI
    cv::Mat roi = frame(valid_bbox);
    
    // Convert sang HSV để kiểm tra saturation (màu sắc)
    cv::Mat hsv;
    if (roi.channels() == 3) {
        cv::cvtColor(roi, hsv, cv::COLOR_BGR2HSV);
    } else {
        // Nếu đã là grayscale thì chuyển sang BGR rồi HSV
        cv::Mat bgr;
        cv::cvtColor(roi, bgr, cv::COLOR_GRAY2BGR);
        cv::cvtColor(bgr, hsv, cv::COLOR_BGR2HSV);
    }
    
    // Tách các channel
    std::vector<cv::Mat> channels;
    cv::split(hsv, channels);
    cv::Mat saturation = channels[1]; // S channel (0-255)
    cv::Mat value = channels[2];        // V channel (brightness, 0-255)
    
    // Tính mean saturation và value
    cv::Scalar mean_sat = cv::mean(saturation);
    cv::Scalar mean_val = cv::mean(value);
    
    float avg_saturation = static_cast<float>(mean_sat[0]);
    float avg_value = static_cast<float>(mean_val[0]);
    
    // Nếu saturation thấp (< 30) và value thấp (< 100) thì là đen/xám
    // Hoặc nếu value rất thấp (< 50) thì là đen
    if (avg_saturation < 30.0f && avg_value < 100.0f) {
        return true; // Màu xám (không có màu sắc rõ ràng và tối)
    }
    if (avg_value < 50.0f) {
        return true; // Màu đen (rất tối)
    }
    
    return false; // Có màu sắc, không phải đen/xám
}

bool BallTracker::update(const std::vector<cv::Rect>& detections, cv::Mat& frame, cv::Point2f& outCenter) {
    // 1. Map detections với tracking objects hiện có (Hungarian lite)
    std::vector<int> used_ids;
    
    for (const auto& box : detections) {
        // Filter ngay từ đầu: bỏ qua detection màu đen/xám
        if (isBlackOrGray(box, frame)) {
            continue; // Bỏ qua detection này
        }
        
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
        
        // Tính brightness cho detection hiện tại
        float brightness = computeBrightness(box, frame);
        
        if (matched_id == -1) {
            matched_id = next_id++;
            tracking_objects[matched_id] = {center, box, 0, 0.0f, brightness};
        } else {
            // Kiểm tra chuyển động: nếu di chuyển quá ít thì coi như nhiễu, không cập nhật
            float move_dist = cv::norm(center - tracking_objects[matched_id].pos);
            float min_move_threshold = 5.0f; // Ngưỡng tối thiểu để coi là chuyển động (pixels)
            
            if (move_dist >= min_move_threshold) {
                tracking_objects[matched_id].total_dist += move_dist;
                tracking_objects[matched_id].pos = center;
                tracking_objects[matched_id].bbox = box;
                tracking_objects[matched_id].miss_count = 0;
                // Cập nhật brightness (moving average để ổn định)
                tracking_objects[matched_id].avg_brightness = 
                    0.7f * tracking_objects[matched_id].avg_brightness + 0.3f * brightness;
            } else {
                // Bóng không chuyển động, không cập nhật vị trí (coi như nhiễu)
                // Tăng miss_count để logic remove lost objects loại bỏ sớm hơn
                tracking_objects[matched_id].miss_count++;
                // Không thêm vào used_ids để logic remove lost objects xử lý
                matched_id = -1;
            }
        }
        if (matched_id != -1) {
            used_ids.push_back(matched_id);
        }
    }
    
    // Remove lost objects
    for (auto it = tracking_objects.begin(); it != tracking_objects.end();) {
        bool found = false;
        for (int id : used_ids) if (id == it->first) found = true;
        
        if (!found) {
            it->second.miss_count++;
            // Loại bỏ nhanh hơn nếu object không chuyển động (miss_count cao từ trước)
            int max_miss = (it->second.total_dist < 20.0f) ? 5 : 10; // Object ít di chuyển thì loại bỏ sớm hơn
            if (it->second.miss_count > max_miss) {
                it = tracking_objects.erase(it);
            } else {
                ++it;
            }
        } else {
            ++it;
        }
    }
    
    // Tìm Main Ball: Kết hợp total_dist và brightness (filter shadow)
    int main_id = -1;
    float max_score = 0;
    float min_brightness_threshold = 80.0f; // Ngưỡng tối thiểu để loại bỏ shadow (0-255)
    
    for (auto& [id, obj] : tracking_objects) {
        // Loại bỏ object quá tối (shadow)
        if (obj.avg_brightness < min_brightness_threshold) {
            continue;
        }
        
        // Score = total_dist * brightness_factor
        // Brightness factor: normalize brightness (0-255) thành 0.5-1.5 để ưu tiên bóng sáng
        float brightness_factor = 0.5f + (obj.avg_brightness / 255.0f) * 1.0f;
        float score = obj.total_dist * brightness_factor;
        
        if (score > max_score) {
            max_score = score;
            main_id = id;
        }
    }
    
    // Kiểm tra total_dist của main ball (không dùng max_score vì đã có brightness factor)
    float main_total_dist = (main_id != -1) ? tracking_objects[main_id].total_dist : 0.0f;
    if (main_id != -1 && main_total_dist > 50) {
        TrackedObj& mainBall = tracking_objects[main_id];
        last_ball_size = mainBall.bbox.size();
        
        // Kalman
        cv::Point2f predicted = kalman.update(mainBall.pos.x, mainBall.pos.y);
        
        // Kiểm tra nhảy vị trí bất thường (validation logic) - so sánh với vị trí thực tế
        if (position_history.size() > 0) {
             float d = cv::norm(mainBall.pos - position_history.back());
             if (d > 400) { // New ball detected far away
                 kalman.reset();
                 position_history.clear();
             }
        }

        // Lưu vị trí thực tế (trung tâm bbox) vào history thay vì predicted
        position_history.push_back(mainBall.pos);
        if (position_history.size() > 5) position_history.erase(position_history.begin());
        
        outCenter = predicted;
        
        // Vẽ ball history
        for(const auto& p : position_history) {
            cv::circle(frame, p, 4, cv::Scalar(0,255,0), -1);
        }
        cv::rectangle(frame, mainBall.bbox, cv::Scalar(255,255,255), 2);
        
        // Vẽ lại điểm bounce nếu có
        if (has_bounce_point && bounce_point.x >= 0 && bounce_point.y >= 0) {
            cv::circle(frame, bounce_point, 10, cv::Scalar(0,0,255), -1); // Điểm đỏ lớn hơn
            cv::circle(frame, bounce_point, 12, cv::Scalar(0,0,255), 2); // Viền đỏ
        }
        
        return true;
    } else {
        // No detection -> Try Predict
        if (kalman.isInitialized()) {
             cv::Point2f predicted = kalman.predict();
             outCenter = predicted;
             cv::putText(frame, "PREDICTING", cv::Point(50,50), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0,255,255), 2);
             
             // Không thêm predicted vào history vì không có vị trí thực tế
             // History chỉ lưu vị trí thực tế từ detection
             
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
    cv::Point2f p2 = currentPos; // Dùng vị trí thực tế của bóng thay vì predicted
    
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
                 
                 // Lưu điểm bounce để vẽ lại ở các frame sau
                 bounce_point = inter;
                 has_bounce_point = true;
                 
                 // Vẽ điểm bounce đỏ rõ ràng
                 cv::circle(frame, inter, 10, cv::Scalar(0,0,255), -1); // Điểm đỏ lớn
                 cv::circle(frame, inter, 12, cv::Scalar(0,0,255), 2); // Viền đỏ
                 
                 // CHECK IN/OUT
                 std::string result = Utils::checkOutIn(linePt1, linePt2, outRefPoint, inter);
                 cv::putText(frame, result, cv::Point(frame.cols - 200, 100), cv::FONT_HERSHEY_SIMPLEX, 2, cv::Scalar(0,255,255), 3);
             }
        }
    } else if (angle >= 150) {
        bounce_flag = false;
    }
}