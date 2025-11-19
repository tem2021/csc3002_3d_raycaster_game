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
#include "entities/Enemy.h"

// In this Renderer, I use the old styled fixed-function pipeline API (Legacy OpenGL)

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
        // use default texture unit 0; No shader
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texID);
            
        // Apply brightness based on wall orientation
        float brightness = hit.isVertical ? 
            RenderConfig::WALL_BRIGHTNESS_V : 
            RenderConfig::WALL_BRIGHTNESS_H;

        //textureColor Union brightness -> Actual color [Under GL_MODULATE]
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
    int startY = screenHeight_ - 75;
    
    oss << "Health: " << player.getHealth() << " / " << PlayerConfig::MAX_HEALTH;
    drawText(10, startY, oss.str());

    oss.str("");
    oss << "Angle: " << std::fixed << std::setprecision(2) << player.getAngle();
    drawText(10, startY + 20, oss.str());
    
    oss.str("");
    oss << "Pos X: " << std::fixed << std::setprecision(2) << player.getPosition().x;
    drawText(10, startY + 40, oss.str());
    
    oss.str("");
    oss << "Pos Y: " << std::fixed << std::setprecision(2) << player.getPosition().y;
    drawText(10, startY + 60, oss.str());
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
}

void Renderer::drawText(int x, int y, const std::string& text) {
    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2i(x, y);
    
    for (char c : text) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, c);
    }
}

void Renderer::drawEnemies3D(const std::vector<Enemy>& enemies,
                             const Player& player,
                             const Map& map,
                             const std::vector<RayHit>& rayHits)
{
    int numRays = rayHits.size();
    float fov = GameConfig::FOV * Math::DEG_TO_RAD;

    // ------ 1. 按距离排序：远的先画 ------
    struct EnemyInfo {
        const Enemy* e;
        float dist;
        float angle;
    };

    std::vector<EnemyInfo> sorted;
    sorted.reserve(enemies.size());

    Vec2 pp = player.getPosition();

    for (const auto& e : enemies) {
        Vec2 ep = e.getPosition();
        float dx = ep.x - pp.x;
        float dy = ep.y - pp.y;
        float dist = std::sqrt(dx*dx + dy*dy);

        float angle = std::atan2(dy, dx) - player.getAngle();
        while (angle < -Math::PI) angle += Math::TWO_PI;
        while (angle >  Math::PI) angle -= Math::TWO_PI;

        // 不在视野就跳过
        if (std::fabs(angle) > fov/2) continue;

        sorted.push_back({ &e, dist, angle });
    }

    // 按距离从远到近排序
    std::sort(sorted.begin(), sorted.end(),
              [](auto& a, auto& b){ return a.dist > b.dist; });


    // ------ 2. 对每个敌人逐列渲染 ------
    for (auto& info : sorted) {
    //  const Enemy* enemy = info.e;
        float dist = info.dist;
        float enemyAngle = info.angle;

        // 屏幕 X 中心位置（对应射线）
        float ratio = (enemyAngle + fov/2) / fov;
        int centerRay = ratio * numRays;

        if (centerRay < 0 || centerRay >= numRays)
            continue;

        // sprite 大小
        float height = map.getTileSize() * screenHeight_ / dist;
        float half = height * 0.5f;

        int spriteScreenWidth = (int)height; // 方块怪物：宽度=高度

        float screenCenterX = (float)centerRay / numRays * screenWidth_;
        float centerY = screenHeight_ / 2;

        // ------ 逐列渲染 ------ 
        for (int xOffset = -spriteScreenWidth/2; xOffset <= spriteScreenWidth/2; xOffset++) {
            int rayId = centerRay + (xOffset * numRays / screenWidth_);
            if (rayId < 0 || rayId >= numRays) continue;

            // 用射线判断墙是否挡住这“一列”
            if (rayHits[rayId].hit && rayHits[rayId].distance < dist)
                continue;   // 这一列被墙挡住 → 不画

            float screenX = screenCenterX + xOffset;

            // 画这一列
            glColor3f(1, 0, 0);
            glBegin(GL_LINES);
            glVertex2f(screenX, centerY - half);
            glVertex2f(screenX, centerY + half);
            glEnd();
        }
    }
}



