#include "Classifier.h"

Classifier::Classifier(const string& model_file, const string& trained_file) {
    Caffe::set_mode(Caffe::GPU);

    /* Load the network. */
    net_.reset(new Net<float>(model_file, TEST));
    net_->CopyTrainedLayersFrom(trained_file);

    Blob<float>* input_layer = net_->input_blobs()[0];
    num_channels_ = input_layer->channels();

    input_geometry_ = cv::Size(input_layer->width(), input_layer->height());
}


cv::Mat Classifier::Predict(const cv::Mat& img) {
    Blob<float>* input_layer = net_->input_blobs()[0];
    input_layer->Reshape(1, num_channels_,
        input_geometry_.height, input_geometry_.width);

    /* Forward dimension change to all layers. */
    net_->Reshape();

    std::vector<cv::Mat> input_channels;
    WrapInputLayer(input_channels);

    Preprocess(img, input_channels);

    net_->Forward();

    /* Copy the output layer to a std::vector */
    Blob<float>* output_layer = net_->output_blobs()[0];

    return BlobToMat(output_layer);
}

cv::Mat Classifier::Predict(const vector<cv::Mat>& input) {
    for (unsigned int i = 0; i < net_->input_blobs().size(); i++)
    {
        Blob<float>* input_layer = net_->input_blobs()[i];
        input_layer->Reshape(1, num_channels_,
            input_geometry_.height, input_geometry_.width);
    }

    /* Forward dimension change to all layers. */
    net_->Reshape();

    std::vector<std::vector<cv::Mat>> input_channels;
    WrapInputLayer(input_channels);

    Preprocess(input, input_channels);

    net_->Forward();

    /* Copy the output layer to a std::vector */
    Blob<float>* output_layer = net_->output_blobs()[0];

    return BlobToMat(output_layer);
}

cv::Mat Classifier::BlobToMat(Blob<float>* layer)
{
    cv::Mat mergedImage = cv::Mat(layer->height(), layer->width(), CV_32F,
        const_cast<float *>(layer->cpu_data()));

    return mergedImage;
}

/* Wrap the input layer of the network in separate cv::Mat objects
* (one per channel). This way we save one memcpy operation and we
* don't need to rely on cudaMemcpy2D. The last preprocessing
* operation will write the separate channels directly to the input
* layer. */
void Classifier::WrapInputLayer(std::vector<cv::Mat>& input_channels, int idx) {
    Blob<float>* input_layer = net_->input_blobs()[idx];

    int width = input_layer->width();
    int height = input_layer->height();
    float* input_data = input_layer->mutable_cpu_data();
    for (int i = 0; i < input_layer->channels(); ++i) {
        cv::Mat channel(height, width, CV_32FC1, input_data);
        input_channels.push_back(channel);
        input_data += width * height;
    }
}

void Classifier::WrapInputLayer(std::vector<std::vector<cv::Mat>>& input_channels) {
    for (unsigned int i = 0; i < net_->input_blobs().size(); i++)
    {
        std::vector<cv::Mat> input;
        WrapInputLayer(input, i);

        input_channels.push_back(input);
    }
}

void Classifier::Preprocess(const std::vector<cv::Mat>& images,
    std::vector<std::vector<cv::Mat>>& input_channels)
{
    for (unsigned int i = 0; i < net_->input_blobs().size(); i++)
    {
        Preprocess(images[i], input_channels[i], i);
    }
}

void Classifier::Preprocess(const cv::Mat& img,
    std::vector<cv::Mat>& input_channels, int idx) {
    /* Convert the input image to the input image format of the network. */
    cv::Mat sample;
    if (img.channels() == 3 && num_channels_ == 1)
        cv::cvtColor(img, sample, cv::COLOR_BGR2GRAY);
    else if (img.channels() == 4 && num_channels_ == 1)
        cv::cvtColor(img, sample, cv::COLOR_BGRA2GRAY);
    else if (img.channels() == 4 && num_channels_ == 3)
        cv::cvtColor(img, sample, cv::COLOR_BGRA2BGR);
    else if (img.channels() == 1 && num_channels_ == 3)
        cv::cvtColor(img, sample, cv::COLOR_GRAY2BGR);
    else
        sample = img;

    cv::Mat sample_resized;
    if (sample.size() != input_geometry_)
        cv::resize(sample, sample_resized, input_geometry_);
    else
        sample_resized = sample;

    cv::Mat sample_float;
    if (num_channels_ == 3)
        sample_resized.convertTo(sample_float, CV_32FC3);
    else
        sample_resized.convertTo(sample_float, CV_32FC1);

    /* This operation will write the separate BGR planes directly to the
    * input layer of the network because it is wrapped by the cv::Mat
    * objects in input_channels. */
    cv::split(sample_float, input_channels);

    CHECK(reinterpret_cast<float*>(input_channels.at(0).data)
        == net_->input_blobs()[idx]->cpu_data())
        << "Input channels are not wrapping the input layer of the network.";
}