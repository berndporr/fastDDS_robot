#include "rt_bp.h"
#include <math.h>

#include <torch/torch.h>
#include <iostream>

void printTensorInfo(const at::Tensor& tensor, const std::string& name = "") {
    if (!name.empty()) {
        std::cout << "Tensor: " << name << std::endl;
    }

    // Shape / Sizes
    std::cout << "  Sizes: " << tensor.sizes() << std::endl;

    // Number of elements
    std::cout << "  Numel: " << tensor.numel() << std::endl;

    // Strides
    std::cout << "  Strides: ";
    for (auto s : tensor.strides()) std::cout << s << " ";
    std::cout << std::endl;

    // Device
    std::cout << "  Device: " << tensor.device() << std::endl;

    // Data type
    std::cout << "  Dtype: " << tensor.dtype() << std::endl;

    // Requires gradient?
    std::cout << "  Requires grad: " << std::boolalpha << tensor.requires_grad() << std::endl;

    // Is contiguous?
    std::cout << "  Is contiguous: " << std::boolalpha << tensor.is_contiguous() << std::endl;

    // Is sparse or quantized
    std::cout << "  Is sparse: " << std::boolalpha << tensor.is_sparse() << std::endl;
    std::cout << "  Is quantized: " << std::boolalpha << tensor.is_quantized() << std::endl;

    // Memory layout
    std::cout << "  Memory format: " << tensor.suggest_memory_format() << std::endl;

    // Optional: print first few values if small
    if (tensor.numel() <= 20) {
        std::cout << "  Values: " << tensor << std::endl;
    }

    std::cout << "---------------------------------" << std::endl;
}

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
    const at::Tensor data = MobileNetV2qFeatures::preprocess(img_bgr).to(torch::kCPU);

    // turn it into a batch
    const at::Tensor input_batch = data.unsqueeze(0);

    // det features
    const at::Tensor features_batch = features.forward(input_batch);

    printTensorInfo(features_batch,"features_batch");

    // calc steering
    const at::Tensor steering_batch = torch::relu(steerer.sequ->forward(features_batch));

    const at::Tensor steering_output = steering_batch.squeeze();

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
