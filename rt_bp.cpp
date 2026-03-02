#include "rt_bp.h"
#include <math.h>

#include <torch/torch.h>
#include <iostream>

RtBP::RtBP()
{
    torch::manual_seed(1);
    optimizer = std::make_shared<torch::optim::SGD>(steerer->parameters(),0);
}

RtBP::~RtBP()
{
    if (thr.joinable())
        thr.join();
}

void RtBP::worker(cv::Mat img, float error, bool doLearn)
{
    isRunning = true;
    const float steering = doSyncStep(img, error, doLearn);
    if (aiSteeringCallback)
        aiSteeringCallback(steering);
    isRunning = false;
}

bool RtBP::doAsyncStep(cv::Mat img, float error, bool doLearn)
{
    if (isRunning)
    {
        return false;
    }
    if (thr.joinable())
    {
        thr.join();
    }
    thr = std::thread(&RtBP::worker, this, img, error, doLearn);
    return true;
}

float RtBP::doSyncStep(cv::Mat img_bgr, float desired_phi, bool doLearn)
{
    const at::Tensor data = MobileNetV2qFeatures::preprocess(img_bgr);

    // turn it into a batch
    const at::Tensor input_batch = data.unsqueeze(0);

    // det features
    const at::Tensor features_batch = features.forward(input_batch);

    // printTensorInfo(features_batch,"features_batch");

    at::Tensor phi = steerer->forward(features_batch);

    phi = phi.squeeze();
    float actual_phi = phi.data_ptr<float>()[0];

    // do we learn?
    if (doLearn)
    {
        float error = desired_phi - actual_phi;
        optimizer->zero_grad();
        torch::Tensor gradient = torch::tensor({-error});
        phi.retain_grad();
        phi.backward(gradient);
        optimizer->step();
    }
    return actual_phi;
}
