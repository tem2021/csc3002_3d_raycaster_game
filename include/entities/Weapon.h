#ifndef WEAPON_H
#define WEAPON_H

#include "core/Types.h"
#include "world/Raycaster.h"

// 武器命中检测类
// 功能：从玩家位置发射一条正中间的射线，检测是否击中墙壁并返回墙壁特征
class WeaponHitDetector {
public:
    // 发射一条射线并返回命中信息
    // origin: 射线起点（玩家位置）
    // angle: 射线方向（玩家朝向角度）
    // raycaster: 用于射线投射的raycaster实例
    static RayHit fireRay(const Vec2& origin, float angle, const Raycaster& raycaster);
};

#endif // WEAPON_H
