#include "game.h"

#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
#include <limits.h>

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

static float game_sqrt(float x) {
  return sqrtf(x);
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

  //printf("x [%d, %d), y [%d, %d)\n", x, x2, y, y2);

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

static void draw_gamepad(drawing_buffer_t *buffer, gamepad_input_t *input) {
  draw_rectangle(buffer, 10, 10, 100, 100, RGBA(100, 100, 100, 255));
  draw_rectangle(buffer,
                 50 * (1.0f + input->left_stick.x),
                 50 * (1.0f + input->left_stick.y),
                 20, 20, RGBA(50, 50, 50, 255));
}

typedef struct {
  vector2_t top_left;
  vector2_t size;
} rectangle_t;

static rectangle_t translate_coords(game_t *game, drawing_buffer_t *buffer, entity_t *e) {
  rectangle_t result = {};
  float buffer_h = buffer->height;
  float pixels_per_meter = game->pixels_per_meter;

  result.top_left.x = (e->pos.x - e->size.x * 0.5f) * pixels_per_meter;
  result.top_left.y = buffer_h - (e->pos.y + e->size.y * 0.5f) * pixels_per_meter;

  result.size.x = e->size.x * pixels_per_meter;
  result.size.y = e->size.y * pixels_per_meter;
  return result;
}

static vector2_t translate_point_coords(game_t *game, drawing_buffer_t *buffer, vector2_t point) {
  vector2_t result = {};
  float buffer_h = buffer->height;
  float pixels_per_meter = game->pixels_per_meter;

  result.x = point.x * pixels_per_meter;
  result.y = buffer_h - point.y * pixels_per_meter;

  return result;
}

static void game_render(game_t *game, drawing_buffer_t *buffer) {
  // NOTE: most time we spend here currently
  memset(buffer->buffer, 0, buffer->width * buffer->height * sizeof(*buffer->buffer));
  draw_gamepad(buffer, &game->gamepad_visualize_data);

  rectangle_t char_rect = translate_coords(game, buffer, &game->character);
  vector2_t camera_coords = translate_point_coords(game, buffer, game->camera_center);

  vector2_t offset_pixels = V2(-camera_coords.x + buffer->width * 0.5f, 0.0f);

  uint32_t mc_color = RGBA(255, 0, 0, 255);
  if (game->character.collision_state & COLLISION_STATE_LEFT ||
      game->character.collision_state & COLLISION_STATE_RIGHT) {
    mc_color |= RGBA(0, 255, 0, 255);
  }
  if (game->character.collision_state & COLLISION_STATE_BOTTOM) {
    mc_color |= RGBA(0, 0, 255, 255);
  }

  draw_rectangle(buffer,
                 char_rect.top_left.x + offset_pixels.x, char_rect.top_left.y + offset_pixels.y,
                 char_rect.size.x, char_rect.size.y,
                 mc_color
                );
  for (uint32_t i = 0; i < game->entities_count; ++i) {
    entity_t *e = game->entities + i;
    switch (e->type) {
    case ENTITY_TYPE_WALL: {
      rectangle_t r = translate_coords(game, buffer, e);
      uint32_t color = RGBA(e->color.r, e->color.g, e->color.b, e->color.a);

      draw_rectangle(buffer, r.top_left.x + offset_pixels.x, r.top_left.y + offset_pixels.y, r.size.x, r.size.y, color);
    } break;
    default: {
      assert(0);
    }
    }
  }
}

static int
game_push_entity(game_t *game, float x, float y, float w, float h, entity_type_t t, rgba_t color) {
  int result = INT_MAX;
  entity_t *e = game->entities + (game->entities_count++);
  *e = (entity_t){};
  e->pos.x = x;
  e->pos.y = y;
  e->size.x = w;
  e->size.y = h;
  e->type = t;
  e->color = color;
  return result;
}

static int
game_push_mc(game_t *game, float x, float y, float w, float h) {
  int result = game_push_entity(game, x, y, w, h, ENTITY_TYPE_MC, (rgba_t){0, 0, 0, 0});
  return result;
}
static int
game_push_wall(game_t *game, float x, float y, float w, float h) {
  int result = game_push_entity(game, x, y, w, h, ENTITY_TYPE_WALL,
                                (rgba_t){0, rand()%255, rand()%255, 255});
  return result;
}

static int
game_push_wall_by_top_left(game_t *game, float x, float y, float w, float h) {
  int result = game_push_wall(game, x + w/2.f, y - h/2.f, w, h);
  return result;
}

void game_init(game_t *game) {
  game->is_inited = true;
  game->character.speed = (vector2_t){0.0f, 0.0f};
  game->character.accel = (vector2_t){0.0f, 0.0f};
  game->pixels_per_meter = 50.0f;
  game->entities_count = 0;
  game->character.pos = (vector2_t){5.0f, 3.525f};
  game->character.size = (vector2_t){1.0f, 1.0f};

  game_push_wall_by_top_left(game, 0.0f, 3.0f, 30.0f, 3.0f);

  game_push_wall_by_top_left(game, 15.0f, 6.0f, 1.0f, 3.0f);

  game_push_wall_by_top_left(game, 25.0f, 4.0f, 3.0f, 1.0f);
  game_push_wall_by_top_left(game, 26.0f, 5.0f, 2.0f, 1.0f);
  game_push_wall_by_top_left(game, 27.0f, 6.0f, 1.0f, 1.0f);
}

gamepad_input_t old = {};
static void debug_print_gamepad_state(gamepad_input_t *gamepad) {
  bool should_print = false;
  should_print = should_print || (old.left_stick.x != gamepad->left_stick.x);
  should_print = should_print || (old.left_stick.y != gamepad->left_stick.y);

  should_print = should_print || (old.right_stick.x != gamepad->right_stick.x);
  should_print = should_print || (old.right_stick.y != gamepad->right_stick.y);

  should_print = should_print || (old.left_trigger != gamepad->left_trigger);
  should_print = should_print || (old.right_trigger != gamepad->right_trigger);

  should_print = should_print || (gamepad->a.transitions);
  should_print = should_print || (gamepad->b.transitions);
  should_print = should_print || (gamepad->x.transitions);
  should_print = should_print || (gamepad->y.transitions);
  should_print = should_print || (gamepad->select.transitions);
  should_print = should_print || (gamepad->start.transitions);
  should_print = should_print || (gamepad->left_bumper.transitions);
  should_print = should_print || (gamepad->right_bumper.transitions);

  should_print = should_print || (gamepad->left_stick_button.transitions);
  should_print = should_print || (gamepad->right_stick_button.transitions);

  should_print = should_print || (gamepad->dpad_up.transitions);
  should_print = should_print || (gamepad->dpad_down.transitions);
  should_print = should_print || (gamepad->dpad_left.transitions);
  should_print = should_print || (gamepad->dpad_right.transitions);

  if (should_print) {
#define BUTTON_EV(b) ((b.pressed && b.transitions % 2) ? "press" \
                      : ((!b.pressed && b.transitions % 2) ? "release": ""))

    static int i = 0;
    printf("%d ", i++);

    printf("LS (x=%f, y=%f, pr=%d, ev=%s) ",
           gamepad->left_stick.x,
           gamepad->left_stick.y,
           gamepad->left_stick_button.pressed,
           BUTTON_EV(gamepad->left_stick_button));

    printf("RS (x=%f, y=%f, pr=%d, ev=%s) ",
           gamepad->right_stick.x,
           gamepad->right_stick.y,
           gamepad->right_stick_button.pressed,
           BUTTON_EV(gamepad->right_stick_button));

    printf("LT %f ", gamepad->left_trigger);
    printf("RT %f ", gamepad->right_trigger);

    printf("Select (pr=%d ev=%s) ", gamepad->select.pressed, BUTTON_EV(gamepad->select));
    printf("Start (pr=%d ev=%s) ", gamepad->start.pressed, BUTTON_EV(gamepad->start));
    printf("A (pr=%d ev=%s) ", gamepad->a.pressed, BUTTON_EV(gamepad->a));
    printf("B (pr=%d ev=%s) ", gamepad->b.pressed, BUTTON_EV(gamepad->b));
    printf("X (pr=%d ev=%s) ", gamepad->x.pressed, BUTTON_EV(gamepad->x));
    printf("Y (pr=%d ev=%s) ", gamepad->y.pressed, BUTTON_EV(gamepad->y));
    printf("LB (pr=%d ev=%s) ", gamepad->left_bumper.pressed, BUTTON_EV(gamepad->left_bumper));
    printf("RB (pr=%d ev=%s) ", gamepad->right_bumper.pressed, BUTTON_EV(gamepad->right_bumper));

    printf("dpad_up (pr=%d ev=%s) ", gamepad->dpad_up.pressed, BUTTON_EV(gamepad->dpad_up));
    printf("dpad_down (pr=%d ev=%s) ", gamepad->dpad_down.pressed, BUTTON_EV(gamepad->dpad_down));
    printf("dpad_left (pr=%d ev=%s) ", gamepad->dpad_left.pressed, BUTTON_EV(gamepad->dpad_left));
    printf("dpad_right (pr=%d ev=%s) ", gamepad->dpad_right.pressed, BUTTON_EV(gamepad->dpad_right));
    printf("\n");

  }
  old = *gamepad;
}

