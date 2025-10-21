//https://www.youtube.com/watch?v=gYRrGTC7GtA

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

#include <math.h>
#include <stdio.h>
#include <stdbool.h>
#define PI 3.1415926535
#define DR 0.0174533    //one degree in radians

float px, py, pdx, pdy, pa; //player position
bool showinfo;

void drawPlayer(){
    glColor3f(1,1,0); // 设置颜色为黄色
    glPointSize(8);   //设置点的大小为8像素
    glBegin(GL_POINTS);   //开始绘制点
    glVertex2f(px, py);   //指定点的位置坐标
    glEnd();    //结束绘制
                
    glLineWidth(3);
    glBegin(GL_LINES);
    glVertex2i(px, py);
    glVertex2i(px + pdx * 5, py + pdy * 5);
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

int mapX = 8, mapY = 8, mapS = 64;
int map[] = {
    1, 1, 1, 1, 1, 1, 1, 1,
    1, 0, 0, 0, 0, 0, 0, 1, 
    1, 0, 1, 1, 0, 1, 0, 1, 
    1, 0, 0, 0, 0, 1, 0, 1, 
    1, 0, 1, 1, 0, 1, 0, 1, 
    1, 0, 0, 0, 0, 1, 0, 1, 
    1, 0, 0, 0, 0, 0, 0, 1, 
    1, 1, 1, 1, 1, 1, 1, 1,
};

float dist(float ax, float ay, float bx, float by, float ang) {
    return ( sqrtf((bx - ax)*(bx - ax) + (by - ay) * (by - ay)));
}

void drawMap2D(){
    int x, y, xo, yo;
    for (y = 0; y < mapY; y++) {
        for (x = 0; x < mapX; x++) {
            if (map[y*mapX + x] == 1){
                glColor3f(1,1,1);
            }else {
                glColor3f(0,0,0);
            }
            xo=x*mapS; 
            yo=y*mapS;
            glBegin(GL_QUADS); //开始绘制四边形
            glVertex2i(xo + 1, yo + 1);
            glVertex2i(xo + 1, yo + mapS - 1);
            glVertex2i(xo + mapS - 1, yo + mapS -1);
            glVertex2i(xo + mapS - 1, yo + 1);
            glEnd();
        }
    }
}

void drawRays2D(){
    int mx, my, mp, dof; 
    float rx, ry, ra, xo, yo, disT;
    ra = pa - DR * 30; 
    for (int r = 0; r < 120; r++){
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
            ry = (((int) py >> 6 ) << 6) - 1e-3 ;   // （int)py >> 6 表示把py先取整，接下来舍去低的6位，这相当于在十进制中除以64并且取整。((int) py >> 6 ) << 6 的作用其实就是再乘64得到最终的y坐标
            rx = (ry - py) * 1/tan(ra) + px; 
            yo = - 64;      // y 方向的步长(注意格子大小是64)
            xo = yo * 1/tan(ra);     // x方向的步长
        } 
        if (ra < PI) {
            ry = (((int) py >> 6 ) << 6) + 64 + 1e-3; 
            rx = (ry - py) * 1/tan(ra) + px; 
            yo = 64; 
            xo = yo * 1/tan(ra);
        } 
        if ( fabs(ra) <= 1e-4 || fabs(ra-PI) <= 1e-4){
            rx = px;
            ry = py;
            dof = 8;
        } 
        while (dof < 8) {
            mx = (int)(rx) >> 6;
            my = (int)(ry) >> 6;
            mp = my * mapX + mx;
            if (mp > 0 && mp < mapX * mapY && map[mp] == 1) { 
                hx = rx;
                hy = ry;
                disH = dist(px, py, hx, hy, ra);
                dof = 8;
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
            rx = (((int) px >> 6 ) << 6) - 1e-3 ;   
            ry = (rx - px) * tan(ra) + py; 
            xo = - 64;     
            yo = xo * tan(ra);     
        } 
        if (ra < PI/2 || ra > PI/2*3) {
            rx = (((int) px >> 6 ) << 6) + 64 + 1e-3; 
            ry = (rx - px) * tan(ra) + py; 
            xo = 64; 
            yo = xo * tan(ra);
        } 
        if ( fabs(ra - PI/2) <= 1e-4 || fabs(ra-PI/2*3) <= 1e-4){
            rx = px;
            ry = py;
            dof = 8;
        } 
        while (dof < 8) {
            mx = (int)(rx) >> 6;
            my = (int)(ry) >> 6;
            mp = my * mapX + mx;
            if (mp > 0 && mp < mapX * mapY && map[mp] == 1) { 
                dof = 8;
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
        glLineWidth(1);
        glBegin(GL_LINES);
        glVertex2f(px,py);
        glVertex2f(rx, ry);
        glEnd();
        // Draw 3D Walls 1024 * 512
        float ca = pa - ra; 
        if (ca < 0) {
            ca += 2 * PI;
        } 
        if (ca > 2 * PI) {
            ca -= 2 * PI;
        }
        disT = disT * cos(ca);
        float lineH = mapS * 512/disT; 
        if (lineH > 512) {
            lineH = 512;
        }
        float lineO = 256 - lineH/2;    //？也许调整之后有俯仰角
        glLineWidth(4.5);
        glBegin(GL_LINES);
        glVertex2f(r*4.5 + 515, lineO);
        glVertex2f(r*4.5 + 515, lineH + lineO);
        glEnd();
        ra += DR/2;
    }
}


//程序会把新一帧画面先画在后缓冲区，等画好后再一次性切换到前缓冲区里
void display(){
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //清除屏幕上的颜色缓冲区和深度缓冲区
    drawMap2D();    //先绘制地图
    drawPlayer();   //绘制玩家
    drawRays2D();   //初始化视野
    infoList();
    glutSwapBuffers();  //交换前后缓冲区，显示渲染结果
}

//unsigned 用来减少存储空间，增加代码可读性，表示这个变量不会出现负数
void buttons(unsigned char key, int x, int y){
    if(key=='a'){
        pa -= 0.1; 
        if (pa < 0){ pa += 2*PI; } 
        pdx = cos(pa) * 5; 
        pdy = sin(pa) * 5;
    }
    if(key=='d'){
        pa += 0.1; 
        if (pa > 2 * PI){ pa -= 2*PI; } 
        pdx = cos(pa) * 5; 
        pdy = sin(pa) * 5;
    }
    if(key=='w'){
        px += pdx;
        py += pdy;
    }
    if(key=='s'){
        px -= pdx;
        py -= pdy;
    }
    if(key=='i'){
        showinfo = !showinfo;   //逻辑非, 或者可以用1-x, 或者 1^x （异或运算)
    }
    glutPostRedisplay();    //请求刷新窗口,GLUT主循环(glutMainLoop)检测到之后调用glutDisplayFunc,也就是display()
}

void init(){
    glClearColor(0.3,0.3,0.3,0);   //设置清屏颜色为灰色 [0黑， 1白]
    gluOrtho2D(0,1024,512,0);     //设定而为正交投影坐标系
    showinfo = 0;
    px = 300; py = 300; pa = 0;   //玩家初始位置出现了
    pdx = cos(pa) * 5; pdy = sin(pa) * 5;     //初始的方向指向
}

/*** 
 C/C++的标准写法
 1. int argc: 命令行参数的个数(argument count)
 2. char* argv[]: 命令行参数的内容，是一个字符串数组，每一个元素都是一个参数
    e.g. ./myapp hello world ==> argc = 3; argv[0] = ./myapp argv[1] = hello argv[2] = world
 3. *指针，表示指向某个数据的地址
    char* argv[] 里 char* 是一个字符型指针，指向字符串的地址
    int* p 是整形指针，指向一个整数地址
 4. &取地址，表示获取变量的内存地址
    &argc 就是argc变量的地址，用于传给需要指针的函数
    e.g. int a = 5; int* p = &a; *p = 20 (通过指针把a改成20)
***/

int main(int argc, char* argv[]){
    glutInit(&argc, argv);   // 初始化GLUT库，处理命令行参数
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);   //设置显示模式,双缓冲+RGB颜色
    glutInitWindowSize(1024,512);    //设置窗口大小
    glutCreateWindow("Raycaster");   //创建标题为Raycaster的窗口
    init();     //调用用户自定义初始化函数，设置背景色和坐标系等
    glutDisplayFunc(display);     //注册display函数为显示事件回调，每次刷新时调用display()
    glutKeyboardFunc(buttons);    //注册buttons函数为键盘事件回调，当用户按键时自动调用它
    glutMainLoop();     //进入GLUT事件主循环，程序一直运行直到关闭窗口
}
