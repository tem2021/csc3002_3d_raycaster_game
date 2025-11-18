#include "rendering/Renderer.h"
#include "core/Config.h"

#ifdef _WIN32
    #include <GL/freeglut.h>
    #include <GL/gl.h>
#elif defined(__APPLE__)
    #include <GLUT/glut.h>
    #include <OpenGL/gl.h>
#elif defined(__linux__)
    #include <GL/freeglut.h>
    #include <GL/gl.h>
#endif

#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>

Renderer::Renderer(int screenWidth, int screenHeight)
    : screenWidth_(screenWidth), screenHeight_(screenHeight) {
    centerX_ = screenWidth_ / 2;
    centerY_ = screenHeight_ / 2;
}

void Renderer::clear() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::draw3DView(const std::vector<RayHit>& rayHits, 
                          const Player& player, const Map& map) {
    int numRays = rayHits.size();
    
    for (int i = 0; i < numRays; ++i) {
        const RayHit& hit = rayHits[i];
        
        if (!hit.hit) continue;
        
        float rayAngle = player.getAngle() - (GameConfig::FOV * Math::DEG_TO_RAD / 2.0f) + 
                        (i * GameConfig::FOV * Math::DEG_TO_RAD / numRays);
        float ca = player.getAngle() - rayAngle;
        while (ca < 0.0f) ca += Math::TWO_PI;
        while (ca > Math::TWO_PI) ca -= Math::TWO_PI;
        
        float correctedDist = hit.distance * std::cos(ca);
        correctedDist = std::max(correctedDist, RenderConfig::MIN_WALL_DISTANCE);
        
        // draw wall with texture
        drawWall(i, correctedDist, hit, map);
        
        // calculate wall height
        float lineH = map.getTileSize() * screenHeight_ / correctedDist;
        lineH = std::min(lineH, static_cast<float>(screenHeight_));
        float lineO = (screenHeight_ / 2.0f) - lineH / 2.0f;
        
        drawFloor(i, lineO + lineH, correctedDist, map);
        drawCeiling(i, lineO, correctedDist);
    }
}

void Renderer::drawWall(int screenX, float distance, const RayHit& hit, const Map& map) {
    distance = std::max(distance, RenderConfig::MIN_WALL_DISTANCE);
    float lineH = map.getTileSize() * screenHeight_ / distance;
    
    // Calculate texture coordinates before clamping wall height
    float texStartY = 0.0f;
    float texEndY = 1.0f;
    
    // If wall is taller than screen, we need to clip texture coordinates
    if (lineH > screenHeight_) {
        float visibleRatio = screenHeight_ / lineH;
        float clipAmount = (1.0f - visibleRatio) / 2.0f;
        texStartY = clipAmount;
        texEndY = 1.0f - clipAmount;
        lineH = screenHeight_;
    }
    
    float lineO = (screenHeight_ / 2.0f) - lineH / 2.0f;
    
    // Get texture for this wall type
    GLuint texID = textureManager_.getTextureID(hit.wallType);
    
    if (texID > 0) {
        // Draw textured wall
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texID);
        
        // Apply brightness based on wall orientation
        float brightness = hit.isVertical ? 
            RenderConfig::WALL_BRIGHTNESS_V : 
            RenderConfig::WALL_BRIGHTNESS_H;
        glColor3f(brightness, brightness, brightness);
        
        // Draw quad with correct texture coordinates
        glBegin(GL_QUADS);
        glTexCoord2f(hit.wallHitX, texStartY); 
        glVertex2f(screenX, lineO);
        
        glTexCoord2f(hit.wallHitX, texEndY); 
        glVertex2f(screenX, lineO + lineH);
        
        glTexCoord2f(hit.wallHitX, texEndY); 
        glVertex2f(screenX + 1, lineO + lineH);
        
        glTexCoord2f(hit.wallHitX, texStartY); 
        glVertex2f(screenX + 1, lineO);
        glEnd();
        
        glDisable(GL_TEXTURE_2D);
    } else {
        // Fallback: draw colored wall if no texture
        float brightness = hit.isVertical ? 
            RenderConfig::WALL_BRIGHTNESS_V : 
            RenderConfig::WALL_BRIGHTNESS_H;
        
        glColor3f(brightness, brightness, brightness);
        glLineWidth(1.0f);
        glBegin(GL_LINES);
        glVertex2f(screenX, lineO);
        glVertex2f(screenX, lineO + lineH);
        glEnd();
    }
}

void Renderer::drawFloor(int screenX, float wallBottom, float distance, const Map& map) {
    float floorStart = std::max(wallBottom, static_cast<float>(screenHeight_) / 2.0f);
    
    float maxDist = map.getTileSize() * RenderConfig::MAX_FLOOR_DIST;
    float brightness = 1.0f - (distance / maxDist);
    brightness = std::clamp(brightness, RenderConfig::MIN_BRIGHTNESS, 1.0f);
    brightness *= RenderConfig::FLOOR_BRIGHTNESS;
    
    glBegin(GL_LINES);
    glColor3f(brightness, brightness, brightness);
    glVertex2f(screenX, floorStart);
    glColor3f(0.0f, 0.0f, 0.0f);
    glVertex2f(screenX, screenHeight_);
    glEnd();
}

void Renderer::drawCeiling(int screenX, float wallTop, float) {
    float ceilingEnd = std::min(wallTop, static_cast<float>(screenHeight_) / 2.0f);
    
    float brightness = RenderConfig::CEILING_BRIGHTNESS * RenderConfig::MIN_BRIGHTNESS;
    glColor3f(brightness, brightness, brightness);
    
    glBegin(GL_LINES);
    glVertex2f(screenX, 0);
    glVertex2f(screenX, ceilingEnd);
    glEnd();
}

void Renderer::drawCrosshair() {
    float crossLengthY = screenHeight_ * RenderConfig::CROSSHAIR_SIZE;
    float crossLengthX = crossLengthY;
    float crossWidth = crossLengthX * 0.2f;
    
    glColor3f(1.0f, 1.0f, 1.0f);
    glLineWidth(crossWidth);
    
    glBegin(GL_LINES);
    glVertex2f(centerX_ - crossLengthX / 2, centerY_);
    glVertex2f(centerX_ + crossLengthX / 2, centerY_);
    glVertex2f(centerX_, centerY_ - crossLengthY / 2);
    glVertex2f(centerX_, centerY_ + crossLengthY / 2);
    glEnd();
    
    glLineWidth(1.0f);
}

void Renderer::drawDebugInfo(const Player& player, bool show) {
    if (!show) return;
    
    std::ostringstream oss;
    
    oss << "Angle: " << std::fixed << std::setprecision(2) << player.getAngle();
    drawText(5, 15, oss.str());
    
    oss.str("");
    oss << "Pos X: " << std::fixed << std::setprecision(2) << player.getPosition().x;
    drawText(5, 25, oss.str());
    
    oss.str("");
    oss << "Pos Y: " << std::fixed << std::setprecision(2) << player.getPosition().y;
    drawText(5, 35, oss.str());
}

void Renderer::drawText(int x, int y, const std::string& text) {
    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2i(x, y);
    
    for (char c : text) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, c);
    }
}

void Renderer::present() {
    glutSwapBuffers();
}
