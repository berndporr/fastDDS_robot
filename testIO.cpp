#include "zetabot.h"
#include <ncurses.h>

int running = 1;

int main (int, char **)
{
    ZetaBot zetabot;
    try
    {
        zetabot.start ();
    }
    catch (const char *tmp)
    {
        fprintf (stderr, "\n%s\n", tmp);
        abort ();
    }
    initscr ();
    noecho ();
    clear ();
    mvaddstr (
        0, 0,
        "l)eft wheel, r)ight wheel, f)orward, b)ackward, SPACE=stop, ESC=end");
    refresh ();
    float l = 0;
    float r = 0;
    char tmp[256];
    while (running)
    {
        // blocking so that the main program sleeps here
        int ch = getchar ();
        switch (ch)
        {
        case 27:
            running = 0;
            break;

        case 'l':
            l = l + 0.1f;
            zetabot.setLeftWheelSpeed (l);
            sprintf (tmp, "Increasing right speed to %f         ", l);
            mvaddstr (6, 0, tmp);
            refresh ();
            break;

        case 'r':
            r = r + 0.1f;
            zetabot.setRightWheelSpeed (r);
            sprintf (tmp, "Increasing right speed to %f         ", r);
            mvaddstr (6, 0, tmp);
            refresh ();
            break;

        case 'b':
            l = l - 0.05f;
            r = r - 0.05f;
            zetabot.setLeftWheelSpeed (l);
            zetabot.setRightWheelSpeed (r);
            sprintf (tmp, "Backwards       l=%f, r=%f                     ", l,
                     r);
            mvaddstr (6, 0, tmp);
            refresh ();
            break;

        case 'f':
            l = l + 0.05f;
            r = r + 0.05f;
            zetabot.setLeftWheelSpeed (l);
            zetabot.setRightWheelSpeed (r);
            sprintf (tmp, "Forwards       l=%f, r=%f                     ", l,
                     r);
            mvaddstr (6, 0, tmp);
            refresh ();
            break;

        case ' ':
            l = 0;
            r = 0;
            zetabot.setLeftWheelSpeed (l);
            zetabot.setRightWheelSpeed (r);
            mvaddstr (6, 0, "Stopping                            ");
            refresh ();
            break;

        default:
            break;
        }
    }
    zetabot.stop ();
    endwin ();
}
