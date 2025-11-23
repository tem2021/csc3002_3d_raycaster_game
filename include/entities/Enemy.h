#ifndef ENEMY_H
#define ENEMY_H

#include "core/Types.h"
#include "world/Map.h"
#include <cmath>

class Enemy {
public:
    static constexpr int DEFAULT_TEXTURE_ID = 200;

    Enemy(const Vec2& pos, float speed, int textureId = DEFAULT_TEXTURE_ID)
        : position_(pos), speed_(speed), textureId_(textureId) {}

    const Vec2& getPosition() const { return position_; }

    int getTextureId() const { return textureId_; }

    void update(const Vec2& playerPos, const Map& map);

    // ---- 新增：允许 Game.cpp 进行修正 ----
    void setPosition(const Vec2& p) { position_ = p; }
    void addOffset(const Vec2& delta) {
        position_.x += delta.x;
        position_.y += delta.y;
    }

private:
    Vec2 position_;
    float speed_;
    float timeOffset_ = (rand() % 1000) * 0.01f;
    float radius_ = 10.0f;

    int textureId_;
};
#endif // ENEMY_H