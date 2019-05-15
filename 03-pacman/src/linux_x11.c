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
#include <dlfcn.h>
#include <libgen.h>
#include <linux/limits.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <X11/Xlib.h>
#include <X11/X.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/extensions/XShm.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "game.h"

#ifndef NDEBUG
#define assert(x) do { if (!(x)) { abort(); } } while(0)
#else
#define assert(x)
#endif

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
  XEvent event;

  int eventsUnprocessed = XPending(display);
  while (eventsUnprocessed--) {
    XNextEvent(display, &event);
    if (event.type == KeyPress) {
      KeySym key = XLookupKeysym(&event.xkey, 0);
      if (key == XK_Left) {
        input->left.pressed = true;
      } else if (key == XK_Right) {
        input->right.pressed = true;
      } else if (key == XK_Up) {
        input->up.pressed = true;
      } else if (key == XK_Down) {
        input->down.pressed = true;
      } else if (key == XK_p) {
        input->pause = !input->pause;
        printf("Pause is now %i \n", input->pause);
      }
    } else if (event.type == KeyRelease) {
      KeySym key = XLookupKeysym(&event.xkey, 0);
      if (key == XK_Left) {
        input->left.pressed = false;
      } else if (key == XK_Right) {
        input->right.pressed = false;
      } else if (key == XK_Up) {
        input->up.pressed = false;
      } else if (key == XK_Down) {
        input->down.pressed = false;
      } else if (key == XK_F2) {
        debug_input->start_recording = true;
      } else if (key == XK_F3) {
        debug_input->stop_recording = true;
      } else if (key == XK_F4) {
        debug_input->stop_playing = true;
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

int main(int argc, char **argv) {
  const char *game_library_name = "libgame.so";
  const char *path_to_game_library;

  (void)argc;

  {
    char exe_basedir_buffer[3 * PATH_MAX];
    char *exe_basedir;
    if (argv[0][0] == '/') {
      char *it = argv[0];
      char *write_it = exe_basedir_buffer;
      while (*it) *write_it++ = *it++;
      exe_basedir = dirname(exe_basedir_buffer);
    } else {
      char cwd_buffer[2 * PATH_MAX];
      char *write_it = getcwd(cwd_buffer, sizeof(cwd_buffer));
      printf("cwd: %s\n", write_it);
      while (*++write_it) {}
      *write_it++ = '/';

      char *it = argv[0];
      while ((*write_it++ = *it++)) {};
      exe_basedir = dirname(cwd_buffer);
      it = cwd_buffer;
      write_it = exe_basedir_buffer;
      while ((*write_it++ = *it++)) {}
      exe_basedir = exe_basedir_buffer;
    }

    printf("%s\n", exe_basedir);

    char *it = exe_basedir;
    while (*++it != 0) {}
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
  window = XCreateSimpleWindow(
    display, rootWindow,
    0, 0, /*pos*/
    window_width, window_height, /*width height*/
    0, /*border width*/
    0, /*border pixel*/
    0  /*background pixel*/);
  XStoreName(display, window, GAME_NAME);
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

  input_t input = {};
  struct timespec sleep_interval;
  sleep_interval.tv_sec = 0;
  sleep_interval.tv_nsec = 0;

  struct timespec frame_timing_before, frame_timing_after, game_timer, game_timer_old;
  time_t last_sec = frame_timing_before.tv_sec;
  uint fps = 0;

  clock_gettime(CLOCK_MONOTONIC, &frame_timing_before);

#define GAME_MEMORY_ADDRESS ((void *)(TERABYTES(32)))
#define GAME_MEMORY_USAGE_BYTES MEGABYTES(4)
#define PAGE_SIZE (4 * 1024)

  assert((uint64_t)GAME_MEMORY_ADDRESS % PAGE_SIZE == 0);
  assert(GAME_MEMORY_USAGE_BYTES % PAGE_SIZE == 0);


  void *memory = mmap(GAME_MEMORY_ADDRESS, GAME_MEMORY_USAGE_BYTES,
                      PROT_READ | PROT_WRITE,
                      MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED,
                      -1, 0);

  assert(memory == GAME_MEMORY_ADDRESS);
  memset(memory, 0, GAME_MEMORY_USAGE_BYTES);

  debug_repeat_loop_data_t loop_data = {};
  loop_data.memory = mmap(0, GAME_MEMORY_USAGE_BYTES,
                                  PROT_READ | PROT_WRITE,
                                  MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  assert(loop_data.memory != 0);

  loop_data.inputs_size_in_elements = 60/*FPS*/ * 120/*seconds*/;
  int max_recorded_input_size = loop_data.inputs_size_in_elements * sizeof(input_t);
  loop_data.inputs = mmap(0, max_recorded_input_size,
                                  PROT_READ | PROT_WRITE,
                                  MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  assert(loop_data.inputs != NULL);

  uint32_t *back_image_buffer = (uint32_t *) malloc(image_buffer_size);
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

  while(shouldContinue) {
    stat(path_to_game_library, &game_library_file_stat_new);
    if (game_library_file_stat_old.st_mtim.tv_sec < game_library_file_stat_new.st_mtim.tv_sec
        && game_library_file_stat_new.st_mtim.tv_sec + 1 < time(NULL)) {
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
    game_timer_old = game_timer;

    if (debug_input.start_recording) {
      memcpy(loop_data.memory, memory, GAME_MEMORY_USAGE_BYTES);
      loop_data.next_input_index_to_write = 0;
      loop_data.is_recording = true;
      printf("Start recording\n");
    }
    if (loop_data.is_recording) {
      assert(loop_data.next_input_index_to_write < loop_data.inputs_size_in_elements);
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
      //printf("\rFPS = %u              ", fps);
      //fflush(stdout);
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
