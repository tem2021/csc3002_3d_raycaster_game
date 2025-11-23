#include "core/Game.h"
#include "core/Config.h"
#include "data/maps/level1.h"

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

Game::Game() : screenWidth_(0), screenHeight_(0) {}

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
    texManager.loadTexture(200, HIPPO_1_DATA);

    //Weapon Texture
    texManager.loadTexture(10, PISTOL_DATA);
    
    std::cout << "✓ All textures loaded successfully!" << std::endl;
}

void Game::handleInput() {
    processPlayerInput();
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
}

void Game::update() {
    Vec2 playerPos = player_->getPosition();

    for (auto& e : enemies_) {
        e.update(playerPos, *map_);
    }

    float minDist = 8.0f;
    float pushStrength = 0.2f;

    for (int i = 0; i < enemies_.size(); i++) {
        for (int j = i + 1; j < enemies_.size(); j++) {

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

std::vector<Vec2> Game::findDistributedSpawnPoints(int count)
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

    for (int i = 0; i < candidates.size() && result.size() < count; i += step) {
        result.push_back(candidates[i]);
    }

    return result;
}
