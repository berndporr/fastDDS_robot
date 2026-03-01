#ifndef __RT_BP_H_
#define __RT_BP_H_

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <opencv2/opencv.hpp>
#include <torch/torch.h>
#include <thread>
#include "mobilenet_v2qfeatures.h"

class RtBP
{
public:
	using AISteeringCallback = std::function<void(float)>;

	struct SteererImpl : torch::nn::Module
	{
		SteererImpl()
		{
			cell_weights = register_parameter(
				"cell_weights",
				torch::randn({1280, 7, 7}));

			cell_bias = register_parameter(
				"cell_bias",
				torch::zeros({7, 7}));
			spatialFc = register_module(
				"spatial_fc",
				torch::nn::Linear(49, 1));
			torch::nn::init::xavier_normal_(cell_weights, 0.0001);
			torch::nn::init::zeros_(cell_bias);
			torch::nn::init::xavier_normal_(spatialFc->weight, 0.0001);
			torch::nn::init::zeros_(spatialFc->bias);
		}
		torch::Tensor forward(torch::Tensor x)
		{
			auto cell_scores = (x * cell_weights.unsqueeze(0)).sum(1) + cell_bias.unsqueeze(0);
			cell_scores = torch::atan(cell_scores);
			auto B = cell_scores.size(0);
			// [B, 7, 7] → [B, 49]
			auto flat = cell_scores.view({B, 49});
			// Linear(49 → 1)
			torch::Tensor out = spatialFc->forward(flat);
			return out;
		}
		torch::Tensor cell_weights; // [1280, 7, 7]
		torch::Tensor cell_bias;	// [7, 7]
		std::shared_ptr<torch::nn::LinearImpl> spatialFc;
	};
	TORCH_MODULE(Steerer);

	void setLearningRate(float mu)
	{
		for (auto &group : optimizer->param_groups())
		{
			static_cast<torch::optim::SGDOptions &>(group.options()).lr(mu);
		}
	}

	RtBP();

	virtual ~RtBP();

	virtual float doSyncStep(cv::Mat img, float error, bool doLearn);
	virtual bool doAsyncStep(cv::Mat img, float error, bool doLearn);

	AISteeringCallback aiSteeringCallback;

private:
	void worker(cv::Mat img, float error, bool doLearn);

	MobileNetV2qFeatures features;
	Steerer steerer;
	std::shared_ptr<torch::optim::SGD> optimizer;
	std::thread thr;
	std::atomic<bool> isRunning = false;
	// Path to the pretrained weights file
	const int nClasses = 2;
};

#endif
