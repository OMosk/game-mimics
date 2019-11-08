#include "game.h"
#include "assets.h"

#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>

#define RGB(r, g, b) ((b) + ((g) << 8) + ((r) << 16))
#define RGBA(r, g, b, a) ((b) + ((g) << 8) + ((r) << 16) + ((a) << 24))
//                              B     G            R
#define kColorWhite (255 + (255 << 8) + (255 << 16))
#define kColorYellow (0 + (255 << 8) + (255 << 16))
// static const uint kColorGrey  = 150 + (150 << 8) + (150 << 16);
// static const uint kColorGreen = 0 + (255 << 8) + (0 << 16);
#define kColorRed (0 + (0 << 8) + (255 << 16))
#define kColorBlue RGB(0, 0, 255)
#define kColorGreen RGB(0, 255, 0)
#define kColorCyan RGB(0, 255, 255)

#define M_PI 3.1415926

static float game_sqr(float x) { return x * x; }

static float game_sqrt(float x) { return sqrtf(x); }

static float game_floor(float x) { return floorf(x); }

static int game_count_trailing_zeroes(uint32_t x) { return __builtin_ctz(x); }

static bool was_pressed(button_t b) {
  return (b.pressed && b.transitions > 0) ||
         (!b.pressed && b.transitions > 0 && b.transitions % 2 == 0);
}

static void draw_rectangle(drawing_buffer_t *buffer, int x, int y, int width,
                           int height, uint32_t color) {
  int x2 = x + width;
  int y2 = y + height;
  if (x < 0)
    x = 0;
  if (y < 0)
    y = 0;
  if (x2 >= (int)buffer->width)
    x2 = buffer->width - 1;
  if (y2 >= (int)buffer->height)
    y2 = buffer->height - 1;

  for (int i = y; i < y2; ++i) {
    for (int j = x; j < x2; ++j) {
      buffer->buffer[i * buffer->width + j] = color;
    }
  }
}

static void draw_bitmap(drawing_buffer_t *buffer, imagebuffer_t bitmap, int x,
                        int y) {
  int x_min = MAX(0, x);
  int y_min = MAX(0, y);
  int x_max = MIN((int)buffer->width, x + bitmap.w);
  int y_max = MIN((int)buffer->height, y + bitmap.h);

  for (int it_y = 0; it_y < bitmap.h; ++it_y) {
    int dest_y = y + it_y;
    if (dest_y < y_min || dest_y >= y_max) {
      continue;
    }
    rgba_t *row = (rgba_t *)bitmap.data + it_y * bitmap.w;
    for (int it_x = 0; it_x < bitmap.w; ++it_x) {
      int dest_x = x + it_x;
      if (dest_x < x_min || dest_x >= x_max) {
        continue;
      }
      rgba_t *bg_pixel =
          (rgba_t *)buffer->buffer + dest_y * buffer->width + dest_x;

      rgba_t *fg_pixel = row + it_x;

      rgba_t new_pixel = {0};
      new_pixel.r =
          bg_pixel->r + (fg_pixel->r - bg_pixel->r) * (fg_pixel->a / 255.0f);
      new_pixel.g =
          bg_pixel->g + (fg_pixel->g - bg_pixel->g) * (fg_pixel->a / 255.0f);
      new_pixel.b =
          bg_pixel->b + (fg_pixel->b - bg_pixel->b) * (fg_pixel->a / 255.0f);
      *bg_pixel = new_pixel;
    }
  }
}

static vector2_t rotate_point(vector2_t p, float s, float c) {
  return V2(c * p.x - s * p.y, s * p.x + c * p.y);
}

