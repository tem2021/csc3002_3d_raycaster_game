#include "rendering/Renderer.h"
#include "core/Config.h"
#include "core/Types.h"
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
        float centerY = static_cast<float>(screenHeight_) / 2;

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
