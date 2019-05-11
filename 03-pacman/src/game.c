#include "game.h"

#define _DEFAULT_SOURCE
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <math.h>
#include <float.h>

//static const uint16_t empty_space = 0;
static const uint16_t pacdot = 1;
static const uint16_t wall = 2;
static const uint16_t pacman = 3;

uint16_t level[FIELD_HEIGHT][FIELD_WIDTH] = {
  {2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,},
  {2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,},
  {2,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,},
  {2,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,},
  {2,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,},
  {2,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,},
  {2,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,},
  {2,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,},
  {2,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,},
  {2,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,},
  {2,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,},
  {2,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,},
  {2,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,},
  {2,0,1,0,0,0,0,0,0,0,0,0,0,3,0,0,0,0,0,0,0,0,0,0,0,0,2,},
  {2,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,},
  {2,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,},
  {2,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,},
  {2,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,},
  {2,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,},
  {2,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,},
  {2,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,},
  {2,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,},
  {2,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,},
  {2,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,},
  {2,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,},
  {2,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,},
  {2,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,},
  {2,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,},
  {2,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,},
  {2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,},
  {2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,},
};

//                              B     G            R
static const uint kColorWhite = 255 + (255 << 8) + (255 << 16);
static const uint kColorYellow = 0 + (255 << 8) + (255 << 16);
//static const uint kColorGrey  = 150 + (150 << 8) + (150 << 16);
//static const uint kColorGreen = 0 + (255 << 8) + (0 << 16);
static const uint kColorRed = 0 + (0 << 8) + (255 << 16);

static void draw_rectangle(drawing_buffer_t *buffer, int x, int y,
                           int width, int height, uint32_t color) {
  int x2 = x + width;
  int y2 = y + height;
  if (x < 0) x = 0;
  if (y < 0) y = 0;
  if (x2 >= (int)buffer->width) x2 = buffer->width - 1;
  if (y2 >= (int)buffer->height) y2 = buffer->height - 1;

  for (int i = y; i < y2; ++i) {
    for (int j = x; j < x2; ++j) {
      buffer->buffer[i * buffer->width + j] = color;
    }
  }
}

static void draw_grid(drawing_buffer_t *buffer) {
  for (int i = 0; i <= FIELD_HEIGHT; ++i) {
    for (int j = 0; j <= FIELD_WIDTH; ++j) {
      draw_rectangle(buffer,
                     j *TILE_SIZE_IN_PIXELS - 1,  i * TILE_SIZE_IN_PIXELS - 1,
                     2, 2,
                     kColorRed);

    }
  }
}

static void draw_pacdot(drawing_buffer_t *buffer, int i, int j) {
  int x = j * TILE_SIZE_IN_PIXELS;
  int y = i * TILE_SIZE_IN_PIXELS;
  int s = TILE_SIZE_IN_PIXELS / 4;
  x += (TILE_SIZE_IN_PIXELS - s)/2;
  y += (TILE_SIZE_IN_PIXELS - s)/2;
  draw_rectangle(buffer, x, y, s, s, kColorWhite);
}

static void draw_pacman(drawing_buffer_t *buffer, vector2_t *pos) {
  int s = TILE_SIZE_IN_PIXELS * 3. / 4.;
  float x = pos->x + (TILE_SIZE_IN_PIXELS - s)/2;
  float y = pos->y + (TILE_SIZE_IN_PIXELS - s)/2;
  draw_rectangle(buffer, x, y, s, s, kColorYellow);
}

