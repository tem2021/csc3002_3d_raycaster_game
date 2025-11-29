#include "core/Game.h"
#include "core/Config.h"
#include "entities/Weapon.h"
#include "data/maps/level1.h"
#include <cmath>

// Include texture data (only those with 3D array format)
#include "data/textures/brick.h"
#include "data/textures/wood.h"
#include "data/textures/metal.h"
#include "data/textures/stone.h"
#include "data/textures/grass.h"
#include "data/textures/dirt.h"
#include "data/textures/glass.h"
#include "data/textures/marble.h"
#include "data/textures/concrete.h"
#include "data/textures/CUHK_SZ.h"
#include "data/textures/Hajimi.h"
#include "data/textures/pistol.h"
#include "data/textures/Hippo_1.h"
#include "data/textures/Hippo_2.h"
#include "data/textures/Hippo_3.h"
#include "data/textures/unfiredgun.h"
#include "data/textures/firedgun.h"

#include "rendering/TextureManager.h"

#include <GL/freeglut_std.h>
#include <iostream>

#ifdef _WIN32
    #include <GL/freeglut.h>
#elif defined(__APPLE__)
    #include <GLUT/glut.h>
#elif defined(__linux__)
    #include <GL/freeglut.h>
#endif

Game::Game() : screenWidth_(0), screenHeight_(0), deltaTime_(0.0166667f) {}

void Game::init() {
    // gain the size of the screen
    screenWidth_ = glutGet(GLUT_SCREEN_WIDTH);
    screenHeight_ = glutGet(GLUT_SCREEN_HEIGHT);
    
    // create the map
    int tileSize = screenHeight_ / MapData::LEVEL1_HEIGHT;
    map_ = std::make_unique<Map>(
        MapData::LEVEL1,
        MapData::LEVEL1_WIDTH,
        MapData::LEVEL1_HEIGHT,
        MapData::LEVEL1_INIT_X,
        MapData::LEVEL1_INIT_Y,
        tileSize
    );
    
    // create the player
    float moveSpeed = GameConfig::MOVE_SPEED_FACTOR * tileSize;
    int health = PlayerConfig::MAX_HEALTH;
    player_ = std::make_unique<Player>(
        map_->getInitPosition(),
        0.0f,  // initial angle
        moveSpeed,
        health
    );
    
    // create the raycaster instance
    raycaster_ = std::make_unique<Raycaster>(*map_);
    
    // create the renderer instance
    renderer_ = std::make_unique<Renderer>(screenWidth_, screenHeight_);
    
    // create the inputManager instance
    inputManager_ = std::make_unique<InputManager>(
        screenWidth_ / 2,
        screenHeight_ / 2
    );
    // --- init enemies ---
    enemies_.clear();

    // 新的全地图均匀生成（例如 20 个）
    int enemyCount = 40;
    auto spawnPoints = findDistributedSpawnPoints(enemyCount);

    for (auto& pos : spawnPoints) {
        enemies_.push_back(Enemy(pos, 0.3f));
}

    
    // Load all textures
    loadTextures();
}

void Game::loadTextures() {
    std::cout << "Loading textures..." << std::endl;
    
    // get the textureManager from renderer
    TextureManager& texManager = renderer_->getTextureManager();
    
    // Load textures with their IDs (matching map data)
    // Wall Texture
    texManager.loadTexture(1, BRICK_DATA);
    texManager.loadTexture(2, WOOD_DATA);
    texManager.loadTexture(3, METAL_DATA);
    texManager.loadTexture(4, STONE_DATA);
    texManager.loadTexture(5, GRASS_DATA);    // Floor texture
    texManager.loadTexture(6, DIRT_DATA);
    texManager.loadTexture(7, GLASS_DATA); 
    texManager.loadTexture(8, MARBLE_DATA);
    texManager.loadTexture(9, CONCRETE_DATA);
    texManager.loadTexture(100, CUHK_SZ_DATA);
    texManager.loadTexture(101, HAJIMI_DATA);

    // Enemy Texture
    texManager.loadTexture(200, HIPPO_1_DATA);  // 普通
    texManager.loadTexture(201, HIPPO_2_DATA);  // 愤怒
    texManager.loadTexture(202, HIPPO_3_DATA);  // 乖巧

    //Weapon Texture
    texManager.loadTexture(300, PISTOL_DATA);
    texManager.loadTexture(301, UNFIREDGUN_DATA);
    texManager.loadTexture(302, FIREDGUN_DATA);

    std::cout << "✓ All textures loaded successfully!" << std::endl;
}

