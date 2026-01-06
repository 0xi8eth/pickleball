#ifndef LINE_DETECTOR_H
#define LINE_DETECTOR_H

#include <opencv2/opencv.hpp>
#include <vector>

struct CourtLine {
    cv::Point2f pt1;
    cv::Point2f pt2;
    float length;
};

class LineDetector {
public:
    std::vector<CourtLine> detect(const cv::Mat& frame);
    // Chọn line dài nhất làm line biên (đơn giản hóa logic chọn line của Streamlit)
    bool getMainLine(const cv::Mat& frame, CourtLine& outLine);
};

#endif