static void game_render(game_t *game, drawing_buffer_t *buffer) {
  // NOTE: most time we spend here currently
  memset(buffer->buffer, 0, buffer->width * buffer->height * sizeof(*buffer->buffer));

  for (int i = 0; i < FIELD_HEIGHT; ++i) {
    for (int j = 0; j < FIELD_WIDTH; ++j) {
      if (level[i][j] == wall) {
        draw_rectangle(buffer,
                       j *TILE_SIZE_IN_PIXELS,  i * TILE_SIZE_IN_PIXELS,
                       TILE_SIZE_IN_PIXELS, TILE_SIZE_IN_PIXELS,
                       kColorWhite);
      }

      if (level[i][j] == pacdot) {
        draw_pacdot(buffer, i, j);
      }
    }
  }

  //draw_pacman(buffer, &game->pacman_pos);
  draw_pacman(buffer, &game->pacman.pos);

  draw_grid(buffer);
  (void) game;
}

void game_init(game_t *game) {
  game->is_inited = true;
  for (int i = 0; i < FIELD_HEIGHT; ++i) {
    for (int j = 0; j < FIELD_WIDTH; ++j) {
      if (level[i][j] == pacman) {
        game->pacman.pos.x = j * TILE_SIZE_IN_PIXELS;
        game->pacman.pos.y = i * TILE_SIZE_IN_PIXELS;
      }
    }
  }
  game->pacman.direction = game->pacman.next_desired_direction = DIRECTION_NONE;
  game->pacman.velocity = PACMAN_VELOCITY;
}

static const vector2_t vector_up = {0, -1};
static const vector2_t vector_down = {0, 1};
static const vector2_t vector_left = {-1, 0};
static const vector2_t vector_right = {1, 0};

