#ifndef __RT_BP_H_
#define __RT_BP_H_

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <opencv2/opencv.hpp>
#include <torch/torch.h>
#include <thread>
#include "mobilenet_v2q.h"

class RtBP
{
public:
	using AISteeringCallback = std::function<void(float)>;

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

	MobileNetV2q model;
	torch::optim::SGD *optimizer = nullptr;
	std::thread thr;
	std::atomic<bool> isRunning = false;
	// Path to the pretrained weights file
	const int nClasses = 2;
};

#endif
