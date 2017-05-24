#define _SCL_SECURE_NO_WARNINGS

#include <iostream>
#include "Classifier.h"

int main(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0]
            << " deploy.prototxt network.caffemodel"
            << " img1 img2 ..." << std::endl;
        return 1;
    }

    ::google::InitGoogleLogging(argv[0]);

    string model_file = argv[1];
    string trained_file = argv[2];

    Classifier classifier(model_file, trained_file);

    vector<cv::Mat> images;
    for (int i = 3; i < argc; i++)
    {
        images.push_back(cv::imread(argv[i]));
        CHECK(!images[i - 3].empty()) << "Unable to decode image " << argv[i];
    }

    cv::Mat prediction = classifier.Predict(images);
    prediction.convertTo(prediction, CV_8U);

    cv::imshow("Display window", prediction);
    cv::waitKey(0);
}
