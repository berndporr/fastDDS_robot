#include "alphabot.h"
#include "wheel_subscriber.h"
#include "libcam2opencv.h"
#include <ncurses.h>
#include <iostream>

constexpr int batteryRowNo = 3;
constexpr int missedFramesRowNo = 5;
constexpr int infoRowNo = 7;
constexpr int eventRowNo = 9;

// callback every 100ms
class DisplaySensorCallback : public AlphaBot::BatteryCallback
{
public:
	virtual void hasBatteryVoltage(float v)
	{
		char tmp[256];
		sprintf(tmp,
				"Power = %1.1f Volt    ", v);
		mvaddstr(batteryRowNo, 0, tmp);
		refresh();
	}
};

struct RobotController
{
	void send2motors()
	{
		float l = speed + steering;
		if (l < 0)
			l = 0;
		float r = speed - steering;
		if (r < 0)
			r = 0;
		if (reverse)
		{
			alphabot.setLeftWheelSpeed(-speed + steering);
			alphabot.setRightWheelSpeed(-speed - steering);
		}
		else
		{
			alphabot.setLeftWheelSpeed(speed + steering);
			alphabot.setRightWheelSpeed(speed - steering);
		}
	}

	void setThrottle(float t)
	{
		speed = t;
		send2motors();
	}

	void setSteering(float s)
	{
		steering = s;
		send2motors();
	}

	float speed = 0;
	float steering = 0;
	bool reverse = false;
	AlphaBot alphabot;
};

struct CameraCallback : Libcam2OpenCV::Callback
{
	virtual void hasFrame(const cv::Mat &frame, const libcamera::ControlList &)
	{
	    frameNumber++;
	    if (savingFrame) return;
	    savingFrame = true;
	    char tmp[256];
	    sprintf(tmp,"/tmp/frame%05d.png",frameNumber);
	    imwrite(tmp, save_img);
	    savingFrame = false;
	}
    std::atomic<bool> savingFrame = false;
    int frameNumber = 0;
};

int main(int, char **)
{
	DisplaySensorCallback displaySensorCallback;
	RobotSubscriber mysub;
	RobotController robotController;
	Libcam2OpenCV camera;
	CameraCallback cameraCallback;

	////////////////////////////////////////////////////
	// Init
	if (!mysub.init())
	{
		std::cerr << "Could not init the subscriber." << std::endl;
		return -1;
	}
	mysub.registerSteeringCallback([&](float s)
	    {
	    cameraCallbackAIlogic.manualSteering = s;
		robotController.setSteering(s);
		char tmp[256];
		sprintf(tmp,"Steering: %f",s);
		mvaddstr(missedFramesRowNo, 0, tmp);
	    });
	mysub.registerThrottleCallback([&](float t)
	    {
		robotController.setThrottle(t);
		char tmp[256];
		sprintf(tmp,"Throttle: %f",t);
		mvaddstr(missedFramesRowNo, 0, tmp);
	    });

	camera.registerCallback(&cameraCallbackAIlogic);

	// create an instance of the settings
	Libcam2OpenCVSettings settings;

	// set the framerate (default is variable framerate)
	settings.framerate = 30;

	robotController.alphabot.registerBatteryCallback(&displaySensorCallback);
	robotController.alphabot.start();

	robotController.setSteering(0);
	robotController.setThrottle(0);

	// start the camera with these settings
	camera.start(settings);

	/////////////////////////////////////////////////////////
	// main display loop. This is using _blocking_ getchar so
	// everything realtime is in callbacks defined above!
	initscr();
	noecho();
	clear();
	mvaddstr(0, 0, "SPACE=toggle auto/manual, ESC=end");
	refresh();
	bool running = true;
	while (running)
	{
		// blocking so that the main program sleeps here
		int ch = getchar();
		switch (ch)
		{
		case 27:
			running = false;
			break;

		default:
			break;
		}
	}
	robotController.alphabot.stop();
	camera.stop();
	endwin();
}
