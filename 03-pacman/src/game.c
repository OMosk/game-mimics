#include "game.h"

#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <math.h>
#include <float.h>

static const uint16_t empty_space = 0;
static const uint16_t pacdot = 1;
static const uint16_t wall = 2;
static const uint16_t pacman = 3;
static const uint16_t spawn = 4;
static const uint16_t cruise_target = 5;

static const uint16_t level[FIELD_HEIGHT][FIELD_WIDTH] = {
  {2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,},
  {2,5,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,5,2,},
  {2,1,2,2,2,1,2,1,2,2,1,2,2,1,2,2,1,2,2,1,2,1,2,2,2,1,2,},
  {2,1,2,2,2,1,2,1,1,2,1,2,2,1,2,2,1,2,1,1,2,1,2,2,2,1,2,},
  {2,1,1,1,1,1,1,2,1,1,1,1,2,1,2,1,1,1,1,2,1,1,1,1,1,1,2,},
  {2,1,2,2,2,1,2,2,2,2,2,1,2,1,2,1,2,2,2,2,2,1,2,2,2,1,2,},
  {2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,},
  {2,1,2,2,1,2,2,2,2,2,1,2,1,2,1,2,1,2,2,2,2,2,1,2,2,1,2,},
  {2,1,2,2,1,2,1,1,1,2,1,2,1,2,1,2,1,2,1,1,1,2,1,2,2,1,2,},
  {2,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,2,},
  {2,1,2,2,2,2,2,2,2,1,2,1,2,2,2,1,2,1,2,2,2,2,2,2,2,1,2,},
  {2,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,2,},
  {2,1,2,2,2,2,2,2,2,1,2,1,2,2,2,1,2,1,2,2,2,2,2,2,2,1,2,},
  {2,1,1,1,2,2,2,1,1,1,1,1,1,3,1,1,1,1,1,1,2,2,2,1,1,1,2,},
  {2,1,2,1,1,2,1,1,2,2,1,1,2,1,2,1,2,1,2,1,1,2,1,1,2,1,2,},
  {2,1,2,2,1,1,1,2,2,2,2,1,2,1,2,1,2,1,2,2,1,1,1,2,2,1,2,},
  {2,1,2,1,1,2,1,1,2,2,2,1,2,1,2,1,2,1,2,1,1,2,1,1,2,1,2,},
  {2,1,1,1,2,2,2,1,1,1,2,1,2,1,2,1,2,1,2,1,2,2,2,1,1,1,2,},
  {2,1,2,1,2,1,1,1,2,1,1,4,1,1,1,1,1,1,1,1,1,1,2,1,2,1,2,},
  {2,1,2,1,2,1,2,2,2,1,2,2,2,1,2,2,2,1,2,1,2,1,2,1,2,1,2,},
  {2,1,2,1,2,1,1,2,1,1,2,2,4,4,4,2,2,1,2,1,2,1,2,1,2,1,2,},
  {2,1,1,1,1,1,2,2,2,1,2,2,2,2,2,2,2,1,2,2,2,1,1,1,1,1,2,},
  {2,1,2,2,2,1,2,2,2,1,2,2,2,2,2,2,2,1,2,1,2,1,2,2,2,1,2,},
  {2,1,2,2,2,1,1,1,1,1,2,2,1,1,1,2,2,1,2,1,2,1,2,2,2,1,2,},
  {2,1,1,1,1,1,2,2,2,1,2,1,1,2,1,1,2,1,2,1,2,1,1,1,1,1,2,},
  {2,1,2,1,2,1,1,1,1,1,1,1,2,2,2,1,1,1,1,1,1,1,2,1,2,1,2,},
  {2,1,2,1,2,2,2,1,2,2,1,2,2,2,2,2,1,2,2,1,2,2,2,1,2,1,2,},
  {2,1,2,1,1,1,1,1,2,2,1,1,1,1,1,1,1,2,2,1,1,1,1,1,2,1,2,},
  {2,1,2,2,2,2,2,1,2,2,1,2,2,2,2,2,1,2,2,1,2,2,2,2,2,1,2,},
  {2,5,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,5,2,},
  {2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,},
};

