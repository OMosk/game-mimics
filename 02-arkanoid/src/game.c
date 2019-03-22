#include "game.h"

#define _DEFAULT_SOURCE
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <time.h>

//                              B     G            R
static const uint kColorWhite = 255 + (255 << 8) + (255 << 16);
//static const uint kColorGrey  = 150 + (150 << 8) + (150 << 16);
static const uint kColorGreen = 0 + (255 << 8) + (0 << 16);
//static const uint kColorRed = 0 + (0 << 8) + (255 << 16);

static void draw_cell(uint x, uint y, uint *buffer, uint image_width) {
  for (uint i = y; i < y + TILE_SIZE; ++i) {
    for (uint j = x; j < x + TILE_SIZE; ++j) {
      buffer[i * image_width + j] = kColorWhite;
    }
  }
}

static void
draw_platform(uint x, uint y, uint *buffer, uint image_width) {
  for (uint i = y; i < y + PLATFORM_HEIGHT_IN_PIXELS; ++i) {
    for (uint j = x; j < x + PLATFORM_WIDTH_IN_PIXELS; ++j) {
      buffer[i * image_width + j] = kColorGreen;
    }
  }
}

void Render(uint *buffer, uint image_width, uint image_height, game_t *game) {
  (void)game;
  bzero(buffer, image_width * image_height * sizeof(uint));

  uint y = OUTPUT_IMAGE_HEIGHT - PLATFORM_HEIGHT_IN_PIXELS - TILE_SIZE;
  uint x = (OUTPUT_IMAGE_WIDTH - PLATFORM_WIDTH_IN_PIXELS) * game->platform_position;

  draw_platform(x, y, buffer, image_width);

  for (uint i = 0; i < sizeof(game->cells)/sizeof(game->cells[0]); ++i) {
    cell_position_t *cell = &game->cells[i];
    uint x = (OUTPUT_IMAGE_WIDTH - TILE_SIZE) * cell->x;
    uint y = (OUTPUT_IMAGE_HEIGHT - TILE_SIZE) * cell->y;
    draw_cell(x, y, buffer, image_width);
  }
}

void game_init(game_t *game) {
  game->platform_position = 0.;

  for (int i = 0; i < CELLS_ROWS; ++i) {
    for (int j = 0; j < CELLS_COLUMNS; ++j) {
      game->cells[i * CELLS_COLUMNS + j].x = j * 1. / CELLS_COLUMNS;
      game->cells[i * CELLS_COLUMNS + j].y = 0.1 + i * 0.3;
    }
  }
}

void game_tick(game_t *game, input_t *input) {
  //    if (game->game_over) return;

  switch (input->pressed) {
  case PRESSED_LEFT:
    game->platform_position -= 2. / DESIRED_FPS;
    break;
  case PRESSED_RIGHT:
    game->platform_position += 2. / DESIRED_FPS;
    break;
  case PRESSED_NONE:
    break;
  }

  if (game->platform_position < 0.) game->platform_position = 0.;
  if (game->platform_position > 1.) game->platform_position = 1.;
}


