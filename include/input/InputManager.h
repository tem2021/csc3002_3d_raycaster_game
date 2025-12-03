#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <array>

class InputManager {
public:
    InputManager(int centerX, int centerY);
    
    void handleKeyDown(unsigned char key);
    void handleKeyUp(unsigned char key);
    void handleSpecialKeyDown(int key);  // shift for example
    void handleSpecialKeyUp(int key);
    void handleMouseMove(int x, int y);
    void handleMouseButton(int button, int state, int x, int y);
    
    bool isKeyPressed(unsigned char key) const;
    bool isSprintPressed() const { return sprintPressed_; }
    bool isFirePressed() const { return firePressed_; }
    bool consumeReloadPress();
    float consumeMouseDelta();
    bool shouldShowInfo() const { return showInfo_; }
    bool shouldExit() const { return exitRequested_; }
    bool consumeFireClick();

private:
    std::array<bool, 256> keyStates_;
    float mouseDeltaX_;
    int centerX_;
    int centerY_;
    bool showInfo_;
    bool exitRequested_;
    bool sprintPressed_;
    bool firePressed_;
    bool reloadPressed_;
    bool fireClicked_;

    // use to normalize 'a' and 'A' to 'a'
    unsigned char normalizeKey(unsigned char key) const;   
};

#endif // INPUT_MANAGER_H
