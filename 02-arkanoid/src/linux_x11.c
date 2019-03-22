#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <time.h>
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

//Maybe I'll find a way to easily scale/stretch image to different size
//so it can fill whole window
static uint window_width = OUTPUT_IMAGE_WIDTH;
static uint window_height = OUTPUT_IMAGE_HEIGHT;

static void X11EventLoop(input_t *input) {
  XEvent event;

  int eventsUnprocessed = XPending(display);
  while (eventsUnprocessed--) {
    XNextEvent(display, &event);
    if (event.type == KeyPress) {
      if (XLookupKeysym(&event.xkey, 0) == XK_Left) {
        input->pressed = PRESSED_LEFT;
      } else if (XLookupKeysym(&event.xkey, 0) == XK_Right) {
        input->pressed = PRESSED_RIGHT;
      }
    } else if (event.type == KeyRelease) {
      if (XLookupKeysym(&event.xkey, 0) == XK_Left && input->pressed == PRESSED_LEFT) {
        input->pressed = PRESSED_NONE;
      } else if (XLookupKeysym(&event.xkey, 0) == XK_Right && input->pressed == PRESSED_RIGHT) {
        input->pressed = PRESSED_NONE;
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

static int game_tXErrorHandler(Display *display, XErrorEvent *error) {
  FatalError("Caught X11 error");
  (void)display;
  (void)error;
  abort();
  return 0;
}
static int game_tXIOErrorHandler(Display *display) {
  FatalError("Caught fatal X11 error");
  (void)display;
  return 0;
}

int main(/*int argc, char **argv*/) {
  display = XOpenDisplay(NULL);
  if (!display) {
    FatalError("Failed to open display");
  }
  XSetErrorHandler(game_tXErrorHandler);

  XSetIOErrorHandler(game_tXIOErrorHandler);

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

  long event_mask = ResizeRedirectMask | StructureNotifyMask | KeyPressMask
    | KeyReleaseMask;
  XSelectInput(display, window, event_mask);
  XMapWindow(display, window);

  Visual *visual = DefaultVisual(display, screen);

  const uint image_width = OUTPUT_IMAGE_WIDTH;
  const uint image_height = OUTPUT_IMAGE_HEIGHT;
  const uint image_buffer_size = 4 * image_width * image_height;
  uint *image_buffer = (uint *) malloc(image_buffer_size);
  bzero(image_buffer, image_buffer_size);

  int depth = DefaultDepth(display, screen);
  XImage *image = XCreateImage(display, visual, depth, ZPixmap, 0, (char *)image_buffer,
                               image_width, image_height, 32, 0);
  if (!image) {
    FatalError("Failed to create image");
  }

  game_t game;
  game_init(&game);
  input_t input;
  input.pressed = PRESSED_NONE;
  struct timespec sleep_interval;
  sleep_interval.tv_sec = 0;
  sleep_interval.tv_nsec = FRAME_DURATION_MICROSEC * 1000;

  while(shouldContinue) {
    X11EventLoop(&input);

    game_tick(&game, &input);

    Render(image_buffer, image_width, image_height, &game);

    XPutImage(display, window, DefaultGC(display, screen), image,
              0, 0, 0, 0,
              image_width, image_height);
    XFlush(display);

    nanosleep(&sleep_interval, NULL);
  }

  XCloseDisplay(display);

  return 0;
}
