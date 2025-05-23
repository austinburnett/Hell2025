#include "GL_shadow_map_array.h"
#include <glad/glad.h>
#include <iostream>

void OpenGLShadowCubeMapArray::Init(unsigned int numberOfCubemaps, int size) {
    m_numberOfCubemaps = numberOfCubemaps;
    m_size = size;
    glGenFramebuffers(1, &m_handle);
    glGenTextures(1, &m_depthTexture);
    glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, m_depthTexture);

    // Allocate storage for cubemap array
    glTexStorage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 1, GL_DEPTH_COMPONENT16, size, size, 6 * m_numberOfCubemaps);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    // Bind framebuffer and attach the cubemap array
    glBindFramebuffer(GL_FRAMEBUFFER, m_handle);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_depthTexture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Framebuffer not complete!\n";
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Create the texture view to alias the cubemap array as a 2D texture array
    glGenTextures(1, &m_textureView);  // Generate a new texture for the view
    glTextureView(
        m_textureView,                  // The texture view handle
        GL_TEXTURE_2D_ARRAY,            // Alias it as a 2D texture array
        m_depthTexture,                 // The original cubemap array texture
        GL_DEPTH_COMPONENT16,           // Format matches the original
        0,                              // Mip level start
        1,                              // Number of mipmap levels
        0,                              // First layer (start from the first layer)
        6 * m_numberOfCubemaps          // Number of layers (6 faces per cubemap)
    );
}

void OpenGLShadowCubeMapArray::CleanUp() {
    glDeleteTextures(1, &m_depthTexture);
    glDeleteTextures(1, &m_textureView);
    glDeleteFramebuffers(1, &m_handle);
}

void OpenGLShadowCubeMapArray::ClearDepth() {
    glViewport(0, 0, m_size, m_size);
    glBindFramebuffer(GL_FRAMEBUFFER, m_handle);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
