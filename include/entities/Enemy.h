#ifndef ENEMY_H
#define ENEMY_H

#include "core/Types.h"
#include "world/Map.h"
#include <cmath>
#include <cstdlib>

class Enemy {
public:
    // Hippo
    static constexpr int HIPPO_NORMAL = 200;
    static constexpr int HIPPO_ANGRY  = 201;
    static constexpr int HIPPO_HAPPY  = 202;

    // Panda
    static constexpr int PANDA_NORMAL = 210;
    static constexpr int PANDA_ANGRY  = 211;
    static constexpr int PANDA_HAPPY  = 212;

    // Monkey
    static constexpr int MONKEY_NORMAL = 220;
    static constexpr int MONKEY_ANGRY  = 221;
    static constexpr int MONKEY_HAPPY  = 222;

    // 敌人情绪状态
    enum class EmotionState {
        Normal,
        Angry,
        Happy
    };

    // 敌人类型（物种）
    enum class EnemyType {
        Hippo,
        Panda,
        Monkey
    };

    // 构造函数
    Enemy(const Vec2& pos, float speed, EnemyType type)
        :  type_(type), position_(pos),speed_(speed)
    {
        switch (type_) {
            case EnemyType::Hippo:
                textureId_ = HIPPO_NORMAL;
                attackDamage_ = 5;   // 河马中等
                break;

            case EnemyType::Panda:
                textureId_ = PANDA_NORMAL;
                attackDamage_ = 8;    // 熊猫最高
                break;

            case EnemyType::Monkey:
                textureId_ = MONKEY_NORMAL;
                attackDamage_ = 3;    // 猴子攻击低
                break;
        }

        health_ = 100;
        alive_ = true;
    }

    const Vec2& getPosition() const { return position_; }
    int getHealth() const { return health_; }
    bool isAlive() const { return alive_; }
    int getTextureId() const { return textureId_; }
    int getAttackDamage() const { return attackDamage_; }
    EmotionState getEmotionState() const { return state_; }
    EnemyType getType() const { return type_; }

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
    void onFedSuccess();  // 喂食成功 → 敌人消失

    int attackCooldownFrames_ = 0;  // 攻击冷却
    // === 连续正确喂食计数 ===
    int  getCorrectFeedCount() const { return correctFeedCount_; }
    void resetCorrectFeedCount() { correctFeedCount_ = 0; }
    void incrementCorrectFeedCount() { ++correctFeedCount_; }

private:
    EnemyType type_;       // 物种
    Vec2 position_;
    int health_;
    bool alive_;
    float speed_;
    float timeOffset_ = (rand() % 1000) * 0.01f;
    float radius_ = 10.0f;
    bool isAggro_ = false;  // 自动愤怒
    bool manualOverride_ = false;   // 手动 override（按键 1/2）
    int manualOverrideFrames_ = 0;
    int attackDamage_ = 5;   // 默认扣 5 点血

    int textureId_;
    int correctFeedCount_ = 0;  // 同一只动物连续喂对的次数

    // 情绪系统
    EmotionState state_ = EmotionState::Normal;
    int stateFramesRemaining_ = 0;   // 剩余帧数

};
#endif
