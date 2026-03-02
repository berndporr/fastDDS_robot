#include "rt_bp.h"
#include <chrono>

void asyncCallback(float v)
{
    printf("Async steering: %f\n", v);
}

int main(int, char **)
{
    fprintf(stderr, "Loading RtBP.\n");
    RtBP rtbp;
    cv::Mat img = cv::imread("road.jpg");
    float steering = rtbp.doSyncStep(img, 0, true);
    printf("Steering = %f\n", steering);
    const int n = 1000;
    rtbp.setLearningRate(0.1);
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < n; i++)
    {
        float desired = (float)i / (float)n - 0.5;
        steering = rtbp.doSyncStep(img, desired, true);
        printf("Desired = %f, Actual = %f\n", desired, steering);
    }
    printf("\n");
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
    printf("Time taken: %ld miliseconds for %d steps which gives a framerate of %f Hz.\n",
           duration.count(), n, n * 1000 / (float)duration.count());

    rtbp.aiSteeringCallback = asyncCallback;
    rtbp.doAsyncStep(img, 0.5, true);

    return 0;
}
