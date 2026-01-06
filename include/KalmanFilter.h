#ifndef KALMAN_WRAPPER_H
#define KALMAN_WRAPPER_H

#include <opencv2/opencv.hpp>

class KalmanWrapper {
public:
    KalmanWrapper();
    void init(float px, float py);
    cv::Point2f update(float x, float y); // Correct & Predict
    cv::Point2f predict();                // Chỉ predict
    bool isInitialized() const { return initialized; }
    void reset();

private:
    cv::KalmanFilter kf;
    bool initialized = false;
};

#endif