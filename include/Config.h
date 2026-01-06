#ifndef CONFIG_H
#define CONFIG_H

#include <string>

namespace Config {
    const std::string SOURCE_VIDEO_PATH = "data/In.mp4";
    const std::string TARGET_VIDEO_PATH = "Out.mp4";
    const std::string MODEL_PATH = "data/model_ver2.onnx"; 
    const float CONF_THRESHOLD = 0.2f;
    const float SCORE_THRESHOLD = 0.4f;
    const float NMS_THRESHOLD = 0.4f;
    
    // Kalman
    const float PROCESS_NOISE = 0.5f;
    const float MEASUREMENT_NOISE = 5.0f;
}

#endif // CONFIG_H