static int direction_is_horizontal(direction_t direction) {
  return direction == DIRECTION_LEFT || direction == DIRECTION_RIGHT 
    || direction == DIRECTION_NONE;
}
static int direction_is_vertical(direction_t direction) {
  return direction == DIRECTION_UP || direction == DIRECTION_DOWN 
    || direction == DIRECTION_NONE;
}
static void do_snap_to_grid_movement(moving_entity_t *entity, float seconds_elapsed) {
  if (entity->next_desired_direction != entity->direction) {
    if ((direction_is_vertical(entity->next_desired_direction)
        && direction_is_vertical(entity->direction))
        ||
        (direction_is_horizontal(entity->next_desired_direction)
        && direction_is_horizontal(entity->direction))) {
      entity->direction = entity->next_desired_direction;
    }
  }

  vector2_t directions[] = {
    vector_up, vector_right, vector_down, vector_left, {0, 0}
  };
  vector2_t turn_points[] = {
    {
      ((((int) entity->pos.x)/TILE_SIZE_IN_PIXELS)/* - 1*/) * TILE_SIZE_IN_PIXELS,
      ((((int) entity->pos.y)/TILE_SIZE_IN_PIXELS)/* - 1*/) * TILE_SIZE_IN_PIXELS
    },
    {
      ((((int) entity->pos.x)/TILE_SIZE_IN_PIXELS)/* - 1*/) * TILE_SIZE_IN_PIXELS,
      ((((int) entity->pos.y)/TILE_SIZE_IN_PIXELS) + 1) * TILE_SIZE_IN_PIXELS
    },
    {
      ((((int) entity->pos.x)/TILE_SIZE_IN_PIXELS) + 1) * TILE_SIZE_IN_PIXELS,
      ((((int) entity->pos.y)/TILE_SIZE_IN_PIXELS)/* - 1*/) * TILE_SIZE_IN_PIXELS
    },
    {
      ((((int) entity->pos.x)/TILE_SIZE_IN_PIXELS) + 1) * TILE_SIZE_IN_PIXELS,
      ((((int) entity->pos.y)/TILE_SIZE_IN_PIXELS) + 1) * TILE_SIZE_IN_PIXELS
    },
    {
      ((((int) entity->pos.x)/TILE_SIZE_IN_PIXELS)/* - 1*/) * TILE_SIZE_IN_PIXELS,
      ((((int) entity->pos.y)/TILE_SIZE_IN_PIXELS) - 1) * TILE_SIZE_IN_PIXELS
    },
    {
      ((((int) entity->pos.x)/TILE_SIZE_IN_PIXELS) - 1) * TILE_SIZE_IN_PIXELS,
      ((((int) entity->pos.y)/TILE_SIZE_IN_PIXELS)/* - 1*/) * TILE_SIZE_IN_PIXELS
    },
    {
      ((((int) entity->pos.x)/TILE_SIZE_IN_PIXELS) - 1) * TILE_SIZE_IN_PIXELS,
      ((((int) entity->pos.y)/TILE_SIZE_IN_PIXELS) - 1) * TILE_SIZE_IN_PIXELS
    },

  };
  vector2_t new_desired_direction = directions[entity->next_desired_direction];
  vector2_t direction_vector = directions[entity->direction];
  vector2_t new_pos = {0, 0};

  if (entity->next_desired_direction != entity->direction) {
    vector2_t *found_turn_point = NULL;
    float min_time_to_turn_point = FLT_MAX;
    for (int i = 0; i < ARR_LEN(turn_points); ++i) {
      vector2_t turn_point = turn_points[i];
      printf("(%f %f) -> (%f %f) (%d) -> (%d)\n",
             entity->pos.x, entity->pos.y,
             turn_point.x, turn_point.y,
             entity->direction, entity->next_desired_direction
             );
      vector2_t shift_to_turn_point = VECTOR2_SUB(turn_point, entity->pos);
      float distance_sqr = VECTOR2_SQR_LEN(shift_to_turn_point);
      float distance = sqrtf(distance_sqr);
      float time_to_turn_point = distance / entity->velocity;
      if (time_to_turn_point < seconds_elapsed) {
        if (time_to_turn_point < min_time_to_turn_point) {
          found_turn_point = &turn_points[i];
          min_time_to_turn_point = time_to_turn_point;
        }
      }
    }
    if (found_turn_point) {
        float remaining_time = seconds_elapsed - min_time_to_turn_point;
        new_pos = VECTOR2_ADD(*found_turn_point,
          VECTOR2_MULT_NUMBER(new_desired_direction, remaining_time * entity->velocity));
        entity->direction = entity->next_desired_direction;
    } else {
      vector2_t shift = VECTOR2_MULT_NUMBER(direction_vector, entity->velocity * seconds_elapsed);
      new_pos = VECTOR2_ADD(entity->pos, shift);
    }
  } else {
    vector2_t shift = VECTOR2_MULT_NUMBER(direction_vector, entity->velocity * seconds_elapsed);
    new_pos = VECTOR2_ADD(entity->pos, shift);
  }

  {
    int i = (int)new_pos.y / TILE_SIZE_IN_PIXELS;
    int i_1 = (int)(new_pos.y + TILE_SIZE_IN_PIXELS - 1) / TILE_SIZE_IN_PIXELS;
    int j = (int)new_pos.x / TILE_SIZE_IN_PIXELS;
    int j_1 = (int)(new_pos.x + TILE_SIZE_IN_PIXELS - 1) / TILE_SIZE_IN_PIXELS;
    if (level[i][j] != wall && level[i_1][j_1] != wall) {
      entity->pos = new_pos;
    }
  }

}

void game_tick(void *memory, input_t *input, drawing_buffer_t *buffer) {
  game_t *game = (game_t *) memory;

  if (!game->is_inited) {
    game_init(game);
  }

  if (input->pause) {
    return;
  }
  if (input->up.pressed) {
    game->pacman.next_desired_direction = DIRECTION_UP;
  }
  if (input->down.pressed) {
    game->pacman.next_desired_direction = DIRECTION_DOWN;
  }
  if (input->left.pressed) {
    game->pacman.next_desired_direction = DIRECTION_LEFT;
  }
  if (input->right.pressed) {
    game->pacman.next_desired_direction = DIRECTION_RIGHT;
  }

  do_snap_to_grid_movement(&game->pacman, input->seconds_elapsed);

  game_render(game, buffer);
}
