#pragma once
#define _SCL_SECURE_NO_WARNINGS

#if BUILD_DLL
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __declspec(dllimport)
#endif

// disable warning from caffe
#pragma warning(push, 0)        
#include <caffe/caffe.hpp>
#pragma warning(pop)

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <string>
#include <vector>

using namespace caffe;
using std::string;

class Classifier {
public:
    EXPORT Classifier(const string& model_file, const string& trained_file);

    EXPORT cv::Mat Predict(const cv::Mat& img);
    EXPORT cv::Mat Predict(const vector<cv::Mat>& input);

private:
    void WrapInputLayer(std::vector<cv::Mat>& input_channels, int idx = 0);
    void WrapInputLayer(std::vector<std::vector<cv::Mat>>& input_channels);

    void Preprocess(const cv::Mat& img, std::vector<cv::Mat>& input_channels, int idx = 0);
    void Preprocess(const vector<cv::Mat>& images, std::vector<std::vector<cv::Mat>>& input_channels);

    cv::Mat BlobToMat(Blob<float>* layer);

private:
    shared_ptr<Net<float> > net_;
    cv::Size input_geometry_;
    int num_channels_;
};