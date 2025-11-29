#include "entities/Enemy.h"
#include "core/Config.h"
#include <cmath>

// 每种情绪持续的帧数（按 60fps 算，大约 0.5 秒）
constexpr int ANGRY_DURATION_FRAMES = GameConfig::TARGET_FPS / 2; // 30 帧
constexpr int HAPPY_DURATION_FRAMES = GameConfig::TARGET_FPS / 2; // 30 帧

// ----------------------------------------------------------
// 连续空间 + 蛇形 + 8方向导航 + 强化滑墙
// ----------------------------------------------------------

void Enemy::update(const Vec2& playerPos, const Map& map)
{
    float dx = playerPos.x - position_.x;
    float dy = playerPos.y - position_.y;
    float distToPlayer = std::sqrt(dx * dx + dy * dy);

    int ts = map.getTileSize();  // tileSize 例如 11~12 像素

    // 愤怒/恢复距离（基于 tile）
    const float agroDistance    = ts * 2.0f;   // 靠近 12 格
    const float deAgroDistance  = ts * 2.0f;   // 离开 16 格才恢复


    // 手动覆盖优先级（按键 1/2 触发）
    if (manualOverride_) {
        manualOverrideFrames_--;
        if (manualOverrideFrames_ <= 0) {
            manualOverride_ = false;
        }
    }


    // 自动愤怒 / 自动恢复（只有非手动状态时执行）
    // 且 Happy 不会被自动AI覆盖

    if (!manualOverride_ && state_ != EmotionState::Happy)
    {
        // ---- 自动进入愤怒 ----
        if (!isAggro_ && distToPlayer < agroDistance) {
            isAggro_ = true;
            state_ = EmotionState::Angry;
            textureId_ = ANGRY_TEXTURE_ID;
        }

        // ---- 自动恢复普通 ----
        if (isAggro_ && distToPlayer > deAgroDistance) {
            isAggro_ = false;
            state_ = EmotionState::Normal;
            textureId_ = NORMAL_TEXTURE_ID;
        }
    }

    // 情绪计时（用于 Happy / 手动 Angry）
    if (!isAggro_ && state_ != EmotionState::Normal)
    {
        if (stateFramesRemaining_ > 0) {
            stateFramesRemaining_--;
        }

        if (stateFramesRemaining_ <= 0) {
            state_ = EmotionState::Normal;
            textureId_ = NORMAL_TEXTURE_ID;
        }
    }

    float dist = distToPlayer;

    if (dist < 0.01f) return;
    // --- A. 在玩家面前一个合理距离停下来 ---

    float attackRange = 15.0f;   // 可以调大调小

    // 如果已经接近玩家，就停下（不向前移动）
    if (dist < attackRange) {
        // 但仍然保持朝向玩家（不转身逃跑，不乱动）
        // 保持敌人原地“准备攻击”的状态
        timeOffset_ += 0.015f;   // 保留蛇形动画的小抖动（可删）
        return;                  // 不执行后续移动逻辑
    }

    // -------------------------
    // 1. 基本方向
    // -------------------------
    Vec2 forward = { dx / dist, dy / dist };
    Vec2 perp = { -forward.y, forward.x };

    // -------------------------
    // 2. 蛇形（保持原来的效果）
    // -------------------------
    timeOffset_ += (dist < 5.0f ? 0.05f : 0.02f);
    float snake = std::sin(timeOffset_ * 2.5f);
    float lateral = snake * 0.40f;

    // -------------------------
    // 3. tile-level 转弯辅助（加强版）
    // -------------------------
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
        forward.x * 0.6f + tileAssist.x * 0.35f + perp.x * lateral,
        forward.y * 0.6f + tileAssist.y * 0.35f + perp.y * lateral
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
    // 7. 强化滑墙逻辑
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

void Enemy::onFedWrong()
{
    // 手动覆盖自动 AI
    manualOverride_ = true;
    manualOverrideFrames_ = 30;

    state_ = EmotionState::Angry;
    textureId_ = ANGRY_TEXTURE_ID;
    stateFramesRemaining_ = 30;  // 原来保持 0.5 秒
}

void Enemy::takeDamage(int damage) {
    if (!alive_) return;
    
    health_ -= damage;
    if (health_ <= 0) {
        health_ = 0;
        alive_ = false;
    }
}
void Enemy::onFedCorrect()
{
    // 手动覆盖自动 AI
    manualOverride_ = true;
    manualOverrideFrames_ = 30;

    state_ = EmotionState::Happy;
    textureId_ = HAPPY_TEXTURE_ID;
    stateFramesRemaining_ = 30;
}


void Enemy::resetEmotion()
{
    state_ = EmotionState::Normal;
    stateFramesRemaining_ = 0;
    textureId_ = NORMAL_TEXTURE_ID;
}