static float segment_ray_intersection(segment_t segment1, ray_t ray, float def_response) {
  float result = def_response;

  segment_t segment2 = {ray[0], VECTOR2_ADD(ray[0], ray[1])};
  float x1 = segment1[0].x;
  float y1 = segment1[0].y;
  float x2 = segment1[1].x;
  float y2 = segment1[1].y;

  float x3 = segment2[0].x;
  float y3 = segment2[0].y;
  float x4 = segment2[1].x;
  float y4 = segment2[1].y;

  float denominator = (x1 - x2)*(y3 - y4) - (y1 - y2)*(x3 - x4);
  if (fabsf(denominator) > 1e-6f) {
    float t = ((x1 - x3)*(y3 - y4) - (y1 - y3)*(x3 -x4))
      / denominator;

    float u = - ((x1 - x2)*(y1 - y3) - (y1 - y2)*(x1 -x3))
      / denominator;

    assert(!isnan(t));
    assert(!isnan(u));
    assert(!isinf(t));
    assert(!isinf(u));

    if (t >= 0 && t <= 1 && u >= 0) {
      result = u;
    }
  }

  return result;
}

static float
get_collision_time(entity_t *stationary, entity_t *moving,
                   vector2_t dp, float dt, vector2_t *hit_line) {
  float result = dt;

 // vector2_t stationary_center = VECTOR2_ADD(stationary->pos,
 //   V2(stationary->size.x * 0.5f, -stationary->size.y * 0.5f));

  vector2_t stationary_center = stationary->pos; // X

  vector2_t stat_box = VECTOR2_ADD(stationary->size, moving->size);
  stat_box = VECTOR2_ADD(stat_box, V2(-0.01f, -0.01f));

  vector2_t half_size = VECTOR2_MULT_NUMBER(stat_box, 0.5f);
  /*
   *  A   B
   *    X
   *  C   D
   */
  vector2_t A = VECTOR2_ADD(stationary_center, V2(-half_size.x, half_size.y));
  vector2_t B = VECTOR2_ADD(stationary_center, V2(half_size.x, half_size.y));
  vector2_t C = VECTOR2_ADD(stationary_center, V2(-half_size.x, -half_size.y));
  vector2_t D = VECTOR2_ADD(stationary_center, V2(half_size.x, -half_size.y));

  segment_t segments[] = {
    {A, B}, {B, D}, {D, C}, {C, A}
  };
  vector2_t dot = moving->pos;
  ray_t ray = {dot, dp};

  int hit_segment = -1;

  for (uint32_t i = 0; i < ARR_LEN(segments); ++i) {
    float tmp_result = segment_ray_intersection(segments[i], ray, dt);
    if (tmp_result < result) {
      result = tmp_result;
      hit_segment = i;
    }
  }

  if (hit_segment >= 0 && hit_line) {
    *hit_line = VECTOR2_SUB(segments[hit_segment][0], segments[hit_segment][1]);
  }

  return result;
}

