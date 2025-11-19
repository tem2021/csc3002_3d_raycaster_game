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



void Renderer::drawHorizontalPlane(const Player& player, const Map& map, 
                                   GLuint textureID, float brightness, bool isFloor) {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    
    Vec2 playerPos = player.getPosition();
    float playerAngle = player.getAngle();
    float tileSize = map.getTileSize();
    float eyeHeight = tileSize * RenderConfig::PLAYER_EYE_HEIGHT;
    float fovRad = GameConfig::FOV * Math::DEG_TO_RAD;
    
    int numStrips = RenderConfig::FLOOR_STRIPS;
    float stripHeight = (screenHeight_ / 2.0f) / numStrips;
    int numColumns = 32;
    float columnWidth = screenWidth_ / static_cast<float>(numColumns);
    
    for (int strip = 0; strip < numStrips; strip++) {
        float y1, y2;
        float screenRatio1, screenRatio2;
        
        if (isFloor) {
            // Floor: render from horizon downward
            y1 = screenHeight_ / 2.0f + strip * stripHeight;
            y2 = y1 + stripHeight;
            screenRatio1 = (y1 - screenHeight_ / 2.0f) / (screenHeight_ / 2.0f);
            screenRatio2 = (y2 - screenHeight_ / 2.0f) / (screenHeight_ / 2.0f);
        } else {
            // Ceiling: render from top to horizon
            y2 = strip * stripHeight;
            y1 = y2 + stripHeight;
            screenRatio2 = (screenHeight_ / 2.0f - y2) / (screenHeight_ / 2.0f);
            screenRatio1 = (screenHeight_ / 2.0f - y1) / (screenHeight_ / 2.0f);
        }
        
        screenRatio1 = std::max(screenRatio1, 0.001f);
        screenRatio2 = std::max(screenRatio2, 0.001f);
        
        float straightDist1 = eyeHeight / screenRatio1;
        float straightDist2 = eyeHeight / screenRatio2;
        
        glColor3f(brightness, brightness, brightness);
        
        for (int col = 0; col < numColumns; col++) {
            float leftX = col * columnWidth;
            float rightX = (col + 1) * columnWidth;
            
            // Calculate ray angles
            float leftRatio = leftX / screenWidth_;
            float rightRatio = rightX / screenWidth_;
            float leftAngle = playerAngle - fovRad / 2.0f + leftRatio * fovRad;
            float rightAngle = playerAngle - fovRad / 2.0f + rightRatio * fovRad;
            
            // Calculate horizontal angle offset from center (fisheye correction)
            float leftAngleOffset = leftAngle - playerAngle;
            float rightAngleOffset = rightAngle - playerAngle;
            
            float dist1Left = straightDist1 / std::cos(leftAngleOffset);
            float dist1Right = straightDist1 / std::cos(rightAngleOffset);
            float dist2Left = straightDist2 / std::cos(leftAngleOffset);
            float dist2Right = straightDist2 / std::cos(rightAngleOffset);
            
            // Calculate ray direction vectors
            float leftDirX = std::cos(leftAngle);
            float leftDirY = std::sin(leftAngle);
            float rightDirX = std::cos(rightAngle);
            float rightDirY = std::sin(rightAngle);
            
            // Calculate world positions for all 4 corners
            float x1Left = playerPos.x + leftDirX * dist1Left;
            float y1Left = playerPos.y + leftDirY * dist1Left;
            float x1Right = playerPos.x + rightDirX * dist1Right;
            float y1Right = playerPos.y + rightDirY * dist1Right;
            
            float x2Left = playerPos.x + leftDirX * dist2Left;
            float y2Left = playerPos.y + leftDirY * dist2Left;
            float x2Right = playerPos.x + rightDirX * dist2Right;
            float y2Right = playerPos.y + rightDirY * dist2Right;
            
            glBegin(GL_QUADS);
            glTexCoord2f(x1Left / tileSize, y1Left / tileSize);
            glVertex2f(leftX, y1);
            
            glTexCoord2f(x1Right / tileSize, y1Right / tileSize);
            glVertex2f(rightX, y1);
            
            glTexCoord2f(x2Right / tileSize, y2Right / tileSize);
            glVertex2f(rightX, y2);
            
            glTexCoord2f(x2Left / tileSize, y2Left / tileSize);
            glVertex2f(leftX, y2);
            glEnd();
        }
    }
    
    glDisable(GL_TEXTURE_2D);
}

void Renderer::drawFloorTiled(const Player& player, const Map& map) {
    GLuint floorTexID = textureManager_.getTextureID(4);
    drawHorizontalPlane(player, map, floorTexID, RenderConfig::FLOOR_BRIGHTNESS, true);
}

void Renderer::drawCeilingTiled(const Player& player, const Map& map) {
    GLuint ceilingTexID = textureManager_.getTextureID(50);
    drawHorizontalPlane(player, map, ceilingTexID, RenderConfig::CEILING_BRIGHTNESS, false);
}


void Renderer::present() {
    glutSwapBuffers();
}
