#ifndef WEAPON_H
#define WEAPON_H

#include <string>

enum class WeaponType {
    PISTOL,
    SHOTGUN,
    RIFLE
};

class Weapon {
public:
    Weapon(WeaponType type, int damage, int maxAmmo, float fireRate, float range);
    
    // Weapon actions
    bool fire();  // Returns true if weapon was fired successfully
    void reload();
    void update(float deltaTime);  // Update cooldown timer
    
    // Getters
    WeaponType getType() const { return type_; }
    int getDamage() const { return damage_; }
    int getCurrentAmmo() const { return currentAmmo_; }
    int getMaxAmmo() const { return maxAmmo_; }
    float getRange() const { return range_; }
    bool canFire() const;
    bool needsReload() const { return currentAmmo_ <= 0; }
    
    // Setters
    void addAmmo(int amount);
    
private:
    WeaponType type_;
    int damage_;
    int currentAmmo_;
    int maxAmmo_;
    float fireRate_;      // Time between shots in seconds
    float range_;         // Maximum effective range
    float cooldownTimer_; // Current cooldown timer
    
    void resetCooldown();
};

#endif // WEAPON_H
