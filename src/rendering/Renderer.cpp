#include "rendering/Renderer.h"
#include "core/Config.h"
#include "core/Types.h"
#include "entities/Weapon.h"
#include "entities/Player.h"
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
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

}

void Renderer::clear() {
    // clear the depth and color buffer for each pixel
    // the clear method is defined by glClearColor & glClearDepth
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::draw3DView(const std::vector<RayHit>& rayHits, 
                          const Player& player, const Map& map) {
   
    int numRays = rayHits.size();
    float HalfFOV = GameConfig::FOV * Math::DEG_TO_RAD / 2.0f;
    float angleStep = GameConfig::FOV * Math::DEG_TO_RAD / numRays;
    Vec2 playerPos = player.getPosition();

    // the distance between camera to screen is constructed using screenHeight
    float distanceToProjectedPlane = screenHeight_; 
    // float projectedPlaneHeight = distanceToProjectedPlane;
    float projectedPlaneWidth = distanceToProjectedPlane * std::tan(HalfFOV) * 2.0f;

    float previousScreenX = 0;
    float screenX;
    
    for (int i = 0; i < numRays; ++i) {

        // alia the rayHits[i] as hit
        const RayHit& hit = rayHits[i];
        
        if (!hit.hit) continue;

        float rayAngle = player.getAngle() - HalfFOV + (i * angleStep);
        float ca = player.getAngle() - rayAngle;
        float deltaAngle = HalfFOV - i * angleStep;

        // Here I modify the original Raycaster to ensure the linear performance
        // get X intersection of casting ray on Projected Plane
        screenX = projectedPlaneWidth / 2.0f - distanceToProjectedPlane * std::tan(deltaAngle);     
        // transfer to corresponding screen pixel: this will linearly stretch or compress the projection on X axis
        float stretchRatio = screenWidth_ /  projectedPlaneWidth;
        screenX = screenX * stretchRatio;

        float correctedDist = hit.distance * std::cos(ca);

        // ensure the correctness of rendering wall textures when player is very close to the wall
        correctedDist = std::max(correctedDist, RenderConfig::MIN_WALL_DISTANCE);

        // calculate wall height and draw wall with texture
        // stretch the projectedH too to ensure shapes of object stay the same
        float projectedH = map.getTileSize() * screenHeight_ / correctedDist;
        projectedH = projectedH * stretchRatio;

        drawHorizontalPlane(true, screenX, screenX - previousScreenX, projectedH, map.getTileSize(), ca, rayAngle, playerPos);
        drawHorizontalPlane(false, screenX, screenX - previousScreenX, projectedH, map.getTileSize(), ca, rayAngle, playerPos);
        drawWall(screenX, screenX - previousScreenX, projectedH, hit);

        previousScreenX = screenX;
    }
}

void Renderer::drawHUD(const Player& player) {
    drawHealthBar(player);
    drawCurrentWeapon();
}

Vec2 Renderer::realPos(float distanceToProjectedPlane, 
        float tileSize,
        float projectedH, 
        float cosCa,  
        float cosRayAngle, 
        float sinRayAngle,
        Vec2& playerPos) 
{
    // assume the cameraHeight is tileSize/2
    // tana = (h / 2 )/ distanceToProjectedPlane;
    // depth = (tileSize / 2) / tana
    float depth = tileSize * distanceToProjectedPlane / projectedH;
    float dist = depth / cosCa;

    float x = playerPos.x + dist * cosRayAngle; 
    float y = playerPos.y + dist * sinRayAngle;

    return Vec2(x, y);
}

void Renderer::drawHorizontalPlane(bool isFloor, float screenX, float deltaX, float projectedH, float tileSize, float ca, float rayAngle, Vec2& playerPos) {
    if (projectedH > screenHeight_ ) return;

    // ensure the rendering width is no smaller than 1.0f
    deltaX = std::max(1.0f, deltaX);
    float startScreenX = screenX - deltaX; 
    startScreenX = std::max(0.0f, startScreenX);

    // useful const
    const float cosRayAngle = std::cos(rayAngle);
    const float sinRayAngle = std::sin(rayAngle);
    const float cosCa = std::cos(ca);
    const float quadHeight = screenHeight_ / RenderConfig::FOV_HEIGHT / RenderConfig::RESOLUTION_RATIO;

    // rendering pixel position and corresponding tex position
    Vec2 texPos(0.0f, 0.0f);
    float pixelH = screenHeight_;
    float pixelY = isFloor ? screenHeight_ : 0;

    // Draw textured wall
    // use default texture unit 0; No shader
    GLuint floorID = isFloor ? 
        textureManager_.getTextureID(RenderConfig::FLOOR_TEXTURE_ID) 
        : textureManager_.getTextureID(RenderConfig::CEILING_TEXTURE_ID);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, floorID);
    glBegin(GL_QUADS);

    if (isFloor) 
        glColor3f(RenderConfig::FLOOR_BRIGHTNESS, RenderConfig::FLOOR_BRIGHTNESS, RenderConfig::FLOOR_BRIGHTNESS);
    else 
        glColor3f(RenderConfig::CEILING_BRIGHTNESS, RenderConfig::CEILING_BRIGHTNESS,RenderConfig::CEILING_BRIGHTNESS);

    while (pixelH > projectedH){
        Vec2 rp = realPos(screenHeight_, tileSize, pixelH, cosCa, cosRayAngle, sinRayAngle,playerPos);
        texPos.x = std::fmod(std::fabs(rp.x), tileSize) / tileSize;
        texPos.y = std::fmod(std::fabs(rp.y), tileSize) / tileSize;

        float endY = isFloor ? pixelY - quadHeight : pixelY + quadHeight;
        // Draw quad with correct texture coordinates 
        // Floor Texture
        glTexCoord2f(texPos.x, texPos.y ); 
        glVertex2f(startScreenX, endY);
        
        glTexCoord2f(texPos.x, texPos.y ); 
        glVertex2f(startScreenX, pixelY);
        
        glTexCoord2f(texPos.x, texPos.y ); 
        glVertex2f(screenX, pixelY);
        
        glTexCoord2f(texPos.x, texPos.y ); 
        glVertex2f(screenX, endY);

        pixelY =  isFloor ? pixelY - quadHeight : pixelY + quadHeight;
        pixelH -= 2 * quadHeight;
    }

    glEnd();
    glDisable(GL_TEXTURE_2D);
    glColor3f(1.0f, 1.0f, 1.0f);
}

