#include "rt_bp.h"
#include "alphabot.h"
#include "wheel_subscriber.h"
#include "libcam2opencv.h"
#include <ncurses.h>
#include <iostream>

constexpr int batteryRowNo = 3;
constexpr int missedFramesRowNo = 5;
constexpr int infoRowNo = 7;
constexpr int eventRowNo = 9;

constexpr float learningRate = 0.005;

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

struct CameraCallbackAIlogic : Libcam2OpenCV::Callback
{
	FILE *logger = nullptr;
	RtBP *rtbp = nullptr;
	float manualSteering = 0;
	float nnSteering = 0;
	int nMissedFrames = 0;
	bool autonomous = false;
	virtual void hasFrame(const cv::Mat &frame, const libcamera::ControlList &)
	{
		float currentError = 0;
		bool doLearn = false;
		float steeringLogger = manualSteering;
		if (!autonomous)
		{
			currentError = manualSteering - nnSteering;
			doLearn = true;
		}
		else
		{
			steeringLogger = 0;
		}
		if (logger)
			fprintf(logger, "%f %f %f\n", currentError, steeringLogger, nnSteering);
		if (rtbp)
		{
			bool m = rtbp->doAsyncStep(frame, currentError, doLearn);
			if (!m)
			{
				nMissedFrames++;
				char tmp[256];
				sprintf(tmp,
						"missedFrames = %d    ", nMissedFrames);
				mvaddstr(missedFramesRowNo, 0, tmp);
				refresh();
			}
			else
			{
				if (nMissedFrames > 0)
				{
					mvaddstr(missedFramesRowNo, 0, "                              ");
				}
				nMissedFrames = 0;
			}
		}
	}
	void setNNsteering(float v)
	{
		nnSteering = v;
	}
};

int main(int, char **)
{
	DisplaySensorCallback displaySensorCallback;
	RobotSubscriber mysub;
	RobotController robotController;
	RtBP rtbp;
	Libcam2OpenCV camera;
	CameraCallbackAIlogic cameraCallbackAIlogic;

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
	    if (!cameraCallbackAIlogic.autonomous) {
		robotController.setSteering(s);
		char tmp[256];
		sprintf(tmp,"Steering: %f",s);
		mvaddstr(missedFramesRowNo, 0, tmp);
	    } });
	mysub.registerThrottleCallback([&](float t)
								   {
	    if (!cameraCallbackAIlogic.autonomous) {
		robotController.setThrottle(t);
		char tmp[256];
		sprintf(tmp,"Throttle: %f",t);
		mvaddstr(missedFramesRowNo, 0, tmp);
	    } });
	mysub.registerButtonCallback([&](int idx)
								 {
	    if (idx < 7) return;
	    if (idx > 10) return;
	    cameraCallbackAIlogic.autonomous = !cameraCallbackAIlogic.autonomous;
	    if (cameraCallbackAIlogic.autonomous) {
		mvaddstr(infoRowNo, 0, "Autonomous                            ");
	    } else {
		mvaddstr(infoRowNo, 0, "Manual                                ");		    
	    } });

	camera.registerCallback(&cameraCallbackAIlogic);

	rtbp.aiSteeringCallback = [&](float s)
	{
		cameraCallbackAIlogic.setNNsteering(s);
		if (cameraCallbackAIlogic.autonomous)
		{
			robotController.setSteering(s);
		}
	};
	cameraCallbackAIlogic.rtbp = &rtbp;

	rtbp.setLearningRate(learningRate);

	// create an instance of the settings
	Libcam2OpenCVSettings settings;

	// set the framerate (default is variable framerate)
	settings.framerate = 30;

	cameraCallbackAIlogic.logger = fopen("log.tsv", "wt");

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
	if (cameraCallbackAIlogic.logger)
		fclose(cameraCallbackAIlogic.logger);
}
