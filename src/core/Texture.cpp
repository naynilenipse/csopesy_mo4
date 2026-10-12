#include "core/Texture.h"

#include <cstdio>
#include <string>

#include <GLFW/glfw3.h>   // pulls in the OpenGL headers

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Windows' gl.h only knows OpenGL 1.1, so define this constant ourselves if missing.
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

bool Texture::Load(const char* path) {
    Free();

    int channels = 0;
    unsigned char* pixels = stbi_load(path, &width, &height, &channels, 4);   // force RGBA
    if (!pixels) {
        std::string backup = std::string("../") + path;
        pixels = stbi_load(backup.c_str(), &width, &height, &channels, 4);
    }
    if (!pixels) {
        std::printf("[Texture] Failed to load %s\n", path);
        width = height = 0;
        return false;
    }

    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    stbi_image_free(pixels);
    return true;
}

void Texture::Free() {
    if (id) {
        glDeleteTextures(1, &id);
        id = 0;
    }
    width = height = 0;
}