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

	struct Steerer : torch::nn::Module
	{
		const char *classifierModuleName = "Steerer";
		Steerer()
		{
			sequ = torch::nn::Sequential(
				torch::nn::Linear(MobileNetV2qFeatures::N_OUTPUT_FEATURES, 2));

			register_module(classifierModuleName, sequ);
			for (auto &module : sequ->modules(/*include_self=*/false))
			{
				if (auto M = dynamic_cast<torch::nn::LinearImpl *>(module.get()))
				{
					torch::nn::init::normal_(M->weight, 0.0, 0.01);
					torch::nn::init::zeros_(M->bias);
				}
			}
		}
		torch::nn::Sequential sequ{nullptr};
	};

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
