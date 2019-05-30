#ifndef GAME_H
#define GAME_H

#include <stdbool.h>
#include <stdint.h>

typedef unsigned int uint;

#define GAME_NAME "Platformer"
#define FRAME_DURATION_NANOSEC 16600000

#define OUTPUT_IMAGE_WIDTH 800
#define OUTPUT_IMAGE_HEIGHT 600

#define BYTES(X) (X)
#define MEGABYTES(X) (1024ull * BYTES(X))
#define GIGABYTES(X) (1024ull * MEGABYTES(X))
#define TERABYTES(X) (1024ull * GIGABYTES(X))

#define ARR_LEN(arr) (sizeof(arr) / sizeof(arr[0]))

#ifndef NDEBUG
#define assert(x) do { if (!(x)) { abort(); } } while(0)
#else
#define assert(x)
#endif

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

typedef struct {
  float x, y;
} vector2_t;

typedef struct {
  int x, y;
} ivector2_t;

#define VECTOR2_ADD(a, b) ((vector2_t){(a).x + (b).x, (a).y + (b).y})
#define VECTOR2_SUB(a, b) ((vector2_t){(a).x - (b).x, (a).y - (b).y})
#define VECTOR2_MULT_NUMBER(a, b) ((vector2_t){(a).x * (b), (a).y * (b)})
#define VECTOR2_SCALAR_MULT(a, b) ((a).x * (b).x + (a).y * (b).y)
#define VECTOR2_SQR_LEN(a) VECTOR2_SCALAR_MULT(a, a)

typedef struct {
  bool pressed;
} button_t;

typedef struct {
  button_t left, right, up, down;
  float seconds_elapsed;
  bool pause;
  bool restart;
} input_t;

typedef struct {
  uint32_t *buffer;
  uint32_t width;
  uint32_t height;
} drawing_buffer_t;

typedef struct {
  vector2_t pos;
} moving_entity_t;

typedef struct {
  uint8_t *data;
  int size;
} buffer_t;

buffer_t platform_load_file(const char *filename);

typedef struct {
  uint32_t *data;
  int w, h;
} imagebuffer_t;

imagebuffer_t game_decode_bmp(buffer_t buffer);

typedef struct {
  uint8_t r, g, b, a;
} rgba_t;

typedef struct {
  bool is_inited;
  bool over;
  vector2_t pos;
  vector2_t size;
  float pixels_per_meter;
} game_t;

void game_tick(void *memory, input_t *input, drawing_buffer_t *buffer);

#endif
