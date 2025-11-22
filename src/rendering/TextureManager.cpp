#include "rendering/TextureManager.h"
#include <iostream>

TextureManager::TextureManager() {}

TextureManager::~TextureManager() {
    cleanup();
}

void TextureManager::loadTexture(int id, const unsigned char data[64][64][3]) {
    GLuint textureID;
    
    //allocate the available textureID and bind it to the GL_TEXTURE_2D
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    //set the texture parameter
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    //upload the specific texture data
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 64, 64, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    
    textures_[id] = textureID;
    
    std::cout << "✓ Loaded texture ID " << id << " (OpenGL ID: " << textureID << ")" << std::endl;
}

void TextureManager::loadTextureRGBA(int id, const unsigned char data[64][64][4]) {
    GLuint textureID;
    
    //allocate the available textureID and bind it to the GL_TEXTURE_2D
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    //set the texture parameter
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    //upload the specific texture data with alpha channel
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    
    textures_[id] = textureID;
    
    std::cout << "✓ Loaded RGBA texture ID " << id << " (OpenGL ID: " << textureID << ")" << std::endl;
}

GLuint TextureManager::getTextureID(int wallType) const {
    // std::unordered_map<int, GLuint>::const_iterator 
    auto it = textures_.find(wallType);
    if (it != textures_.end()) {
        return it->second;
    }
    // Return 0 (no texture) if not found
    return 0;
}

bool TextureManager::hasTexture(int wallType) const {
    return textures_.find(wallType) != textures_.end();
}

void TextureManager::cleanup() {
    // pair<int, GLuint> 
    for (auto& pair : textures_) {
        // -> for pointer ; . for reference/object
        glDeleteTextures(1, &pair.second);
    }
    textures_.clear();
    std::cout << "✓ Cleaned up all textures" << std::endl;
}
