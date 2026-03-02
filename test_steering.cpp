#include "rt_bp.h"
#include <chrono>
#include <filesystem>
namespace fs = std::filesystem;

void asyncCallback(float v)
{
    printf("Async steering: %f\n", v);
}

struct ImageDataset
{
    std::vector<std::filesystem::path> images;

    ImageDataset(const fs::path &root)
    {
        for (const auto &p : fs::directory_iterator(root))
        {
            if (p.is_regular_file())
            {
                images.push_back(p.path());
            }
        }
    }

    std::filesystem::path getRandomImage() {
        const int random_value = std::rand() % (images.size());
        return images[random_value];
    }
};

int main(int, char **)
{
    fprintf(stderr, "Loading RtBP.\n");
    RtBP rtbp;
    ImageDataset datasetLeft("road/l");
    ImageDataset datasetRight("road/r");
    cv::Mat img = cv::imread("road.jpg");
    float steering = rtbp.doSyncStep(img, 0, true);
    printf("Steering = %f\n", steering);
    const int n = 1000;
    rtbp.setLearningRate(0.01);
    auto start = std::chrono::high_resolution_clock::now();
    FILE* f = fopen("log.tsv","wt");
    for (int i = 0; i < n; i++)
    {
        float phi_des = 0;
        if (std::rand() > (RAND_MAX/2)) {
            img = cv::imread(datasetLeft.getRandomImage());
            phi_des = 1;
        } else {
            img = cv::imread(datasetRight.getRandomImage());
            phi_des = -1;
        }
        steering = rtbp.doSyncStep(img, phi_des, true);
        float err = phi_des - steering;
        printf("Desired = %f, Steering = %f, Err = %f\n", phi_des, steering, err);
        fprintf(f,"%f\t%f\t%f\n",steering,phi_des,err);
    }
    fclose(f);
    printf("\n");
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
    printf("Time taken: %ld miliseconds for %d steps which gives a framerate of %f Hz.\n",
           duration.count(), n, n * 1000 / (float)duration.count());

    rtbp.aiSteeringCallback = asyncCallback;
    rtbp.doAsyncStep(img, 0.5, true);

    return 0;
}
