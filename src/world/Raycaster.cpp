#include "world/Raycaster.h"
#include "core/Config.h"
#include <cmath>

Raycaster::Raycaster(const Map& map) : map_(map) {}

std::vector<RayHit> Raycaster::castRays(const Vec2& origin, float startAngle, 
                                         float fov, int numRays) const {
    std::vector<RayHit> hits;
    hits.reserve(numRays);
    
    float rayStep = (fov * Math::DEG_TO_RAD) / numRays;
    float angle = startAngle - (fov * Math::DEG_TO_RAD / 2.0f);
    
    for (int i = 0; i < numRays; ++i) {
        hits.push_back(castRay(origin, angle));
        angle += rayStep;
    }
    
    return hits;
}

RayHit Raycaster::castRay(const Vec2& origin, float angle) const {
    // 规范化角度
    while (angle < 0.0f) angle += Math::TWO_PI;
    while (angle > Math::TWO_PI) angle -= Math::TWO_PI;
    
    // 检查水平和垂直交点
    RayHit hHit = checkHorizontalIntersections(origin, angle);
    RayHit vHit = checkVerticalIntersections(origin, angle);
    
    // 返回最近的击中
    if (hHit.hit && vHit.hit) {
        return (hHit.distance < vHit.distance) ? hHit : vHit;
    } else if (hHit.hit) {
        return hHit;
    } else {
        return vHit;
    }
}

RayHit Raycaster::checkHorizontalIntersections(const Vec2& origin, float angle) const {
    RayHit hit;
    int mapS = map_.getTileSize();
    float epsilon = mapS * Math::EPSILON;
    
    // 确定光线方向
    float ry, rx, yo, xo;
    
    if (angle > Math::PI) {  // 朝上
        ry = (static_cast<int>(origin.y) / mapS) * mapS - epsilon;
        rx = (ry - origin.y) / std::tan(angle) + origin.x;
        yo = -mapS;
        xo = yo / std::tan(angle);
    } else if (angle < Math::PI) {  // 朝下
        ry = (static_cast<int>(origin.y) / mapS) * mapS + mapS + epsilon;
        rx = (ry - origin.y) / std::tan(angle) + origin.x;
        yo = mapS;
        xo = yo / std::tan(angle);
    } else {  // 水平方向
        return hit;  // 不会击中水平线
    }
    
    // 步进检查
    int dof = 0;
    int maxDof = map_.getWidth();
    
    while (dof < maxDof) {
        int mx = static_cast<int>(rx) / mapS;
        int my = static_cast<int>(ry) / mapS;
        
        if (map_.isWall(mx, my)) {
            hit.hit = true;
            hit.hitPoint = Vec2{rx, ry};
            hit.distance = origin.distanceTo(hit.hitPoint);
            hit.isVertical = false;
            hit.wallType = map_.getWallType(mx, my);
            // 计算墙面碰撞的X坐标 (0.0-1.0)
            hit.wallHitX = rx / mapS - std::floor(rx / mapS);
            break;
        }
        
        rx += xo;
        ry += yo;
        dof++;
    }
    
    return hit;
}

RayHit Raycaster::checkVerticalIntersections(const Vec2& origin, float angle) const {
    RayHit hit;
    int mapS = map_.getTileSize();
    float epsilon = mapS * Math::EPSILON;
    
    float rx, ry, xo, yo;
    
    if (angle > Math::HALF_PI && angle < Math::HALF_PI * 3) {  // 朝左
        rx = (static_cast<int>(origin.x) / mapS) * mapS - epsilon;
        ry = (rx - origin.x) * std::tan(angle) + origin.y;
        xo = -mapS;
        yo = xo * std::tan(angle);
    } else if (angle < Math::HALF_PI || angle > Math::HALF_PI * 3) {  // 朝右
        rx = (static_cast<int>(origin.x) / mapS) * mapS + mapS + epsilon;
        ry = (rx - origin.x) * std::tan(angle) + origin.y;
        xo = mapS;
        yo = xo * std::tan(angle);
    } else {  // 垂直方向
        return hit;
    }
    
    int dof = 0;
    int maxDof = map_.getHeight();
    
    while (dof < maxDof) {
        int mx = static_cast<int>(rx) / mapS;
        int my = static_cast<int>(ry) / mapS;
        
        if (map_.isWall(mx, my)) {
            hit.hit = true;
            hit.hitPoint = Vec2{rx, ry};
            hit.distance = origin.distanceTo(hit.hitPoint);
            hit.isVertical = true;
            hit.wallType = map_.getWallType(mx, my);
            // 计算墙面碰撞的Y坐标 (0.0-1.0)
            hit.wallHitX = ry / mapS - std::floor(ry / mapS);
            break;
        }
        
        rx += xo;
        ry += yo;
        dof++;
    }
    
    return hit;
}
