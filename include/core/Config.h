#ifndef CONFIG_H
#define CONFIG_H

namespace Math {
    constexpr float PI = 3.1415926535f;
    constexpr float TWO_PI = 6.2831853071f;
    constexpr float HALF_PI = 1.5707963267f;
    constexpr float DEG_TO_RAD = 0.0174533f;
    constexpr float EPSILON = 1e-5f;
}

namespace GameConfig {
    constexpr float ROTATION_SPEED = 0.0005f;
    constexpr float MOVE_SPEED_FACTOR = 0.1f;
    constexpr float FOV = 60.0f;
    constexpr int TARGET_FPS = 60;
    constexpr int FRAME_TIME_MS = 1000 / TARGET_FPS;
}

namespace RenderConfig {
    constexpr float WALL_BRIGHTNESS_H = 0.8f;  // 水平墙面亮度
    constexpr float WALL_BRIGHTNESS_V = 0.6f;  // 垂直墙面亮度
    constexpr float FLOOR_BRIGHTNESS = 0.3f;
    constexpr float CEILING_BRIGHTNESS = 0.2f;
    constexpr float MIN_BRIGHTNESS = 0.15f;
    constexpr float MAX_FLOOR_DIST = 10.0f;
    constexpr float CROSSHAIR_SIZE = 0.01f;
}

#endif // CONFIG_H
