#include "game.h"

#include <cstring>
#include <cstdlib>
#include <ctime>

//                              B     G            R
static const uint kColorWhite = 255 + (255 << 8) + (255 << 16);
static const uint kColorGrey  = 150 + (150 << 8) + (150 << 16);
static const uint kColorGreen = 0 + (255 << 8) + (0 << 16);
static const uint kColorRed = 0 + (0 << 8) + (255 << 16);

void Render(uint *buffer, uint image_width, uint image_height, Game *game) {
    bzero(buffer, image_width * image_height * sizeof(uint));

    //Draw gridlines

    for(uint i = 0; i <= kTilesY; ++i) {
        const uint row_offset = i * image_width * kTilesSize;
        for (uint j = 0; j < kTilesX * kTilesSize; ++j) {
            buffer[row_offset + j] = kColorWhite;
        }
    }

    for(uint i = 0; i < kTilesSize * kTilesY; ++i) {
        const uint row_offset = i * image_width;
        for (uint j = 0; j <= kTilesSize * kTilesX; j += kTilesSize) {
            buffer[row_offset + j] = kColorWhite;
        }
    }

    //Draw tiles
    auto fill_cell = [&] (uint x, uint y, uint color) {
        for (uint i = y * kTilesSize; i < (y + 1) * kTilesSize; ++i) {
            for (uint j = x * kTilesSize; j < (x + 1) * kTilesSize; ++j) {
                buffer[i * image_width + j] = color;
            }
        }
    };

    uint color = kColorGrey;
    if (game->game_over) color = kColorRed;

    for (uint i = 0; i < kTilesY; ++i) {
        for (uint j = 0; j < kTilesX; ++j) {
            auto cell = game->field[i][j];
            if (cell == 0) {
                continue;
            }
            fill_cell(j, i, color);
        }
    }
    fill_cell(game->fruit.x, game->fruit.y, kColorGreen);
}

Game::Game() {
    bzero(this, sizeof(Game));
    bzero(field, kTilesX * kTilesY * sizeof(unsigned char));

    head.x = kTilesX / 2;
    head.y = kTilesY / 2;
    direction = Direction::UP;

    fruit.x = kTilesX / 3;
    fruit.y = kTilesX / 3;

    field[head.y][head.x] = 2;
    field[head.y+1][head.x] = 1;

    game_over = false;

    srand(time(NULL));
}

void Game::Tick(Input *input) {
    if (game_over) return;

    switch (input->pressed) {
    case Input::Pressed::LEFT:
        direction = (Direction)((4 + (int)direction - 1) % 4);
        break;
    case Input::Pressed::RIGHT:
        direction = (Direction)(((int)direction + 1) % 4);
        break;
    case Input::Pressed::NONE:
        break;
    }

    Pos new_head = head;
    switch (direction) {
    case Direction::UP:
        --new_head.y;
        break;
    case Direction::RIGHT:
        ++new_head.x;
        break;
    case Direction::DOWN:
        ++new_head.y;
        break;
    case Direction::LEFT:
        --new_head.x;
        break;
    }

    if (new_head.x < 0 || new_head.x >= (int)kTilesX ||
        new_head.y < 0 || new_head.y >= (int)kTilesY ||
        field[new_head.y][new_head.x] > 0) {
        game_over = true;
        return;
    }
    int head_value = field[head.y][head.x];

    if (fruit == new_head) {
        ++head_value;
    }

    Pos places_for_fruit[kTilesX * kTilesY];
    int places_for_fruit_len = 0;

    for (uint i = 0; i < kTilesY; ++i) {
        for (uint j = 0; j < kTilesX; ++j) {
            if (field[i][j] > 0)  {
                if (fruit != new_head) {
                    --field[i][j];
                }
            } else {
                places_for_fruit[places_for_fruit_len].x = j;
                places_for_fruit[places_for_fruit_len].y = i;
                ++places_for_fruit_len;
            }
        }
    }

    if (fruit == new_head) {
        fruit = places_for_fruit[rand() % places_for_fruit_len];
    }

    field[new_head.y][new_head.x] = head_value;
    head = new_head;
}


