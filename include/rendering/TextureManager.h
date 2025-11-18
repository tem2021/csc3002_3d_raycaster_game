#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H

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

#include <unordered_map>

class TextureManager {
public:
    TextureManager();
    ~TextureManager();
    
    // Load a 64x64 texture
    void loadTexture(int id, const unsigned char data[64][64][3]);
    
    // Get OpenGL texture ID for a wall type
    GLuint getTextureID(int wallType) const;
    
    // Check if texture exists
    bool hasTexture(int wallType) const;
    
    // Cleanup all textures
    void cleanup();
    
private:
    std::unordered_map<int, GLuint> textures_;
    
    // Disable copy
    TextureManager(const TextureManager&) = delete;
    TextureManager& operator=(const TextureManager&) = delete;
};

#endif // TEXTURE_MANAGER_H
