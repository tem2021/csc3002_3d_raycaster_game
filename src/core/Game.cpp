#include "core/Game.h"
#include "core/Config.h"
#include "data/maps/level1.h"

#ifdef _WIN32
    #include <GL/freeglut.h>
#elif defined(__APPLE__)
    #include <GLUT/glut.h>
#elif defined(__linux__)
    #include <GL/freeglut.h>
#endif

Game::Game() : screenWidth_(0), screenHeight_(0) {}

void Game::init() {
    // 获取屏幕尺寸
    screenWidth_ = glutGet(GLUT_SCREEN_WIDTH);
    screenHeight_ = glutGet(GLUT_SCREEN_HEIGHT);
    
    // 创建地图
    int tileSize = screenHeight_ / MapData::LEVEL1_HEIGHT;
    map_ = std::make_unique<Map>(
        MapData::LEVEL1,
        MapData::LEVEL1_WIDTH,
        MapData::LEVEL1_HEIGHT,
        MapData::LEVEL1_INIT_X,
        MapData::LEVEL1_INIT_Y,
        tileSize
    );
    
    // 创建玩家
    float moveSpeed = GameConfig::MOVE_SPEED_FACTOR * tileSize;
    player_ = std::make_unique<Player>(
        map_->getInitPosition(),
        0.0f,  // 初始角度
        moveSpeed
    );
    
    // 创建光线投射器
    raycaster_ = std::make_unique<Raycaster>(*map_);
    
    // 创建渲染器
    renderer_ = std::make_unique<Renderer>(screenWidth_, screenHeight_);
    
    // 创建输入管理器
    inputManager_ = std::make_unique<InputManager>(
        screenWidth_ / 2,
        screenHeight_ / 2
    );
}

void Game::handleInput() {
    processPlayerInput();
}

void Game::processPlayerInput() {
    // 处理键盘移动
    if (inputManager_->isKeyPressed('w') || inputManager_->isKeyPressed('W')) {
        player_->moveForward(*map_);
    }
    if (inputManager_->isKeyPressed('s') || inputManager_->isKeyPressed('S')) {
        player_->moveBackward(*map_);
    }
    if (inputManager_->isKeyPressed('a') || inputManager_->isKeyPressed('A')) {
        player_->strafeLeft(*map_);
    }
    if (inputManager_->isKeyPressed('d') || inputManager_->isKeyPressed('D')) {
        player_->strafeRight(*map_);
    }
    
    // 处理鼠标旋转
    float mouseDelta = inputManager_->consumeMouseDelta();
    if (mouseDelta != 0.0f) {
        player_->rotate(mouseDelta * GameConfig::ROTATION_SPEED);
    }
}

void Game::update() {
    // 未来可以在这里添加游戏逻辑更新
    // 例如：敌人AI、物理模拟等
}

void Game::render() {
    renderer_->clear();
    
    // 投射光线
    auto rayHits = raycaster_->castRays(
        player_->getPosition(),
        player_->getAngle(),
        GameConfig::FOV,
        screenWidth_
    );
    
    // 渲染 3D 视图
    renderer_->draw3DView(rayHits, *player_, *map_);
    
    // 渲染准星
    renderer_->drawCrosshair();
    
    // 渲染调试信息
    renderer_->drawDebugInfo(*player_, inputManager_->shouldShowInfo());
    
    renderer_->present();
}

bool Game::shouldExit() const {
    return inputManager_->shouldExit();
}