void Renderer::drawFloorTiled(const Player& player, const Map& map) {
    GLuint floorTexID = textureManager_.getTextureID(4);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, floorTexID);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    
    Vec2 playerPos = player.getPosition();
    float playerAngle = player.getAngle();
    float tileSize = map.getTileSize();
    float eyeHeight = tileSize * RenderConfig::PLAYER_EYE_HEIGHT;
    float fovRad = GameConfig::FOV * Math::DEG_TO_RAD;
    float brightness = RenderConfig::FLOOR_BRIGHTNESS;
    
    int numStrips = RenderConfig::FLOOR_STRIPS;
    float stripHeight = (screenHeight_ / 2.0f) / numStrips;
    int numColumns = 32; // Subdivide horizontally for correct perspective
    float columnWidth = screenWidth_ / static_cast<float>(numColumns);
    
    for (int strip = 0; strip < numStrips; strip++) {
        float nearY = screenHeight_ / 2.0f + strip * stripHeight;
        float farY = nearY + stripHeight;
        
        float nearScreenRatio = (nearY - screenHeight_ / 2.0f) / (screenHeight_ / 2.0f);
        float farScreenRatio = (farY - screenHeight_ / 2.0f) / (screenHeight_ / 2.0f);
        nearScreenRatio = std::max(nearScreenRatio, 0.001f);
        farScreenRatio = std::max(farScreenRatio, 0.001f);
        
        float nearDist = eyeHeight / nearScreenRatio;
        float farDist = eyeHeight / farScreenRatio;
        
        glColor3f(brightness, brightness, brightness);
        
        // Subdivide each strip horizontally to maintain perspective correctness
        for (int col = 0; col < numColumns; col++) {
            float leftX = col * columnWidth;
            float rightX = (col + 1) * columnWidth;
            
            // Calculate ray angles for left and right edges of this column
            float leftRatio = leftX / screenWidth_;
            float rightRatio = rightX / screenWidth_;
            float leftAngle = playerAngle - fovRad / 2.0f + leftRatio * fovRad;
            float rightAngle = playerAngle - fovRad / 2.0f + rightRatio * fovRad;
            
            // Calculate world positions for all 4 corners
            float nearLeftX = playerPos.x + std::cos(leftAngle) * nearDist;
            float nearLeftY = playerPos.y + std::sin(leftAngle) * nearDist;
            float nearRightX = playerPos.x + std::cos(rightAngle) * nearDist;
            float nearRightY = playerPos.y + std::sin(rightAngle) * nearDist;
            
            float farLeftX = playerPos.x + std::cos(leftAngle) * farDist;
            float farLeftY = playerPos.y + std::sin(leftAngle) * farDist;
            float farRightX = playerPos.x + std::cos(rightAngle) * farDist;
            float farRightY = playerPos.y + std::sin(rightAngle) * farDist;
            
            glBegin(GL_QUADS);
            glTexCoord2f(nearLeftX / tileSize, nearLeftY / tileSize);
            glVertex2f(leftX, nearY);
            
            glTexCoord2f(nearRightX / tileSize, nearRightY / tileSize);
            glVertex2f(rightX, nearY);
            
            glTexCoord2f(farRightX / tileSize, farRightY / tileSize);
            glVertex2f(rightX, farY);
            
            glTexCoord2f(farLeftX / tileSize, farLeftY / tileSize);
            glVertex2f(leftX, farY);
            glEnd();
        }
    }
    
    glDisable(GL_TEXTURE_2D);
}

void Renderer::drawCeilingTiled(const Player& player, const Map& map) {
    GLuint ceilingTexID = textureManager_.getTextureID(50);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, ceilingTexID);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    
    Vec2 playerPos = player.getPosition();
    float playerAngle = player.getAngle();
    float tileSize = map.getTileSize();
    float eyeHeight = tileSize * RenderConfig::PLAYER_EYE_HEIGHT;
    float fovRad = GameConfig::FOV * Math::DEG_TO_RAD;
    float brightness = RenderConfig::CEILING_BRIGHTNESS;
    
    int numStrips = RenderConfig::FLOOR_STRIPS;
    float stripHeight = (screenHeight_ / 2.0f) / numStrips;
    int numColumns = 32; // Subdivide horizontally for correct perspective
    float columnWidth = screenWidth_ / static_cast<float>(numColumns);
    
    for (int strip = 0; strip < numStrips; ++strip) {
        float farY = strip * stripHeight;
        float nearY = farY + stripHeight;
        
        float farScreenRatio = (screenHeight_ / 2.0f - farY) / (screenHeight_ / 2.0f);
        float nearScreenRatio = (screenHeight_ / 2.0f - nearY) / (screenHeight_ / 2.0f);
        farScreenRatio = std::max(farScreenRatio, 0.001f);
        nearScreenRatio = std::max(nearScreenRatio, 0.001f);
        
        float farDist = eyeHeight / farScreenRatio;
        float nearDist = eyeHeight / nearScreenRatio;
        
        glColor3f(brightness, brightness, brightness);
        
        // Subdivide each strip horizontally to maintain perspective correctness
        for (int col = 0; col < numColumns; col++) {
            float leftX = col * columnWidth;
            float rightX = (col + 1) * columnWidth;
            
            // Calculate ray angles for left and right edges of this column
            float leftRatio = leftX / screenWidth_;
            float rightRatio = rightX / screenWidth_;
            float leftAngle = playerAngle - fovRad / 2.0f + leftRatio * fovRad;
            float rightAngle = playerAngle - fovRad / 2.0f + rightRatio * fovRad;
            
            // Calculate world positions for all 4 corners
            float farLeftX = playerPos.x + std::cos(leftAngle) * farDist;
            float farLeftY = playerPos.y + std::sin(leftAngle) * farDist;
            float farRightX = playerPos.x + std::cos(rightAngle) * farDist;
            float farRightY = playerPos.y + std::sin(rightAngle) * farDist;
            
            float nearLeftX = playerPos.x + std::cos(leftAngle) * nearDist;
            float nearLeftY = playerPos.y + std::sin(leftAngle) * nearDist;
            float nearRightX = playerPos.x + std::cos(rightAngle) * nearDist;
            float nearRightY = playerPos.y + std::sin(rightAngle) * nearDist;
            
            glBegin(GL_QUADS);
            glTexCoord2f(farLeftX / tileSize, farLeftY / tileSize);
            glVertex2f(leftX, farY);
            
            glTexCoord2f(farRightX / tileSize, farRightY / tileSize);
            glVertex2f(rightX, farY);
            
            glTexCoord2f(nearRightX / tileSize, nearRightY / tileSize);
            glVertex2f(rightX, nearY);
            
            glTexCoord2f(nearLeftX / tileSize, nearLeftY / tileSize);
            glVertex2f(leftX, nearY);
            glEnd();
        }
    }
    
    glDisable(GL_TEXTURE_2D);
}

void Renderer::present() {
    glutSwapBuffers();
}
