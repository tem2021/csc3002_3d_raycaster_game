//https://www.youtube.com/watch?v=gYRrGTC7GtA

#include <GL/freeglut_std.h>
#include <cmath>
#ifdef _WIN32
	// Windows (MinGW or Visual Studio with FreeGLUT)
	#include <GL/freeglut.h>
	#include <GL/gl.h>
	#include <GL/glu.h>
#elif defined(__APPLE__)
	// macOS (Apple's native GLUT)
	#include <GLUT/glut.h>
	#include <OpenGL/gl.h>
	#include <OpenGL/glu.h>
#elif defined(__linux__)
    // Linux (FreeGLUT)
    #include <GL/freeglut.h>
    #include <GL/gl.h>
    #include <GL/glu.h>
#endif

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h> // for memset
                    
const float PI = 3.1415926535;
const float DR = 0.0174533;   // one degree in radians
const int EDGE_BUFFER = 10;

float px, py, pdx, pdy, pa; // player position
float mouseDeltaX = 0; // movement for mouse at each frame
float moveSpeed = 19.0f;
float rotationSpeed = 0.0005f; // radians per frame
int width = 1980, height = 1020;
int centerX = width/2, centerY = height/2;
int mapX = 8, mapY = 8, mapS = height/mapY; // mapS: size of each square
bool showinfo;

int map[8][8] = {
	{1, 1, 1, 1, 1, 1, 1, 1},
	{1, 0, 0, 0, 0, 0, 0, 1},
	{1, 0, 0, 0, 0, 1, 0, 1},
	{1, 0, 0, 0, 0, 1, 0, 1},
	{1, 0, 1, 1, 0, 1, 0, 1},
	{1, 0, 0, 0, 0, 1, 0, 1},
	{1, 0, 0, 0, 0, 0, 0, 1},
	{1, 1, 1, 1, 1, 1, 1, 1},
};

void drawPlayer(){
    glColor3f(1,1,0); // 设置颜色为黄色
    glPointSize(8);   //设置点的大小为8像素
    glBegin(GL_POINTS);   //开始绘制点
    glVertex2f(px, py);   //指定点的位置坐标
    glEnd();    //结束绘制
                
    glLineWidth(3);
    glBegin(GL_LINES);
    glVertex2i(px, py);
    glVertex2i(px + pdx * moveSpeed, py + pdy * moveSpeed);
    glEnd();
}

// 用于调试查看变量
void drawText(int x, int y, const char* text){    //text的类型是const char*,是指向常量字符的指针，意味着不能通过这个指针去修改它指向的字符内容
    //用来绘制位图字符（文本）的起始位置，通常用于2d坐标系,其中0.3f强调了这是float(单精度浮点数)，默认0.3是double
    glColor3i(0,0,0);
    glRasterPos2i(x, y);
    /***  字符串是一串char,最后以\0结尾，例如"abc"在内存里是'a','b','c','\0'
     如果 abc ---> const char* text
     那么 *text = 'a'; *(text + 1) = 'b' ; 一直到 *text = '\0'
    ***/
    while (*text) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, *text); 
        text++;
    }
}

void infoList(){
    char infopa[32];    //清空字符串颁发 infopa[0] = '\0'
    sprintf(infopa, "pa: %.2f", pa);   //格式化字符串
    char infopx[32];
    sprintf(infopx, "px: %.2f", px); 
    char infopy[32];
    sprintf(infopy, "py: %.2f", py); 
    if (showinfo == 1){
        drawText(5, 15, infopa); 
        drawText(5, 25, infopx);
        drawText(5, 35, infopy);
    }
}

float dist(float ax, float ay, float bx, float by, float ang) {
    return ( sqrtf((bx - ax)*(bx - ax) + (by - ay) * (by - ay)));
}

void drawMap2D(){
    int mapPixelWidth = mapX * mapS; 
    int mapPixelHeight = mapY * mapS; 
    float scaleX = width * 0.5f / mapPixelWidth; 
    float scaleY = height * 1.0f / mapPixelHeight;

	for (int y = 0; y < mapY; y++) {
		for (int x = 0; x < mapX; x++) {
			if (map[y][x] == 1) glColor3f(1,1,1);
			else glColor3f(0,0,0);

			int xo = (int)(x * mapS * scaleX); 
            int yo = (int)(y * mapS * scaleY);

            glBegin(GL_QUADS);
            glVertex2i(xo + 1, yo + 1);
            glVertex2i(xo + 1, yo + (int)(mapS * scaleY) - 1);
            glVertex2i(xo + (int)(mapS * scaleX) - 1, yo + (int)(mapS * scaleY) -1);
            glVertex2i(xo + (int)(mapS * scaleX) - 1, yo + 1);
            glEnd();
		}
	}	
}

