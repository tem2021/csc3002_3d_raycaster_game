#include "world/Map.h"

Map::Map(const int sourceMap[MAX_HEIGHT][MAX_WIDTH], 
         int width, int height,
         int initX, int initY, 
         int tileSize) 
    : width_(width), height_(height), tileSize_(tileSize) {
    
    // copy the data from the sourceMap
    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            tiles_[y][x] = sourceMap[y][x];
        }
    }
    
    // calculate the initial position (centered at the square)
    initPosition_.x = tileSize_ * (initX - 0.5f);
    initPosition_.y = tileSize_ * (initY - 0.5f);
}

bool Map::isWall(int x, int y) const {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) {
        return true;  
    }
    return tiles_[y][x] > 0;  // 任何非0值都是墙
}

int Map::getWallType(int x, int y) const {
    if (x < 0 || x >= width_ || y < 0 || y >= height_) {
        return 1;  // 边界默认为类型1
    }
    return tiles_[y][x];
}