void Game::handleInput() {
    processPlayerInput();
    processWeaponInput();
}

// =====================
// helper: 找最近的敌人
// =====================
int getClosestEnemyIndex(const std::vector<Enemy>& enemies, const Player& player)
{
    if (enemies.empty()) return -1;

    Vec2 playerPos = player.getPosition();
    float bestDist = 1e9;
    int bestIndex = -1;

    for (unsigned int i = 0; i < enemies.size(); i++) {
        float dx = enemies[i].getPosition().x - playerPos.x;
        float dy = enemies[i].getPosition().y - playerPos.y;
        float dist = std::sqrt(dx*dx + dy*dy);

        if (dist < bestDist) {
            bestDist = dist;
            bestIndex = i;
        }
    }
    return bestIndex;
}

void Game::processPlayerInput() {
    // whether press shift, use the multiplier on Config.h
    float speedMultiplier = inputManager_->isSprintPressed() ? GameConfig::SPRINT_MULTIPLIER : 1.0f;

    // deal with the keyboard movement
    if (inputManager_->isKeyPressed('w')) {
        player_->moveForward(*map_, speedMultiplier);
    }
    if (inputManager_->isKeyPressed('s')) {
        player_->moveBackward(*map_, speedMultiplier);
    }
    if (inputManager_->isKeyPressed('a')) {
        player_->strafeLeft(*map_, speedMultiplier);
    }
    if (inputManager_->isKeyPressed('d')) {
        player_->strafeRight(*map_, speedMultiplier);
    }
    
    // deal with the mouse rotation
    float mouseDelta = inputManager_->consumeMouseDelta();
    if (mouseDelta != 0.0f) {
        player_->rotate(mouseDelta * GameConfig::ROTATION_SPEED);
    }

    int idx = getClosestEnemyIndex(enemies_, *player_);

    if (idx != -1) {
        if (inputManager_->isKeyPressed('1')) {
            enemies_[idx].onFedWrong();
            std::cout << "[DEBUG] Angry enemy index = " << idx << "\n";
        }
        
        if (inputManager_->isKeyPressed('2')) {
            enemies_[idx].onFedCorrect();
            std::cout << "[DEBUG] Happy enemy index = " << idx << "\n";
        }
    }
}

void Game::update() {
    // Update weapon cooldown
    if (player_->getWeapon()) {
        player_->getWeapon()->update(deltaTime_);
    }
    
    Vec2 playerPos = player_->getPosition();

    for (auto& e : enemies_) {
        if (e.isAlive()) {
            e.update(playerPos, *map_);
        }
    }

    float minDist = 8.0f;
    float pushStrength = 0.2f;

    for (unsigned int i = 0; i < enemies_.size(); i++) {
        for (unsigned int j = i + 1; j < enemies_.size(); j++) {

            Vec2 p1 = enemies_[i].getPosition();
            Vec2 p2 = enemies_[j].getPosition();

            float dx = p2.x - p1.x;
            float dy = p2.y - p1.y;
            float dist = std::sqrt(dx*dx + dy*dy);

            if (dist < minDist && dist > 0.001f) {
                float overlap = (minDist - dist) * 0.5f;

                float ux = dx / dist;
                float uy = dy / dist;

                enemies_[i].addOffset({-ux * overlap * pushStrength,
                                       -uy * overlap * pushStrength});

                enemies_[j].addOffset({ ux * overlap * pushStrength,
                                        uy * overlap * pushStrength});
            }
        }
    }
    // ============================================
    // 3. 敌人攻击玩家（贴脸距离 + 3 秒冷却）
    // ============================================
    for (auto& e : enemies_) {

        // 敌人与玩家的距离
        float dx = e.getPosition().x - player_->getPosition().x;
        float dy = e.getPosition().y - player_->getPosition().y;
        float dist = std::sqrt(dx*dx + dy*dy);

        // 贴脸攻击距离，可调整（15 像素）
        const float attackRange = 15.0f;

        // 判断贴脸
        if (dist < attackRange) {

            // 如果冷却结束，可以攻击
            if (e.attackCooldownFrames_ <= 0) {

                player_->takeDamagePlayer(5);   // 扣血 5 点

                std::cout << "[HIT] Player took 5 damage! HP = "
                          << player_->getHealth() << "\n";

                // 设置冷却：3 秒（FPS * 3）
                e.attackCooldownFrames_ = GameConfig::TARGET_FPS * 3;
            }
        }

        // 冷却计时递减
        if (e.attackCooldownFrames_ > 0) e.attackCooldownFrames_--;
    }
}

