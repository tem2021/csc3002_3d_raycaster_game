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
    // float playerAngle = 0.0f; // 若要使用玩家角度：传入参数 or 全局获取
    // float viewInfluence = std::sin(playerAngle - angleToPlayer);

    // ---- 计算蛇形偏移量 ----
    float snake = std::sin(timeOffset_ * 2.5f); // 频率可调
    float strafeFactor = 0.35f;                // 左右摆的幅度

    float lateral =  snake * 0.5f;

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

    // 封装：判断某点是否碰墙
    auto isBlocked = [&](float x, float y) {
        int tileX = (int)x / ts;
        int tileY = (int)y / ts;
        return map.isWall(tileX, tileY);
    };

    // 封装：判断圆形 hitbox 是否被阻挡
    auto circleBlocked = [&](float cx, float cy) {
        return (
            isBlocked(cx + radius_, cy) ||
            isBlocked(cx - radius_, cy) ||
            isBlocked(cx, cy + radius_) ||
            isBlocked(cx, cy - radius_)
        );
    };

    // ---- 先尝试整体移动 ----
    if (!circleBlocked(newPos.x, newPos.y)) {
        position_ = newPos;
        return;
    }

    // ---- 整体不行 → 尝试滑动（仅 X）----
    Vec2 tryX = { newPos.x, position_.y };
    if (!circleBlocked(tryX.x, tryX.y)) {
        position_.x = tryX.x;
        return;
    }

    // ---- 尝试滑动（仅 Y）----
    Vec2 tryY = { position_.x, newPos.y };
    if (!circleBlocked(tryY.x, tryY.y)) {
        position_.y = tryY.y;
        return;
    }

    // 三个方向都被墙挡 → 什么都不做（原地）


}
