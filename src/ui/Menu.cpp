#include "Menu.h"

Menu::Menu(): selectedIndex_(0),
      		  pendingAction_(MenuAction::NONE) {
    items_.push_back("Start Game");
    items_.push_back("Exit");
}

void Menu::nextItem() {
    selectedIndex_ = (selectedIndex_ + 1) % items_.size();
}

void Menu::prevItem() {
    selectedIndex_ = (selectedIndex_ - 1 + items_.size()) % items_.size();
}

void Menu::selectItem() {
    if (selectedIndex_ == 0) {
        pendingAction_ = MenuAction::START_GAME;
    }
    else if (selectedIndex_ == 1) {
        pendingAction_ = MenuAction::EXIT_GAME;
    }
}

Menu::MenuAction Menu::consumeAction() {
    MenuAction a = pendingAction_;
    pendingAction_ = MenuAction::NONE;
    return a;
}
