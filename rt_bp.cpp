#include "rt_bp.h"
#include <math.h>

RtBP::RtBP()
{
    torch::manual_seed(1);

    optimizer = std::make_shared<torch::optim::SGD>(steerer.sequ->parameters(), 0);
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

float RtBP::doSyncStep(cv::Mat img_bgr, float error, bool doLearn)
{
    auto data = features.preprocess(img_bgr);

    // turn it into a batch
    const auto input_batch = data.unsqueeze(0);

    // det features
    const auto features_batch = features.forward(input_batch);
    // calc steering
    const auto steering_batch = torch::relu(steerer.sequ->forward(features_batch));

    const auto steering_output = steering_batch.squeeze();

    // do we learn?
    if (doLearn)
    {
        optimizer->zero_grad();
        torch::Tensor gradient = torch::tensor({error, error});
        steering_output.retain_grad();
        steering_output.backward(gradient);
        optimizer->step();
    }

    // one dim output tensor
    auto a = steering_output.accessor<float, 1>();
    return a[0] - a[1];
}
