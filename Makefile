OS_NAME = ${shell uname -s}
SRC = raycaster.cpp
TARGET = raycaster

# Default: Linux
LIBS = -lGL -lGLU -lglut -lm

# macOS
ifeq (${OS_NAME}, Darwin) 
	LIBS = -framework OpenGL -framework GLUT -lm 
endif 

# windows
ifeq (${OS_NAME}, Windows_NT) 
	LIBS = -lfreeglut -lopengl32 -lglut32 -lm 
endif 

${TARGET} : ${SRC}
	gcc -Wall -g ${SRC} -o ${TARGET} ${LIBS}

clean: 
	rm -f ${TARGET}