void Renderer::drawWall(float screenX, float deltaX, float projectedH, const RayHit& hit) {

    // ensure the rendering width is no smaller than 1.0f
    deltaX = std::max(1.0f, deltaX);
    float startScreenX = screenX - deltaX; 
    startScreenX = std::max(0.0f, startScreenX);
    
    // calculate texture coordinates for each hit
    float texStartY = 0.0f;
    float texEndY = 1.0f;
    
    // If wall is taller than screen, we need to clip texture coordinates
    if (projectedH > screenHeight_) {
        float visibleRatio = screenHeight_ / projectedH;
        float clipAmount = (1.0f - visibleRatio) / 2.0f;
        texStartY = clipAmount;
        texEndY = 1.0f - clipAmount;
        projectedH = screenHeight_;
    }
    
    // Starting point of the wall 
    float startH = (screenHeight_ / 2.0f) - projectedH / 2.0f;
    
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
        glVertex2f(startScreenX, startH);
        
        glTexCoord2f(hit.wallHitX, texEndY); 
        glVertex2f(startScreenX, startH + projectedH);
        
        glTexCoord2f(hit.wallHitX, texEndY); 
        glVertex2f(screenX, startH + projectedH);
        
        glTexCoord2f(hit.wallHitX, texStartY); 
        glVertex2f(screenX, startH);
        glEnd();

        glDisable(GL_TEXTURE_2D);
    } 

    glColor3f(1.0f, 1.0f, 1.0f);
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

    Vec2 playerPos = player.getPosition();

    // --- 1. 生成深度缓冲（墙的距离）---
    std::vector<float> depthBuffer(numRays);
    for (int i = 0; i < numRays; i++) {
        depthBuffer[i] = rayHits[i].distance;
    }

    // --- 2. 敌人按距离排序（远的先画）---
    struct EnemyInfo {
        const Enemy* e;
        float dist;
        float angle;
    };

    std::vector<EnemyInfo> sorted;
    sorted.reserve(enemies.size());

    for (const auto& e : enemies) {
        Vec2 ep = e.getPosition();

        float dx = ep.x - playerPos.x;
        float dy = ep.y - playerPos.y;
        float dist = std::sqrt(dx * dx + dy * dy);

        float angle = std::atan2(dy, dx) - player.getAngle();

        // 角度归一化到 [-π, π]
        while (angle < -Math::PI) angle += Math::TWO_PI;
        while (angle >  Math::PI) angle -= Math::TWO_PI;

        // 不在视野范围 → 跳过
        if (std::fabs(angle) > fov / 2) continue;

        sorted.push_back({ &e, dist, angle });
    }

    std::sort(sorted.begin(), sorted.end(),
              [](auto& a, auto& b){ return a.dist > b.dist; });

    // --- 3. 逐个绘制敌人（从远到近）---
    for (const auto& info : sorted) {
        const Enemy* enemy = info.e;
        float dist  = info.dist;
        float angle = info.angle;

        // 将角度映射到射线索引
        float ratio = (angle + fov / 2) / fov;
        int centerRay = ratio * numRays;

        if (centerRay < 0 || centerRay >= numRays)
            continue;

        // 敌人的 sprite 高度（线性投影）
        float spriteHeight = map.getTileSize() * screenHeight_ / dist;
        float halfH = spriteHeight * 0.5f;

        // 宽度 = 高度
        float spriteWidth = spriteHeight;

        // 敌人中心投影到屏幕 X 坐标
        float screenCenterX = (float)centerRay / (float)numRays * screenWidth_;
        float screenCenterY = screenHeight_ / 2.0f;

        // ====== 1. 加载敌人贴图 ======
        GLuint tex = textureManager_.getTextureID(enemy->getTextureId());
        if (tex == 0) continue;

        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, tex);
        glColor3f(1, 1, 1);

        // --- 4. 逐列绘制 sprite ---
        for (int px = -spriteWidth / 2; px <= spriteWidth / 2; px++) {

            // 像素对应的射线索引
            int rayId = centerRay + (px * numRays / screenWidth_);

            if (rayId < 0 || rayId >= numRays)
                continue;

            // --- 深度遮挡 ---
            if (depthBuffer[rayId] < dist)
                continue;

            float screenX = screenCenterX + px;

            // ====== 2. 计算贴图 U 坐标 ======
            float u = (float)(px + spriteWidth / 2) / spriteWidth;
            float uNext = (float)(px + spriteWidth / 2 + 1) / spriteWidth;

            float x1 = screenX;
            float x2 = screenX + 1;

            // ====== 3. 用贴图替代红线 ======
            glBegin(GL_QUADS);
                glTexCoord2f(u,     0.0f); glVertex2f(x1, screenCenterY - halfH);
                glTexCoord2f(u,     1.0f); glVertex2f(x1, screenCenterY + halfH);
                glTexCoord2f(uNext, 1.0f); glVertex2f(x2, screenCenterY + halfH);
                glTexCoord2f(uNext, 0.0f); glVertex2f(x2, screenCenterY - halfH);
            glEnd();
        }

        glDisable(GL_TEXTURE_2D);
    }
}