#define RGB(r, g, b) ((b) + ((g) << 8) + ((r) << 16))
#define RGBA(r, g, b, a) ((b) + ((g) << 8) + ((r) << 16) + ((a) << 24))
//                              B     G            R
#define kColorWhite  (255 + (255 << 8) + (255 << 16))
#define kColorYellow  (0 + (255 << 8) + (255 << 16))
//static const uint kColorGrey  = 150 + (150 << 8) + (150 << 16);
//static const uint kColorGreen = 0 + (255 << 8) + (0 << 16);
#define kColorRed  (0 + (0 << 8) + (255 << 16))
#define kColorBlue  RGB(0, 0, 255)
#define kColorGreen  RGB(0, 255, 0)
#define kColorCyan  RGB(0, 255, 255)

static float game_sqr(float x) {
  return x*x;
}

static int game_count_trailing_zeroes(uint32_t x) {
  return __builtin_ctz(x);
}

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
/*
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
*/
static void draw_bitmap(drawing_buffer_t *buffer, imagebuffer_t bitmap, int x, int y) {
  int x_min = MAX(0, x);
  int y_min = MAX(0, y);
  int x_max = MIN((int)buffer->width, x + bitmap.w);
  int y_max = MIN((int)buffer->height, y + bitmap.h);

  for (int it_y = 0; it_y < bitmap.h; ++it_y) {
    int dest_y = y + it_y;
    if (dest_y < y_min || dest_y >= y_max) {
      continue;
    }
    //rgba_t *row = (rgba_t *)bitmap.data + (bitmap.h - it_y - 1) * bitmap.w;
    rgba_t *row = (rgba_t *)bitmap.data + it_y * bitmap.w;
    for (int it_x = 0; it_x < bitmap.w; ++it_x) {
      int dest_x = x + it_x;
      if (dest_x < x_min || dest_x >= x_max) {
        continue;
      }
      rgba_t *bg_pixel = (rgba_t *)buffer->buffer + dest_y * buffer->width + dest_x;

      rgba_t *fg_pixel = row + it_x;

      rgba_t new_pixel = {0};
      new_pixel.r = bg_pixel->r + (fg_pixel->r - bg_pixel->r) * (fg_pixel->a / 255.0f);
      new_pixel.g = bg_pixel->g + (fg_pixel->g - bg_pixel->g) * (fg_pixel->a / 255.0f);
      new_pixel.b = bg_pixel->b + (fg_pixel->b - bg_pixel->b) * (fg_pixel->a / 255.0f);
      *bg_pixel = new_pixel;
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

static void draw_pacman(drawing_buffer_t *buffer, pacman_t *pacman) {
  int x = pacman->movement_data.pos.x;
  int y = pacman->movement_data.pos.y;
  draw_bitmap(buffer, pacman->texture, x, y);
}

static const uint ghost_colors[] = {
  kColorRed, kColorBlue, kColorGreen, kColorCyan
};

static void draw_ghost(drawing_buffer_t *buffer, ghost_entity_t *entity) {
  //int s = ENTITY_SIZE_IN_PIXELS;
  vector2_t *pos = &entity->movement_data.pos;
  float x = pos->x; // + (TILE_SIZE_IN_PIXELS - s)/2;
  float y = pos->y; // + (TILE_SIZE_IN_PIXELS - s)/2;
  draw_bitmap(buffer, entity->texture, x, y);
  //draw_rectangle(buffer, x, y, s, s, ghost_colors[entity->color]);
}

static void game_render(game_t *game, drawing_buffer_t *buffer) {
  // NOTE: most time we spend here currently
  memset(buffer->buffer, 0, buffer->width * buffer->height * sizeof(*buffer->buffer));

  uint wall_color = RGB(128, 128, 255);
  if (game->over) {
    wall_color = game->game_over_color;
  }

  for (int i = 0; i < FIELD_HEIGHT; ++i) {
    for (int j = 0; j < FIELD_WIDTH; ++j) {
      if (game->level[i][j] == wall) {
        draw_rectangle(buffer,
                       j *TILE_SIZE_IN_PIXELS,  i * TILE_SIZE_IN_PIXELS,
                       TILE_SIZE_IN_PIXELS, TILE_SIZE_IN_PIXELS,
                       wall_color);
      }

      if (game->level[i][j] == pacdot) {
        draw_pacdot(buffer, i, j);
      }
    }
  }

  draw_pacman(buffer, &game->pacman);
  for (uint i = 0; i < ARR_LEN(game->ghosts); ++i) {
    draw_ghost(buffer, game->ghosts + i);
  }

  //draw_grid(buffer);
}

void game_init(game_t *game) {
  uint ghost_idx = 0;
  uint cruise_target_idx = 0;
  game->is_inited = true;
  game->stat = (const game_stat_t){0};
  for (int i = 0; i < FIELD_HEIGHT; ++i) {
    for (int j = 0; j < FIELD_WIDTH; ++j) {
      if (level[i][j] == pacman) {
        game->pacman.movement_data.pos.x = j * TILE_SIZE_IN_PIXELS;
        game->pacman.movement_data.pos.y = i * TILE_SIZE_IN_PIXELS;
      }
      if (level[i][j] == pacdot) {
        game->stat.pacdots_left++;
      }
      if (level[i][j] == spawn && ghost_idx < ARR_LEN(game->ghosts)) {
        game->ghosts[ghost_idx].movement_data.pos.x = j * TILE_SIZE_IN_PIXELS;
        game->ghosts[ghost_idx].movement_data.pos.y = i * TILE_SIZE_IN_PIXELS;
        ghost_idx++;
      }

      if (level[i][j] == cruise_target && cruise_target_idx < ARR_LEN(game->ghosts)) {
        game->ghosts[cruise_target_idx].cruise_target.x = j;
        game->ghosts[cruise_target_idx].cruise_target.y = i;
        cruise_target_idx++;
      }

    }
  }
  memcpy(game->level, level, sizeof(level));
  game->pacman.movement_data.direction
    = game->pacman.movement_data.next_desired_direction = DIRECTION_NONE;
  game->pacman.movement_data.velocity = PACMAN_VELOCITY;
  for (uint i = 0; i < ARR_LEN(game->ghosts); ++i) {
    game->ghosts[i].color = (ghost_color_t) i;
    game->ghosts[i].movement_data.velocity = GHOST_VELOCITY;
    game->ghosts[i].time_till_cruise = i * 5;
    game->ghosts[i].time_till_chase = (i + 1) * 5;
    game->ghosts[i].time_till_course_correction = 1.0f;
    game->ghosts[i].cur_time_till_course_correction = 1.0f;
    game->ghosts[i].state = GHOST_STATE_INACTIVITY;
  }
  game->over = false;

  buffer_t red_ghost_image_buffer = platform_load_file("../assets/red.bmp");
  buffer_t blue_ghost_image_buffer = platform_load_file("../assets/blue.bmp");
  buffer_t green_ghost_image_buffer = platform_load_file("../assets/green.bmp");
  buffer_t cyan_ghost_image_buffer = platform_load_file("../assets/cyan.bmp");
  buffer_t pacman_image_buffer = platform_load_file("../assets/pacman.bmp");

  assert(red_ghost_image_buffer.size > 0);
  assert(blue_ghost_image_buffer.size > 0);
  assert(green_ghost_image_buffer.size > 0);
  assert(cyan_ghost_image_buffer.size > 0);
  assert(pacman_image_buffer.size > 0);

  game->ghosts[0].texture = game_decode_bmp(red_ghost_image_buffer);
  game->ghosts[1].texture = game_decode_bmp(blue_ghost_image_buffer);
  game->ghosts[2].texture = game_decode_bmp(green_ghost_image_buffer);
  game->ghosts[3].texture = game_decode_bmp(cyan_ghost_image_buffer);
  game->pacman.texture = game_decode_bmp(pacman_image_buffer);
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
static bool is_tile_free(vector2_t new_pos) {
  int i = (int)new_pos.y / TILE_SIZE_IN_PIXELS;
  int i_1 = (int)(new_pos.y + TILE_SIZE_IN_PIXELS - 1) / TILE_SIZE_IN_PIXELS;
  int j = (int)new_pos.x / TILE_SIZE_IN_PIXELS;
  int j_1 = (int)(new_pos.x + TILE_SIZE_IN_PIXELS - 1) / TILE_SIZE_IN_PIXELS;
  return level[i][j] != wall && level[i_1][j_1] != wall;
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

  bool use_default_move = true;

  if (entity->next_desired_direction != entity->direction) {
    vector2_t *found_turn_point = NULL;
    float min_time_to_turn_point = FLT_MAX;
    for (uint i = 0; i < ARR_LEN(turn_points); ++i) {
      vector2_t turn_point = turn_points[i];
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
      vector2_t next_tile = VECTOR2_ADD(*found_turn_point,
        VECTOR2_MULT_NUMBER(new_desired_direction, TILE_SIZE_IN_PIXELS));

      if (is_tile_free(next_tile)) {
        float remaining_time = seconds_elapsed - min_time_to_turn_point;
        new_pos = VECTOR2_ADD(*found_turn_point,
          VECTOR2_MULT_NUMBER(new_desired_direction, remaining_time * entity->velocity));

        entity->direction = entity->next_desired_direction;
        use_default_move = false;
      }
    }
  }

  if (use_default_move) {
    vector2_t shift =
      VECTOR2_MULT_NUMBER(direction_vector, entity->velocity * seconds_elapsed);
    new_pos = VECTOR2_ADD(entity->pos, shift);
  }

  if (is_tile_free(new_pos)) {
    entity->pos = new_pos;
  }
}

static ivector2_t float_to_index_position(vector2_t position) {
  return (ivector2_t) {
    (int) (position.x + TILE_SIZE_IN_PIXELS*0.5) / TILE_SIZE_IN_PIXELS,
    (int) (position.y + TILE_SIZE_IN_PIXELS*0.5) / TILE_SIZE_IN_PIXELS
  };
}

static bool are_entities_collide(vector2_t pos1, vector2_t pos2) {
  vector2_t shift = VECTOR2_SUB(pos1, pos2);
  float shift_sqr_len = VECTOR2_SQR_LEN(shift);
  return (shift_sqr_len < game_sqr(ENTITY_SIZE_IN_PIXELS));
}

static void get_wave_field(int16_t *field,
                           ivector2_t *queue,
                           int width, int height, ivector2_t desired_pos) {
  (void)height;
  queue[0] = desired_pos;
  int field_queue_len = 1;

  for (int i = 0; i < field_queue_len; ++i) {
    ivector2_t *idx = queue + i;
    assert(idx->x >= 0);
    assert(idx->y >= 0);
    assert(idx->x < FIELD_WIDTH);
    assert(idx->y < FIELD_HEIGHT);
    int path_len = field[idx->y*width + idx->x];
    int new_path_len = path_len + 1;
    ivector2_t possible_moves[] = {
      {idx->x - 1, idx->y},
      {idx->x + 1, idx->y},
      {idx->x, idx->y - 1},
      {idx->x, idx->y + 1},
    };

    for (uint j = 0; j < ARR_LEN(possible_moves); ++j) {
      ivector2_t *next_tile = possible_moves + j;
      if (level[next_tile->y][next_tile->x] == wall) {
        continue;
      }
      if (next_tile->x == desired_pos.x && next_tile->y == desired_pos.y) {
        continue;
      }
      if (field[next_tile->y*width + next_tile->x] == 0 ||
          field[next_tile->y*width + next_tile->x] > new_path_len) {
        queue[field_queue_len++] = *next_tile;
        field[next_tile->y*width + next_tile->x] = new_path_len;
      }
    }
  }
}

static direction_t get_next_direction(int16_t *field, int w, int h, ivector2_t pos,
                                      direction_t cur_movement) {
  (void)h;
  direction_t directions[] = {
    DIRECTION_RIGHT, DIRECTION_UP, DIRECTION_LEFT, DIRECTION_DOWN
  };
  direction_t result = cur_movement;

  int16_t cur_path_len = field[pos.y*w + pos.x];
  int16_t searched_path_len = cur_path_len - 1;
  ivector2_t possible_moves[] = {
    {pos.x + 1, pos.y},
    {pos.x, pos.y - 1},
    {pos.x - 1, pos.y},
    {pos.x, pos.y + 1},
  };
  for (uint j = 0; j < ARR_LEN(possible_moves); ++j) {
    ivector2_t *move = possible_moves + j;
    if (field[move->y*w + move->x] == searched_path_len) {
      result = directions[j];
      break;
    }
  }

  return result;
}

static void update_ghosts(game_t *game,
                          ghost_entity_t *ghosts, int ghosts_size,
                          float secs_elapsed) {
  ivector2_t pacman_tile_pos = float_to_index_position(game->pacman.movement_data.pos);

  int16_t wave_field[FIELD_HEIGHT][FIELD_WIDTH] = {};
  ivector2_t wave_field_queue[FIELD_HEIGHT * FIELD_WIDTH] = {};

  get_wave_field((int16_t *)wave_field,
                 wave_field_queue, FIELD_WIDTH, FIELD_HEIGHT, pacman_tile_pos);

  for (int i = 0; i < ghosts_size; ++i) {
    ghost_entity_t *ghost = ghosts + i;
    ivector2_t ghost_tile_pos = float_to_index_position(ghost->movement_data.pos);

    if (ghost->state == GHOST_STATE_INACTIVITY) {
      ghost->time_till_cruise -= secs_elapsed;
      if (ghost->time_till_cruise < 0) {
        ghost->state = GHOST_STATE_CRUISING;
        get_wave_field((int16_t *)ghost->wave_field, wave_field_queue,
                       FIELD_WIDTH, FIELD_HEIGHT, ghost->cruise_target);
      }
      ghost->movement_data.next_desired_direction = DIRECTION_NONE;
      ghost->movement_data.direction = DIRECTION_NONE;
    } else if (ghost->state == GHOST_STATE_CRUISING) {
      ghost->time_till_chase -= secs_elapsed;
      ghost->movement_data.next_desired_direction
        = get_next_direction((int16_t *)ghost->wave_field,
                             FIELD_WIDTH, FIELD_HEIGHT, ghost_tile_pos,
                             ghost->movement_data.direction);
      if (ghost->time_till_chase < 0) {
        ghost->state = GHOST_STATE_CHAISING;
        memcpy(ghost->wave_field, wave_field, sizeof(wave_field));
      }
    } else if (ghost->state == GHOST_STATE_CHAISING) {
      ghost->cur_time_till_course_correction -= secs_elapsed;
      if (ghost->cur_time_till_course_correction < 0) {
        ghost->cur_time_till_course_correction = ghost->time_till_course_correction;
        memcpy(ghost->wave_field, wave_field, sizeof(wave_field));
      }
      ghost->movement_data.next_desired_direction
        = get_next_direction((int16_t *)ghost->wave_field,
                             FIELD_WIDTH, FIELD_HEIGHT,
                             ghost_tile_pos,
                             ghost->movement_data.direction);
    }
  }

  for (int i = 0; i < ghosts_size; ++i) {
    do_snap_to_grid_movement(&(ghosts + i)->movement_data, secs_elapsed);
  }

}

void game_tick(void *memory, input_t *input, drawing_buffer_t *buffer) {
  game_t *game = (game_t *) memory;

  if (!game->is_inited) {
    game_init(game);
  }
  if (input->restart) {
    game_init(game);
    input->restart = false;
  }

  if (!game->over) {
    if (input->pause) {
      return;
    }
    if (input->up.pressed) {
      game->pacman.movement_data.next_desired_direction = DIRECTION_UP;
    }
    if (input->down.pressed) {
      game->pacman.movement_data.next_desired_direction = DIRECTION_DOWN;
    }
    if (input->left.pressed) {
      game->pacman.movement_data.next_desired_direction = DIRECTION_LEFT;
    }
    if (input->right.pressed) {
      game->pacman.movement_data.next_desired_direction = DIRECTION_RIGHT;
    }

    do_snap_to_grid_movement(&game->pacman.movement_data, input->seconds_elapsed);

    update_ghosts(game, game->ghosts, ARR_LEN(game->ghosts), input->seconds_elapsed);

    ivector2_t pacman_idx_pos = float_to_index_position(game->pacman.movement_data.pos);
    if (game->level[pacman_idx_pos.y][pacman_idx_pos.x] == pacdot) {
      game->level[pacman_idx_pos.y][pacman_idx_pos.x] = empty_space;
    }

    for (uint i = 0; i < ARR_LEN(game->ghosts); ++i) {
      if (are_entities_collide(game->pacman.movement_data.pos, game->ghosts[i].movement_data.pos)) {
        game->over = true;
        game->game_over_color = ghost_colors[game->ghosts[i].color];
        break;
      }
    }
  }

  game_render(game, buffer);
}

static uint32_t game_read_16bytes(uint8_t *buffer) {
  uint16_t result = 0;
  result |= (*buffer++) << 0;
  result |= (*buffer++) << 8;
  return result;
}

static uint32_t game_read_32bytes(uint8_t *buffer) {
  uint32_t result = 0;
  result |= (*buffer++) << 0;
  result |= (*buffer++) << 8;
  result |= (*buffer++) << 16;
  result |= (*buffer++) << 24;
  return result;
}

static void game_store_32bytes(uint8_t *buffer, uint32_t n) {
  *buffer++ = (uint8_t)((n & 0x000000FF)      );
  *buffer++ = (uint8_t)((n & 0x0000FF00) >>  8);
  *buffer++ = (uint8_t)((n & 0x00FF0000) >> 16);
  *buffer++ = (uint8_t)((n & 0xFF000000) >> 24);
}

imagebuffer_t game_decode_bmp(buffer_t buffer) {
  //http://www.ece.ualberta.ca/~elliott/ee552/studentAppNotes/2003_w/misc/bmp_file_format/bmp_file_format.htm
  imagebuffer_t result = {0};

  uint32_t data_offset = game_read_32bytes((uint8_t*)(buffer.data + 0xA));
  uint32_t size = game_read_32bytes((uint8_t*)(buffer.data + 0x22));
  uint32_t width = game_read_32bytes((uint8_t*)(buffer.data + 0x12));
  uint32_t height = game_read_32bytes((uint8_t*)(buffer.data + 0x16));
  uint16_t bits_per_pixel = game_read_16bytes((uint8_t*)(buffer.data + 0x1C));
  uint32_t compression = game_read_32bytes((uint8_t*)(buffer.data + 0x1E));

#define BI_BITFIELDS 3
  assert(compression == BI_BITFIELDS);
  assert(bits_per_pixel == 32);

  uint32_t rmask, gmask, bmask, amask;
  rmask = game_read_32bytes((uint8_t*)(buffer.data + 0x36 + 0));
  gmask = game_read_32bytes((uint8_t*)(buffer.data + 0x36 + 4));
  bmask = game_read_32bytes((uint8_t*)(buffer.data + 0x36 + 8));
  amask = ((((uint32_t)-1) ^ rmask) ^ gmask) ^ bmask;

  uint32_t rshift, gshift, bshift, ashift;
  rshift = game_count_trailing_zeroes(rmask);
  gshift = game_count_trailing_zeroes(gmask);
  bshift = game_count_trailing_zeroes(bmask);
  ashift = game_count_trailing_zeroes(amask);

  result.w = width;
  result.h = height;
  result.data = (uint32_t *) (buffer.data + data_offset);

  uint8_t *it = buffer.data + data_offset;
  uint8_t *end = buffer.data + size;//+ buffer.size;
  //uint8_t *end = buffer.data + buffer.size;

  while (it < end) {
    uint32_t old_value = game_read_32bytes(it);

    uint32_t r = (old_value & rmask) >> rshift;
    uint32_t g = (old_value & gmask) >> gshift;
    uint32_t b = (old_value & bmask) >> bshift;
    uint32_t a = (old_value & amask) >> ashift;

    uint32_t new_value = RGBA(r, g, b, a);
    game_store_32bytes(it, new_value);
    it += 4;
  }

  uint32_t tmp_row[width * 4];
  for (uint y = 0; y < height/2; ++y) {
    memcpy(tmp_row, result.data + y * width, width * 4);
    memcpy(result.data + y * width, result.data + (height - y - 1) * width, width * 4);
    memcpy(result.data + (height - y - 1) * width, tmp_row, width * 4);
  }

  return result;
}
