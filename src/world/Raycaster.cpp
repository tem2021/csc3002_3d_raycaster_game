#include "world/Raycaster.h"
#include "core/Config.h"
#include <cmath>

Raycaster::Raycaster(const Map& map) : map_(map) {}

std::vector<RayHit> Raycaster::castRays(const Vec2& origin, float startAngle, float fov, int numRays) const {
    // use the vector to store all RayHits for every angles 
    // casting-Ray's angle : from player's angle - fov/2 to player's angle to fov/2

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
    // normalize the angle to [0, 2π]
    while (angle < 0.0f) angle += Math::TWO_PI;
    while (angle > Math::TWO_PI) angle -= Math::TWO_PI;
    
    // check the hitting points vertically and horizontally
    RayHit hHit = checkHorizontalIntersections(origin, angle);
    RayHit vHit = checkVerticalIntersections(origin, angle);
    
    // return the nearest hitting point
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
    
    // yo, xo: in each step, how far the ray goes 
    // ry, rx: the endpoints of the rays at each step
    float ry, rx, yo, xo;
    
    // since I initialize the GLUT using x grows from left to right 
    // and y grows from top to bottom, the angle grows clockwise.  
    // angle 0 looks like ->; angle π/2 points directly downwards
   
    // initialize the first step
    // points up
    if (angle > Math::PI) {  

        // -epsilon: ensure the endpoints is above the horizontal lines
        // to avoid possible mistakes due to the floating point
        ry = (static_cast<int>(origin.y) / mapS) * mapS - epsilon;
        rx = (ry - origin.y) / std::tan(angle) + origin.x;

        yo = -mapS;
        xo = yo / std::tan(angle);
    }
    
    // points down
    else if (angle < Math::PI) {  

        // +epsilon: ensure the endpoints is below the horizontal lines
        ry = (static_cast<int>(origin.y) / mapS) * mapS + mapS + epsilon;
        rx = (ry - origin.y) / std::tan(angle) + origin.x;
        yo = mapS;
        xo = yo / std::tan(angle);
    }
   
    // points horizontally
    else {  
        return hit;  // never hit the horizontal edges of the walls
    }
    
    // step by step check
    int depth = 0; 
    int maxDepth = map_.getWidth();  
    
    while (depth < maxDepth) {
        int mx = static_cast<int>(rx) / mapS;
        int my = static_cast<int>(ry) / mapS;
        
        if (map_.isWall(mx, my)) {
            hit.hit = true;
            hit.hitPoint = Vec2{rx, ry};
            hit.distance = origin.distanceTo(hit.hitPoint);
            hit.isVertical = false;
            hit.wallType = map_.getWallType(mx, my);
           
            // calculate the x coordinate of the hitting point correspoding the to wall
            hit.wallHitX = rx / mapS - mx;
            break;
        }
        
        rx += xo;
        ry += yo;
        depth++;
    }
    
    return hit;
}

RayHit Raycaster::checkVerticalIntersections(const Vec2& origin, float angle) const {
    RayHit hit;
    int mapS = map_.getTileSize();
    float epsilon = mapS * Math::EPSILON;
    
    float rx, ry, xo, yo;
    
    // point left
    if (angle > Math::HALF_PI && angle < Math::HALF_PI * 3) {  
        rx = (static_cast<int>(origin.x) / mapS) * mapS - epsilon;
        ry = (rx - origin.x) * std::tan(angle) + origin.y;
        xo = -mapS;
        yo = xo * std::tan(angle);
    } 
    
    // point right
    else if (angle < Math::HALF_PI || angle > Math::HALF_PI * 3) {  
        rx = (static_cast<int>(origin.x) / mapS) * mapS + mapS + epsilon;
        ry = (rx - origin.x) * std::tan(angle) + origin.y;
        xo = mapS;
        yo = xo * std::tan(angle);
    } 
    
    // point vertically
    else { 
        return hit;
    }
    
    int depth = 0;
    int maxDepth = map_.getHeight();
    
    while (depth < maxDepth) {
        int mx = static_cast<int>(rx) / mapS;
        int my = static_cast<int>(ry) / mapS;
        
        if (map_.isWall(mx, my)) {
            hit.hit = true;
            hit.hitPoint = Vec2{rx, ry};
            hit.distance = origin.distanceTo(hit.hitPoint);
            hit.isVertical = true;
            hit.wallType = map_.getWallType(mx, my);
            hit.wallHitX = ry / mapS - my;
            break;
        }
        
        rx += xo;
        ry += yo;
        depth++;
    }
    
    return hit;
}
