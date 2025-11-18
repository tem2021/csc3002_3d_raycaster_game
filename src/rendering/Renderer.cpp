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
    // clear the depth and color buffer for each pixel
    // the clear method is defined by glClearColor & glClearDepth
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::draw3DView(const std::vector<RayHit>& rayHits, 
                          const Player& player, const Map& map) {
   
    // Draw floor and ceiling background first
    drawFloorTiled(player, map);
    drawCeilingTiled(player, map);
    
    int numRays = rayHits.size();
    
    for (int i = 0; i < numRays; ++i) {

        // alia the rayHits[i] as hit
        const RayHit& hit = rayHits[i];
        
        if (!hit.hit) continue;
        
        float rayAngle = 
        player.getAngle() - (GameConfig::FOV * Math::DEG_TO_RAD / 2.0f) + 
        (i * GameConfig::FOV * Math::DEG_TO_RAD / numRays);

        float ca = player.getAngle() - rayAngle;

        while (ca < 0.0f) ca += Math::TWO_PI;
        while (ca > Math::TWO_PI) ca -= Math::TWO_PI;
        
        float correctedDist = hit.distance * std::cos(ca);

        // ensure the correctness of rendering wall textures when player is very close to the wall
        correctedDist = std::max(correctedDist, RenderConfig::MIN_WALL_DISTANCE);

        // draw wall with texture
        drawWall(i, correctedDist, hit, map);
    }
}

void Renderer::drawWall(int screenX, float distance, const RayHit& hit, const Map& map) {

    // calculate wall height
    // this is the direct result of the Perspective Projection 
    // the increasing of the angle is suffcient small, hence the endpoints of the rays moves 
    // approximately the same distance each time 
    float lineH = map.getTileSize() * screenHeight_ / distance;
    
    // calculate texture coordinates for each hit
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
    
    // Starting point of the wall 
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
        
        // Draw quad with correct texture coordinates with counter-clockwise
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
    } 
    
    // usually impossible to have texID = 0; just write here in case  
    else {
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

void Renderer::drawHealthValue(const Player& player) {
    std::ostringstream oss;
    
    oss << "Health: " << player.getHealth();
    drawText(5, 15, oss.str());

}

void Renderer::drawHealthBar(const Player& player) {
	int fullWidth = static_cast<int>(screenWidth_ * PlayerConfig::HEALTH_BAR_WIDTH_PERCENT);
	int barHeight = static_cast<int>(screenWidth_ * PlayerConfig::HEALTH_BAR_HEIGHT_PERCENT);
	int x = PlayerConfig::HEALTH_BAR_MARGIN;
	int y = PlayerConfig::HEALTH_BAR_MARGIN;

	// Normalise health to be a float between 0.0 and 1.0
	float hp = static_cast<float>(player.getHealth()) / static_cast<float>(PlayerConfig::MAX_HEALTH);
	if (hp < 0.0f) hp = 0.0f;
	if (hp > 1.0f) hp = 1.0f;

	int filledWidth = static_cast<int>(fullWidth * hp);

	glDisable(GL_TEXTURE_2D);

	// Background bar (dark grey)
	glColor3f(0.12f, 0.12f, 0.12f);
	glBegin(GL_QUADS);
		glVertex2f(x, y);
		glVertex2f(x + fullWidth, y);
		glVertex2f(x + fullWidth, y + barHeight);
		glVertex2f(x, y + barHeight);
	glEnd();

	// Health color is more green at high health, turns redder at lower health
	float fillR = 1.0f - hp;
	float fillG = hp;
	glColor3f(fillR, fillG, 0.0f);

	// Only draw fill if width > 0
	if (filledWidth > 0) {
		glBegin(GL_QUADS);
			glVertex2f(x, y);
			glVertex2f(x + filledWidth, y);
			glVertex2f(x + filledWidth, y + barHeight);
			glVertex2f(x, y + barHeight);
		glEnd();
	}

	// Border outline
	glColor3f(0.0f, 0.0f, 0.0f);
	glLineWidth(1.5f);
	glBegin(GL_LINE_LOOP);
		glVertex2f(x, y);
		glVertex2f(x + fullWidth, y);
		glVertex2f(x + fullWidth, y + barHeight);
		glVertex2f(x, y + barHeight);
	glEnd();
	glLineWidth(1.0f);

	// Draw health value to the right of the health bar
	std::ostringstream oss;
	oss << player.getHealth() << " / " << PlayerConfig::MAX_HEALTH;
	
	// Vertically center the text
	int textX = x + fullWidth + PlayerConfig::HEALTH_BAR_MARGIN;
	int textY = y + (barHeight / 2) + 4;
	drawText(textX, textY, oss.str());

	// Restore color to white
	glColor3f(1.0f, 1.0f, 1.0f);
}

void Renderer::drawText(int x, int y, const std::string& text) {
    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2i(x, y);
    
    for (char c : text) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, c);
    }
}

