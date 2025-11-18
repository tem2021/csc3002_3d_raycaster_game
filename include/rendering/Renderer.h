#ifndef RENDERER_H
#define RENDERER_H

#include "core/Types.h"
#include "entities/Player.h"
#include "world/Map.h"
#include "rendering/TextureManager.h"
#include <vector>
#include <string>

class Renderer {
public:
    Renderer(int screenWidth, int screenHeight);
    
    void clear();
    void draw3DView(const std::vector<RayHit>& rayHits, 
                    const Player& player, const Map& map);
    void drawCrosshair();
    void drawDebugInfo(const Player& player, bool show);
    void present();
    
    TextureManager& getTextureManager() { return textureManager_; }
    
    int getScreenWidth() const { return screenWidth_; }
    int getScreenHeight() const { return screenHeight_; }
    
private:
    int screenWidth_;
    int screenHeight_;
    int centerX_;
    int centerY_;
    
    TextureManager textureManager_;
    
    void drawText(int x, int y, const std::string& text);
    void drawWall(int screenX, float distance, const RayHit& hit, const Map& map);
    void drawFloor(int screenX, float wallBottom, float distance, const Map& map);
    void drawCeiling(int screenX, float wallTop, float distance);
    void drawFloorTiled(const Player& player, const Map& map);
    void drawCeilingTiled(const Player& player, const Map& map);
};

#endif // RENDERER_H
