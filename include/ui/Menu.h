#ifndef MENU_H
#define MENU_H

#include <vector>
#include <string>

class Menu {
public:
    Menu();

    void nextItem();
    void prevItem();
    void selectItem();

    int getSelectedIndex() const { return selectedIndex_; }
    const std::vector<std::string>& getItems() const { return items_; }

    enum class MenuAction {
        NONE,
        START_GAME,
        EXIT_GAME
    };

    MenuAction consumeAction(); // returns action once, then resets

private:
    std::vector<std::string> items_;
    int selectedIndex_;

    MenuAction pendingAction_;
};

#endif