void Renderer::drawFloorTiled(const Player& player, const Map& map) {
    // Get floor texture (using grass texture for floor)
    GLuint floorTexID = textureManager_.getTextureID(5); // grass 
    
    if (floorTexID == 0) {
        // Fallback: draw gray floor
        glColor3f(0.3f, 0.3f, 0.3f);
        glBegin(GL_QUADS);
        glVertex2f(0, screenHeight_ / 2);
        glVertex2f(screenWidth_, screenHeight_ / 2);
        glVertex2f(screenWidth_, screenHeight_);
        glVertex2f(0, screenHeight_);
        glEnd();
        return;
    }
    
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, floorTexID);
    
    // Enable perspective correction hint for better texture quality
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    
    Vec2 playerPos = player.getPosition();
    float playerAngle = player.getAngle();
    float tileSize = map.getTileSize();
    
    // Calculate view frustum
    float fovRad = GameConfig::FOV * Math::DEG_TO_RAD;
    float leftAngle = playerAngle - fovRad / 2.0f;
    float rightAngle = playerAngle + fovRad / 2.0f;
    
    // Apply brightness
    float brightness = RenderConfig::FLOOR_BRIGHTNESS;
    glColor3f(brightness, brightness, brightness);
    
    // Draw floor in multiple horizontal strips for better perspective
    int numStrips = RenderConfig::FLOOR_STRIPS;  // More strips = better perspective but slower
    float stripHeight = (screenHeight_ / 2.0f) / numStrips;
    
    for (int strip = 0; strip < numStrips; ++strip) {
        float nearY = screenHeight_ / 2.0f + strip * stripHeight;
        float farY = nearY + stripHeight;
        
        // Calculate distances for this strip
        float nearDist = tileSize * (0.5f + strip * 0.5f);
        float farDist = tileSize * (0.5f + (strip + 1) * 0.5f);
        
        // Calculate world coordinates for strip corners
        float nearLeftX = playerPos.x + std::cos(leftAngle) * nearDist;
        float nearLeftY = playerPos.y + std::sin(leftAngle) * nearDist;
        float nearRightX = playerPos.x + std::cos(rightAngle) * nearDist;
        float nearRightY = playerPos.y + std::sin(rightAngle) * nearDist;
        
        float farLeftX = playerPos.x + std::cos(leftAngle) * farDist;
        float farLeftY = playerPos.y + std::sin(leftAngle) * farDist;
        float farRightX = playerPos.x + std::cos(rightAngle) * farDist;
        float farRightY = playerPos.y + std::sin(rightAngle) * farDist;
        
        // Draw strip
        glBegin(GL_QUADS);
        glTexCoord2f(nearLeftX / tileSize, nearLeftY / tileSize);
        glVertex2f(0, nearY);
        
        glTexCoord2f(nearRightX / tileSize, nearRightY / tileSize);
        glVertex2f(screenWidth_, nearY);
        
        glTexCoord2f(farRightX / tileSize, farRightY / tileSize);
        glVertex2f(screenWidth_, farY);
        
        glTexCoord2f(farLeftX / tileSize, farLeftY / tileSize);
        glVertex2f(0, farY);
        glEnd();
    }
    
    glDisable(GL_TEXTURE_2D);
}

void Renderer::drawCeilingTiled(const Player& player, const Map& map) {
    // Get ceiling texture (using dark ceiling texture)
    GLuint ceilingTexID = textureManager_.getTextureID(7); // ceiling
    
    if (ceilingTexID == 0) {
        // Fallback: draw dark gray ceiling
        glColor3f(0.15f, 0.15f, 0.15f);
        glBegin(GL_QUADS);
        glVertex2f(0, 0);
        glVertex2f(screenWidth_, 0);
        glVertex2f(screenWidth_, screenHeight_ / 2);
        glVertex2f(0, screenHeight_ / 2);
        glEnd();
        return;
    }
    
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, ceilingTexID);
    
    // Enable perspective correction hint for better texture quality
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    
    Vec2 playerPos = player.getPosition();
    float playerAngle = player.getAngle();
    float tileSize = map.getTileSize();
    
    // Calculate view frustum
    float fovRad = GameConfig::FOV * Math::DEG_TO_RAD;
    float leftAngle = playerAngle - fovRad / 2.0f;
    float rightAngle = playerAngle + fovRad / 2.0f;
    
    // Apply brightness
    float brightness = RenderConfig::CEILING_BRIGHTNESS;
    glColor3f(brightness, brightness, brightness);
    
    // Draw ceiling in multiple horizontal strips for better perspective
    int numStrips = RenderConfig::FLOOR_STRIPS;
    float stripHeight = (screenHeight_ / 2.0f) / numStrips;
    
    for (int strip = 0; strip < numStrips; ++strip) {
        // Ceiling goes from top (0) to middle (screenHeight/2)
        // Reverse order: strip 0 is farthest, strip N is nearest
        float farY = strip * stripHeight;
        float nearY = farY + stripHeight;
        
        // Calculate distances (reversed for ceiling)
        float farDist = tileSize * (0.5f + (numStrips - strip - 1) * 0.5f);
        float nearDist = tileSize * (0.5f + (numStrips - strip) * 0.5f);
        
        // Calculate world coordinates for strip corners
        float farLeftX = playerPos.x + std::cos(leftAngle) * farDist;
        float farLeftY = playerPos.y + std::sin(leftAngle) * farDist;
        float farRightX = playerPos.x + std::cos(rightAngle) * farDist;
        float farRightY = playerPos.y + std::sin(rightAngle) * farDist;
        
        float nearLeftX = playerPos.x + std::cos(leftAngle) * nearDist;
        float nearLeftY = playerPos.y + std::sin(leftAngle) * nearDist;
        float nearRightX = playerPos.x + std::cos(rightAngle) * nearDist;
        float nearRightY = playerPos.y + std::sin(rightAngle) * nearDist;
        
        // Draw strip
        glBegin(GL_QUADS);
        glTexCoord2f(farLeftX / tileSize, farLeftY / tileSize);
        glVertex2f(0, farY);
        
        glTexCoord2f(farRightX / tileSize, farRightY / tileSize);
        glVertex2f(screenWidth_, farY);
        
        glTexCoord2f(nearRightX / tileSize, nearRightY / tileSize);
        glVertex2f(screenWidth_, nearY);
        
        glTexCoord2f(nearLeftX / tileSize, nearLeftY / tileSize);
        glVertex2f(0, nearY);
        glEnd();
    }
    
    glDisable(GL_TEXTURE_2D);
}

void Renderer::present() {
    glutSwapBuffers();
}
