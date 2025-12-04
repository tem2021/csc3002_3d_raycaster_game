#include "core/Game.h"
#include "core/Config.h"
#include "entities/Weapon.h"
#include "entities/Enemy.h"
#include "data/maps/level1.h"
#include <cmath>
#include <random>
#include <algorithm> 

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
#include "data/textures/Panda_1.h"
#include "data/textures/Panda_2.h"
#include "data/textures/Panda_3.h"
#include "data/textures/Monkey_1.h"
#include "data/textures/Monkey_2.h"
#include "data/textures/Monkey_3.h"
#include "data/textures/unfiredgun.h"
#include "data/textures/firedgun.h"
#include "data/textures/Watermelon_idle.h"
#include "data/textures/Watermelon_throw.h"
#include "data/textures/Bamboo_idle.h"
#include "data/textures/Bamboo_throw.h"
#include "data/textures/Banana_idle.h"
#include "data/textures/Banana_throw.h"


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
    screenWidth_ = GameConfig::WINDOW_WIDTH;
    screenHeight_ = GameConfig::WINDOW_HEIGHT;
    
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
        health,
        0 // initial amount of kills
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

    int enemyCount = 20;    // 新的全地图均匀生成40个
    auto spawnPoints = findDistributedSpawnPoints(enemyCount);

    for (auto& pos : spawnPoints) {

        int r = rand() % 3;
        Enemy::EnemyType type;
        float speed;

        if (r == 0) {
            type = Enemy::EnemyType::Hippo;
            speed = 0.32f;  // 河马
        }
        else if (r == 1) {
            type = Enemy::EnemyType::Panda;
            speed = 0.20f;  // 熊猫（最慢）
        }
        else {
            type = Enemy::EnemyType::Monkey;
            speed = 0.26f;  // 猴子（中速）
        }

        enemies_.push_back(Enemy(pos, speed, type));
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

    // Hippo
    texManager.loadTexture(200, HIPPO_1_DATA);  // 普通
    texManager.loadTexture(201, HIPPO_2_DATA);  // 愤怒
    texManager.loadTexture(202, HIPPO_3_DATA);  // 乖巧
    // Panda
    texManager.loadTexture(210, PANDA_1_DATA);
    texManager.loadTexture(211, PANDA_2_DATA);
    texManager.loadTexture(212, PANDA_3_DATA);

    // Monkey
    texManager.loadTexture(220, MONKEY_1_DATA);
    texManager.loadTexture(221, MONKEY_2_DATA);
    texManager.loadTexture(222, MONKEY_3_DATA);

    //Weapon Texture
    texManager.loadTexture(300, PISTOL_DATA);
    texManager.loadTexture(301, UNFIREDGUN_DATA);
    texManager.loadTexture(302, FIREDGUN_DATA);

    // Fruit Weapon Textures
    // 西瓜：310/311
    texManager.loadTexture(310, WATERMELON_IDLE_DATA);
    texManager.loadTexture(311, WATERMELON_THROW_DATA);

    // 竹子：312/313
    texManager.loadTexture(312, BAMBOO_IDLE_DATA);
    texManager.loadTexture(313, BAMBOO_THROW_DATA);

    // 香蕉：314/315
    texManager.loadTexture(314, BANANA_IDLE_DATA);
    texManager.loadTexture(315, BANANA_THROW_DATA);

    std::cout << "✓ All textures loaded successfully!" << std::endl;
}

void Game::handleInput() {
    processPlayerInput();
    processWeaponInput();
}

// 判断某种水果是不是喂对了这只动物
static bool isCorrectFruitForEnemy(FruitType fruit, Enemy::EnemyType type) {
    switch (type) {
    case Enemy::EnemyType::Hippo:  // 河马吃西瓜
        return fruit == FruitType::Watermelon;
    case Enemy::EnemyType::Panda:  // 熊猫吃竹子
        return fruit == FruitType::Bamboo;
    case Enemy::EnemyType::Monkey: // 猴子吃香蕉
        return fruit == FruitType::Banana;
    }
    return false;
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

    // --- 武器切换：1/2/3 在三种水果枪之间切换 ---
    Weapon* w = player_->getWeapon();
    if (w && player_->hasWeapon()) {
        if (inputManager_->isKeyPressed('1')) {
            w->setFruitType(FruitType::Watermelon); // 西瓜枪（喂河马）
        }
        if (inputManager_->isKeyPressed('2')) {
            w->setFruitType(FruitType::Bamboo);     // 竹子枪（喂熊猫）
        }
        if (inputManager_->isKeyPressed('3')) {
            w->setFruitType(FruitType::Banana);     // 香蕉枪（喂猴子）
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

    // 敌人攻击玩家（贴脸距离 + 3 秒冷却）
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

                player_->takeDamagePlayer(e.getAttackDamage());  // 扣血 5 点

                std::cout << "[HIT] Player took 5 damage! HP = "
                          << player_->getHealth() << "\n";

                // 设置冷却：3 秒（FPS * 3）
                e.attackCooldownFrames_ = GameConfig::TARGET_FPS * 3;
            }
        }

        // 冷却计时递减
        if (e.attackCooldownFrames_ > 0) e.attackCooldownFrames_--;

        if (!e.isAlive() && !e.wasCountedAsKill()) {
            player_->incrementKills();
            e.markCountedAsKill();
        }
    }
}

