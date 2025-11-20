#ifndef CONFIG_H
#define CONFIG_H

namespace Math {
    constexpr float PI = 3.1415926535f;
    constexpr float TWO_PI = 6.2831853071f;
    constexpr float HALF_PI = 1.5707963267f;
    constexpr float DEG_TO_RAD = 0.0174533f;
    constexpr float RAD_TO_DEG = 57.2957795f;
    constexpr float EPSILON = 1e-5f;
}

namespace GameConfig {
    constexpr float FOV = 60.0f;    // between 0.0f to 180.0f
                                   
    constexpr float ROTATION_SPEED = 0.0005f;
    constexpr float MOVE_SPEED_FACTOR = 0.1f;
    constexpr float SPRINT_MULTIPLIER = 2.0f;   // SPRINT_SPEED / MOVE_SPEED
    constexpr int TARGET_FPS = 60;
    constexpr int FRAME_TIME_MS = 1000 / TARGET_FPS;
    constexpr int WINDOW_WIDTH = 1280;
    constexpr int WINDOW_HEIGHT = 720;
}

namespace RenderConfig {
    constexpr float RESOLUTION_RATIO = 6;   // Larger for higher resolution
    constexpr int FLOOR_TEXTURE_ID = 4;     // DEFINE the TEXTURE you want to use
    constexpr int CEILING_TEXTURE_ID = 2;
                                           
    constexpr float WALL_BRIGHTNESS_H = 0.8f;  // BRIGHTNESS_OF_HORIZONTAL_WALL
    constexpr float WALL_BRIGHTNESS_V = 0.6f;  // BRIGHTNESS_OF_VERTICAL_WALL
    constexpr float FLOOR_BRIGHTNESS = 0.5f;
    constexpr float CEILING_BRIGHTNESS = 0.5f;
    constexpr float FOV_HEIGHT =  53.13010235415598f;
    constexpr float CROSSHAIR_SIZE = 0.01f;
    constexpr float MIN_WALL_DISTANCE = 0.1f;  // Minimum distance to prevent texture distortion
}

namespace PlayerConfig {
    constexpr int MAX_HEALTH = 100;
    constexpr float HEALTH_BAR_WIDTH_PERCENT = 0.25f;
    constexpr float HEALTH_BAR_HEIGHT_PERCENT = 0.015f;
    constexpr int HEALTH_BAR_MARGIN = 10;
}

#endif // CONFIG_H
