#include "entities/Enemy.h"
#include "core/Config.h"

void Enemy::update(const Vec2& playerPos, const Map& map)
{
    // ---- 基本信息 ----
    float dx = playerPos.x - position_.x;
    float dy = playerPos.y - position_.y;
    float dist = std::sqrt(dx*dx + dy*dy);

    if (dist < 5.0f) {
        // 贴脸时减少蛇形抖动，避免乱跳
        timeOffset_ += 0.05f;
    } else {
        timeOffset_ += 0.02f;
    }

    // ---- 计算朝向玩家的主方向 ----
    Vec2 forward = { dx / dist, dy / dist };

    // ---- 计算横向方向（垂直向量）----
    Vec2 perp = { -forward.y, forward.x };

    // ---- 玩家视角影响怪物偏移 ----
    // 越朝向怪物的方向看，怪物越向右偏一点
    float angleToPlayer = std::atan2(dy, dx);
    float playerAngle = 0.0f; // 若要使用玩家角度：传入参数 or 全局获取
    float viewInfluence = std::sin(playerAngle - angleToPlayer);

    // ---- 计算蛇形偏移量 ----
    float snake = std::sin(timeOffset_ * 2.5f); // 频率可调
    float strafeFactor = 0.35f;                // 左右摆的幅度

    float lateral = snake * 0.7f + viewInfluence * 0.5f;

    Vec2 finalDir = forward + perp * (lateral * strafeFactor);

    // ---- 归一化 ----
    float len = std::sqrt(finalDir.x*finalDir.x + finalDir.y*finalDir.y);
    finalDir.x /= len;
    finalDir.y /= len;

    // ---- 计算新位置 ----
    Vec2 newPos = {
        position_.x + finalDir.x * speed_,
        position_.y + finalDir.y * speed_
    };

    // ---- 碰撞检查 ----
    int ts = map.getTileSize();
    int mx = (int)newPos.x / ts;
    int my = (int)newPos.y / ts;

    if (!map.isWall(mx, my)) {
        position_ = newPos;
    }
}

void Enemy::takeDamage(int damage) {
    if (!alive_) return;
    
    health_ -= damage;
    if (health_ <= 0) {
        health_ = 0;
        alive_ = false;
    }
}
