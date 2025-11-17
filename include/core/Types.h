#ifndef TYPES_H
#define TYPES_H

#include <cmath>

// 2D Vectors
struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;
    
    Vec2() = default;
    Vec2(float x, float y) : x(x), y(y) {}
    
    Vec2 operator+(const Vec2& other) const {
        return Vec2{x + other.x, y + other.y};
    }
    
    Vec2 operator-(const Vec2& other) const {
        return Vec2{x - other.x, y - other.y};
    }
    
    Vec2 operator*(float scalar) const {
        return Vec2{x * scalar, y * scalar};
    }
    
    float distanceTo(const Vec2& other) const {
        float dx = other.x - x;
        float dy = other.y - y;
        return std::sqrt(dx * dx + dy * dy);
    }
    
    float length() const {
        return std::sqrt(x * x + y * y);
    }
    
    Vec2 normalized() const {
        float len = length();
        if (len > 0.0f) {
            return Vec2{x / len, y / len};
        }
        return Vec2{0.0f, 0.0f};
    }
};

// Results of RayHit
struct RayHit {
    bool hit = false;
    float distance = 0.0f;
    Vec2 hitPoint;
    bool isVertical = false;  // Whether hit the wall vertically
    int wallType = 0;         // wallType 
    float wallHitX = 0.0f;    // Hit place (0.0-1.0)
};

#endif // TYPES_H