static float
point_to_segment_distance_sqr(segment_t segment, vector2_t point) {
  float result = 0.0f;
  vector2_t v = VECTOR2_SUB(point, segment[0]);
  vector2_t s = VECTOR2_SUB(segment[1], segment[0]);

  vector2_t proj = VECTOR2_MULT_NUMBER(s,
    VECTOR2_SCALAR_MULT(v, s) / VECTOR2_SQR_LEN(s));

  vector2_t intersection_point = VECTOR2_ADD(segment[0], proj);

  float AB_len = game_sqrt(VECTOR2_SQR_LEN(VECTOR2_SUB(segment[1], segment[0])));
  float AX_len = game_sqrt(VECTOR2_SQR_LEN(VECTOR2_SUB(intersection_point, segment[0])));
  float XB_len = game_sqrt(VECTOR2_SQR_LEN(VECTOR2_SUB(segment[1], intersection_point)));

  if (fabs(AB_len - AX_len - XB_len) < 1e-4) {
    //intersection inside segment
    result = VECTOR2_SQR_LEN(VECTOR2_SUB(intersection_point, point));
  } else {
    float d1 = VECTOR2_SQR_LEN(VECTOR2_SUB(segment[0], point));
    float d2 = VECTOR2_SQR_LEN(VECTOR2_SUB(segment[1], point));
    result = MIN(d1, d2);
  }

  return result;
}

