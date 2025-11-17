#ifndef ENEMY_H
#define ENEMY_H

#include "core/Types.h"
#include "world/Map.h"
#include <cmath>

class Enemy {
public:
    Enemy(const Vec2& pos, float speed)
        : position_(pos), speed_(speed) {}

    const Vec2& getPosition() const { return position_; }

    void update(const Vec2& playerPos, const Map& map);

private:
    Vec2 position_;
    float speed_;
};

#endif
