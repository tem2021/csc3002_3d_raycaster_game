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
    
    bool isKeyPressed(unsigned char key) const;
    bool isSprintPressed() const { return sprintPressed_; }
    float consumeMouseDelta();
    bool shouldShowInfo() const { return showInfo_; }
    bool shouldExit() const { return exitRequested_; }
    
private:
    std::array<bool, 256> keyStates_;
    float mouseDeltaX_;
    int centerX_;
    int centerY_;
    bool showInfo_;
    bool exitRequested_;
    bool sprintPressed_;

    // use to normalize 'a' and 'A' to 'a'
    unsigned char normalizeKey(unsigned char key) const;   
};

#endif // INPUT_MANAGER_H