void drawCrosshair() {
    float crossLengthY = height * 0.01f;
    float crossLengthX = crossLengthY;
    float crossWidth = crossLengthX * 0.2f;

    glColor3f(1.0f, 1.0f, 1.0f); 
    glLineWidth(crossWidth);

    glBegin(GL_LINES);
    glVertex2f(centerX - crossLengthX / 2, centerY);
    glVertex2f(centerX + crossLengthX / 2, centerY);
    glVertex2f(centerX, centerY - crossLengthY / 2);
    glVertex2f(centerX, centerY + crossLengthY / 2);
    glEnd();

    glLineWidth(1.0f); 
}

void draw3Dview(){
    int mx, my, dof; 
    float rx, ry, ra, xo, yo, disT;
//    int numberofRays = (int)(width * 0.5); //half of the screen shows 3d view
    int numberofRays = width;
    float fov = 60.0f; // field of view 
    float rayStep = (fov * DR) / numberofRays;
    float epsilon = mapS * 1e-5f;

    ra = pa - (fov * DR / 2);
    for (int r = 0; r < numberofRays; r++){
        if (ra < 0) {
            ra += 2 * PI;
        }
        if (ra > 2 * PI) {
            ra -= 2* PI;
        }
        // Check Horizontal Lines
        dof = 0;    // depth of field，显示射线步进的最大次数
        float disH = 1e5, hx = px, hy = py;
        if (ra > PI) {      // 顺时针，朝上是ra > PI的情况
            ry = (((int) py / mapS ) * mapS) - epsilon ; 
            rx = (ry - py) * 1/tan(ra) + px; 
            yo = - mapS;      // y 方向的步长(注意格子大小是64)
            xo = yo * 1/tan(ra);     // x方向的步长
        } 
        if (ra < PI) {
            ry = (((int) py / mapS ) * mapS) + mapS + epsilon; 
            rx = (ry - py) * 1/tan(ra) + px; 
            yo = mapS; 
            xo = yo * 1/tan(ra);
        } 
        if ( fabs(ra) <= 1e-4 || fabs(ra-PI) <= 1e-4){
            rx = px;
            ry = py;
            dof = mapX;
        } 
        while (dof < mapX) {
            mx = (int)(rx) / mapS;
            my = (int)(ry) / mapS;            
            if ( mx >= 0 && mx < mapX && my >= 0 && my < mapY && map[my][mx] == 1) {
                hx = rx;
                hy = ry;
                disH = dist(px, py, hx, hy, ra);
                dof = mapX;
            } else {
                rx += xo;
                ry += yo;
                dof +=1;
            }
        }

        // Check Vertical Lines
        dof = 0;   
        float disV = 1e5, vx = px, vy = py;
        if (ra > PI/2 && ra < PI/2*3 ) {      
            rx = (((int) px / mapS) * mapS) - epsilon ;   
            ry = (rx - px) * tan(ra) + py; 
            xo = - mapS;     
            yo = xo * tan(ra);     
        } 
        if (ra < PI/2 || ra > PI/2*3) {
            rx = (((int) px / mapS) * mapS) + mapS + epsilon; 
            ry = (rx - px) * tan(ra) + py; 
            xo = mapS; 
            yo = xo * tan(ra);
        } 
        if ( fabs(ra - PI/2) <= 1e-4 || fabs(ra-PI/2*3) <= 1e-4){
            rx = px;
            ry = py;
            dof = mapY;
        } 
        while (dof < mapY) {
            mx = (int)(rx) / mapS;
            my = (int)(ry) / mapS;            
            if ( mx >= 0 && mx < mapX && my >= 0 && my < mapY && map[my][mx] == 1) {
                dof = mapY;
                vx = rx;
                vy = ry;
                disV = dist(px, py, vx, vy, ra);
            } else {
                rx += xo;
                ry += yo;
                dof +=1;
            }
        }
        if (disH < disV) { rx = hx; ry = hy; disT = disH; glColor3f(0.8,0.8,0.8);}
        if (disV < disH) { rx = vx; ry = vy; disT = disV; glColor3f(0.6, 0.6, 0.6);}
//        glLineWidth(1);
//        glBegin(GL_LINES);
//        glVertex2f(px,py);
//        glVertex2f(rx, ry);
//        glEnd();

        // Draw 3D Walls width * height
        float ca = pa - ra; 
        if (ca < 0) {
            ca += 2 * PI;
        } 
        if (ca > 2 * PI) {
            ca -= 2 * PI;
        }
        disT = disT * cos(ca);

        //float wallScreenX = r * (width * 0.5f / numberofRays) + width * 0.5f;
        float wallScreenX = r;
        float lineH = mapS * height / disT; // 高度比例
        if (lineH > height) lineH = height;
        float lineO = (height * 1.0f / 2) - lineH/2;

        glLineWidth(1); // 适配线宽
        glBegin(GL_LINES);
        glVertex2f(wallScreenX, lineO);
        glVertex2f(wallScreenX, lineO + lineH);
        glEnd();
        ra += rayStep; // update the ray's angle
    }
}

