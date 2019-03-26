#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdbool.h>
#include <stdint.h>

#include <time.h>
#include <unistd.h>
#include <errno.h>

#include <X11/Xlib.h>
#include <X11/X.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/Xrandr.h>
#include <sys/ipc.h>
#include <sys/shm.h>

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

static long long timespec_elapsed(struct timespec *before, struct timespec *after) {
  return (long long)(after->tv_sec - before->tv_sec) * 1000000000LL
    + after->tv_nsec - before->tv_nsec;
}

// static long long game_gettime_delay(clockid_t id) {
//   struct timespec before, after;
//   clock_gettime(id, &before);
//   clock_gettime(id, &after);
//   return timespec_elapsed(&before, &after);
// }

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

  XRRScreenConfiguration *conf = XRRGetScreenInfo(display, rootWindow);
  short current_rate = XRRConfigCurrentRate(conf);
  printf("Refresh rate is %d\n", current_rate);

  window = XCreateSimpleWindow(
    display, rootWindow,
    0, 0, /*pos*/
    window_width, window_height, /*width height*/
    0, /*border width*/
    0, /*border pixel*/
    0  /*background pixel*/);
  XStoreName(display, window, "Arkanoid");
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

  int major, minor, pixmaps;
  if (XShmQueryVersion(display, &major, &minor, &pixmaps)) {
    printf("SharedMemory available: %d.%d pixmaps=%d\n", major, minor, pixmaps);
  } else {
    FatalError("SharedMemory Extension is not available");
  }

  XShmSegmentInfo xshm_segment_info;
  memset(&xshm_segment_info, 0, sizeof(xshm_segment_info));

  int depth = DefaultDepth(display, screen);
  XImage *image = XShmCreateImage(display, visual, depth, ZPixmap,
                                  NULL, &xshm_segment_info,
                                  image_width, image_height);

  if (!image) {
    FatalError("Failed to create image");
  }

  xshm_segment_info.shmid = shmget(IPC_PRIVATE, image_buffer_size,
                                   IPC_CREAT|0777);
  if (xshm_segment_info.shmid < 0) {
    perror(strerror(errno));
    FatalError("Failed to get shared memory");
  }
  xshm_segment_info.shmaddr = shmat(xshm_segment_info.shmid, NULL, 0);
  if(!xshm_segment_info.shmaddr) {
    perror(strerror(errno));
    FatalError("Failed to map shared memory to process");
  }

  xshm_segment_info.readOnly = 0;

  image->data = xshm_segment_info.shmaddr;
  uint *image_buffer = (uint *) image->data;

  if (!XShmAttach(display, &xshm_segment_info)) {
    FatalError("Failed to attach XShm");
  }

  game_t game;
  game_init(&game);
  input_t input;
  input.pressed = PRESSED_NONE;
  struct timespec sleep_interval;
  sleep_interval.tv_sec = 0;
  sleep_interval.tv_nsec = 0;

  uint32_t *back_image_buffer = (uint32_t *) malloc(image_buffer_size);

//  long long clock_delay = game_gettime_delay(CLOCK_MONOTONIC);

  struct timespec frame_timing_before, frame_timing_after;
  time_t last_sec = frame_timing_before.tv_sec;
  uint fps = 0;

  clock_gettime(CLOCK_MONOTONIC, &frame_timing_before);

  while(shouldContinue) {
    X11EventLoop(&input);

    game_tick(&game, &input);

    game_render(back_image_buffer, image_width, image_height, &game);

    clock_gettime(CLOCK_MONOTONIC, &frame_timing_after);

    long long passed = timespec_elapsed(&frame_timing_before, &frame_timing_after);

    long long to_sleep = FRAME_DURATION_NANOSEC - passed;
    sleep_interval.tv_nsec = to_sleep - 100 * 1000;

    if (frame_timing_after.tv_sec == last_sec) {
      ++fps;
    } else {
      printf("\rFPS = %u              ", fps);
      fflush(stdout);
      fps = 1;
      last_sec = frame_timing_after.tv_sec;
    }

    clock_nanosleep(CLOCK_MONOTONIC, 0, &sleep_interval, NULL);
    clock_gettime(CLOCK_MONOTONIC, &frame_timing_before);

    memcpy(image_buffer, back_image_buffer, image_buffer_size);
    XShmPutImage(display, window, DefaultGC(display, screen), image,
                 0, 0, 0, 0, image_width, image_height, 0);
    XFlush(display);
  }

  XCloseDisplay(display);

  return 0;
}
