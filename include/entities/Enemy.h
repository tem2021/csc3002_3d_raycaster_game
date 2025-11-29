#ifndef ENEMY_H
#define ENEMY_H

#include "core/Types.h"
#include "world/Map.h"
#include <cmath>

class Enemy {
public:
    static constexpr int NORMAL_TEXTURE_ID = 200;
    static constexpr int ANGRY_TEXTURE_ID  = 201;
    static constexpr int HAPPY_TEXTURE_ID  = 202;

    static constexpr int DEFAULT_TEXTURE_ID = NORMAL_TEXTURE_ID;

    // 敌人情绪状态
    enum class EmotionState {
        Normal,
        Angry,
        Happy
    };

    Enemy(const Vec2& pos, float speed, int textureId = DEFAULT_TEXTURE_ID)
        : position_(pos), health_(50), alive_(true), speed_(speed),
        textureId_(textureId){}

    const Vec2& getPosition() const { return position_; }
    int getHealth() const { return health_; }
    bool isAlive() const { return alive_; }
    int getTextureId() const { return textureId_; }

    void update(const Vec2& playerPos, const Map& map);
    void takeDamageEnemy(int damage);

    // ---- 新增：允许 Game.cpp 进行修正 ----
    void setPosition(const Vec2& p) { position_ = p; }
    void addOffset(const Vec2& delta) {
        position_.x += delta.x;
        position_.y += delta.y;
    }

    // === 给喂食 / 武器组调用的接口 ===
    void onFedWrong();    // 喂错 → 愤怒形态
    void onFedCorrect();  // 喂对 → 乖巧形态
    void resetEmotion();  // 可选：强制恢复普通形态

    EmotionState getEmotionState() const { return state_; }
    int attackCooldownFrames_ = 0;

private:
    Vec2 position_;
    int health_;
    bool alive_;
    float speed_;
    float timeOffset_ = (rand() % 1000) * 0.01f;
    float radius_ = 10.0f;
    bool isAggro_ = false;
    bool manualOverride_ = false;
    int manualOverrideFrames_ = 0;

    int textureId_;
    
    // 表情状态机
    EmotionState state_ = EmotionState::Normal;
    int stateFramesRemaining_ = 0;   // 剩余帧数
};
#endif
