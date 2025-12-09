#include "rt_bp.h"
#include <math.h>

RtBP::RtBP() {
    torch::manual_seed(1);
    
    torch::DeviceType device_type;
    if (torch::cuda::is_available()) {
	std::cout << "CUDA available! Training on GPU." << std::endl;
	device_type = torch::kCUDA;
    } else {
	std::cout << "Training on CPU." << std::endl;
	device_type = torch::kCPU;
    }
    torch::Device device(device_type);
    
    model.to(device);
    model.train();
    
    optimizer = new torch::optim::SGD(model.parameters(), 0);
}

RtBP::~RtBP() {
    if (thr.joinable()) thr.join();
    delete optimizer;
}

void RtBP::worker(cv::Mat img, float error, bool doLearn) {
    isRunning = true;
    const float steering = doSyncStep(img, error, doLearn);
    if (aiSteeringCallback) aiSteeringCallback(steering);
    isRunning = false;
}

bool RtBP::doAsyncStep(cv::Mat img, float error, bool doLearn) {
    if (isRunning) {
	return false;
    }
    if (thr.joinable()) {
	thr.join();
    }
    thr = std::thread(&RtBP::worker, this, img, error, doLearn);
    return true;
}

float RtBP::doSyncStep(cv::Mat img_bgr, float error, bool doLearn) {
    // Resize to width=200, height=66
    cv::resize(img_bgr, img_bgr, cv::Size(200, 66)); // cv::Size(W,H)

    // Convert BGR -> RGB
    cv::Mat img_rgb;
    cv::cvtColor(img_bgr, img_rgb, cv::COLOR_BGR2RGB);

    // Convert to tensor (HWC)
    torch::Tensor img_tensor = torch::from_blob(
        img_rgb.data, {img_rgb.rows, img_rgb.cols, 3}, torch::kUInt8);

    // Permute to CHW
    torch::Tensor data = img_tensor.permute({2, 0, 1}).clone(); // clone to own the memory

    // forward pass
    output = model.forward(data);

    // do we learn?
    if (doLearn) {
	optimizer->zero_grad();
	torch::Tensor gradient = torch::tensor({error,error});
	output.retain_grad();
	output.backward(gradient);
	optimizer->step();
    }
    
    // one dim output tensor
    auto a = output.accessor<float,1>();
    return a[0] - a[1];
}
