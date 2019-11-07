#define _DEFAULT_SOURCE

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <linux/input-event-codes.h>
#include <linux/input.h>
#include <linux/limits.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include <X11/keysym.h>

#include "game.h"

buffer_t platform_load_file(const char *path) {
  buffer_t result = {0};

  struct stat file_stat;
  if (stat(path, &file_stat) != 0) {
    perror(strerror(errno));
    result.size = -1;
    goto exit;
  }

  result.data = mmap(0, file_stat.st_size, PROT_READ | PROT_WRITE,
                     MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

  int fd = open(path, O_RDONLY);
  result.size = read(fd, result.data, file_stat.st_size);
  if (result.size < 0) {
    perror(strerror(errno));
    result.size = -1;
    goto cleanup;
  }

cleanup:
  close(fd);

exit:
  return result;
}

void FatalError(const char *s) {
  fprintf(stderr, "%s\n", s);
  abort();
}
static Atom wmDeleteWindow;
static bool should_continue = true;
static Display *display;
static Window window;

// Maybe I'll find a way to easily scale/stretch image to different size
// so it can fill whole window
static uint window_width = OUTPUT_IMAGE_WIDTH;
static uint window_height = OUTPUT_IMAGE_HEIGHT;

static long long timespec_elapsed(struct timespec *before,
                                  struct timespec *after) {
  return (long long)(after->tv_sec - before->tv_sec) * 1000000000LL +
         after->tv_nsec - before->tv_nsec;
}

typedef struct {
  void *memory;
  input_t *inputs;
  int inputs_size_in_elements;
  int next_input_index_to_read;
  int next_input_index_to_write;
  int recorded_inputs;
  bool is_recording;
  bool is_playing;
} debug_repeat_loop_data_t;

typedef struct {
  bool start_recording;
  bool stop_recording;
  bool stop_playing;
} debug_input_t;

static void X11EventLoop(input_t *input, debug_input_t *debug_input) {
  (void)debug_input;
  XEvent event;

  input->mouse.left.transitions = 0;
  input->mouse.middle.transitions = 0;
  input->mouse.right.transitions = 0;

  input->keyboard.left.transitions = 0;
  input->keyboard.right.transitions = 0;
  input->keyboard.up.transitions = 0;
  input->keyboard.down.transitions = 0;

  input->keyboard.w.transitions = 0;
  input->keyboard.a.transitions = 0;
  input->keyboard.s.transitions = 0;
  input->keyboard.d.transitions = 0;

  input->keyboard.space.transitions = 0;
  input->keyboard.esc.transitions = 0;
  input->keyboard.enter.transitions = 0;

  int eventsUnprocessed = XPending(display);
  while (eventsUnprocessed--) {
    XNextEvent(display, &event);
    switch (event.type) {
    case KeyPress: {
      KeySym key = XLookupKeysym(&event.xkey, 0);
      switch (key) {
      case XK_Left: {
        input->keyboard.left.pressed = true;
        input->keyboard.left.transitions++;
      } break;
      case XK_Right: {
        input->keyboard.right.pressed = true;
        input->keyboard.right.transitions++;
      } break;
      case XK_Up: {
        input->keyboard.up.pressed = true;
        input->keyboard.up.transitions++;
      } break;
      case XK_Down: {
        input->keyboard.down.pressed = true;
        input->keyboard.down.transitions++;
      } break;
      case XK_w: {
        input->keyboard.w.pressed = true;
        input->keyboard.w.transitions++;
      } break;
      case XK_a: {
        input->keyboard.a.pressed = true;
        input->keyboard.a.transitions++;
      } break;
      case XK_s: {
        input->keyboard.s.pressed = true;
        input->keyboard.s.transitions++;
      } break;
      case XK_d: {
        input->keyboard.d.pressed = true;
        input->keyboard.d.transitions++;
      } break;
      case XK_space: {
        input->keyboard.space.pressed = true;
        input->keyboard.space.transitions++;
      } break;
      case XK_Escape: {
        input->keyboard.esc.pressed = true;
        input->keyboard.esc.transitions++;
      } break;
      case XK_Return: {
        input->keyboard.enter.pressed = true;
        input->keyboard.enter.transitions++;
      } break;
      case XK_p: {
        input->pause = !input->pause;
        printf("Pause is now %i \n", input->pause);
      } break;

      default: {
      }
      }
    } break;
    case KeyRelease: {
      KeySym key = XLookupKeysym(&event.xkey, 0);
      switch (key) {
      case XK_Left: {
        input->keyboard.left.pressed = false;
        input->keyboard.left.transitions++;
      } break;
      case XK_Right: {
        input->keyboard.right.pressed = false;
        input->keyboard.right.transitions++;
      } break;
      case XK_Up: {
        input->keyboard.up.pressed = false;
        input->keyboard.up.transitions++;
      } break;
      case XK_Down: {
        input->keyboard.down.pressed = false;
        input->keyboard.down.transitions++;
      } break;
      case XK_w: {
        input->keyboard.w.pressed = false;
        input->keyboard.w.transitions++;
      } break;
      case XK_a: {
        input->keyboard.a.pressed = false;
        input->keyboard.a.transitions++;
      } break;
      case XK_s: {
        input->keyboard.s.pressed = false;
        input->keyboard.s.transitions++;
      } break;
      case XK_d: {
        input->keyboard.d.pressed = false;
        input->keyboard.d.transitions++;
      } break;
      case XK_space: {
        input->keyboard.space.pressed = false;
        input->keyboard.space.transitions++;
      } break;
      case XK_Escape: {
        input->keyboard.esc.pressed = false;
        input->keyboard.esc.transitions++;
      } break;
      case XK_Return: {
        input->keyboard.enter.pressed = false;
        input->keyboard.enter.transitions++;
      } break;
      default: {
      }
      }
    } break;
    case ResizeRequest: {
      window_width = event.xresizerequest.width;
      window_width = event.xresizerequest.height;
    } break;
    case ClientMessage: {
      if ((Atom)event.xclient.data.l[0] == wmDeleteWindow) {
        should_continue = false;
        break;
      }
    } break;
    case ButtonPress: {
      switch (event.xbutton.button) {
      case Button1: {
        input->mouse.left.pressed = true;
        input->mouse.left.transitions++;
      } break;
      case Button2: {
        input->mouse.middle.pressed = true;
        input->mouse.middle.transitions++;
      } break;
      case Button3: {
        input->mouse.right.pressed = true;
        input->mouse.right.transitions++;
      } break;
      default: {
      }
      }
    } break;
    case ButtonRelease: {
      switch (event.xbutton.button) {
      case Button1: {
        input->mouse.left.pressed = false;
        input->mouse.left.transitions++;
      } break;
      case Button2: {
        input->mouse.middle.pressed = false;
        input->mouse.middle.transitions++;
      } break;
      case Button3: {
        input->mouse.right.pressed = false;
        input->mouse.right.transitions++;
      } break;
      default: {
      }
      }
    } break;
    case MotionNotify: {
      input->mouse.x = event.xmotion.x;
      input->mouse.y = event.xmotion.y;
    } break;
    default: {
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

typedef struct {
  void *handle;
  void (*game_tick)(void *memory, input_t *input, drawing_buffer_t *buffer);
} game_library_t;

void game_library_open(game_library_t *library, const char *filename) {
  dlerror();
  library->handle = dlopen(filename, RTLD_LAZY);
  char *error = dlerror();
  if (!library->handle || error) {
    FatalError(error);
  }
  dlerror();
  library->game_tick = dlsym(library->handle, "game_tick");
  error = dlerror();
  if (!library->game_tick || error) {
    FatalError(error);
  }
}

void game_library_close(game_library_t *library) {
  if (dlclose(library->handle)) {
    char *error = dlerror();
    FatalError(error);
  }
}

int event_dir_filter(const struct dirent *entry) {
  return strncmp("event", entry->d_name, 5) == 0;
}

typedef struct {
  int x_axis_min;
  int x_axis_max;
  int y_axis_min;
  int y_axis_max;
  int rx_axis_min;
  int rx_axis_max;
  int ry_axis_min;
  int ry_axis_max;
  int lt_max;
  int rt_max;
} gamepad_configuration_t;

int look_for_gamepad(gamepad_configuration_t *configuration) {
  int result_fd = -1;
  const char *directory = "/dev/input";
  struct dirent **namelist;
  int n = scandir(directory, &namelist, event_dir_filter, alphasort);
  if (n == -1) {
    return result_fd;
  }

  char path[512] = {0};

  while (n--) {
    if (result_fd < 0) {
      snprintf(path, sizeof(path) - 1, "%s/%s", directory, namelist[n]->d_name);
      int tmp_fd = open(path, O_RDONLY | O_NONBLOCK);
      if (tmp_fd > 0) {
        // permissions and stuff
        unsigned long event_bits;
        if (ioctl(tmp_fd, EVIOCGBIT(0, EV_MAX), &event_bits) != -1) {
          if (event_bits & (1 << EV_KEY)) {
            // key press supported
            unsigned long bits[KEY_MAX / (sizeof(long) * 8) + 1] = {0};
            if (ioctl(tmp_fd, EVIOCGBIT(EV_KEY, KEY_MAX), bits) != -1) {
              if ((bits[BTN_GAMEPAD / (8 * sizeof(long))] >>
                   (BTN_GAMEPAD % (8 * sizeof(long)))) &
                  1) {
                printf("Gamepad found %s\n", path);

                int abs[6] = {};
                ioctl(tmp_fd, EVIOCGABS(ABS_X), abs);
                configuration->x_axis_min = abs[1];
                configuration->x_axis_max = abs[2];

                ioctl(tmp_fd, EVIOCGABS(ABS_Y), abs);
                configuration->y_axis_min = abs[1];
                configuration->y_axis_max = abs[2];

                ioctl(tmp_fd, EVIOCGABS(ABS_RX), abs);
                configuration->rx_axis_min = abs[1];
                configuration->rx_axis_max = abs[2];

                ioctl(tmp_fd, EVIOCGABS(ABS_RY), abs);
                configuration->ry_axis_min = abs[1];
                configuration->ry_axis_max = abs[2];

                ioctl(tmp_fd, EVIOCGABS(ABS_Z), abs);
                configuration->lt_max = abs[2];

                ioctl(tmp_fd, EVIOCGABS(ABS_RZ), abs);
                configuration->rt_max = abs[2];

                result_fd = tmp_fd;
                tmp_fd = -1;
              }
            }
          }
        }
        if (tmp_fd > 0) {
          close(tmp_fd);
        }
      }
    }

    free(namelist[n]);
  }
  free(namelist);

  return result_fd;
}

void handle_joystick_input(int *fd, input_t *input,
                           gamepad_configuration_t *config) {
  input->gamepad.a.transitions = 0;
  input->gamepad.b.transitions = 0;
  input->gamepad.x.transitions = 0;
  input->gamepad.y.transitions = 0;
  input->gamepad.dpad_up.transitions = 0;
  input->gamepad.dpad_down.transitions = 0;
  input->gamepad.dpad_left.transitions = 0;
  input->gamepad.dpad_right.transitions = 0;
  input->gamepad.start.transitions = 0;
  input->gamepad.select.transitions = 0;
  input->gamepad.left_bumper.transitions = 0;
  input->gamepad.right_bumper.transitions = 0;
  input->gamepad.right_stick_button.transitions = 0;
  input->gamepad.left_stick_button.transitions = 0;

  struct input_event ev = {};
  int n;
  while ((n = read(*fd, &ev, sizeof(ev))) == sizeof(ev)) {
    switch (ev.type) {
    case EV_KEY: {
      switch (ev.code) {
      case BTN_A: {
        if (input->gamepad.a.pressed != ev.value) {
          input->gamepad.a.transitions++;
        }
        input->gamepad.a.pressed = ev.value;
      } break;
      case BTN_B: {
        if (input->gamepad.b.pressed != ev.value) {
          input->gamepad.b.transitions++;
        }
        input->gamepad.b.pressed = ev.value;
      } break;
      case BTN_X: {
        if (input->gamepad.x.pressed != ev.value) {
          input->gamepad.x.transitions++;
        }
        input->gamepad.x.pressed = ev.value;
      } break;
      case BTN_Y: {
        if (input->gamepad.y.pressed != ev.value) {
          input->gamepad.y.transitions++;
        }
        input->gamepad.y.pressed = ev.value;
      } break;
      case BTN_SELECT: {
        if (input->gamepad.select.pressed != ev.value) {
          input->gamepad.select.transitions++;
        }
        input->gamepad.select.pressed = ev.value;
      } break;
      case BTN_START: {
        if (input->gamepad.start.pressed != ev.value) {
          input->gamepad.start.transitions++;
        }
        input->gamepad.start.pressed = ev.value;
      } break;
      case BTN_DPAD_UP: {
        if (input->gamepad.dpad_up.pressed != ev.value) {
          input->gamepad.dpad_up.transitions++;
        }
        input->gamepad.dpad_up.pressed = ev.value;
      } break;
      case BTN_DPAD_DOWN: {
        if (input->gamepad.dpad_down.pressed != ev.value) {
          input->gamepad.dpad_down.transitions++;
        }
        input->gamepad.dpad_down.pressed = ev.value;
      } break;
      case BTN_DPAD_LEFT: {
        if (input->gamepad.dpad_left.pressed != ev.value) {
          input->gamepad.dpad_left.transitions++;
        }
        input->gamepad.dpad_left.pressed = ev.value;
      } break;
      case BTN_DPAD_RIGHT: {
        if (input->gamepad.dpad_right.pressed != ev.value) {
          input->gamepad.dpad_right.transitions++;
        }
        input->gamepad.dpad_right.pressed = ev.value;
      } break;
      case BTN_TR: {
        if (input->gamepad.right_bumper.pressed != ev.value) {
          input->gamepad.right_bumper.transitions++;
        }
        input->gamepad.right_bumper.pressed = ev.value;
      } break;
      case BTN_TL: {
        if (input->gamepad.left_bumper.pressed != ev.value) {
          input->gamepad.left_bumper.transitions++;
        }
        input->gamepad.left_bumper.pressed = ev.value;
      } break;

      case BTN_THUMBL: {
        if (input->gamepad.left_stick_button.pressed != ev.value) {
          input->gamepad.left_stick_button.transitions++;
        }
        input->gamepad.left_stick_button.pressed = ev.value;
      } break;
      case BTN_THUMBR: {
        if (input->gamepad.right_stick_button.pressed != ev.value) {
          input->gamepad.right_stick_button.transitions++;
        }
        input->gamepad.right_stick_button.pressed = ev.value;
      } break;
      }
    } break;
    case EV_ABS: {
      switch (ev.code) {
      case ABS_X: {
        input->gamepad.left_stick.x = ev.value;
        if (input->gamepad.left_stick.x < 0) {
          input->gamepad.left_stick.x /= (-config->x_axis_min);
        } else {
          input->gamepad.left_stick.x /= config->x_axis_max;
        }
      } break;
      case ABS_Y: {
        input->gamepad.left_stick.y = ev.value;
        if (input->gamepad.left_stick.y < 0) {
          input->gamepad.left_stick.y /= (-config->y_axis_min);
        } else {
          input->gamepad.left_stick.y /= config->y_axis_max;
        }
      } break;
      case ABS_RX: {
        input->gamepad.right_stick.x = ev.value;
        if (input->gamepad.right_stick.x < 0) {
          input->gamepad.right_stick.x /= (-config->x_axis_min);
        } else {
          input->gamepad.right_stick.x /= config->x_axis_max;
        }
      } break;
      case ABS_RY: {
        input->gamepad.right_stick.y = ev.value;
        if (input->gamepad.right_stick.y < 0) {
          input->gamepad.right_stick.y /= (-config->y_axis_min);
        } else {
          input->gamepad.right_stick.y /= config->y_axis_max;
        }
      } break;
      case ABS_Z: {
        input->gamepad.left_trigger = (float)ev.value / config->lt_max;
      } break;
      case ABS_RZ: {
        input->gamepad.right_trigger = (float)ev.value / config->rt_max;
      } break;
      case ABS_HAT0X: {
        if (ev.value == 0) {
          if (input->gamepad.dpad_left.pressed) {
            input->gamepad.dpad_left.transitions++;
          }
          input->gamepad.dpad_left.pressed = false;
          if (input->gamepad.dpad_right.pressed) {
            input->gamepad.dpad_right.transitions++;
          }
          input->gamepad.dpad_right.pressed = false;
        } else if (ev.value < 0) {
          if (!input->gamepad.dpad_left.pressed) {
            input->gamepad.dpad_left.transitions++;
          }
          input->gamepad.dpad_left.pressed = true;

          if (input->gamepad.dpad_right.pressed) {
            input->gamepad.dpad_right.transitions++;
          }
          input->gamepad.dpad_right.pressed = false;
        } else if (ev.value > 0) {
          if (!input->gamepad.dpad_right.pressed) {
            input->gamepad.dpad_right.transitions++;
          }
          input->gamepad.dpad_right.pressed = true;

          if (input->gamepad.dpad_left.pressed) {
            input->gamepad.dpad_left.transitions++;
          }
          input->gamepad.dpad_left.pressed = false;
        }
      } break;
      case ABS_HAT0Y: {
        if (ev.value == 0) {
          if (input->gamepad.dpad_up.pressed) {
            input->gamepad.dpad_up.transitions++;
          }
          input->gamepad.dpad_up.pressed = false;
          if (input->gamepad.dpad_down.pressed) {
            input->gamepad.dpad_down.transitions++;
          }
          input->gamepad.dpad_down.pressed = false;
        } else if (ev.value < 0) {
          if (!input->gamepad.dpad_up.pressed) {
            input->gamepad.dpad_up.transitions++;
          }
          input->gamepad.dpad_up.pressed = true;

          if (input->gamepad.dpad_down.pressed) {
            input->gamepad.dpad_down.transitions++;
          }
          input->gamepad.dpad_down.pressed = false;
        } else if (ev.value > 0) {
          if (!input->gamepad.dpad_down.pressed) {
            input->gamepad.dpad_down.transitions++;
          }
          input->gamepad.dpad_down.pressed = true;

          if (input->gamepad.dpad_up.pressed) {
            input->gamepad.dpad_up.transitions++;
          }
          input->gamepad.dpad_up.pressed = false;
        }
      } break;
      }
    } break;
    }
  }
  if (errno != EAGAIN) {
    close(*fd);
    *fd = -1;
  }
}

int main(int argc, char **argv, char **envp) {
  const char *game_library_name = "libgame.so";
  const char *path_to_game_library;

  (void)argc;
  (void)envp;

  {
    char exe_basedir_buffer[3 * PATH_MAX];
    char *exe_basedir;
    if (argv[0][0] == '/') {
      char *it = argv[0];
      char *write_it = exe_basedir_buffer;
      while (*it)
        *write_it++ = *it++;
      exe_basedir = dirname(exe_basedir_buffer);
    } else {
      char cwd_buffer[2 * PATH_MAX];
      char *write_it = getcwd(cwd_buffer, sizeof(cwd_buffer));
      printf("cwd: %s\n", write_it);
      while (*++write_it) {
      }
      *write_it++ = '/';

      char *it = argv[0];
      while ((*write_it++ = *it++)) {
      };
      dirname(cwd_buffer);
      it = cwd_buffer;
      write_it = exe_basedir_buffer;
      while ((*write_it++ = *it++)) {
      }
      exe_basedir = exe_basedir_buffer;
    }

    printf("%s\n", exe_basedir);

    char *it = exe_basedir;
    while (*++it != 0) {
    }
    sprintf(it, "/%s", game_library_name);
    path_to_game_library = exe_basedir;
  }

  printf("%s\n", path_to_game_library);

  display = XOpenDisplay(NULL);
  if (!display) {
    FatalError("Failed to open display");
  }
  XSetErrorHandler(game_tXErrorHandler);

  XSetIOErrorHandler(game_tXIOErrorHandler);

  Window rootWindow = XDefaultRootWindow(display);
  window = XCreateSimpleWindow(display, rootWindow, 0, 0,   /*pos*/
                               window_width, window_height, /*width height*/
                               0,                           /*border width*/
                               0,                           /*border pixel*/
                               0 /*background pixel*/);
  XStoreName(display, window, GAME_NAME);
  int screen = DefaultScreen(display);

  wmDeleteWindow = XInternAtom(display, "WM_DELETE_WINDOW", False);
  XSetWMProtocols(display, window, &wmDeleteWindow, 1);

  long event_mask = ResizeRedirectMask | StructureNotifyMask | KeyPressMask |
                    KeyReleaseMask | ButtonPressMask | ButtonReleaseMask |
                    PointerMotionMask;
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
  XImage *image =
      XShmCreateImage(display, visual, depth, ZPixmap, NULL, &xshm_segment_info,
                      image_width, image_height);

  if (!image) {
    FatalError("Failed to create image");
  }

  xshm_segment_info.shmid =
      shmget(IPC_PRIVATE, image_buffer_size, IPC_CREAT | 0777);
  if (xshm_segment_info.shmid < 0) {
    perror(strerror(errno));
    FatalError("Failed to get shared memory");
  }
  xshm_segment_info.shmaddr = shmat(xshm_segment_info.shmid, NULL, 0);
  if (!xshm_segment_info.shmaddr) {
    perror(strerror(errno));
    FatalError("Failed to map shared memory to process");
  }

  xshm_segment_info.readOnly = 0;

  image->data = xshm_segment_info.shmaddr;
  uint *image_buffer = (uint *)image->data;

  if (!XShmAttach(display, &xshm_segment_info)) {
    FatalError("Failed to attach XShm");
  }

  input_t input = {};
  {
    Window tmp;
    int tmpi;
    uint tmpu;
    // broken, returned coordinates are for root window probably
    XQueryPointer(display, rootWindow, &tmp, &tmp, &tmpi, &tmpi, &input.mouse.x,
                  &input.mouse.y, &tmpu);

    //    Pixmap bm_no;
    //    Colormap cmap;
    //    Cursor no_ptr;
    //    XColor black, dummy;
    //    static char bm_no_data[] = {0, 0, 0, 0, 0, 0, 0, 0};
    //
    //    cmap = DefaultColormap(display, DefaultScreen(display));
    //    XAllocNamedColor(display, cmap, "black", &black, &dummy);
    //    bm_no = XCreateBitmapFromData(display, window, bm_no_data, 8, 8);
    //    no_ptr = XCreatePixmapCursor(display, bm_no, bm_no, &black, &black, 0,
    //    0);
    //
    //    XDefineCursor(display, window, no_ptr);
    //    XFreeCursor(display, no_ptr);
    //    if (bm_no != None)
    //      XFreePixmap(display, bm_no);
    //    XFreeColors(display, cmap, &black.pixel, 1, 0);
  }
  struct timespec sleep_interval;
  sleep_interval.tv_sec = 0;
  sleep_interval.tv_nsec = 0;

  struct timespec frame_timing_before, frame_timing_after, game_timer,
      game_timer_old;
  clock_gettime(CLOCK_MONOTONIC, &frame_timing_before);
  time_t last_sec = frame_timing_before.tv_sec;
  uint fps = 0;


#define GAME_MEMORY_ADDRESS ((void *)(TERABYTES(32)))
#define PAGE_SIZE (4 * 1024)
//#define GAME_MEMORY_USAGE_BYTES MEGABYTES(80)
//#define GAME_MEMORY_USAGE_BYTES PAGE_SIZE * 1024 * 20
#define GAME_MEMORY_USAGE_BYTES ((MEGABYTES(80)) / PAGE_SIZE + 1) * PAGE_SIZE


  assert((uint64_t)GAME_MEMORY_ADDRESS % PAGE_SIZE == 0);
  assert(GAME_MEMORY_USAGE_BYTES % PAGE_SIZE == 0);

  void *memory =
      mmap(GAME_MEMORY_ADDRESS, GAME_MEMORY_USAGE_BYTES, PROT_READ | PROT_WRITE,
           MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, -1, 0);

  assert(memory == GAME_MEMORY_ADDRESS);
  memset(memory, 0, GAME_MEMORY_USAGE_BYTES);

  debug_repeat_loop_data_t loop_data = {};
  loop_data.memory = mmap(0, GAME_MEMORY_USAGE_BYTES, PROT_READ | PROT_WRITE,
                          MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  assert(loop_data.memory != 0);

  loop_data.inputs_size_in_elements = 60 /*FPS*/ * 120 /*seconds*/;
  int max_recorded_input_size =
      loop_data.inputs_size_in_elements * sizeof(input_t);
  loop_data.inputs = mmap(0, max_recorded_input_size, PROT_READ | PROT_WRITE,
                          MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  assert(loop_data.inputs != NULL);

  uint32_t *back_image_buffer = (uint32_t *)malloc(image_buffer_size);
  drawing_buffer_t drawing_buffer;
  drawing_buffer.buffer = back_image_buffer;
  drawing_buffer.width = image_width;
  drawing_buffer.height = image_height;

  game_library_t library = {0};
  game_library_open(&library, path_to_game_library);

  clock_gettime(CLOCK_MONOTONIC, &game_timer_old);

  struct stat game_library_file_stat_old;
  struct stat game_library_file_stat_new;
  stat(path_to_game_library, &game_library_file_stat_old);

  gamepad_configuration_t gamepad_configuration = {};
  int joystick_fd = look_for_gamepad(&gamepad_configuration);

  while (should_continue) {
    stat(path_to_game_library, &game_library_file_stat_new);
    if (game_library_file_stat_old.st_mtim.tv_sec <
            game_library_file_stat_new.st_mtim.tv_sec &&
        game_library_file_stat_new.st_mtim.tv_sec + 1 < time(NULL)) {
      game_library_close(&library);
      game_library_open(&library, path_to_game_library);
      game_library_file_stat_old = game_library_file_stat_new;
      printf("libgame.so reloaded\n");
    }

    debug_input_t debug_input = {};
    X11EventLoop(&input, &debug_input);
    clock_gettime(CLOCK_MONOTONIC, &game_timer);
    long long passed = timespec_elapsed(&game_timer_old, &game_timer);
    input.seconds_elapsed = passed * 1e-9;
    if (input.seconds_elapsed > 60.0 * 1e-9 * FRAME_DURATION_NANOSEC) {
      input.seconds_elapsed = 1e-9 * FRAME_DURATION_NANOSEC;
    }
    game_timer_old = game_timer;

    if (joystick_fd < 0) {
      joystick_fd = look_for_gamepad(&gamepad_configuration);
    }
    if (joystick_fd >= 0) {
      handle_joystick_input(&joystick_fd, &input, &gamepad_configuration);
    }

    if (debug_input.start_recording) {
      memcpy(loop_data.memory, memory, GAME_MEMORY_USAGE_BYTES);
      loop_data.next_input_index_to_write = 0;
      loop_data.is_recording = true;
      printf("Start recording\n");
    }
    if (loop_data.is_recording) {
      assert(loop_data.next_input_index_to_write <
             loop_data.inputs_size_in_elements);
      loop_data.inputs[loop_data.next_input_index_to_write++] = input;
    }
    if (debug_input.stop_recording && loop_data.is_recording) {
      loop_data.is_recording = false;
      loop_data.is_playing = true;
      loop_data.next_input_index_to_read = 0;
      loop_data.recorded_inputs = loop_data.next_input_index_to_write;
      memcpy(memory, loop_data.memory, GAME_MEMORY_USAGE_BYTES);
      printf("Stop recording/Start playing\n");
    }
    if (loop_data.is_playing) {
      input = loop_data.inputs[loop_data.next_input_index_to_read];
      loop_data.next_input_index_to_read++;
      if (loop_data.next_input_index_to_read > loop_data.recorded_inputs) {
        loop_data.next_input_index_to_read = 0;
        memcpy(memory, loop_data.memory, GAME_MEMORY_USAGE_BYTES);
      }
    }
    if (debug_input.stop_playing && loop_data.is_playing) {
      loop_data.is_playing = false;
      printf("Stop playing\n");
    }

    library.game_tick(memory, &input, &drawing_buffer);

    clock_gettime(CLOCK_MONOTONIC, &frame_timing_after);
    passed = timespec_elapsed(&frame_timing_before, &frame_timing_after);
    long long to_sleep = FRAME_DURATION_NANOSEC - passed;
    sleep_interval.tv_nsec = to_sleep - 100 * 1000;

    if (frame_timing_after.tv_sec == last_sec) {
      ++fps;
    } else {
      // printf("\rFPS = %u              ", fps);
      // fflush(stdout);
      fps = 1;
      last_sec = frame_timing_after.tv_sec;
    }

    clock_nanosleep(CLOCK_MONOTONIC, 0, &sleep_interval, NULL);
    clock_gettime(CLOCK_MONOTONIC, &frame_timing_before);

    memcpy(image_buffer, back_image_buffer, image_buffer_size);
    XShmPutImage(display, window, DefaultGC(display, screen), image, 0, 0, 0, 0,
                 image_width, image_height, 0);
    XFlush(display);
  }

  free(back_image_buffer);
  XCloseDisplay(display);

  return 0;
}
