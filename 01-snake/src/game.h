#ifndef GAME_H
#define GAME_H

typedef unsigned int uint;

struct Input {
    enum Pressed {NONE, LEFT, RIGHT} pressed;
};

struct Pos {
    int x, y;
};
inline bool operator==(const Pos &a, const Pos &b) {
    return (a.x == b.x) & (a.y == b.y);
}
inline bool operator!=(const Pos &a, const Pos &b) {
    return !(a == b);
}

enum class Direction { UP = 0, RIGHT, DOWN, LEFT };

static const uint kTilesX = 20;
static const uint kTilesY = 20;
static const uint kTilesSize = 15;
static const uint kTurnMicroSec = 200 * 1000;

struct Game {
    unsigned char field[kTilesY][kTilesX];
    Pos head, prev_tail, fruit;
    Direction direction;

    Game();
    void Tick(Input *input);

    bool game_over;
};

void Render(uint *buffer, uint image_width, uint image_height, Game *game);

#endif
