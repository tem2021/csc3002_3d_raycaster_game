#ifndef RENDERER_H
#define RENDERER_H

#include "core/Types.h"
#include "entities/Player.h"
#include "entities/Enemy.h"
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
    void drawEnemies3D(const std::vector<Enemy>& enemies,
                   const Player& player,
                   const Map& map,
                   const std::vector<RayHit>& rayHits);
    void drawCrosshair();
    void drawDebugInfo(const Player& player, bool show);
    void drawHealthValue(const Player& player);
    void drawHealthBar(const Player& player);
    void present();
    
    TextureManager& getTextureManager() { return textureManager_; }
    
    int getScreenWidth() const { return screenWidth_; }
    int getScreenHeight() const { return screenHeight_; }
    
private:
    int screenWidth_;
    int screenHeight_;
    int centerX_;
    int centerY_;
    
    // initialize the TextureManager in Renderer
    TextureManager textureManager_;
    
    Vec2 realPos(float distanceToProjectedPlane, float tileSize, float projectedH, float cosCa, float cosRayAngle, float sinRayAngle, Vec2& playerPos);
    void drawText(int x, int y, const std::string& text);
    void drawHorizontalPlane(float screenX, float deltaX, float projectedH, float tileSize, float ca, float rayAngle, Vec2& playerPos);
    void drawWall(float screenX, float deltaX, float projectedH, const RayHit& hit);
};

#endif // RENDERER_H
