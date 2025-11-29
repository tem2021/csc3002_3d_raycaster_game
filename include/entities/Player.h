#ifndef PLAYER_H
#define PLAYER_H

#include "core/Types.h"
#include "world/Map.h"
#include <memory>

class Weapon;  // Forward declaration

class Player {
public:
    Player(const Vec2& position, float angle, float moveSpeed, int health);
    ~Player();  // Destructor needed for unique_ptr<Weapon>
    
    // Movement
    void moveForward(const Map& map, float speedMultiplier = 1.0f);
    void moveBackward(const Map& map, float speedMultiplier = 1.0f);
    void strafeLeft(const Map& map, float speedMultiplier = 1.0f);
    void strafeRight(const Map& map, float speedMultiplier = 1.0f);
    void rotate(float deltaAngle);
    
    // Getters
    Vec2 getPosition() const { return position_; }
    Vec2 getDirection() const { return direction_; }
    float getAngle() const { return angle_; }
    float getMoveSpeed() const { return moveSpeed_; }
    
    int getHealth() const { return health_; }
    void takeDamage(int amount) { health_ -= amount; if (health_ < 0) health_ = 0; }
    
    // Weapon state management
    bool hasWeapon() const { return hasWeapon_; }
    void equipWeapon() { hasWeapon_ = true; }
    void unequipWeapon() { hasWeapon_ = false; }
    
    // Weapon methods
    Weapon* getWeapon() { return weapon_.get(); }
    const Weapon* getWeapon() const { return weapon_.get(); }
    bool fireWeapon();
    void reloadWeapon();
    
    void takeDamagePlayer(int amount) { health_ -= amount; if (health_ < 0) health_ = 0; }

private:
    Vec2 position_;
    Vec2 direction_;
    float angle_;
    float moveSpeed_;

    int health_;
    bool hasWeapon_;  // 玩家是否持有武器
    std::unique_ptr<Weapon> weapon_;  // 武器指针
    
    // Update the velocity (moveSpeed) component on x and y
    void updateDirection();

    bool willCollide(const Vec2& newPos, const Map& map) const;
    void tryMove(const Vec2& delta, const Map& map);
};

#endif // PLAYER_H
