#include "entities/Enemy.h"

void Enemy::update(const Vec2& playerPos, const Map& map) {
    float dx = playerPos.x - position_.x;
    float dy = playerPos.y - position_.y;
    float dist = std::sqrt(dx * dx + dy * dy);

    if (dist < 1.0f) return;  // 避免贴脸震动

    dx /= dist;
    dy /= dist;

    Vec2 newPos{
        position_.x + dx * speed_,
        position_.y + dy * speed_
    };

    int tileSize = map.getTileSize();
    int mx = static_cast<int>(newPos.x) / tileSize;
    int my = static_cast<int>(newPos.y) / tileSize;

    // 不穿墙
    if (!map.isWall(mx, my)) {
        position_ = newPos;
    }
}