static void draw_bitmap2(drawing_buffer_t *buffer, imagebuffer_t bitmap,
                         ivector2_t coords, ivector2_t sprite_center,
                         float scale, float rotate) {
  float s = sinf(-rotate);
  float c = cosf(-rotate);
  float s_inv = sinf(rotate);
  float c_inv = cosf(rotate);

  vector2_t top_left =
      V2(floorf(-sprite_center.x * scale), floorf(-sprite_center.y * scale));

  vector2_t top_right = V2(ceilf(+(-sprite_center.x + bitmap.w) * scale),
                           floorf(-sprite_center.y * scale));

  vector2_t bottom_right = V2(ceilf(+(-sprite_center.x + bitmap.w) * scale),
                              ceilf(+(-sprite_center.y + bitmap.h) * scale));

  vector2_t bottom_left = V2(floorf(-sprite_center.x * scale),
                             ceilf(+(-sprite_center.y + bitmap.h) * scale));

  vector2_t top_left_n = rotate_point(top_left, s, c);
  vector2_t top_right_n = rotate_point(top_right, s, c);
  vector2_t bottom_left_n = rotate_point(bottom_left, s, c);
  vector2_t bottom_right_n = rotate_point(bottom_right, s, c);

  int x_min = MIN(top_left_n.x, top_right_n.x);
  x_min = MIN(x_min, bottom_left_n.x);
  x_min = MIN(x_min, bottom_right_n.x);

  int y_min = MIN(top_left_n.y, top_right_n.y);
  y_min = MIN(y_min, bottom_left_n.y);
  y_min = MIN(y_min, bottom_right_n.y);

  int x_max = MAX(top_left_n.x, top_right_n.x);
  x_max = MAX(x_max, bottom_left_n.x);
  x_max = MAX(x_max, bottom_right_n.x);

  int y_max = MAX(top_left_n.y, top_right_n.y);
  y_max = MAX(y_max, bottom_left_n.y);
  y_max = MAX(y_max, bottom_right_n.y);

  x_min = MAX(0, x_min + coords.x);
  y_min = MAX(0, y_min + coords.y);
  x_max = MIN((int)buffer->width, x_max + coords.x);
  y_max = MIN((int)buffer->height, y_max + coords.y);

  for (int it_y = y_min; it_y < y_max; ++it_y) {
    for (int it_x = x_min; it_x < x_max; ++it_x) {
      vector2_t p4 = V2(it_x, it_y);
      vector2_t p3 = VECTOR2_SUB(p4, coords);
      vector2_t p2 = rotate_point(p3, s_inv, c_inv);
      vector2_t p1 = VECTOR2_MULT_NUMBER(p2, 1.f / scale);
      vector2_t p0 = VECTOR2_ADD(p1, sprite_center);
      int origin_xi = roundf(p0.x);
      int origin_yi = roundf(p0.y);

      if (origin_xi < 0 || origin_xi >= bitmap.w) {
        continue;
      }
      if (origin_yi < 0 || origin_yi >= bitmap.h) {
        continue;
      }

      rgba_t *bg_pixel = (rgba_t *)buffer->buffer + it_y * buffer->width + it_x;
      rgba_t *fg_pixel =
          (rgba_t *)bitmap.data + origin_yi * bitmap.w + origin_xi;
      rgba_t new_pixel = {0};
      new_pixel.r =
          bg_pixel->r + (fg_pixel->r - bg_pixel->r) * (fg_pixel->a / 255.0f);
      new_pixel.g =
          bg_pixel->g + (fg_pixel->g - bg_pixel->g) * (fg_pixel->a / 255.0f);
      new_pixel.b =
          bg_pixel->b + (fg_pixel->b - bg_pixel->b) * (fg_pixel->a / 255.0f);
      *bg_pixel = new_pixel;
    }
  }
}

static void draw_gamepad(drawing_buffer_t *buffer, gamepad_input_t *input) {
  draw_rectangle(buffer, 10, 10, 100, 100, RGBA(100, 100, 100, 255));
  draw_rectangle(buffer, 50 * (1.0f + input->left_stick.x),
                 50 * (1.0f + input->left_stick.y), 20, 20,
                 RGBA(200, 0, 0, 255));
  // if (game->a_button_timer > 0.0f) {
  if (input->a.pressed) {
    draw_rectangle(buffer, 150, 30, 40, 40, RGBA(200, 0, 0, 255));
  }
}

typedef struct {
  vector2_t top_left;
  vector2_t size;
} rectangle_t;

