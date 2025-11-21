#ifndef ENEMY_H
#define ENEMY_H

#include "core/Types.h"
#include "world/Map.h"
#include <cmath>

class Enemy {
public:
    Enemy(const Vec2& pos, float speed)
        : position_(pos), speed_(speed), health_(50), alive_(true) {}

    const Vec2& getPosition() const { return position_; }
    int getHealth() const { return health_; }
    bool isAlive() const { return alive_; }

    void update(const Vec2& playerPos, const Map& map);
    void takeDamage(int damage);

private:
    Vec2 position_;
    float speed_;
    int health_;
    bool alive_;
    float timeOffset_ = (rand() % 1000) * 0.01f; // 每个怪不同节奏

};

#endif
