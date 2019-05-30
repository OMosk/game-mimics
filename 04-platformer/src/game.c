#include "game.h"

#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
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


static void game_render(game_t *game, drawing_buffer_t *buffer) {
  // NOTE: most time we spend here currently
  memset(buffer->buffer, 0, buffer->width * buffer->height * sizeof(*buffer->buffer));

  draw_rectangle(buffer, game->pos.x * game->pixels_per_meter,
                         /*buffer->height - */game->pos.y * game->pixels_per_meter,
                         game->size.x * game->pixels_per_meter,
                         game->size.y * game->pixels_per_meter,
                         RGBA(255, 0, 0, 255)
                         );
}

void game_init(game_t *game) {
  game->is_inited = true;
  game->pos = (vector2_t){5.0f, 5.0f};
  game->size = (vector2_t){1.0f, 1.0f};
  game->pixels_per_meter = 50.0f;
}

gamepad_input_t old = {};
void debug_print_gamepad_state(gamepad_input_t *gamepad) {
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
  debug_print_gamepad_state(&input->gamepad);

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
