#ifndef __LOGIWHEEL
#define __LOGIWHEEL

#include <thread>
#include <functional>
#include <string>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <linux/joystick.h>
#include <sys/select.h>

class LogiWheel {

public:

    // Navigation buttons on the steering wheel
    static constexpr int BUTTON_LEFT = 100;
    static constexpr int BUTTON_RIGHT = 101;
    static constexpr int BUTTON_UP = 102;
    static constexpr int BUTTON_DOWN = 103;
    static constexpr char DEFAULT_DEVICE[] =
	"/dev/input/by-id/usb-Logitech_Logitech_Racing_Wheel-event-joystick";

    using AnalogueCallback = std::function<void(float)>;
    using ButtonCallback = std::function<void(int)>;
    
    void registerSteeringCallback(AnalogueCallback cb) {
	steeringCallback = cb;
    }

    void registerThrottleCallback(AnalogueCallback cb) {
	throttleCallback = cb;
    }

    void registerBrakeCallback(AnalogueCallback cb) {
	brakeCallback = cb;
    }

    void registerButtonCallback(ButtonCallback cb) {
	buttonCallback = cb;
    }

    bool start(std::string device = DEFAULT_DEVICE) {
	if (running) return true;

        fd = open(device.c_str(), O_RDONLY);
	
	if (fd == -1) {
	    return false;
	}
	eventThread = std::thread(&LogiWheel::run, this);
	return true;
    }
    
    void stop() {
	running = false;
	eventThread.join();
    }

private:

    void run() {
	running = true;
	while (running) 
	{
	    fd_set s_rd;
	    FD_ZERO(&s_rd);
	    FD_SET(fd, &s_rd);
	    int r = select(fd+1, &s_rd, NULL, NULL, &timeout);
	    if (r > 0) {
		r = read(fd, &event, sizeof(event));
		if (r < 0) {
		    fprintf(stderr,"Logi Wheel err = %d\n",r);
		    running = false;
		    close(fd);
		    return;
		}
//		fprintf(stderr,
//			"type = %d, code =%d, value=%d\n",event.type,event.code,event.value);
		if (event.type == EV_ABS && event.code < ABS_TOOL_WIDTH) {
		    int v = event.value;
		    switch (event.code) {
		    case 0:
			if (steeringCallback) steeringCallback((v - 512)/512.0);
			break;
		    case 1:
			if (throttleCallback) throttleCallback(1-v/256.0);
			break;
		    case 2:
			if (brakeCallback) brakeCallback(1-v/256.0);
			break;
		    case 16:
			if (buttonCallback)
			    switch (event.value) {
			    case -1:
				buttonCallback(BUTTON_LEFT);
				break;
			    case 1:
				buttonCallback(BUTTON_RIGHT);
				break;
			    }
			break;
		    case 17:
			if (buttonCallback)
			    switch (event.value) {
			    case -1:
				buttonCallback(BUTTON_UP);
				break;
			    case 1:
				buttonCallback(BUTTON_DOWN);
				break;
			    }
			break;
		    }
		} else if ( (event.type == EV_KEY) && (event.value == 1) ){
		    if (buttonCallback) buttonCallback(event.code - 287);
		}
	    } else if (r < 0) running = false;
	}
	running = false;
	close(fd);
    }

    bool running = false;
    int fd = -1;
    input_event event;
    std::thread eventThread;
    timeval timeout = {1,1};

    AnalogueCallback steeringCallback;
    AnalogueCallback throttleCallback;
    AnalogueCallback brakeCallback;
    ButtonCallback buttonCallback;
};

#endif
