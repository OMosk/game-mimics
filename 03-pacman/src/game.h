#ifndef GAME_H
#define GAME_H

#include <stdbool.h>
#include <stdint.h>

typedef unsigned int uint;

#define GAME_NAME "Pacman"
#define FRAME_DURATION_NANOSEC 16600000

#define FIELD_WIDTH 27
#define FIELD_HEIGHT 31
#define TILE_SIZE_IN_PIXELS 30

#define PACMAN_VELOCITY (70.)
#define GHOST_VELOCITY (PACMAN_VELOCITY * 0.9)
#define ENTITY_SIZE_IN_PIXELS ((TILE_SIZE_IN_PIXELS * 3.) / 4.)

#define OUTPUT_IMAGE_WIDTH (FIELD_WIDTH * TILE_SIZE_IN_PIXELS + 1)
#define OUTPUT_IMAGE_HEIGHT (FIELD_HEIGHT * TILE_SIZE_IN_PIXELS + 1)

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
  bool pressed;
} button_t;

typedef struct {
  button_t left, right, up, down;
  float seconds_elapsed;
  bool pause;
  bool restart;
} input_t;

typedef enum {
  DIRECTION_UP = 0, DIRECTION_RIGHT, DIRECTION_DOWN, DIRECTION_LEFT, DIRECTION_NONE
} direction_t;

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

typedef vector2_t cell_position_t ;

typedef struct {
  uint32_t *buffer;
  uint32_t width;
  uint32_t height;
} drawing_buffer_t;

typedef struct {
  vector2_t pos;
  direction_t direction;
  direction_t next_desired_direction;
  float velocity;
} moving_entity_t;

typedef enum {
  GHOST_COLOR_RED,
  GHOST_COLOR_BLUE,
  GHOST_COLOR_GREEN,
  GHOST_COLOR_CYAN,
} ghost_color_t;

typedef enum {
  GHOST_STATE_INACTIVITY,
  GHOST_STATE_CRUISING,
  GHOST_STATE_CHAISING,
} ghost_state_t;

typedef struct {
  int pacdots_left;
  int points;
} game_stat_t;

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
  moving_entity_t movement_data;
  ghost_color_t color;
  ghost_state_t state;
  ivector2_t cruise_target;
  float time_till_cruise;
  float time_till_chase;
  float time_till_course_correction;
  float cur_time_till_course_correction;
  int32_t wave_field[FIELD_HEIGHT][FIELD_WIDTH];
  imagebuffer_t texture;
} ghost_entity_t;

typedef struct {
  moving_entity_t movement_data;
  imagebuffer_t texture;
} pacman_t;

typedef struct {
  uint8_t r, g, b, a;
} rgba_t;

typedef struct {
  bool is_inited;
  pacman_t pacman;
  game_stat_t stat;
  uint16_t level[FIELD_HEIGHT][FIELD_WIDTH];
  ghost_entity_t ghosts[4];
  bool over;
  uint game_over_color;
} game_t;

void game_tick(void *memory, input_t *input, drawing_buffer_t *buffer);

#endif