static rectangle_t translate_coords(game_t *game, drawing_buffer_t *buffer,
                                    static_body2d_t *e) {
  rectangle_t result = {};
  float buffer_h = buffer->height;
  float pixels_per_meter = game->pixels_per_meter;

  result.top_left.x = (e->pos.x - e->size.x * 0.5f) * pixels_per_meter;
  result.top_left.y =
      buffer_h - (e->pos.y + e->size.y * 0.5f) * pixels_per_meter;

  result.size.x = e->size.x * pixels_per_meter;
  result.size.y = e->size.y * pixels_per_meter;
  return result;
}

static vector2_t translate_point_coords(game_t *game, drawing_buffer_t *buffer,
                                        vector2_t point) {
  vector2_t result = {};
  float buffer_h = buffer->height;
  float pixels_per_meter = game->pixels_per_meter;

  result.x = point.x * pixels_per_meter;
  result.y = buffer_h - point.y * pixels_per_meter;

  return result;
}

static vector2_t translate_screen_coords_to_physics_coords(
    game_t *game, drawing_buffer_t *buffer, vector2_t point) {

  vector2_t result = {};
  float buffer_h = buffer->height;
  float pixels_per_meter = game->pixels_per_meter;

  result.x = point.x / pixels_per_meter;
  result.y = (buffer_h - point.y) / pixels_per_meter;

  return result;
}

static void game_render(game_t *game, input_t *input,
                        drawing_buffer_t *buffer) {
  // NOTE: most time we spend here currently
  memset(buffer->buffer, 255,
         buffer->width * buffer->height * sizeof(*buffer->buffer));


  draw_rectangle(buffer, input->mouse.x, input->mouse.y, 10, 10, RGB(0, 0, 0));

  for (uint32_t i = 0; i < game->active_entities_count; ++i) {
    entity_t *e = game->active_entities[i];
    switch (e->type) {
    case ENTITY_TYPE_MC: {
      vector2_t pos = e->u.mc.body.pos;
      vector2_t size = e->u.mc.body.size;
      uint32_t color = RGBA(0, 0, 0, 255);
      vector2_t top_left = V2(pos.x - size.x/2., pos.y + size.y/2.);
      vector2_t center = V2(pos.x, pos.y);

      top_left = translate_point_coords(game, buffer, top_left);
      center = translate_point_coords(game, buffer, center);


      draw_rectangle(buffer, top_left.x, top_left.y,
                     size.x * game->pixels_per_meter,
                     size.y * game->pixels_per_meter, color);
      {
        ivector2_t screen_coords_i = IV2(center.x, center.y);
        draw_bitmap2(buffer, game->triangle,
                     screen_coords_i,
                     IV2(64, 64), 1.0f, e->u.mc.rotation_angle);

      }
    } break;
    case ENTITY_TYPE_WALL: {
      vector2_t pos = e->u.wall.body.pos;
      vector2_t size = e->u.wall.body.size;
      uint32_t color = *(uint32_t *)&e->u.wall.color;
      vector2_t top_left = V2(pos.x-size.x/2., pos.y + size.y/2.);

      top_left = translate_point_coords(game, buffer, top_left);
      draw_rectangle(buffer, top_left.x, top_left.y,
                     size.x * game->pixels_per_meter,
                     size.y * game->pixels_per_meter, color);
    } break;
    case ENTITY_TYPE_PROJECTILE: {
      vector2_t pos = e->u.projectile.body.pos;
      vector2_t size = e->u.projectile.body.size;
      uint32_t color = RGBA(0, 0, 0, 255);
      vector2_t top_left = V2(pos.x, pos.y + size.y);

      top_left = translate_point_coords(game, buffer, top_left);
      draw_rectangle(buffer, top_left.x, top_left.y,
                     size.x * game->pixels_per_meter,
                     size.y * game->pixels_per_meter, color);
    } break;
    default: {
    }
    }
  }

}

