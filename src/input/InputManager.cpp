#include "input/InputManager.h"
#include <GL/freeglut.h>
#include <GL/freeglut_ext.h>

InputManager::InputManager(int centerX, int centerY)
    : mouseDeltaX_(0.0f), centerX_(centerX), centerY_(centerY),
    showInfo_(false), exitRequested_(false), sprintPressed_(false),
    firePressed_(false), reloadPressed_(false),
    fireClicked_(false)        
{
    keyStates_.fill(false);
}


unsigned char InputManager::normalizeKey(unsigned char key) const {
    if (key >= 'A' && key <= 'Z') return key + ('a' - 'A'); 
    return key; 
}

void InputManager::handleKeyDown(unsigned char key) {
    key = normalizeKey(key);
    keyStates_[key] = true;
    
    if (key == 27) {  // ESC
        exitRequested_ = true;
    }
}

void InputManager::handleKeyUp(unsigned char key) {
    key = normalizeKey(key);
    keyStates_[key] = false;
    
    if (key == 'i') {
        showInfo_ = !showInfo_;
    }
    if (key == 'r') {
        reloadPressed_ = true;
    }
}

void InputManager::handleSpecialKeyDown(int key) {
    if (key == GLUT_KEY_SHIFT_L || key == GLUT_KEY_SHIFT_R) {
        sprintPressed_ = true;
    }
}

void InputManager::handleSpecialKeyUp(int key) {
    if (key == GLUT_KEY_SHIFT_L || key == GLUT_KEY_SHIFT_R) {
        sprintPressed_ = false;
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

void InputManager::handleMouseButton(int button, int state, int, int) {
    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) {
            firePressed_ = true;
            fireClicked_ = true;    
        }
        else {
            firePressed_ = false;
        }
    }
}


bool InputManager::consumeReloadPress() {
    bool pressed = reloadPressed_;
    reloadPressed_ = false;
    return pressed;
}

bool InputManager::consumeFireClick() {
    bool clicked = fireClicked_;
    fireClicked_ = false;    
    return clicked;
}
