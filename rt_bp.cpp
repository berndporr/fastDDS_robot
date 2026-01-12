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

    // Replace the standard classifier by this custom one with
    // only two categories for cats and dogs.
    auto newClassifier = torch::nn::Sequential(
        torch::nn::Dropout(0.2),
        torch::nn::Linear(model.getNinputChannelsOfClassifier(), nClasses));
    model.replaceClassifier(newClassifier);

    optimizer = new torch::optim::SGD(model.getClassifier()->parameters(), 0);

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

    // forward pass
    auto output = model.forward(data.unsqueeze(0)).squeeze();

    // do we learn?
    if (doLearn)
    {
        optimizer->zero_grad();
        torch::Tensor gradient = torch::tensor({error, error});
        output.retain_grad();
        output.backward(gradient);
        optimizer->step();
    }

    // one dim output tensor
    auto a = output.accessor<float, 1>();
    return a[0] - a[1];
}
