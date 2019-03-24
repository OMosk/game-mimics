#ifndef GAME_H
#define GAME_H

#include <stdbool.h>

typedef unsigned int uint;

typedef enum {PRESSED_NONE, PRESSED_LEFT, PRESSED_RIGHT} Pressed;
typedef struct {
  Pressed pressed;
} input_t;

typedef enum {
  DIRECTION_UP = 0, DIRECTION_RIGHT, DIRECTION_DOWN, DIRECTION_LEFT
} direction_t;

#define TILES_X 40
#define TILES_Y 60
#define TILE_SIZE 15
#define PLATFORM_WIDTH_IN_TILES 10
#define PLATFORM_HEIGHT_IN_TILES 2
#define PLATFORM_WIDTH_IN_PIXELS (PLATFORM_WIDTH_IN_TILES * TILE_SIZE)
#define PLATFORM_HEIGHT_IN_PIXELS (PLATFORM_HEIGHT_IN_TILES * TILE_SIZE)
#define PLATFORM_BOTTOM_MARGIN TILE_SIZE

#define DESIRED_FPS 60
#define SIMULATIONS_PER_FRAME 4
#define FRAME_DURATION_MICROSEC  16 * 1000
#define INITIAL_BALL_VELOCITY 8.f

#define OUTPUT_IMAGE_WIDTH (TILES_X * TILE_SIZE + 1)
#define OUTPUT_IMAGE_HEIGHT (TILES_Y * TILE_SIZE + 1)

#define TILE_TO_OUTPUT_IMAGE_WIDTH_RATIO ((float)TILE_SIZE / OUTPUT_IMAGE_WIDTH)
#define TILE_TO_OUTPUT_IMAGE_HEIGHT_RATIO ((float)TILE_SIZE / OUTPUT_IMAGE_HEIGHT)

#define TILE_HORIZONTAL_PADDING ((float)2 / OUTPUT_IMAGE_WIDTH)
#define TILE_VERTICAL_PADDING ((float)2 / OUTPUT_IMAGE_HEIGHT)

#define CELLS_ROWS 4
#define CELLS_COLUMNS 20

typedef struct {
  float x, y;
} vector2_t;

typedef vector2_t cell_position_t ;

typedef struct {
  float platform_position;
  cell_position_t cells[CELLS_ROWS * CELLS_COLUMNS];
  bool cells_destroyed[CELLS_ROWS * CELLS_COLUMNS];
  cell_position_t ball_pos;
  vector2_t ball_direction;
  float ball_velocity;
  bool game_over;
} game_t;

void game_init(game_t *game);
void game_tick(game_t *game, input_t *input);

void game_render(uint *buffer, uint image_width, uint image_height, game_t *game);

#endif