void Game::processWeaponInput() {
    // Handle weapon firing
    if (inputManager_->isFirePressed()) {
        handleWeaponFire();
    }
    
    // Handle reload (consume the reload key press)
    if (inputManager_->consumeReloadPress()) {
        player_->reloadWeapon();
    }
}

void Game::handleWeaponFire() {
    if (!player_->fireWeapon()) {
        return;  // Weapon couldn't fire (no ammo or cooldown)
    }
    
    // Get player info
    Vec2 playerPos = player_->getPosition();
    float playerAngle = player_->getAngle();
    
    // Get weapon stats
    const Weapon* weapon = player_->getWeapon();
    if (!weapon) return;
    
    float weaponRange = weapon->getRange();
    int weaponDamage = weapon->getDamage();
    
    // Cast a ray from the player's position in the direction they're facing
    // This uses the same raycasting logic as the renderer
    RayHit wallHit = raycaster_->castRay(playerPos, playerAngle);
    
    // Determine the maximum distance to check for enemies
    // Either the weapon range or the distance to the wall, whichever is closer
    float maxCheckDistance = weaponRange;
    if (wallHit.hit && wallHit.distance < weaponRange) {
        maxCheckDistance = wallHit.distance;
    }
    
    // Now check which enemy (if any) is hit by this ray
    Enemy* hitEnemy = nullptr;
    float closestEnemyDist = maxCheckDistance;
    
    // Check each enemy to see if the ray intersects with it
    for (auto& enemy : enemies_) {
        if (!enemy.isAlive()) continue;
        
        Vec2 enemyPos = enemy.getPosition();
        
        // Calculate the distance from player to enemy
        Vec2 toEnemy = enemyPos - playerPos;
        float distToEnemy = toEnemy.length();
        
        // Skip if enemy is farther than our max check distance
        if (distToEnemy > closestEnemyDist) continue;
        
        // Calculate the perpendicular distance from enemy to the ray
        // The ray goes in direction (cos(playerAngle), sin(playerAngle))
        Vec2 rayDir = Vec2{std::cos(playerAngle), std::sin(playerAngle)};
        
        // Project toEnemy onto the ray direction
        float projectionLength = toEnemy.x * rayDir.x + toEnemy.y * rayDir.y;
        
        // Skip if enemy is behind the player
        if (projectionLength < 0) continue;
        
        // Calculate the perpendicular distance
        Vec2 projectedPoint = rayDir * projectionLength;
        Vec2 perpVector = toEnemy - projectedPoint;
        float perpDistance = perpVector.length();
        
        // Define enemy hitbox radius (half of tile size is reasonable)
        float enemyRadius = map_->getTileSize() * 0.3f;
        
        // Check if the ray intersects the enemy's hitbox
        if (perpDistance < enemyRadius) {
            // Calculate actual hit distance along the ray
            float actualHitDistance = projectionLength - std::sqrt(enemyRadius * enemyRadius - perpDistance * perpDistance);
            if (actualHitDistance < 0) actualHitDistance = 0;
            
            // This enemy is hit! Check if it's the closest one
            if (actualHitDistance < closestEnemyDist) {
                hitEnemy = &enemy;
                closestEnemyDist = actualHitDistance;
            }
        }
    }
    
    // Apply damage to the hit enemy
    if (hitEnemy) {
        hitEnemy->takeDamageEnemy(weaponDamage);
    }
}


