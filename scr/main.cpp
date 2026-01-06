#include <opencv2/opencv.hpp>
#include <iostream>
#include "Config.h"
#include "YoloDetector.h"
#include "BallTracker.h"
#include "LineDetector.h"

int main() {
    // 1. Setup
    cv::VideoCapture cap(Config::SOURCE_VIDEO_PATH);
    if (!cap.isOpened()) {
        std::cerr << "Cannot open video" << std::endl;
        return -1;
    }

    int width = cap.get(cv::CAP_PROP_FRAME_WIDTH);
    int height = cap.get(cv::CAP_PROP_FRAME_HEIGHT);
    cv::VideoWriter writer(Config::TARGET_VIDEO_PATH, cv::VideoWriter::fourcc('m','p','4','v'), 
                           30, cv::Size(width, height));

    // 2. Init Modules
    YoloDetector detector(Config::MODEL_PATH);
    BallTracker tracker;
    LineDetector lineDetector;

    // 3. Line Selection (Giả lập: Lấy line dài nhất frame đầu tiên)
    cv::Mat firstFrame;
    cap.read(firstFrame);
    CourtLine selectedLine;
    bool lineFound = lineDetector.getMainLine(firstFrame, selectedLine);
    
    // Định nghĩa điểm OUT mẫu (giả sử bên phải line là OUT cho demo)
    // Trong thực tế, bạn cần thuật toán xác định phía hoặc UI click chuột
    cv::Point2f outRefPoint(width, height/2); 

    if (lineFound) {
        std::cout << "Line selected: " << selectedLine.pt1 << " -> " << selectedLine.pt2 << std::endl;
    } else {
        std::cerr << "No court line found!" << std::endl;
    }
    
    cap.set(cv::CAP_PROP_POS_FRAMES, 0); // Reset video

    // 4. Processing Loop
    cv::Mat frame;
    while (cap.read(frame)) {
        // Detect Ball
        auto detections = detector.detect(frame);
        
        // Convert Detection to cv::Rect for tracker
        std::vector<cv::Rect> detectionBoxes;
        for (const auto& det : detections) {
            detectionBoxes.push_back(det.box);
        }
        
        // Tracking & Logic
        cv::Point2f ballCenter;
        if (tracker.update(detectionBoxes, frame, ballCenter)) {
            // Nếu có bóng và có line -> Check Bounce
            if (lineFound) {
                tracker.processBounce(frame, ballCenter, selectedLine.pt1, selectedLine.pt2, outRefPoint);
            }
        }
        
        // Draw Line
        if (lineFound) {
            cv::line(frame, selectedLine.pt1, selectedLine.pt2, cv::Scalar(255, 0, 0), 3);
            cv::circle(frame, outRefPoint, 5, cv::Scalar(0,0,255), -1);
            cv::putText(frame, "OUT REF", outRefPoint, cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0,0,255), 1);
        }

        writer.write(frame);
        
        // Show (Optional on server)
        // cv::imshow("Tennis", frame);
        // if (cv::waitKey(1) == 27) break;
    }

    cap.release();
    writer.release();
    std::cout << "Done! Output saved to " << Config::TARGET_VIDEO_PATH << std::endl;

    return 0;
}