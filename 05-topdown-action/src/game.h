#ifndef GAME_H
#define GAME_H

#include <stdbool.h>
#include <stdint.h>

typedef unsigned int uint;

#define GAME_NAME "TopDown"
#define FRAME_DURATION_NANOSEC 16600000

#define OUTPUT_IMAGE_WIDTH 1920
#define OUTPUT_IMAGE_HEIGHT 1080

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

typedef vector2_t segment_t[2];
typedef vector2_t ray_t[2];

typedef struct {
  int x, y;
} ivector2_t;

#define V2(x, y) ((vector2_t){(x), (y)})
#define VECTOR2_ADD(a, b) ((vector2_t){(a).x + (b).x, (a).y + (b).y})
#define VECTOR2_SUB(a, b) ((vector2_t){(a).x - (b).x, (a).y - (b).y})
#define VECTOR2_MULT_NUMBER(a, b) ((vector2_t){(a).x * (b), (a).y * (b)})
#define VECTOR2_SCALAR_MULT(a, b) ((a).x * (b).x + (a).y * (b).y)
#define VECTOR2_SQR_LEN(a) VECTOR2_SCALAR_MULT(a, a)
#define V2_LEN(a) sqrtf(VECTOR2_SCALAR_MULT(a, a))
#define V2_NORMALIZED(v) V2((v).x/V2_LEN(v), (v).y/V2_LEN(v))

typedef struct {
  bool pressed;
  uint8_t transitions;
} button_t;

typedef struct {
  button_t a, b, x, y;
  button_t select, start;
  button_t dpad_left, dpad_right, dpad_up, dpad_down;
  button_t left_bumper, right_bumper;
  float left_trigger, right_trigger;
  vector2_t left_stick, right_stick;
  button_t left_stick_button, right_stick_button;
} gamepad_input_t;

typedef struct {
  button_t left, middle, right;
} mouse_input_t;

typedef struct {
  button_t left, right, up, down;
  float seconds_elapsed;
  bool pause;
  bool restart;
  gamepad_input_t gamepad;
} input_t;

typedef struct {
  uint32_t *buffer;
  uint32_t width;
  uint32_t height;
} drawing_buffer_t;

typedef struct {
  uint8_t r, g, b, a;
} rgba_t;

typedef enum {
  ENTITY_TYPE_MC,
} entity_type_t;

typedef struct {
  vector2_t pos;
  vector2_t speed;
  vector2_t accel;
  vector2_t size;
  entity_type_t type;
  rgba_t color;
  uint32_t collision_state;
} entity_t;

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
  vector2_t camera_target;
  vector2_t current_position;
} camera_t;

typedef struct {
  imagebuffer_t image;
  ivector2_t offset;
} sprite_t;

typedef struct {
  bool is_inited;
  bool over;
  float pixels_per_meter;
  entity_t entities[256];
  uint32_t entities_count;

} game_t;

void game_tick(void *memory, input_t *input, drawing_buffer_t *buffer);

#endif
