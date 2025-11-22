#include "entities/Enemy.h"
#include "core/Config.h"
#include <cmath>

// ----------------------------------------------------------
// 连续空间 + 蛇形 + 8方向导航 + 强化滑墙
// ----------------------------------------------------------

void Enemy::update(const Vec2& playerPos, const Map& map)
{
    float dx = playerPos.x - position_.x;
    float dy = playerPos.y - position_.y;
    float dist = std::sqrt(dx*dx + dy*dy);

    if (dist < 0.01f) return;

    // -------------------------
    // 1. 基本方向
    // -------------------------
    Vec2 forward = { dx / dist, dy / dist };
    Vec2 perp = { -forward.y, forward.x };

    // -------------------------
    // 2. 蛇形（保持你原来的效果）
    // -------------------------
    timeOffset_ += (dist < 5.0f ? 0.05f : 0.02f);
    float snake = std::sin(timeOffset_ * 2.5f);
    float lateral = snake * 0.40f;

    // -------------------------
    // 3. tile-level 转弯辅助（加强版）
    // -------------------------
    int ts = map.getTileSize();
    int ex = (int)(position_.x / ts);
    int ey = (int)(position_.y / ts);
    int px = (int)(playerPos.x / ts);
    int py = (int)(playerPos.y / ts);

    Vec2 tileAssist = {0,0};

    int dirs8[8][2] = {
        {1,0},{-1,0},{0,1},{0,-1},
        {1,1},{1,-1},{-1,1},{-1,-1}
    };

    float bestScore = -9999.0f;

    for (auto& d : dirs8)
    {
        int nx = ex + d[0];
        int ny = ey + d[1];

        if (nx < 0 || nx >= map.getWidth()) continue;
        if (ny < 0 || ny >= map.getHeight()) continue;

        if (map.isWall(nx, ny)) continue;

        float score = -(std::abs(px - nx) + std::abs(py - ny)); // 越靠近玩家越好

        if (score > bestScore)
        {
            bestScore = score;
            tileAssist = { (float)d[0], (float)d[1] };
        }
    }

    // 归一化
    float lenT = std::sqrt(tileAssist.x*tileAssist.x + tileAssist.y*tileAssist.y);
    if (lenT > 0.01f) {
        tileAssist.x /= lenT;
        tileAssist.y /= lenT;
    }

    // -------------------------
    // 4. 合并方向（重新调权重）
    // -------------------------
    Vec2 finalDir = {
        forward.x * 0.45f + tileAssist.x * 0.35f + perp.x * lateral,
        forward.y * 0.45f + tileAssist.y * 0.35f + perp.y * lateral
    };

    float lenF = std::sqrt(finalDir.x*finalDir.x + finalDir.y*finalDir.y);
    if (lenF > 0.01f) {
        finalDir.x /= lenF;
        finalDir.y /= lenF;
    }

    // -------------------------
    // 5. 圆形碰撞检测
    // -------------------------
    auto isBlocked = [&](float x, float y) {
        int tx = (int)x / ts;
        int ty = (int)y / ts;
        return map.isWall(tx, ty);
    };

    auto circleBlocked = [&](float cx, float cy)
    {
        float r = radius_;

        // 检查 8 方向（圆形包围盒）
        if (isBlocked(cx + r, cy)) return true;
        if (isBlocked(cx - r, cy)) return true;
        if (isBlocked(cx, cy + r)) return true;
        if (isBlocked(cx, cy - r)) return true;

        float rp = r * 0.7071f;  // 1/sqrt(2)

        if (isBlocked(cx + rp, cy + rp)) return true;
        if (isBlocked(cx + rp, cy - rp)) return true;
        if (isBlocked(cx - rp, cy + rp)) return true;
        if (isBlocked(cx - rp, cy - rp)) return true;

        return false;
    };


    // -------------------------
    // 6. 尝试前进
    // -------------------------
    Vec2 newPos = {
        position_.x + finalDir.x * speed_,
        position_.y + finalDir.y * speed_
    };

    if (!circleBlocked(newPos.x, newPos.y)) {
        position_ = newPos;
        return;
    }

    // -------------------------
    // 7. 强化版滑墙逻辑
    // -------------------------
    // 尝试沿 X 滑动
    Vec2 tryX = { newPos.x, position_.y };
    if (!circleBlocked(tryX.x, tryX.y)) {
        position_.x = tryX.x;
        return;
    }

    // 尝试沿 Y 滑动
    Vec2 tryY = { position_.x, newPos.y };
    if (!circleBlocked(tryY.x, tryY.y)) {
        position_.y = tryY.y;
        return;
    }

    // -------------------------
    // 8. 最终尝试：沿墙方向小步挤过去
    // （防止永远卡在墙角）
    // -------------------------
    for (auto& d : dirs8)
    {
        float nx = position_.x + d[0] * 0.4f;
        float ny = position_.y + d[1] * 0.4f;

        if (!circleBlocked(nx, ny)) {
            position_.x = nx;
            position_.y = ny;
            return;
        }
    }
}


