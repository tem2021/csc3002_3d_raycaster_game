#ifndef MAP_H
#define MAP_H

#include "core/Types.h"
#include <array>

class Map {
public:
    static constexpr int MAX_WIDTH = 64;
    static constexpr int MAX_HEIGHT = 64;
    
    Map(const int sourceMap[MAX_HEIGHT][MAX_WIDTH], 
        int width, int height,
        int initX, int initY, 
        int tileSize);
    
    bool isWall(int x, int y) const;
    int getTileSize() const { return tileSize_; }
    int getWidth() const { return width_; }
    int getHeight() const { return height_; }
    Vec2 getInitPosition() const { return initPosition_; }
    
private:
    std::array<std::array<int, MAX_WIDTH>, MAX_HEIGHT> tiles_;
    int width_;
    int height_;
    int tileSize_;
    Vec2 initPosition_;
};

#endif // MAP_H
