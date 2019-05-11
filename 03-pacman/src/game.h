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

#define OUTPUT_IMAGE_WIDTH (FIELD_WIDTH * TILE_SIZE_IN_PIXELS + 1)
#define OUTPUT_IMAGE_HEIGHT (FIELD_HEIGHT * TILE_SIZE_IN_PIXELS + 1)

#define BYTES(X) (X)
#define MEGABYTES(X) (1024ull * BYTES(X))
#define GIGABYTES(X) (1024ull * MEGABYTES(X))
#define TERABYTES(X) (1024ull * GIGABYTES(X))

#define ARR_LEN(arr) (sizeof(arr) / sizeof(arr[0]))

typedef struct {
  bool pressed;
} button_t;

typedef struct {
  button_t left, right, up, down;
  float seconds_elapsed;
  bool pause;
} input_t;

typedef enum {
  DIRECTION_UP = 0, DIRECTION_RIGHT, DIRECTION_DOWN, DIRECTION_LEFT, DIRECTION_NONE
} direction_t;

typedef struct {
  float x, y;
} vector2_t;

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

typedef struct {
  bool is_inited;
  moving_entity_t pacman;
} game_t;

void game_tick(void *memory, input_t *input, drawing_buffer_t *buffer);
#endif
