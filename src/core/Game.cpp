#include "core/Game.h"
#include "core/Config.h"
#include "data/maps/level1.h"

// Include texture data (only those with 3D array format)
#include "data/textures/wood.h"
#include "data/textures/brick.h"
#include "data/textures/metal.h"
#include "data/textures/stone.h"
#include "data/textures/marble.h"
#include "data/textures/grass.h"
#include "data/textures/ceiling.h"
#include "data/textures/CUHK_SZ.h"
#include "data/textures/Hajimi.h"

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
    int health = PlayerConfig::HEALTH;
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
    
    // Load all textures
    loadTextures();
}

void Game::loadTextures() {
    std::cout << "Loading textures..." << std::endl;
    
    auto& texManager = renderer_->getTextureManager();
    
    // Load textures with their IDs (matching map data)
    texManager.loadTexture(1, BRICK_DATA);
    texManager.loadTexture(2, WOOD_DATA);
    texManager.loadTexture(3, METAL_DATA);
    texManager.loadTexture(4, STONE_DATA);
    texManager.loadTexture(5, GRASS_DATA);    // Floor texture
    texManager.loadTexture(7, CEILING_DATA);  // Ceiling texture
    texManager.loadTexture(8, MARBLE_DATA);
    texManager.loadTexture(100, CUHK_SZ_DATA);
    texManager.loadTexture(101, HAJIMI_DATA);
    
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
    // update the game logic in the future 
    // e.g. enemy ai / weapons 
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
    
    // render crosshair
    renderer_->drawCrosshair();
    
    // render debug information
    renderer_->drawDebugInfo(*player_, inputManager_->shouldShowInfo());
    
    renderer_->drawHealthBar(*player_);
    
    renderer_->present();
}

bool Game::shouldExit() const {
    return inputManager_->shouldExit();
}
