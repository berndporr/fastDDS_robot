#include "rt_bp.h"
#include <math.h>

RtBP::RtBP()
{
    torch::manual_seed(1);

    torch::DeviceType device_type;
    if (torch::cuda::is_available())
    {
        std::cout << "CUDA available! Training on GPU." << std::endl;
        device_type = torch::kCUDA;
    }
    else
    {
        std::cout << "Training on CPU." << std::endl;
        device_type = torch::kCPU;
    }
    torch::Device device(device_type);

    optimizer = new torch::optim::SGD(steerer.sequ->parameters(), 0);

    // Send the model to the CPU or GPU
    model.to(device);
}

RtBP::~RtBP()
{
    if (thr.joinable())
        thr.join();
    delete optimizer;
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
    auto data = model.preprocess(img_bgr);

    // turn it into a batch
    const auto input_batch = data.unsqueeze(0);

    // det features
    const auto features_batch = model.forward(input_batch);
    // calc steering
    const auto steering_batch = steerer.sequ->forward(features_batch);

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
