#include "game.h"

#define _DEFAULT_SOURCE
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <math.h>

//                              B     G            R
static const uint kColorWhite = 255 + (255 << 8) + (255 << 16);
//static const uint kColorGrey  = 150 + (150 << 8) + (150 << 16);
static const uint kColorGreen = 0 + (255 << 8) + (0 << 16);
static const uint kColorRed = 0 + (0 << 8) + (255 << 16);

static void draw_cell(uint x, uint y, uint *buffer, uint image_width) {
  for (uint i = y; i < y + TILE_SIZE; ++i) {
    for (uint j = x; j < x + TILE_SIZE; ++j) {
      buffer[i * image_width + j] = kColorWhite;
    }
  }
}

static void draw_ball(uint x, uint y, uint *buffer, uint image_width, uint image_height) {
  for (uint i = y; i < y + TILE_SIZE && i < image_height ; ++i) {
    for (uint j = x; j < x + TILE_SIZE && j < image_width; ++j) {
      buffer[i * image_width + j] = kColorRed;
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

vector2_t vector2_t_normalize(vector2_t v) {
  float length = sqrtf(v.x*v.x + v.y*v.y);
  v.x /= length;
  v.y /= length;
  return v;
}

void game_render(uint *buffer, uint image_width, uint image_height, game_t *game) {
  // NOTE: most time we spend here currently
  memset(buffer, 0, image_width * image_height * sizeof(uint));

  uint y = OUTPUT_IMAGE_HEIGHT - PLATFORM_HEIGHT_IN_PIXELS - PLATFORM_BOTTOM_MARGIN;
  uint x = (OUTPUT_IMAGE_WIDTH - PLATFORM_WIDTH_IN_PIXELS) * game->platform_position;

  draw_platform(x, y, buffer, image_width);

  for (uint i = 0; i < sizeof(game->cells)/sizeof(game->cells[0]); ++i) {
    if (game->cells_destroyed[i]) continue;

    cell_position_t *cell = &game->cells[i];
    uint x = (OUTPUT_IMAGE_WIDTH - TILE_SIZE) * cell->x;
    uint y = (OUTPUT_IMAGE_HEIGHT - TILE_SIZE) * cell->y;
    draw_cell(x, y, buffer, image_width);
  }

  x = game->ball_pos.x;
  y = game->ball_pos.y;
  draw_ball(x, y, buffer, image_width, image_height);
}

void game_init(game_t *game) {
  game->platform_position = 0.5f;

  float horizontal_alignment_margins = 1.f
    - (CELLS_COLUMNS * TILE_TO_OUTPUT_IMAGE_WIDTH_RATIO)
    - ((CELLS_COLUMNS - 1) * TILE_HORIZONTAL_PADDING);
  horizontal_alignment_margins /= 2.f;

  for (int i = 0; i < CELLS_ROWS; ++i) {
    for (int j = 0; j < CELLS_COLUMNS; ++j) {
      game->cells[i * CELLS_COLUMNS + j].x = horizontal_alignment_margins
        + j * (TILE_TO_OUTPUT_IMAGE_WIDTH_RATIO + TILE_HORIZONTAL_PADDING);
      game->cells[i * CELLS_COLUMNS + j].y
        = 0.1 + i * (TILE_TO_OUTPUT_IMAGE_HEIGHT_RATIO + TILE_VERTICAL_PADDING);
    }
  }

  memset(game->cells_destroyed, 0, sizeof(game->cells_destroyed));

  game->ball_pos.x = (OUTPUT_IMAGE_WIDTH - TILE_SIZE) * 0.5f;
  game->ball_pos.y = (OUTPUT_IMAGE_HEIGHT - TILE_SIZE) * 0.5f;

  game->ball_direction.x = 0.0f;//0.707f;
  game->ball_direction.y = 0.707f;
  game->ball_velocity = INITIAL_BALL_VELOCITY;

  game->game_over = false;
}

typedef enum {
  COLLISION_NONE, COLLISION_LEFT, COLLISION_RIGHT, COLLISION_TOP,
  COLLISION_BOTTOM
} collision_t;

float crossproduct(vector2_t p, vector2_t l[2]) {
  return ((p.x-l[0].x)*(l[1].y-l[0].y)) - ((p.y-l[0].y)*(l[1].x-l[0].x));
}

collision_t calculate_collision(vector2_t ball[5], vector2_t cell[4]) {
  bool in = false;
  for (int i = 0; i < 4; ++i) { //vertices
    in = in || ((ball[i].x > cell[0].x) & (ball[i].x < cell[1].x)
      & (ball[i].y > cell[0].y) & (ball[i].y < cell[3].y));
  }
  if (!in) return COLLISION_NONE;

  bool top_or_right = (crossproduct(ball[4], (vector2_t[]) {cell[0], cell[2]}) > 0.f);
  bool bottom_or_right = (crossproduct(ball[4], (vector2_t[]) {cell[1], cell[3]}) > 0.f);

  if (top_or_right) {
    if (bottom_or_right) {
      return COLLISION_RIGHT;
    } else {
      return COLLISION_TOP;
    }
  } else {
    //BOTTOM_OR_LEFT
    if (bottom_or_right) {
      return COLLISION_BOTTOM;
    } else {
      return COLLISION_LEFT;
    }
  }
}

void game_tick(game_t *game, input_t *input) {
  if (game->game_over) return;

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

  float simulation_subframe = 1. / (SIMULATIONS_PER_FRAME);
  for (int i = 0; i < SIMULATIONS_PER_FRAME; ++i) {

    // Borders collision
    if (game->ball_pos.x < 0.f) {
      game->ball_pos.x *= -1;
      game->ball_direction.x *= -1;
    }
    if (game->ball_pos.x + TILE_SIZE > OUTPUT_IMAGE_WIDTH) {
      game->ball_pos.x = (2.f*OUTPUT_IMAGE_WIDTH - game->ball_pos.x - 2.f * TILE_SIZE);
      game->ball_direction.x *= -1;
    }
    if (game->ball_pos.y < 0.f) {
      game->ball_pos.y *= -1;
      game->ball_direction.y *= -1;
    }
    if (game->ball_pos.y + TILE_SIZE > OUTPUT_IMAGE_HEIGHT) {
      game->game_over = true;
      game->ball_pos.y = (2.f * OUTPUT_IMAGE_HEIGHT - game->ball_pos.y - 2.f * TILE_SIZE);
    }


    //Platform collision
    vector2_t platform_top_left, platform_top_right,
              platform_bottom_left, platform_bottom_right;

    platform_top_left.x = game->platform_position
      * (OUTPUT_IMAGE_WIDTH - PLATFORM_WIDTH_IN_PIXELS);
    platform_top_left.y = OUTPUT_IMAGE_HEIGHT
      - PLATFORM_BOTTOM_MARGIN - PLATFORM_HEIGHT_IN_PIXELS;

    platform_top_right.x = platform_top_left.x + PLATFORM_WIDTH_IN_PIXELS;
    platform_top_right.y = platform_top_left.y;

    platform_bottom_right.x = platform_top_left.x;
    platform_bottom_right.y = platform_top_left.y + PLATFORM_HEIGHT_IN_PIXELS;

    platform_bottom_left.x = platform_top_left.x - PLATFORM_WIDTH_IN_PIXELS;
    platform_bottom_left.y = platform_bottom_right.y;

    vector2_t ball_top_left, ball_top_right, ball_bottom_left, ball_bottom_right,
              ball_center;

    ball_top_left.x = game->ball_pos.x;
    ball_top_left.y = game->ball_pos.y;

    ball_top_right.x = ball_top_left.x + TILE_SIZE;
    ball_top_right.y = ball_top_left.y;

    ball_bottom_right.x = ball_top_right.x;
    ball_bottom_right.y = ball_top_right.y + TILE_SIZE;

    ball_bottom_left.x = ball_top_left.x;
    ball_bottom_left.y = ball_top_left.y - TILE_SIZE;

    ball_center.x = (ball_top_left.x + ball_bottom_right.x) / 2.f;
    ball_center.y = (ball_top_left.y + ball_bottom_right.y) / 2.f;

    vector2_t ball_position_vec[5] = { ball_top_left, ball_top_right,
      ball_bottom_right, ball_bottom_left, ball_center
    };

    collision_t collision_with_platform = calculate_collision(
      ball_position_vec,
      (vector2_t[]){platform_top_left, platform_top_right, platform_bottom_right,
        platform_bottom_left}
      );

    switch (collision_with_platform) {
    case COLLISION_LEFT:
    case COLLISION_RIGHT:
      game->ball_direction.x *= -1;
      game->ball_pos.x += game->ball_direction.x * game->ball_velocity;
      break;
    case COLLISION_TOP: {
        float pos_ratio = (ball_center.x - platform_top_left.x) / PLATFORM_WIDTH_IN_PIXELS;
        pos_ratio = (pos_ratio < 0.f ? 0.f : pos_ratio);
        pos_ratio = (pos_ratio > 1.f ? 1.f : pos_ratio);
        game->ball_direction.y *= -1;
        game->ball_direction.x = game->ball_direction.x + (pos_ratio - 0.5f)*0.5f;
        game->ball_direction = vector2_t_normalize(game->ball_direction);
        game->ball_pos.y += game->ball_direction.y * game->ball_velocity;
        break;
      }
    default:
      break;
    }

    for (uint i = 0; i < sizeof(game->cells)/sizeof(game->cells[0]); ++i) {
      if (game->cells_destroyed[i]) continue;
      cell_position_t *cell = &game->cells[i];
      vector2_t top_left, top_right, bottom_left, bottom_right;
      top_left.x = (OUTPUT_IMAGE_WIDTH - TILE_SIZE) * cell->x;
      top_left.y = (OUTPUT_IMAGE_HEIGHT - TILE_SIZE) * cell->y;

      top_right.x = top_left.x + TILE_SIZE;
      top_right.y = top_left.y;

      bottom_right.x = top_right.x;
      bottom_right.y = top_right.y + TILE_SIZE;

      bottom_left.x = top_left.x;
      bottom_left.y = bottom_right.y;

      collision_t collision = calculate_collision(
        ball_position_vec,
        (vector2_t[]){top_left, top_right, bottom_right, bottom_left});

      if (collision == COLLISION_NONE) continue;

      switch (collision) {
      case COLLISION_LEFT:
      case COLLISION_RIGHT:
        game->ball_direction.x *= -1.f;
        //TODO: better bounce implementation
        game->ball_pos.x += game->ball_direction.x * game->ball_velocity;
        break;
      case COLLISION_BOTTOM:
      case COLLISION_TOP:
        game->ball_direction.y *= -1.f;
        game->ball_pos.y += game->ball_direction.y * game->ball_velocity;
        break;
      default:
        break;
      }

      game->cells_destroyed[i] = true;
      game->ball_velocity *= 1.01f;
      break;
    }

    game->ball_pos.x += game->ball_direction.x * game->ball_velocity * simulation_subframe;
    game->ball_pos.y += game->ball_direction.y * game->ball_velocity * simulation_subframe;
  }
}


