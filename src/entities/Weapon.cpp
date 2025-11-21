#include "entities/Weapon.h"

RayHit WeaponHitDetector::fireRay(const Vec2& origin, float angle, const Raycaster& raycaster) {
    // 使用raycaster发射一条射线
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
