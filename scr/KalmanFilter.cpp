#include "KalmanFilter.h"
#include "Config.h"

KalmanWrapper::KalmanWrapper() {
    // 4 trạng thái (x, y, dx, dy), 2 đo lường (x, y)
    kf = cv::KalmanFilter(4, 2, 0); 
    
    // Transition Matrix
    kf.transitionMatrix = (cv::Mat_<float>(4, 4) << 
        1, 0, 1, 0,
        0, 1, 0, 1,
        0, 0, 1, 0,
        0, 0, 0, 1);
        
    // Measurement Matrix
    kf.measurementMatrix = (cv::Mat_<float>(2, 4) <<
        1, 0, 0, 0,
        0, 1, 0, 0);

    // Noise
    cv::setIdentity(kf.processNoiseCov, cv::Scalar::all(Config::PROCESS_NOISE));
    cv::setIdentity(kf.measurementNoiseCov, cv::Scalar::all(Config::MEASUREMENT_NOISE));
    cv::setIdentity(kf.errorCovPost, cv::Scalar::all(1));
}

void KalmanWrapper::init(float px, float py) {
    kf.statePost = (cv::Mat_<float>(4, 1) << px, py, 0, 0);
    initialized = true;
}

cv::Point2f KalmanWrapper::predict() {
    cv::Mat p = kf.predict();
    return cv::Point2f(p.at<float>(0), p.at<float>(1));
}

cv::Point2f KalmanWrapper::update(float x, float y) {
    if (!initialized) {
        init(x, y);
        return predict();
    }
    
    cv::Mat measurement = (cv::Mat_<float>(2, 1) << x, y);
    kf.correct(measurement);
    cv::Mat p = kf.predict();
    return cv::Point2f(p.at<float>(0), p.at<float>(1));
}

void KalmanWrapper::reset() {
    initialized = false;
}