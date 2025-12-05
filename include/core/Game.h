#ifndef GAME_H
#define GAME_H

#include "entities/Player.h"
#include "entities/Enemy.h"
#include <vector>
#include "world/Map.h"
#include "world/Raycaster.h"
#include "rendering/Renderer.h"
#include "input/InputManager.h"
#include <memory>

class Game {
public:
    Game();
    ~Game() = default;
    
    void init();
    void update();
    void render();
    void handleInput();
    void loadTextures();  // Load all textures
    
    // Used for GLUT Callback
    InputManager& getInputManager() { return *inputManager_; }
    bool shouldExit() const;
    
private:
    bool gameOver_ = false;
    void handleGameOverState();
    
    //Owning pointers to core components
    std::unique_ptr<Player> player_;
    std::unique_ptr<Map> map_;
    std::unique_ptr<Raycaster> raycaster_;
    std::unique_ptr<Renderer> renderer_;
    std::unique_ptr<InputManager> inputManager_;
    std::vector<Enemy> enemies_;
    std::vector<Vec2> findDistributedSpawnPoints(unsigned int count);
    int screenWidth_;
    int screenHeight_;
    float deltaTime_;  // Time since last frame
    
    void processPlayerInput();
    void processWeaponInput();
    void handleWeaponFire();
    Vec2 findFreeSpawnPoint();
    Enemy* detectEnemyHit();
};

#endif // GAME_H