void Game::render() {
    renderer_->clear();
    
    // casting rays
    auto rayHits = raycaster_->castRays(
        player_->getPosition(),
        player_->getAngle(),
        GameConfig::FOV,
        //screenWidth_
        GameConfig::FOV * RenderConfig::RESOLUTION_RATIO
    );
    
    // render 3d View
    renderer_->draw3DView(rayHits, *player_, *map_);
    renderer_->drawEnemies3D(enemies_, *player_, *map_, rayHits);

    // render crosshair
    renderer_->drawCrosshair();
    
    // render weapon sprite
    renderer_->drawWeaponSprite(*player_);
    
    // render debug information
    renderer_->drawDebugInfo(*player_, inputManager_->shouldShowInfo());

    // render player HUD
    renderer_->drawHUD(*player_);
    
    renderer_->present();
}

bool Game::shouldExit() const {
    return inputManager_->shouldExit();
}

Vec2 Game::findFreeSpawnPoint() {
    int tileSize = map_->getTileSize();
    Vec2 p = map_->getInitPosition();

    int px = p.x / tileSize;
    int py = p.y / tileSize;

    while (true) {
        // 随机在玩家附近 ±10 tiles
        int x = px + (rand() % 21 - 10);
        int y = py + (rand() % 21 - 10);

        // 越界跳过
        if (x < 0 || x >= map_->getWidth()) continue;
        if (y < 0 || y >= map_->getHeight()) continue;

        // 当前 tile 必须是空地
        if (map_->isWall(x, y)) continue;

        // —— 安全半径检查（避免出生在靠墙位置）——
        // enemy 半径约 10px → tileSize 64px → 检查邻居 1 tile 就足够
        bool safe = true;
        int marginTiles = 1;

        for (int yy = y - marginTiles; yy <= y + marginTiles; yy++) {
            for (int xx = x - marginTiles; xx <= x + marginTiles; xx++) {
                if (xx < 0 || xx >= map_->getWidth()) continue;
                if (yy < 0 || yy >= map_->getHeight()) continue;

                if (map_->isWall(xx, yy)) {
                    safe = false;
                    break;
                }
            }
            if (!safe) break;
        }

        if (!safe) continue;

        // —— 找到安全出生点：返回 tile 中心 —
        return Vec2{
            x * tileSize + tileSize * 0.5f,
            y * tileSize + tileSize * 0.5f
        };
    }
}

std::vector<Vec2> Game::findDistributedSpawnPoints(unsigned int count)
{
    int ts = map_->getTileSize();
    int w = map_->getWidth();
    int h = map_->getHeight();

    std::vector<Vec2> candidates;

    // 1. 收集所有潜在合法 tile
    for (int y = 1; y < h - 1; y++) {
        for (int x = 1; x < w - 1; x++) {

            int tile = map_->getWallType(x, y);
            if (tile == 1) continue; // 墙不行

            // —— 周围必须安全（不靠墙）——
            bool safe = true;
            for (int yy = y - 1; yy <= y + 1; yy++) {
                for (int xx = x - 1; xx <= x + 1; xx++) {
                    if (map_->isWall(xx, yy)) {
                        safe = false;
                        break;
                    }
                }
                if (!safe) break;
            }
            if (!safe) continue;

            // 合格，加入候选位置（tile中心）
            candidates.push_back(Vec2{
                x * ts + ts * 0.5f,
                y * ts + ts * 0.5f
            });
        }
    }

    // 2. 均匀挑选点：简单方式 → 分段抽样
    std::vector<Vec2> result;
    int step = candidates.size() / count;

    for (unsigned int i = 0; i < candidates.size() && result.size() < count; i += step) {
        result.push_back(candidates[i]);
    }

    return result;
}
