#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/X.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include "game.h"

void FatalError(const char *s) {
    fprintf(stderr, "%s\n", s);
    abort();
}
static Atom wmDeleteWindow;
static bool shouldContinue = true;
static Display *display;
static Window window;
static const uint width = 301;
static const uint height = 301;

//Maybe I'll find a way to easily scale/stretch image to different size
//so it can fill whole window
static uint window_width = width;
static uint window_height = height;

void X11EventLoop(Input *input) {
    XEvent event;

    int eventsUnprocessed = XPending(display);
    while (eventsUnprocessed--) {
        XNextEvent(display, &event);
        if (event.type == KeyPress) {
            if (XLookupKeysym(&event.xkey, 0) == XK_Left) {
                input->pressed = Input::Pressed::LEFT;
            } else if (XLookupKeysym(&event.xkey, 0) == XK_Right) {
                input->pressed = Input::Pressed::RIGHT;
            }
        } else if (event.type==ResizeRequest) {
            window_width = event.xresizerequest.width;
            window_width = event.xresizerequest.height;
        } else if (event.type == ClientMessage) {
            if ((Atom)event.xclient.data.l[0] == wmDeleteWindow) {
                shouldContinue = false;
                break;
            }
        }
    }
}


int main(/*int argc, char **argv*/) {
    display = XOpenDisplay(nullptr);
    if (!display) {
        FatalError("Failed to open display");
    }
    XSetErrorHandler([](Display *, XErrorEvent *error) -> int {
        FatalError("Caught X11 error");
        (void)error;
        abort();
        return 0;
    });

    XSetIOErrorHandler([](Display *display) -> int {
        FatalError("Caught fatal X11 error");
        (void)display;
        return 0;
    });

    Window rootWindow = XDefaultRootWindow(display);
    window = XCreateSimpleWindow(
        display, rootWindow,
        0, 0, /*pos*/
        window_width, window_height, /*width height*/
        0, /*border width*/
        0, /*border pixel*/
        0  /*background pixel*/);
    XStoreName(display, window, "Snake");
    int screen = DefaultScreen(display);

    wmDeleteWindow = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, window, &wmDeleteWindow, 1);

    XSelectInput(display, window, ResizeRedirectMask | StructureNotifyMask | KeyPressMask);
    XMapWindow(display, window);

    Visual *visual = DefaultVisual(display, screen);

    const uint image_width = width;
    const uint image_height = height;
    const uint image_buffer_size = 4 * image_width * image_height;
    uint *image_buffer = (uint *) malloc(image_buffer_size);
    bzero(image_buffer, image_buffer_size);

    int depth = DefaultDepth(display, screen);
    XImage *image = XCreateImage(display, visual, depth, ZPixmap, 0, (char *)image_buffer,
                                 image_width, image_height, 32, 0);
    if (!image) {
        FatalError("Failed to create image");
    }

    Game game;
    Input input;

    while(shouldContinue) {
        input.pressed = Input::Pressed::NONE;
        X11EventLoop(&input);

        game.Tick(&input);

        Render(image_buffer, image_width, image_height, &game);

        XPutImage(display, window, DefaultGC(display, screen), image,
                  0, 0, 0, 0,
                  image_width, image_height);
        XFlush(display);

        usleep(kTurnMicroSec);
    }

    XCloseDisplay(display);

    return 0;
}
