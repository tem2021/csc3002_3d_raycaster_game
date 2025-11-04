#include "input/InputManager.h"
#include <GL/freeglut.h>

InputManager::InputManager(int centerX, int centerY) 
    : mouseDeltaX_(0.0f), centerX_(centerX), centerY_(centerY), 
      showInfo_(false), exitRequested_(false) {
    keyStates_.fill(false);
}

void InputManager::handleKeyDown(unsigned char key) {
    keyStates_[key] = true;
    
    if (key == 27) {  // ESC
        exitRequested_ = true;
    }
}

void InputManager::handleKeyUp(unsigned char key) {
    keyStates_[key] = false;
    
    if (key == 'i' || key == 'I') {
        showInfo_ = !showInfo_;
    }
}

void InputManager::handleMouseMove(int x, int y) {
    if (x == centerX_ && y == centerY_) return;
    
    mouseDeltaX_ += (x - centerX_);
    glutWarpPointer(centerX_, centerY_);
}

bool InputManager::isKeyPressed(unsigned char key) const {
    return keyStates_[key];
}

float InputManager::consumeMouseDelta() {
    float delta = mouseDeltaX_;
    mouseDeltaX_ = 0.0f;
    return delta;
}
