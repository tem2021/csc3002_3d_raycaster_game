#include "entities/Weapon.h"
#include "core/Config.h"

bool Weapon::fire() {
    // Start fire animation
    isFiring_ = true;
    fireAnimationTime_ = 0.0f;
    return true;
}

void Weapon::update(float deltaTime) {
    // Update fire animation
    if (isFiring_) {
        fireAnimationTime_ += deltaTime;
        if (fireAnimationTime_ >= RenderConfig::FIRE_ANIMATION_DURATION) {
            isFiring_ = false;
            fireAnimationTime_ = 0.0f;
        }
    }
}

float Weapon::getFireAnimationProgress() const {
    if (!isFiring_) return 0.0f;
    float duration = RenderConfig::FIRE_ANIMATION_DURATION;
    if (duration <= 0.0f) return 0.0f;

    float t = fireAnimationTime_ / duration;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    return t;
}

RayHit WeaponHitDetector::fireRay(const Vec2& origin, float angle, bool hasWeapon, const Raycaster& raycaster) {
    // 只有在持有武器状态下才进行命中检测
    if (!hasWeapon) {
        // 未持有武器时，返回空的RayHit（表示没有命中）
        RayHit emptyHit;
        emptyHit.hit = false;
        return emptyHit;
    }
    
    // 持有武器时，使用raycaster发射一条射线
    // 这条射线从玩家位置出发，沿着玩家朝向的方向（正中间）
    RayHit hit = raycaster.castRay(origin, angle);
    
    // 返回命中信息，包含：
    // - hit.hit: 是否击中墙壁
    // - hit.distance: 到墙壁的距离
    // - hit.hitPoint: 击中点的坐标
    // - hit.isVertical: 是否击中垂直墙面
    // - hit.wallType: 墙壁类型（特征值）
    // - hit.wallHitX: 击中墙壁的X坐标（用于纹理）
    return hit;
}
