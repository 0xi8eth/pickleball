#include "YoloDetector.h"
#include "Config.h"

YoloDetector::YoloDetector(const std::string& modelPath) {
    net = cv::dnn::readNetFromONNX(modelPath);
    // Dùng CUDA nếu có
    // net.setPreferableBackend(cv::dnn::DNN_BACKEND_CUDA);
    // net.setPreferableTarget(cv::dnn::DNN_TARGET_CUDA);
}

std::vector<Detection> YoloDetector::detect(cv::Mat& frame) {
    std::vector<Detection> detections;
    
    cv::Mat blob;
    // YOLOv8 thường dùng 640x640, scale 1/255
    cv::dnn::blobFromImage(frame, blob, 1.0/255.0, cv::Size(640, 640), cv::Scalar(), true, false);
    net.setInput(blob);
    
    std::vector<cv::Mat> outputs;
    net.forward(outputs, net.getUnconnectedOutLayersNames());
    
    // Xử lý output (giả định YOLOv8: 1 x 84 x 8400)
    // 84 = 4 box coords + 80 classes (hoặc ít hơn tùy model custom)
    // Ở đây model custom có thể chỉ có 2 class (ball, line) -> 6 channels
    
    cv::Mat output = outputs[0]; // Shape: [1, channels, anchors]
    // Transpose để dễ xử lý: [1, anchors, channels]
    cv::Mat data = output.reshape(1, output.size[1]); // reshape(channels, rows)
    data = data.t(); // Transpose
    
    float* pdata = (float*)data.data;
    int rows = data.rows; 
    int dimensions = data.cols; // x, y, w, h, score_class1, ...

    std::vector<int> class_ids;
    std::vector<float> confidences;
    std::vector<cv::Rect> boxes;
    
    float x_factor = (float)frame.cols / 640.0;
    float y_factor = (float)frame.rows / 640.0;

    for (int i = 0; i < rows; ++i) {
        float* row_ptr = data.ptr<float>(i);
        
        // Tìm class score cao nhất
        float max_score = 0.0;
        int class_id = -1;
        // Bắt đầu từ index 4 (0,1,2,3 là bbox)
        for(int j = 4; j < dimensions; ++j) {
            if (row_ptr[j] > max_score) {
                max_score = row_ptr[j];
                class_id = j - 4;
            }
        }
        
        if (max_score >= Config::CONF_THRESHOLD) {
            // Chỉ lấy class Ball (giả sử Ball ID = 0)
            if (class_id == 0) { 
                float cx = row_ptr[0] * x_factor;
                float cy = row_ptr[1] * y_factor;
                float w = row_ptr[2] * x_factor;
                float h = row_ptr[3] * y_factor;
                
                int left = int(cx - w / 2);
                int top = int(cy - h / 2);
                
                boxes.push_back(cv::Rect(left, top, int(w), int(h)));
                confidences.push_back(max_score);
                class_ids.push_back(class_id);
            }
        }
    }
    
    std::vector<int> indices;
    cv::dnn::NMSBoxes(boxes, confidences, Config::SCORE_THRESHOLD, Config::NMS_THRESHOLD, indices);
    
    for (int idx : indices) {
        Detection det;
        det.class_id = class_ids[idx];
        det.confidence = confidences[idx];
        det.box = boxes[idx];
        detections.push_back(det);
    }
    
    return detections;
}