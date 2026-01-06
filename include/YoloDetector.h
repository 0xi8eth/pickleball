#ifndef YOLO_DETECTOR_H
#define YOLO_DETECTOR_H

#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <vector>

struct Detection {
    int class_id;
    float confidence;
    cv::Rect box;
};

class YoloDetector {
public:
    YoloDetector(const std::string& modelPath);
    std::vector<Detection> detect(cv::Mat& frame);

private:
    cv::dnn::Net net;
    std::vector<std::string> outNames;
};

#endif