static entity_t *game_alloc_entity(game_t *game) {
  entity_t *result = NULL;
  if (game->next_free_entity) {
    result = (entity_t *)game->next_free_entity;
    game->next_free_entity = game->next_free_entity->next;
  } else {
    if (game->entities_count >= ARR_LEN(game->entities_pool)) {
      printf("%s:%d entities pool was exhausted\n", __FILE__, __LINE__);
      // Pool exhaustion
      abort();
    }
    result = game->entities_pool + game->entities_count;
    game->entities_count++;
  }
  game->active_entities[game->active_entities_count++] = result;
  return result;
}

static float lerp(float a, float b, float t) { return a + t * (b - a); }

static void game_init_mc(game_t *game, entity_t *e) {
  (void)game;
  *e = (entity_t){};
  e->type = ENTITY_TYPE_MC;
  e->u.mc.body.pos = V2(5, 5);
  e->u.mc.body.size = V2(1, 1);
}

//create wall by specifying bottom-left corner coordinates and size
static void game_create_wall(game_t *game, float x, float y, float w, float h) {
  entity_t *e = game_alloc_entity(game);
  e->type = ENTITY_TYPE_WALL;
  e->u.wall.body.pos.x = x + w / 2.0;
  e->u.wall.body.pos.y = y + h / 2.0;
  e->u.wall.body.size.x = w;
  e->u.wall.body.size.y = h;
  *(uint32_t *)&e->u.wall.color = rand() % UINT32_MAX;
}

static void game_init_level(game_t *game) {
  game_create_wall(game, 0, 0, 1, 1);
  game_create_wall(game, 1, 0, 10, 1);
  //scale and relation
}

static void game_init(game_t *game) {
  memset(game, 0, sizeof(game_t));
  game->allocator.base = (uint8_t *)(game + 1);

  game->is_inited = true;

  const buffer_t source_triangle_file = {
      (uint8_t *)&_binary_assets_triangle_bmp_start,
      (uint8_t *)&_binary_assets_triangle_bmp_end -
          (uint8_t *)&_binary_assets_triangle_bmp_start};

  buffer_t triangle_file = {};
  triangle_file.size = source_triangle_file.size;
  triangle_file.data = game_allocate(&game->allocator, triangle_file.size, 16);
  memcpy(triangle_file.data, source_triangle_file.data, triangle_file.size);

  game->triangle = game_decode_bmp(triangle_file);

  game->pixels_per_meter = 100;

  game->mc = game_alloc_entity(game);
  game_init_mc(game, game->mc);

  game_init_level(game);
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
#define BUTTON_EV(b)                                                           \
  ((b.pressed && b.transitions % 2)                                            \
       ? "press"                                                               \
       : ((!b.pressed && b.transitions % 2) ? "release" : ""))

    static int i = 0;
    printf("%d ", i++);

    printf("LS (x=%f, y=%f, pr=%d, ev=%s) ", gamepad->left_stick.x,
           gamepad->left_stick.y, gamepad->left_stick_button.pressed,
           BUTTON_EV(gamepad->left_stick_button));

    printf("RS (x=%f, y=%f, pr=%d, ev=%s) ", gamepad->right_stick.x,
           gamepad->right_stick.y, gamepad->right_stick_button.pressed,
           BUTTON_EV(gamepad->right_stick_button));

    printf("LT %f ", gamepad->left_trigger);
    printf("RT %f ", gamepad->right_trigger);

    printf("Select (pr=%d ev=%s) ", gamepad->select.pressed,
           BUTTON_EV(gamepad->select));
    printf("Start (pr=%d ev=%s) ", gamepad->start.pressed,
           BUTTON_EV(gamepad->start));
    printf("A (pr=%d ev=%s) ", gamepad->a.pressed, BUTTON_EV(gamepad->a));
    printf("B (pr=%d ev=%s) ", gamepad->b.pressed, BUTTON_EV(gamepad->b));
    printf("X (pr=%d ev=%s) ", gamepad->x.pressed, BUTTON_EV(gamepad->x));
    printf("Y (pr=%d ev=%s) ", gamepad->y.pressed, BUTTON_EV(gamepad->y));
    printf("LB (pr=%d ev=%s) ", gamepad->left_bumper.pressed,
           BUTTON_EV(gamepad->left_bumper));
    printf("RB (pr=%d ev=%s) ", gamepad->right_bumper.pressed,
           BUTTON_EV(gamepad->right_bumper));

    printf("dpad_up (pr=%d ev=%s) ", gamepad->dpad_up.pressed,
           BUTTON_EV(gamepad->dpad_up));
    printf("dpad_down (pr=%d ev=%s) ", gamepad->dpad_down.pressed,
           BUTTON_EV(gamepad->dpad_down));
    printf("dpad_left (pr=%d ev=%s) ", gamepad->dpad_left.pressed,
           BUTTON_EV(gamepad->dpad_left));
    printf("dpad_right (pr=%d ev=%s) ", gamepad->dpad_right.pressed,
           BUTTON_EV(gamepad->dpad_right));
    printf("\n");
  }
  old = *gamepad;
}

