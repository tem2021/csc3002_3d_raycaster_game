#include "entities/Weapon.h"

Weapon::Weapon(WeaponType type, int damage, int maxAmmo, float fireRate, float range)
    : type_(type), damage_(damage), currentAmmo_(maxAmmo), maxAmmo_(maxAmmo),
      fireRate_(fireRate), range_(range), cooldownTimer_(0.0f) {
}

bool Weapon::fire() {
    if (!canFire()) {
        return false;
    }
    
    currentAmmo_--;
    resetCooldown();
    return true;
}

void Weapon::reload() {
    currentAmmo_ = maxAmmo_;
}

void Weapon::update(float deltaTime) {
    if (cooldownTimer_ > 0.0f) {
        cooldownTimer_ -= deltaTime;
        if (cooldownTimer_ < 0.0f) {
            cooldownTimer_ = 0.0f;
        }
    }
}

bool Weapon::canFire() const {
    return currentAmmo_ > 0 && cooldownTimer_ <= 0.0f;
}

void Weapon::addAmmo(int amount) {
    currentAmmo_ += amount;
    if (currentAmmo_ > maxAmmo_) {
        currentAmmo_ = maxAmmo_;
    }
}

void Weapon::resetCooldown() {
    cooldownTimer_ = fireRate_;
}
