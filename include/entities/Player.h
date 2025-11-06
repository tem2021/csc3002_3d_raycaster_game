#ifndef PLAYER_H
#define PLAYER_H

#include "core/Types.h"
#include "world/Map.h"

class Player {
public:
    Player(const Vec2& position, float angle, float moveSpeed);
    
    // Movement
    void moveForward(const Map& map, float speedMultiplier = 1.0f);
    void moveBackward(const Map& map, float speedMultiplier = 1.0f);
    void strafeLeft(const Map& map, float speedMultiplier = 1.0f);
    void strafeRight(const Map& map, float speedMultiplier = 1.0f);
    void rotate(float deltaAngle);
    
    // Getters
    Vec2 getPosition() const { return position_; }
    float getAngle() const { return angle_; }
    float getMoveSpeed() const { return moveSpeed_; }
    Vec2 getDirection() const { return direction_; }
    
private:
    Vec2 position_;
    Vec2 direction_;
    float angle_;
    float moveSpeed_;
    
    void updateDirection();
    bool willCollide(const Vec2& newPos, const Map& map) const;
    void tryMove(const Vec2& delta, const Map& map);
};

#endif // PLAYER_H
