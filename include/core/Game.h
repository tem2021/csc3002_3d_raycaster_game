#ifndef GAME_H
#define GAME_H

#include "entities/Player.h"
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
    
    // 用于 GLUT 回调
    InputManager& getInputManager() { return *inputManager_; }
    bool shouldExit() const;
    
private:
    std::unique_ptr<Player> player_;
    std::unique_ptr<Map> map_;
    std::unique_ptr<Raycaster> raycaster_;
    std::unique_ptr<Renderer> renderer_;
    std::unique_ptr<InputManager> inputManager_;
    
    int screenWidth_;
    int screenHeight_;
    
    void processPlayerInput();
};

#endif // GAME_H