void Renderer::drawWeaponSprite(const Player& player) {
    // Only draw when weapon is equipped
    if (!player.hasWeapon()) {
        return;
    }
    
    const Weapon* weapon = player.getWeapon();
    if (!weapon) {
        return;
    }
    
    // Constants for weapon sprite rendering
    constexpr int TEXTURE_ID_UNFIRED_GUN = 200;
    constexpr int TEXTURE_ID_FIRED_GUN = 201;
    constexpr float WEAPON_SPRITE_HEIGHT_RATIO = 0.25f;  // 25% of screen height
    constexpr float WEAPON_SPRITE_BOTTOM_MARGIN = 20.0f;  // Distance from bottom in pixels
    
    // Select texture based on weapon state
    bool isFiring = weapon->isFiring();
    int textureId = isFiring ? TEXTURE_ID_FIRED_GUN : TEXTURE_ID_UNFIRED_GUN;
    
    // Get OpenGL texture ID
    GLuint texID = textureManager_.getTextureID(textureId);
    if (texID == 0) {
        return;  // Texture not loaded
    }
    
    // Weapon sprite dimensions (keep square to match 64x64 texture)
    float spriteHeight = screenHeight_ * WEAPON_SPRITE_HEIGHT_RATIO;
    float spriteWidth = spriteHeight;
    
    // Position: bottom center of screen
    float spriteX = centerX_ - spriteWidth / 2.0f;
    float spriteY = screenHeight_ - spriteHeight - WEAPON_SPRITE_BOTTOM_MARGIN;
    
    // Enable 2D texture and blending for transparency
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Bind texture
    glBindTexture(GL_TEXTURE_2D, texID);
    
    // Draw textured quad
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);  // White color (no tint)
    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(spriteX, spriteY);
        glTexCoord2f(1.0f, 0.0f); glVertex2f(spriteX + spriteWidth, spriteY);
        glTexCoord2f(1.0f, 1.0f); glVertex2f(spriteX + spriteWidth, spriteY + spriteHeight);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(spriteX, spriteY + spriteHeight);
    glEnd();
    
    // Disable texture and blending
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);

// Example to help you load texture
void Renderer::drawCurrentWeapon() {
    GLuint pistolID = textureManager_.getTextureID(10);  //ID define on Game.cpp
    float pistol_width = RenderConfig::PISTOL_TEXTURE_SIZE;
    float pistol_height = pistol_width;

    float draw_x = screenWidth_ - pistol_width - 10.0f;
    float draw_y = screenHeight_ - pistol_height - 10.0f;

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, pistolID);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f); 


    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(draw_x, draw_y);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(draw_x, draw_y + pistol_height);
        glTexCoord2f(1.0f, 1.0f); glVertex2f(draw_x + pistol_width, draw_y + pistol_height);
        glTexCoord2f(1.0f, 0.0f); glVertex2f(draw_x + pistol_width, draw_y);
    glEnd();                                            

    glDisable(GL_TEXTURE_2D);
    glColor3f(1.0f, 1.0f, 1.0f);
}

void Renderer::present() {
    glutSwapBuffers();
}
