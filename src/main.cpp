#include <GL/freeglut_std.h>
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

// Unique Global Game Instance
std::unique_ptr<Game> g_game;

// GLUT Callback Function
void displayCallback() {
    g_game->handleInput();
    g_game->update();
    g_game->render();
    
    if (g_game->shouldExit()) {
        exit(0);
    }
}

void timerCallback(int) {
    glutPostRedisplay();
    glutTimerFunc(GameConfig::FRAME_TIME_MS, timerCallback, 0);
}

void keyboardDownCallback(unsigned char key, int, int) {
    g_game->getInputManager().handleKeyDown(key);
}

void keyboardUpCallback(unsigned char key, int, int) {
    g_game->getInputManager().handleKeyUp(key);
}

void specialKeyDownCallback(int key, int, int) {
    g_game->getInputManager().handleSpecialKeyDown(key);
}

void specialKeyUpCallback(int key, int, int ) {
    g_game->getInputManager().handleSpecialKeyUp(key);
}

void mouseMotionCallback(int x, int y) {
    g_game->getInputManager().handleMouseMove(x, y);
}

void mouseButtonCallback(int button, int state, int x, int y) {
    g_game->getInputManager().handleMouseButton(button, state, x, y);
}

int main(int argc, char* argv[]) {
    // Initialize GLUT
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    
    // Create the Game
    g_game = std::make_unique<Game>();
    
    // Enter the Fullscreen Game
    // glutEnterGameMode();

    // Enter the Window Game
    glutInitWindowSize(GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT);
    glutCreateWindow("Raycaster");
    
    // Initialize the Game
    g_game->init();

    // Ignore Key Repeat 
    glutIgnoreKeyRepeat(1);
    
    // Set OpenGL
    int width = glutGet(GLUT_SCREEN_WIDTH);
    int height = glutGet(GLUT_SCREEN_HEIGHT);
    glClearColor(0.3f, 0.3f, 0.3f, 0.0f);
    gluOrtho2D(0, width, height, 0);
    
    // Hide Cursor
    glutSetCursor(GLUT_CURSOR_NONE);
    
    // Register Callback 
    glutDisplayFunc(displayCallback);
    glutKeyboardFunc(keyboardDownCallback);
    glutKeyboardUpFunc(keyboardUpCallback);
    glutSpecialFunc(specialKeyDownCallback);
    glutSpecialUpFunc(specialKeyUpCallback);
    glutPassiveMotionFunc(mouseMotionCallback);
    glutMouseFunc(mouseButtonCallback);
    glutTimerFunc(GameConfig::FRAME_TIME_MS, timerCallback, 0);
    
    // Enter the Main Loop
    glutMainLoop();
    
    return 0;
}
