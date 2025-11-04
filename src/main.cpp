#ifdef _WIN32
    #include <GL/freeglut.h>
    #include <GL/gl.h>
    #include <GL/glu.h>
#elif defined(__APPLE__)
    #include <GLUT/glut.h>
    #include <OpenGL/gl.h>
    #include <OpenGL/glu.h>
#elif defined(__linux__)
    #include <GL/freeglut.h>
    #include <GL/gl.h>
    #include <GL/glu.h>
#endif

#include "core/Game.h"
#include "core/Config.h"
#include <memory>
#include <cstdlib>

// 全局游戏实例（用于 GLUT 回调）
std::unique_ptr<Game> g_game;

// GLUT 回调函数
void displayCallback() {
    g_game->handleInput();
    g_game->update();
    g_game->render();
    
    if (g_game->shouldExit()) {
        exit(0);
    }
}

void timerCallback(int value) {
    glutPostRedisplay();
    glutTimerFunc(GameConfig::FRAME_TIME_MS, timerCallback, 0);
}

void keyboardDownCallback(unsigned char key, int x, int y) {
    g_game->getInputManager().handleKeyDown(key);
}

void keyboardUpCallback(unsigned char key, int x, int y) {
    g_game->getInputManager().handleKeyUp(key);
}

void mouseMotionCallback(int x, int y) {
    g_game->getInputManager().handleMouseMove(x, y);
}

int main(int argc, char* argv[]) {
    // 初始化 GLUT
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    
    // 创建游戏
    g_game = std::make_unique<Game>();
    
    // 进入全屏模式
    glutEnterGameMode();
    
    // 初始化游戏
    g_game->init();
    
    // 设置 OpenGL
    int width = glutGet(GLUT_SCREEN_WIDTH);
    int height = glutGet(GLUT_SCREEN_HEIGHT);
    glClearColor(0.3f, 0.3f, 0.3f, 0.0f);
    gluOrtho2D(0, width, height, 0);
    
    // 隐藏鼠标
    glutSetCursor(GLUT_CURSOR_NONE);
    
    // 注册回调
    glutDisplayFunc(displayCallback);
    glutKeyboardFunc(keyboardDownCallback);
    glutKeyboardUpFunc(keyboardUpCallback);
    glutPassiveMotionFunc(mouseMotionCallback);
    glutTimerFunc(GameConfig::FRAME_TIME_MS, timerCallback, 0);
    
    // 进入主循环
    glutMainLoop();
    
    return 0;
}
