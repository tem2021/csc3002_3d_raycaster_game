#include "core/Game.h"
#include "core/Config.h"
#include "data/maps/level1.h"
#include <GL/freeglut_std.h>

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
    player_ = std::make_unique<Player>(
        map_->getInitPosition(),
        0.0f,  // initial angle
        moveSpeed
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
    Vec2 p = map_->getInitPosition();

    // 放在玩家正前方 50px（非常近，一定能看到）
    enemies_.clear();
    enemies_.push_back(Enemy(Vec2{p.x + 50, p.y}, 0.0f));

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
}


void Game::render() {
    renderer_->clear();
    
    // casting rays
    auto rayHits = raycaster_->castRays(
        player_->getPosition(),
        player_->getAngle(),
        GameConfig::FOV,
        screenWidth_
    );
    
    // render 3d View
    renderer_->draw3DView(rayHits, *player_, *map_);
    renderer_->drawEnemies3D(enemies_, *player_, *map_, rayHits);

    // render crosshair
    renderer_->drawCrosshair();
    
    // render debug information
    renderer_->drawDebugInfo(*player_, inputManager_->shouldShowInfo());
    
    renderer_->present();
}

bool Game::shouldExit() const {
    return inputManager_->shouldExit();
}
