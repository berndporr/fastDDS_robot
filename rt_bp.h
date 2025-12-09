#ifndef __RT_BP_H_
#define __RT_BP_H_

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <opencv2/opencv.hpp>
#include <torch/torch.h>
#include <thread>

class RtBP {

public:
	
using AISteeringCallback = std::function<void(float)>;
    
// A libtorch C++ module implementing the network you specified.
// Input: 3 x 66 x 200 (YUV planes, normalized in forward)
// Convs produce feature maps and spatial dims that match your spec.
// FC layers: 100 -> 50 -> 10 -> 2

    struct Net : public torch::nn::Module {
	// Convolutional layers
	torch::nn::Conv2d conv1{nullptr}, conv2{nullptr}, conv3{nullptr}, conv4{nullptr}, conv5{nullptr};
	
	// Fully connected layers
	torch::nn::Linear fc1{nullptr}, fc2{nullptr}, fc3{nullptr}, fc_out{nullptr};
	
	Net() {
	    // conv1: 3 -> 24, kernel 5x5, stride 2, no padding
	    conv1 = register_module("conv1", torch::nn::Conv2d(
					torch::nn::Conv2dOptions(3, 24, /*kernel_size=*/5).stride(2).padding(0)));
	    
	    // conv2: 24 -> 36, kernel 5x5, stride 2, no padding
	    conv2 = register_module("conv2", torch::nn::Conv2d(
					torch::nn::Conv2dOptions(24, 36, 5).stride(2).padding(0)));
	    
	    // conv3: 36 -> 48, kernel 5x5, stride 2, no padding
	    conv3 = register_module("conv3", torch::nn::Conv2d(
					torch::nn::Conv2dOptions(36, 48, 5).stride(2).padding(0)));
	    
	    // conv4: 48 -> 64, kernel 3x3, stride 1, no padding
	    conv4 = register_module("conv4", torch::nn::Conv2d(
					torch::nn::Conv2dOptions(48, 64, 3).stride(1).padding(0)));
	    
	    // conv5: 64 -> 64, kernel 3x3, stride 1, no padding
	    conv5 = register_module("conv5", torch::nn::Conv2d(
					torch::nn::Conv2dOptions(64, 64, 3).stride(1).padding(0)));
	    
	    // Fully connected layers
	    // After conv5, spatial dims: 1 x 18, channels=64 -> flattened size = 64*1*18 = 1152
	    const int flattened = 64 * 1 * 18;
	    fc1 = register_module("fc1", torch::nn::Linear(flattened, 100));
	    fc2 = register_module("fc2", torch::nn::Linear(100, 50));
	    fc3 = register_module("fc3", torch::nn::Linear(50, 10));
	    fc_out = register_module("fc_out", torch::nn::Linear(10, 2));

	    // Optional weight initialization (Xavier for convs and fcs)
	    for (auto &module : this->modules(/*include_self=*/false)) {
		if (auto M = dynamic_cast<torch::nn::Conv2dImpl*>(module.get())) {
		    torch::nn::init::xavier_uniform_(M->weight);
		    if (M->options.bias()) torch::nn::init::constant_(M->bias, 0.0);
		} else if (auto M = dynamic_cast<torch::nn::LinearImpl*>(module.get())) {
		    torch::nn::init::xavier_uniform_(M->weight);
		    torch::nn::init::constant_(M->bias, 0.0);
		}
	    }
	}

	// Expect x shape: {3, 66, 200}
	torch::Tensor forward(torch::Tensor x) {
	    x = x.to(torch::kFloat32) / 255.0;
	    
	    // conv stack with ReLU activations
	    x = torch::relu(conv1->forward(x)); // -> 24 x 31 x 98
	    x = torch::relu(conv2->forward(x)); // -> 36 x 14 x 47
	    x = torch::relu(conv3->forward(x)); // -> 48 x 5 x 22
	    x = torch::relu(conv4->forward(x)); // -> 64 x 3 x 20
	    x = torch::relu(conv5->forward(x)); // -> 64 x 1 x 18

	    // Flatten
	    x = torch::flatten(x);

	    // Fully connected layers
	    x = torch::atan(fc1->forward(x)); // 100
	    x = torch::atan(fc2->forward(x)); // 50
	    x = torch::atan(fc3->forward(x)); // 10
	    x = torch::atan(fc_out->forward(x)); // 2
	    return x;
	}
    };

    void setLearningRate(float mu) {
	for (auto& group : optimizer->param_groups()) {
            static_cast<torch::optim::SGDOptions&>(group.options()).lr(mu);
        }
    }

    RtBP();

    virtual ~RtBP();

    virtual float doSyncStep(cv::Mat img, float error, bool doLearn);
    virtual bool doAsyncStep(cv::Mat img, float error, bool doLearn);

    AISteeringCallback aiSteeringCallback;

private:
    void worker(cv::Mat img, float error, bool doLearn);
    
    Net model;
    torch::Tensor output;
    torch::optim::SGD* optimizer = nullptr;
    std::thread thr;
    std::atomic<bool> isRunning = false;
};

#endif
