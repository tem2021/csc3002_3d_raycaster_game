#include "entities/Player.h"
#include "entities/Weapon.h"
#include "core/Config.h"
#include <cmath>

Player::Player(const Vec2& position, float angle, float moveSpeed, int health)
    : position_(position), angle_(angle), moveSpeed_(moveSpeed), health_(health), hasWeapon_(true) {
    updateDirection();
    weapon_ = std::make_unique<Weapon>();
}

Player::~Player() = default;

void Player::updateDirection() {
    direction_.x = std::cos(angle_) * moveSpeed_;
    direction_.y = std::sin(angle_) * moveSpeed_;
}

void Player::rotate(float deltaAngle) {
    angle_ += deltaAngle;
    
    // Normalize the angle to [0, 2π]
    while (angle_ < 0.0f) {
        angle_ += Math::TWO_PI;
    }
    while (angle_ > Math::TWO_PI) {
        angle_ -= Math::TWO_PI;
    }
    
    updateDirection();
}

bool Player::willCollide(const Vec2& newPos, const Map& map) const {
    int tileSize = map.getTileSize();
    int mapX = static_cast<int>(newPos.x) / tileSize;
    int mapY = static_cast<int>(newPos.y) / tileSize;
    
    return map.isWall(mapX, mapY);
}

void Player::tryMove(const Vec2& delta, const Map& map) {
    Vec2 newPos = position_ + delta;
    
    if (!willCollide(newPos, map)) {
        position_ = newPos;
    }
}

void Player::moveForward(const Map& map, float speedMultiplier) {
    tryMove(direction_ * speedMultiplier, map);
}

void Player::moveBackward(const Map& map, float speedMultiplier) {
    tryMove(direction_ * -1.0f * speedMultiplier, map);
}

void Player::strafeLeft(const Map& map, float speedMultiplier) {
    Vec2 strafeDir{std::sin(angle_) * moveSpeed_ * speedMultiplier, 
                   -std::cos(angle_) * moveSpeed_ * speedMultiplier};
    tryMove(strafeDir, map);
}

void Player::strafeRight(const Map& map, float speedMultiplier) {
    Vec2 strafeDir{-std::sin(angle_) * moveSpeed_ * speedMultiplier, 
                   std::cos(angle_) * moveSpeed_ * speedMultiplier};
    tryMove(strafeDir, map);
}

bool Player::fireWeapon() {
    if (!weapon_) return false;
    return weapon_->fire();
}

void Player::reloadWeapon() {
    if (weapon_) {
        weapon_->reload();
    }
}
