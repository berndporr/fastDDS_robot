#include "wheel_subscriber.h"
#include "zetabot.h"
#include <iostream>
#include <ncurses.h>

struct RobotController
{
    void send2motors ()
    {
        float l = speed + steering;
        if (l < 0)
            l = 0;
        float r = speed - steering;
        if (r < 0)
            r = 0;
        if (reverse)
        {
            zetabot.setLeftWheelSpeed (-speed + steering * speed);
            zetabot.setRightWheelSpeed (-speed - steering * speed);
        }
        else
        {
            zetabot.setLeftWheelSpeed (speed + steering * speed);
            zetabot.setRightWheelSpeed (speed - steering * speed);
        }
    }

    void setThrottle (float t)
    {
        speed = t;
        send2motors ();
    }

    void setSteering (float s)
    {
        steering = s;
        send2motors ();
    }

    float speed = 0;
    float steering = 0;
    bool reverse = false;
    ZetaBot zetabot;
};

struct InfoScreen
{
    static constexpr int steeringRowNo = 7;
    static constexpr int throttleRowNo = 9;

    /**
     * Inits the info screen.
     * Sleeping till the user presses ESC.
     **/
    void run ()
    {
        initscr ();
        noecho ();
        clear ();
        mvaddstr (0, 0, "fastDDS Robot. Press any key to end.");
        refresh ();
        printSteering (0);
        printThrottle (0);
        // sleeping till a key is pressed.
        getchar ();
        endwin ();
    }
    void printSteering (float s)
    {
        char tmp[256];
        sprintf (tmp, "Steering: %f", s);
        mvaddstr (steeringRowNo, 0, tmp);
        refresh ();
    }
    void printThrottle (float t)
    {
        char tmp[256];
        sprintf (tmp, "Throttle: %f", t);
        mvaddstr (throttleRowNo, 0, tmp);
        refresh ();
    }
};

int main (int, char **)
{
    RobotSubscriber mysub;
    RobotController robotController;
    InfoScreen infoScreen;

    ////////////////////////////////////////////////////
    // Init
    if (!mysub.init ())
    {
        std::cerr << "Could not init the subscriber." << std::endl;
        return -1;
    }
    mysub.registerSteeringCallback ([&] (float s) {
        robotController.setSteering (s);
        infoScreen.printSteering (s);
    });
    mysub.registerThrottleCallback ([&] (float t) {
        robotController.setThrottle (t);
        infoScreen.printThrottle (t);
    });

    robotController.zetabot.start ();

    robotController.setSteering (0);
    robotController.setThrottle (0);

    // sleeps till the user presses ESC
    infoScreen.run ();

    // stopping the robot
    robotController.zetabot.stop ();
}