// Game.cpp

void Game::processWeaponInput() {
    // Handle weapon firing
    if (inputManager_->consumeFireClick()) {   
        handleWeaponFire();
    }

    // Handle reload (consume the reload key press)
    if (inputManager_->consumeReloadPress()) {
        player_->reloadWeapon();
    }
}


void Game::handleWeaponFire() {
    // 1. 触发武器的开火动画（如果 fire 返回 false，可用来做冷却/没子弹）
    if (!player_->fireWeapon()) {
        return;
    }

    Weapon* weapon = player_->getWeapon();
    if (!weapon) return;

    // 2. 用中心射线找出打中的那只敌人
    Enemy* e = detectEnemyHit();
    if (!e) {
        // 没打中任何动物
        return;
    }

    // 3. 判断当前水果是不是喂对了这只敌人
    bool correct = isCorrectFruitForEnemy(
        weapon->getFruitType(),
        e->getType()
    );

    if (correct) {
        // 喂对了：播放开心表情 & 连续计数 +1
        e->onFedCorrect();
        e->incrementCorrectFeedCount();

        // 同一只敌人连续喂对三次 → 成功喂服，敌人消失
        if (e->getCorrectFeedCount() >= 3) {
            e->onFedSuccess();
        }
    }
    else {
        // 喂错了：播放生气表情，并且把连续计数清零
        e->onFedWrong();
        e->resetCorrectFeedCount();
    }
}


Enemy* Game::detectEnemyHit() {
    // 玩家位置和朝向
    Vec2 playerPos = player_->getPosition();
    float playerAngle = player_->getAngle();

    // 先用地图射线算出前面最近的墙，射线不能穿墙
    RayHit wallHit = raycaster_->castRay(playerPos, playerAngle);

    // 最大检测距离 = 墙距离（没有墙就给一个很大的值）
    float maxCheckDistance = wallHit.hit ? wallHit.distance : 1e9f;

    Enemy* hitEnemy = nullptr;
    float closestEnemyDist = maxCheckDistance;

    // 一条正中心射线：找所有在这条射线上的敌人，选最近的一只
    for (auto& enemy : enemies_) {
        if (!enemy.isAlive()) continue;

        Vec2 enemyPos = enemy.getPosition();
        Vec2 toEnemy = enemyPos - playerPos;
        float distToEnemy = toEnemy.length();

        // 超过墙/武器最大距离就不用管
        if (distToEnemy > closestEnemyDist) continue;

        // 射线方向向量
        Vec2 rayDir{ std::cos(playerAngle), std::sin(playerAngle) };

        // 把敌人向量投影到射线方向上
        float projectionLength = toEnemy.x * rayDir.x + toEnemy.y * rayDir.y;

        // 在玩家后面就跳过
        if (projectionLength < 0) continue;

        // 垂直距离
        Vec2 projectedPoint = rayDir * projectionLength;
        Vec2 perpVector = toEnemy - projectedPoint;
        float perpDistance = perpVector.length();

        // 敌人圆形 hitbox 半径（大概是 tileSize 的 0.3）
        float enemyRadius = map_->getTileSize() * 0.3f;

        // 射线是否穿过圆
        if (perpDistance < enemyRadius) {
            float actualHitDistance = projectionLength
                - std::sqrt(enemyRadius * enemyRadius
                    - perpDistance * perpDistance);
            if (actualHitDistance < 0) actualHitDistance = 0;

            if (actualHitDistance < closestEnemyDist) {
                hitEnemy = &enemy;
                closestEnemyDist = actualHitDistance;
            }
        }
    }

    return hitEnemy; // 可能为 nullptr（没打中）
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
            if (tile != 0) continue;   // 只允许空地

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
    // 打乱候选点顺序确保分布均匀
    if (!candidates.empty()) {
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(candidates.begin(), candidates.end(), g);
    }
    // 均匀挑选点：简单方式 → 分段抽样
    std::vector<Vec2> result;
    int step = candidates.size() / count;

    for (unsigned int i = 0; i < candidates.size() && result.size() < count; i += step) {
        result.push_back(candidates[i]);
    }

    return result;
}