static float segment_ray_intersection(segment_t segment1, ray_t ray,
                                      float def_response) {
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

  float denominator = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
  if (fabsf(denominator) > 1e-6f) {
    float t = ((x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4)) / denominator;

    float u = -((x1 - x2) * (y1 - y3) - (y1 - y2) * (x1 - x3)) / denominator;

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

static float get_collision_time(static_body2d_t *stationary,
                                static_body2d_t *moving, vector2_t dp, float dt,
                                vector2_t *hit_line) {
  float result = dt;

  vector2_t stationary_center = stationary->pos; // X

  vector2_t stat_box = VECTOR2_ADD(stationary->size, moving->size);
  // stat_box = VECTOR2_ADD(stat_box, V2(-0.01f, -0.01f));

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

  segment_t segments[] = {{A, B}, {B, D}, {D, C}, {C, A}};
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

static float point_to_segment_distance_sqr(segment_t segment, vector2_t point) {
  float result = 0.0f;
  vector2_t v = VECTOR2_SUB(point, segment[0]);
  vector2_t s = VECTOR2_SUB(segment[1], segment[0]);

  vector2_t proj =
      VECTOR2_MULT_NUMBER(s, VECTOR2_SCALAR_MULT(v, s) / VECTOR2_SQR_LEN(s));

  vector2_t intersection_point = VECTOR2_ADD(segment[0], proj);

  float AB_len =
      game_sqrt(VECTOR2_SQR_LEN(VECTOR2_SUB(segment[1], segment[0])));
  float AX_len =
      game_sqrt(VECTOR2_SQR_LEN(VECTOR2_SUB(intersection_point, segment[0])));
  float XB_len =
      game_sqrt(VECTOR2_SQR_LEN(VECTOR2_SUB(segment[1], intersection_point)));

  if (fabs(AB_len - AX_len - XB_len) < 1e-4) {
    // intersection inside segment
    result = VECTOR2_SQR_LEN(VECTOR2_SUB(intersection_point, point));
  } else {
    float d1 = VECTOR2_SQR_LEN(VECTOR2_SUB(segment[0], point));
    float d2 = VECTOR2_SQR_LEN(VECTOR2_SUB(segment[1], point));
    result = MIN(d1, d2);
  }

  return result;
}

static bool is_colliding(static_body2d_t *stationary, static_body2d_t *moving,
                         float epsilon, segment_t *hit_line) {
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

  segment_t segments[] = {{A, B}, {B, D}, {D, C}, {C, A}};
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
    (*hit_line)[0] = segments[hit_segment][0];
    (*hit_line)[1] = segments[hit_segment][1];
  }

  return result;
}

typedef struct {
  vector2_t acceleration;
  vector2_t drag;
  vector2_t max_speed;
} movement_spec_t;

static void move_entity(game_t *game, entity_t *target_entity, body2d_t *c,
                        movement_spec_t *spec, float dt) {

  c->accel = spec->acceleration;
  c->speed = VECTOR2_ADD(c->speed, VECTOR2_MULT_NUMBER(c->accel, dt));
  vector2_t drag =
      V2(c->speed.x * spec->drag.x * dt, c->speed.y * spec->drag.y * dt);
  c->speed = VECTOR2_ADD(c->speed, drag);

  for (int i = 0; i < 4 && dt > 1e-8f; ++i) {
    float actual_dt = dt;
    vector2_t new_dp = V2(0.0f, 0.0f);

    if (game_sqrt(VECTOR2_SQR_LEN(c->speed)) > 1e-8f) {
      for (uint32_t i = 0; i < game->active_entities_count; ++i) {
        entity_t *o = game->active_entities[i];
        if (o == target_entity) {
          continue;
        }
        switch (o->type) {
        case ENTITY_TYPE_WALL: {
          vector2_t wall = V2(0.0f, 0.0f);
          float tmp_dt = get_collision_time(&o->u.wall.body, (static_body2d_t *)c,
                                         c->speed, dt, &wall);
          if (tmp_dt < actual_dt) {
            actual_dt = tmp_dt;
            new_dp = VECTOR2_MULT_NUMBER(wall,
              VECTOR2_SCALAR_MULT(wall, c->speed) / VECTOR2_SQR_LEN(wall));
          }

        } break;
        case ENTITY_TYPE_PROJECTILE: {
        } break;
        case ENTITY_TYPE_MC: {
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

void game_shoot_projectile(game_t *game, vector2_t initial_pos,
                           vector2_t shooter_speed, vector2_t direction) {
  entity_t *projectile = game_alloc_entity(game);
  entity_t *p = projectile;
  *p = (entity_t){};
  p->type = ENTITY_TYPE_PROJECTILE;
  p->u.projectile.body.pos = initial_pos;
  p->u.projectile.body.speed = V2_MULT_NUMBER(direction, PROJECTILE_SPEED);
  p->u.projectile.body.speed = V2_ADD(p->u.projectile.body.speed, shooter_speed);
  p->u.projectile.body.size = V2(0.1, 0.1);
  p->u.projectile.left_to_live = 5;
}

void game_handle_input(game_t *game, input_t *input, drawing_buffer_t *buffer) {
  vector2_t cursor_screen_coords = V2(input->mouse.x, input->mouse.y);
  vector2_t cursor_physics_coords = translate_screen_coords_to_physics_coords(
      game, buffer, cursor_screen_coords);

  vector2_t mc_center = game->mc->u.mc.body.pos;

  vector2_t looking_direction =
      VECTOR2_SUB(cursor_physics_coords, mc_center);

  float angle_radians = atan2f(looking_direction.y, looking_direction.x);
  game->mc->u.mc.rotation_angle = angle_radians - M_PI / 2.0f;

  vector2_t direction = {0};
  if ((input->keyboard.a.pressed)) {
    direction = VECTOR2_ADD(direction, V2(-1, 0));
  }
  if ((input->keyboard.d.pressed)) {
    direction = VECTOR2_ADD(direction, V2(1, 0));
  }
  if ((input->keyboard.w.pressed)) {
    direction = VECTOR2_ADD(direction, V2(0, 1));
  }
  if ((input->keyboard.s.pressed)) {
    direction = VECTOR2_ADD(direction, V2(0, -1));
  }
  if (VECTOR2_SQR_LEN(direction) > 0) {
    direction = V2_NORMALIZED(direction);
  }
  //direction =
  //    rotate_point(direction, sinf(game->rotation), cosf(game->rotation));

  movement_spec_t spec = {};
  spec.acceleration = VECTOR2_MULT_NUMBER(direction, 18.0);
  spec.drag = V2(-6., -6.);
  // spec.max_speed = 10;
  move_entity(game, game->mc, &game->mc->u.mc.body, &spec,
              input->seconds_elapsed);

  if (was_pressed(input->mouse.left)) {
    vector2_t normalized_looking_direction = V2_NORMALIZED(looking_direction);
    game_shoot_projectile(game, mc_center,
                          game->mc->u.mc.body.speed, normalized_looking_direction);
  }

}

void game_update_entities(game_t *game, input_t *input) {
  for (uint32_t i = 0; i < game->active_entities_count; ++i) {
    entity_t *e = game->active_entities[i];
    switch (e->type) {
    case ENTITY_TYPE_PROJECTILE: {
      entity_projectile_t *projectile = &e->u.projectile;

      movement_spec_t spec = {};
      move_entity(game, e, &projectile->body, &spec, input->seconds_elapsed);
      //printf("%f,  %f\n", projectile->body.accel.x, projectile->body.accel.y);

      projectile->left_to_live -= input->seconds_elapsed;
      if (projectile->left_to_live < 0) {
        game->active_entities[i] = game->active_entities[game->active_entities_count-1];
        game->active_entities_count--;
        --i;
        continue;
      }
    } break;
    case ENTITY_TYPE_MC: {
    } break;
    case ENTITY_TYPE_WALL: {
    } break;
    }
  }
}

void game_tick(void *memory, input_t *input, drawing_buffer_t *buffer) {
  game_t *game = (game_t *)memory;

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

  game_handle_input(game, input, buffer);

  game_update_entities(game, input);

  game_render(game, input, buffer);
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
  *buffer++ = (uint8_t)((n & 0x000000FF));
  *buffer++ = (uint8_t)((n & 0x0000FF00) >> 8);
  *buffer++ = (uint8_t)((n & 0x00FF0000) >> 16);
  *buffer++ = (uint8_t)((n & 0xFF000000) >> 24);
}

imagebuffer_t game_decode_bmp(buffer_t buffer) {
  // http://www.ece.ualberta.ca/~elliott/ee552/studentAppNotes/2003_w/misc/bmp_file_format/bmp_file_format.htm
  imagebuffer_t result = {0};

  uint32_t data_offset = game_read_32bytes((uint8_t *)(buffer.data + 0xA));
  uint32_t size = game_read_32bytes((uint8_t *)(buffer.data + 0x22));
  uint32_t width = game_read_32bytes((uint8_t *)(buffer.data + 0x12));
  uint32_t height = game_read_32bytes((uint8_t *)(buffer.data + 0x16));
  uint16_t bits_per_pixel = game_read_16bytes((uint8_t *)(buffer.data + 0x1C));
  uint32_t compression = game_read_32bytes((uint8_t *)(buffer.data + 0x1E));

#define BI_BITFIELDS 3
  assert(compression == BI_BITFIELDS);
  assert(bits_per_pixel == 32);

  uint32_t rmask, gmask, bmask, amask;
  rmask = game_read_32bytes((uint8_t *)(buffer.data + 0x36 + 0));
  gmask = game_read_32bytes((uint8_t *)(buffer.data + 0x36 + 4));
  bmask = game_read_32bytes((uint8_t *)(buffer.data + 0x36 + 8));
  amask = ((((uint32_t)-1) ^ rmask) ^ gmask) ^ bmask;

  uint32_t rshift, gshift, bshift, ashift;
  rshift = game_count_trailing_zeroes(rmask);
  gshift = game_count_trailing_zeroes(gmask);
  bshift = game_count_trailing_zeroes(bmask);
  ashift = game_count_trailing_zeroes(amask);

  result.w = width;
  result.h = height;
  result.data = (uint32_t *)(buffer.data + data_offset);

  uint8_t *it = buffer.data + data_offset;
  uint8_t *end = buffer.data + size; //+ buffer.size;
  // uint8_t *end = buffer.data + buffer.size;

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
  for (uint y = 0; y < height / 2; ++y) {
    memcpy(tmp_row, result.data + y * width, width * 4);
    memcpy(result.data + y * width, result.data + (height - y - 1) * width,
           width * 4);
    memcpy(result.data + (height - y - 1) * width, tmp_row, width * 4);
  }

  return result;
}

void *game_allocate(stack_allocator_t *allocator, uint32_t size,
                    uint32_t alignment) {
  uint32_t adjustment =
      (alignment - ((size_t)allocator->base + allocator->used) % alignment);
  if (adjustment == alignment) {
    adjustment = 0;
  }
  uint8_t *base = allocator->base + allocator->used + adjustment;
  allocator->used += size + adjustment;
  return base;
}
