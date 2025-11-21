#ifndef WEAPON_H
#define WEAPON_H

#include "core/Types.h"
#include "world/Raycaster.h"

// 武器类型枚举
enum class WeaponType {
    PISTOL,
    SHOTGUN,
    RIFLE
};

// 武器类
class Weapon {
public:
    // Default weapon cooldown in seconds
    static constexpr float DEFAULT_COOLDOWN = 0.5f;
    
    Weapon(WeaponType type = WeaponType::PISTOL, int maxAmmo = 10, int damage = 10, float range = 100.0f)
        : type_(type), maxAmmo_(maxAmmo), currentAmmo_(maxAmmo), damage_(damage), range_(range), cooldown_(0.0f) {}
    
    // Getters
    WeaponType getType() const { return type_; }
    int getCurrentAmmo() const { return currentAmmo_; }
    int getMaxAmmo() const { return maxAmmo_; }
    int getDamage() const { return damage_; }
    float getRange() const { return range_; }
    
    // Weapon state
    bool needsReload() const { return currentAmmo_ <= 0; }
    bool canFire() const { return currentAmmo_ > 0 && cooldown_ <= 0.0f; }
    
    // Actions
    bool fire() {
        if (!canFire()) return false;
        currentAmmo_--;
        cooldown_ = DEFAULT_COOLDOWN;
        return true;
    }
    
    void reload() {
        currentAmmo_ = maxAmmo_;
    }
    
    void update(float deltaTime) {
        if (cooldown_ > 0.0f) {
            cooldown_ -= deltaTime;
        }
    }
    
private:
    WeaponType type_;
    int maxAmmo_;
    int currentAmmo_;
    int damage_;
    float range_;
    float cooldown_;
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
