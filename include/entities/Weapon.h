#ifndef WEAPON_H
#define WEAPON_H

#include "core/Types.h"
#include "world/Raycaster.h"

// 三种水果类型
enum class FruitType {
    Watermelon, // 西瓜 → 河马
    Bamboo,     // 竹子 → 熊猫
    Banana      // 香蕉 → 猴子
};

class Weapon {
public:
    Weapon()
        : isFiring_(false)
        , fireAnimationTime_(0.0f)
        , currentFruit_(FruitType::Watermelon) // 默认西瓜
    {
    }

    int getDamage() const { return 0; }
    float getRange() const { return 0.0f; }

    bool fire();
    void reload() {}
    void update(float deltaTime);

    bool isFiring() const { return isFiring_; }

    // 供 Renderer 做补间动画用的 0~1 进度
    float getFireAnimationProgress() const;

    FruitType getFruitType() const { return currentFruit_; }
    void setFruitType(FruitType type) { currentFruit_ = type; }

private:
    bool  isFiring_;
    float fireAnimationTime_;
    FruitType currentFruit_;
};

// 射线命中检测类
class WeaponHitDetector {
public:
    static RayHit fireRay(const Vec2& origin,
        float angle,
        bool hasWeapon,
        const Raycaster& raycaster);
};

#endif // WEAPON_H


