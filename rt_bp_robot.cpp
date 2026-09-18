#include "zetabot.h"
#include "wheel_subscriber.h"
#include <ncurses.h>
#include <iostream>

constexpr int batteryRowNo = 3;
constexpr int missedFramesRowNo = 5;
constexpr int infoRowNo = 7;
constexpr int eventRowNo = 9;

constexpr float learningRate = 0.05;

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
			zetabot.setLeftWheelSpeed(-speed + steering);
			zetabot.setRightWheelSpeed(-speed - steering);
		}
		else
		{
			zetabot.setLeftWheelSpeed(speed + steering);
			zetabot.setRightWheelSpeed(speed - steering);
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
	ZetaBot zetabot;
};

int main(int, char **)
{
	RobotSubscriber mysub;
	RobotController robotController;

	////////////////////////////////////////////////////
	// Init
	if (!mysub.init())
	{
		std::cerr << "Could not init the subscriber." << std::endl;
		return -1;
	}
	mysub.registerSteeringCallback([&](float s)
								   {
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


	robotController.zetabot.start();

	robotController.setSteering(0);
	robotController.setThrottle(0);

	/////////////////////////////////////////////////////////
	// main display loop. This is using _blocking_ getchar so
	// everything realtime is in callbacks defined above!
	initscr();
	noecho();
	clear();
	mvaddstr(0, 0, "fastDDS Racer, ESC=end");
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
	robotController.zetabot.stop();
	endwin();
}