// 256 because ASCII codes fit in 256 elements
bool keystate[256];

//unsigned 用来减少存储空间，增加代码可读性，表示这个变量不会出现负数
void keyDown(unsigned char key, int x, int y) {
	keystate[(unsigned char)key] = true;

    if (key == 27) exit(0);
}

void keyUp(unsigned char key, int x, int y) {
	keystate[(unsigned char)key] = false;
	
	if (key == 'i') {
		showinfo = !showinfo;
	}
}

bool willCollide(float x, float y) {
	int mapXIndex = (int)(x) / mapS;
	int mapYIndex = (int)(y) / mapS;

	// walking off of the map
	if (mapXIndex < 0 || mapXIndex >= mapX || mapYIndex < 0 || mapYIndex >= mapY)
		return true;

	// walking into a wall
	return map[mapYIndex][mapXIndex] == 1;
}


void passiveMouseMove(int x, int y) {
    if (x == centerX && y == centerY) return; 
    mouseDeltaX += (x - centerX);
    glutWarpPointer(centerX, centerY);
}

void mouseMove() {
    if (mouseDeltaX != 0) {
        pa += mouseDeltaX * rotationSpeed; 
        if (pa < 0) pa += 2 * PI;
        if (pa > 2 * PI) pa -= 2 * PI; 
        pdx = cos(pa) * moveSpeed;
        pdy = sin(pa) * moveSpeed;
        mouseDeltaX = 0; 
    }
}

void keyboardMove() {
	if(keystate[(unsigned char)'w']){
		float newX = px + pdx;
		float newY = py + pdy;
		if (!willCollide(newX, newY)) {
			px = newX;
			py = newY;
		}
	}
	if(keystate[(unsigned char)'s']){
		float newX = px - pdx;
		float newY = py - pdy;
		if (!willCollide(newX, newY)) {
			px = newX;
			py = newY;
		}
	}
	if(keystate[(unsigned char)'d']){
        float newX = px - sin(pa) * moveSpeed;
        float newY = py + cos(pa) * moveSpeed; 
		if (!willCollide(newX, newY)) {
			px = newX;
			py = newY;
		}
	}
	if(keystate[(unsigned char)'a']){
        float newX = px + sin(pa) * moveSpeed;
        float newY = py - cos(pa) * moveSpeed; 
		if (!willCollide(newX, newY)) {
			px = newX;
			py = newY;
		}
	}
}

//程序会把新一帧画面先画在后缓冲区，等画好后再一次性切换到前缓冲区里
void display(){
	// update game state
	keyboardMove();
    mouseMove();

	// render game state
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //清除屏幕上的颜色缓冲区和深度缓冲区    
	//drawMap2D();    //先绘制地图
    //drawPlayer();   //绘制玩家
    //drawRays2D();   //初始化视野
    draw3Dview();
    drawCrosshair();
    infoList();
    glutSwapBuffers();  //交换前后缓冲区，显示渲染结果
}

void timerFunc(int value) {
	glutPostRedisplay();
	glutTimerFunc(16, timerFunc, 0); // will run timerFunc again after 16 ms, gives roughly 60 Hz
}

void init(){
    width = glutGet(GLUT_SCREEN_WIDTH);
    height = glutGet(GLUT_SCREEN_HEIGHT);

    glClearColor(0.3,0.3,0.3,0);   //设置清屏颜色为灰色 [0黑， 1白]
    gluOrtho2D(0,width,height,0);     //设定而为正交投影坐标系
    showinfo = 0;
    px = 300; 
    py = 300; 
    pa = 0;   //玩家初始位置出现了
    pdx = cos(pa) * moveSpeed; 
    pdy = sin(pa) * moveSpeed;     //初始的方向指向
	memset(keystate, 0, sizeof(keystate)); // initialise keystate as array of 0
}

int main(int argc, char* argv[]){
    glutInit(&argc, argv);   // 初始化GLUT库，处理命令行参数
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);   //设置显示模式,双缓冲+RGB颜色
    //glutInitWindowSize(width,height);    //设置窗口大小
    //glutInitWindowPosition(0, 0);
    //glutCreateWindow("Raycaster");   //创建标题为Raycaster的窗口
    glutEnterGameMode();

    init();     //调用用户自定义初始化函数，设置背景色和坐标系等
    glutDisplayFunc(display);     //注册display函数为显示事件回调，每次刷新时调用display()
    glutSetCursor(GLUT_CURSOR_NONE);
    
    // user input
	glutKeyboardFunc(keyDown);
	glutKeyboardUpFunc(keyUp);
    glutPassiveMotionFunc(passiveMouseMove);
	
	glutTimerFunc(16, timerFunc, 0);
	
	glutMainLoop();     //进入GLUT事件主循环，程序一直运行直到关闭窗口
}
