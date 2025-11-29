#ifndef WEAPON_H
#define WEAPON_H

#include "core/Types.h"
#include "world/Raycaster.h"

// 武器类 - 最小实现，仅用于编译通过
class Weapon {
public:
    Weapon() : isFiring_(false), fireAnimationTime_(0.0f) {}
    
    // 最小必要接口
    int getDamage() const { return 0; }
    float getRange() const { return 0.0f; }
    
    bool fire();
    void reload() {}
    void update(float deltaTime);
    
    // 武器动画状态
    bool isFiring() const { return isFiring_; }
    
private:
    bool isFiring_;              // 是否正在发射
    float fireAnimationTime_;    // 发射动画计时器
};

// 武器命中检测类
// 功能：从玩家位置发射一条正中间的射线，检测是否击中墙壁并返回墙壁特征
// 注意：只有在玩家持有武器状态下才会返回有效的命中结果
class WeaponHitDetector {
public:
    // 发射一条射线并返回命中信息
    // origin: 射线起点（玩家位置）
    // angle: 射线方向（玩家朝向角度）
    // hasWeapon: 玩家是否持有武器
    // raycaster: 用于射线投射的raycaster实例
    // 返回: 如果玩家持有武器，返回命中信息；否则返回空的RayHit（hit=false）
    static RayHit fireRay(const Vec2& origin, float angle, bool hasWeapon, const Raycaster& raycaster);
};

#endif // WEAPON_H