static bool
is_colliding(entity_t *stationary, entity_t *moving, float epsilon,
             vector2_t *hit_line) {
  bool result = false;

  vector2_t stationary_center = stationary->pos; // X

  vector2_t stat_box = VECTOR2_ADD(stationary->size, moving->size);
  stat_box = VECTOR2_ADD(stat_box, V2(-0.01f, -0.01f));

  vector2_t half_size = VECTOR2_MULT_NUMBER(stat_box, 0.5f);
  /*
   *  A   B
   *    X
   *  C   D
   */
  vector2_t A = VECTOR2_ADD(stationary_center, V2(-half_size.x, half_size.y));
  vector2_t B = VECTOR2_ADD(stationary_center, V2(half_size.x, half_size.y));
  vector2_t C = VECTOR2_ADD(stationary_center, V2(-half_size.x, -half_size.y));
  vector2_t D = VECTOR2_ADD(stationary_center, V2(half_size.x, -half_size.y));

  segment_t segments[] = {
    {A, B}, {B, D}, {D, C}, {C, A}
  };
  vector2_t dot = moving->pos;

  int hit_segment = -1;

  float min_distance = FLT_MAX;

  for (uint32_t i = 0; i < ARR_LEN(segments); ++i) {
    float tmp_result = point_to_segment_distance_sqr(segments[i], dot);
    if (tmp_result < min_distance) {
      min_distance = tmp_result;
      hit_segment = i;
    }
  }

  result = (min_distance < epsilon);
  if (result && hit_segment >= 0 && hit_line) {
    *hit_line = VECTOR2_SUB(segments[hit_segment][0], segments[hit_segment][1]);
  }

  return result;
}

static void move_entity(game_t *game, entity_t *c, vector2_t direction, float dt) {
  c->accel = VECTOR2_MULT_NUMBER(direction, 50.0f);
  c->accel = VECTOR2_ADD(c->accel, V2(0.0f, -9.8f));
  c->speed = VECTOR2_ADD(c->speed, VECTOR2_MULT_NUMBER(c->accel, dt));
  vector2_t drag = VECTOR2_MULT_NUMBER(c->speed, -0.12f);
  c->speed = VECTOR2_ADD(c->speed, drag);

  for (int i = 0; i < 4 && dt > 1e-8f; ++i) {
    float actual_dt = dt;
    vector2_t new_dp = V2(0.0f, 0.0f);

    if (game_sqrt(VECTOR2_SQR_LEN(c->speed)) > 1e-8f) {
      vector2_t wall = V2(0.0f, 0.0f);
      for (uint32_t i = 0; i < game->entities_count; ++i) {
        entity_t *o = game->entities + i;
        switch (o->type) {
        case ENTITY_TYPE_WALL: {
          float tmp_dt = get_collision_time(o, c, c->speed, dt, &wall);
          if (tmp_dt < actual_dt) {
            actual_dt = tmp_dt;
            new_dp = VECTOR2_MULT_NUMBER(wall,
              VECTOR2_SCALAR_MULT(wall, c->speed) / VECTOR2_SQR_LEN(wall));
          }
        } break;
        default: {
          assert(0);
        }
        }
      }
    }

    c->pos = VECTOR2_ADD(c->pos, VECTOR2_MULT_NUMBER(c->speed, actual_dt));
    if (actual_dt != dt) {
      vector2_t normalized_speed = V2_NORMALIZED(c->speed);
      vector2_t stuck_offset = VECTOR2_MULT_NUMBER(normalized_speed, -0.001f);
      c->pos = VECTOR2_ADD(c->pos, stuck_offset);
      c->speed = new_dp;
    }
    dt -= actual_dt;
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
  }
  //debug_print_gamepad_state(&input->gamepad);

  vector2_t stick_vector = input->gamepad.left_stick;
  if (V2_LEN(stick_vector) < 1e-9) {
    stick_vector.x = 0.0f;
    stick_vector.y = 0.0f;
    if (input->left.pressed) {
      stick_vector.x += -1.0f;
    }
    if (input->right.pressed) {
      stick_vector.x += 1.0f;
    }
    if (input->up.pressed) {
      stick_vector.y += -1.0f;
    }
    if (input->down.pressed) {
      stick_vector.y += 1.0f;
    }
    if (V2_LEN(stick_vector) >= 1) {
      stick_vector = V2_NORMALIZED(stick_vector);
    }
  }
  stick_vector.y *= -1.0f;

  move_entity(game, &game->character, stick_vector, input->seconds_elapsed);

  game->character.collision_state = 0;

  for (uint i = 0; i < game->entities_count; ++i) {
    entity_t *e = game->entities + i;
    vector2_t wall = V2(0.0f, 0.0f);
    if (is_colliding(e, &game->character, 1e-2, &wall)) {
      if (fabs(wall.x) < 1e-9) {
        if (wall.x < e->pos.x) {
          game->character.collision_state |= COLLISION_STATE_LEFT;
        } else {
          game->character.collision_state |= COLLISION_STATE_RIGHT;
        }
      }
      if (fabs(wall.y) < 1e-9) {
        if (wall.y < e->pos.y) {
          game->character.collision_state |= COLLISION_STATE_BOTTOM;
        }
      }
    }
  }

  game->gamepad_visualize_data = input->gamepad;

  game->camera_center.x = game->character.pos.x;
  game->camera_center.y = 0;

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
