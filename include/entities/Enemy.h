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
    float timeOffset_ = (rand() % 1000) * 0.01f; // 每个怪不同节奏
    float radius_ = 10.0f;   // 敌人半径（单位像素，可调整）

};

#endif
