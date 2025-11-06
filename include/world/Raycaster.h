#ifndef RAYCASTER_H
#define RAYCASTER_H

#include "core/Types.h"
#include "world/Map.h"
#include <vector>

class Raycaster {
public:
    Raycaster(const Map& map);
    
    // cast multiple rays
    std::vector<RayHit> castRays(const Vec2& origin, float startAngle, 
                                  float fov, int numRays) const;
    
    // cast single ray
    RayHit castRay(const Vec2& origin, float angle) const;
    
private:
    const Map& map_;
    
    RayHit checkHorizontalIntersections(const Vec2& origin, float angle) const;
    RayHit checkVerticalIntersections(const Vec2& origin, float angle) const;
};

#endif // RAYCASTER_H
