#include "PostProcessor.h"
#include <iostream>
#include <vector>

PostProcessor::PostProcessor()
    : initialized(false)
    , blurEnabled(true)
    , screenWidth(0)
    , screenHeight(0)
    , blurRadius(1.0f)
    , blurStrength(0.5f)
    , blurIterations(2)
{
    framebuffers[0] = framebuffers[1] = 0;
    colorTextures[0] = colorTextures[1] = 0;
    depthBuffer = 0;
    quadVAO = quadVBO = 0;
}

PostProcessor::~PostProcessor()
{
    Cleanup();
}

void PostProcessor::Initialize(int width, int height)
{
    if (initialized) {
        Cleanup();
    }
    
    screenWidth = width;
    screenHeight = height;
    
    LoadShaders();
    CreateFramebuffers();
    CreateFullscreenQuad();
    
    initialized = true;
    std::cout << "PostProcessor: Initialized with resolution " << width << "x" << height << std::endl;
}

void PostProcessor::Cleanup()
{
    if (!initialized) return;
    
    // Clean up framebuffers
    if (framebuffers[0] != 0) {
        glDeleteFramebuffers(2, framebuffers);
        framebuffers[0] = framebuffers[1] = 0;
    }
    
    // Clean up textures
    if (colorTextures[0] != 0) {
        glDeleteTextures(2, colorTextures);
        colorTextures[0] = colorTextures[1] = 0;
    }
    
    if (depthBuffer != 0) {
        glDeleteTextures(1, &depthBuffer);
        depthBuffer = 0;
    }
    
    // Clean up quad
    if (quadVAO != 0) {
        glDeleteVertexArrays(1, &quadVAO);
        glDeleteBuffers(1, &quadVBO);
        quadVAO = quadVBO = 0;
    }
    
    initialized = false;
}

void PostProcessor::Resize(int width, int height)
{
    if (width == screenWidth && height == screenHeight) return;
    
    screenWidth = width;
    screenHeight = height;
    
    if (initialized) {
        // Recreate framebuffers with new size
        if (framebuffers[0] != 0) {
            glDeleteFramebuffers(2, framebuffers);
            glDeleteTextures(2, colorTextures);
            glDeleteTextures(1, &depthBuffer);
        }
        CreateFramebuffers();
    }
}

void PostProcessor::LoadShaders()
{
    gaussianBlurShader.loadShaders("gaussian_blur_vertex.glsl", "gaussian_blur_fragment.glsl");
    pressureBlurShader.loadShaders("gaussian_blur_vertex.glsl", "pressure_blur_fragment.glsl");
    velocityBlurShader.loadShaders("gaussian_blur_vertex.glsl", "velocity_blur_fragment.glsl");
    textureShader.loadShaders("texture_vertex.glsl", "texture_fragment.glsl");
}

void PostProcessor::CreateFramebuffers()
{
    // Generate framebuffers
    glGenFramebuffers(2, framebuffers);
    glGenTextures(2, colorTextures);
    glGenTextures(1, &depthBuffer);
    
    for (int i = 0; i < 2; i++) {
        // Create color texture
        glBindTexture(GL_TEXTURE_2D, colorTextures[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, screenWidth, screenHeight, 0, GL_RGB, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        // Setup framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffers[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTextures[i], 0);
        
        // Check framebuffer completeness
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "PostProcessor: Framebuffer " << i << " not complete!" << std::endl;
        }
    }
    
    // Create shared depth buffer
    glBindTexture(GL_TEXTURE_2D, depthBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, screenWidth, screenHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // Attach depth buffer to both framebuffers
    for (int i = 0; i < 2; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffers[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthBuffer, 0);
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void PostProcessor::CreateFullscreenQuad()
{
    // Fullscreen quad vertices (position + texcoord)
    float quadVertices[] = {
        // positions   // texcoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
        
        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };
    
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    
    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    
    // Texture coordinate attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    
    glBindVertexArray(0);
}

void PostProcessor::RenderFullscreenQuad()
{
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void PostProcessor::BeginFrame()
{
    if (!initialized) return;
    
    // Bind first framebuffer for rendering
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffers[0]);
    glViewport(0, 0, screenWidth, screenHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void PostProcessor::EndFrame()
{
    if (!initialized) return;
    
    // Return to default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PostProcessor::BindFramebuffer()
{
    if (!initialized) return;
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffers[0]);
    glViewport(0, 0, screenWidth, screenHeight);
}

void PostProcessor::UnbindFramebuffer()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

GLuint PostProcessor::ApplyGaussianBlur(GLuint inputTexture, float blurRadius, float blurStrength, int iterations)
{
    if (!initialized || !blurEnabled) return inputTexture;
    
    // Store current viewport
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    
    // Disable depth testing for post-processing
    glDisable(GL_DEPTH_TEST);
    
    PingPongBlur(inputTexture, blurRadius, blurStrength, iterations);
    
    // Restore previous state
    glEnable(GL_DEPTH_TEST);
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    
    return colorTextures[0]; // Return final blurred texture
}

GLuint PostProcessor::ApplyPressureBlur(GLuint pressureTexture, float minPressure, float maxPressure, float blurRadius, float blurStrength)
{
    if (!initialized || !blurEnabled) return pressureTexture;
    
    // Store current viewport
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    
    glDisable(GL_DEPTH_TEST);
    glViewport(0, 0, screenWidth, screenHeight);
    
    // Use specialized pressure blur shader
    pressureBlurShader.use();
    pressureBlurShader.setUniform("pressureTexture", 0);
    pressureBlurShader.setUniform("texelSize", glm::vec2(1.0f / screenWidth, 1.0f / screenHeight));
    pressureBlurShader.setUniform("blurRadius", blurRadius);
    pressureBlurShader.setUniform("blurStrength", blurStrength);
    pressureBlurShader.setUniform("minPressure", minPressure);
    pressureBlurShader.setUniform("maxPressure", maxPressure);
    
    // Horizontal pass
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffers[1]);
    glClear(GL_COLOR_BUFFER_BIT);
    pressureBlurShader.setUniform("horizontal", true);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, pressureTexture);
    RenderFullscreenQuad();
    
    // Vertical pass
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffers[0]);
    glClear(GL_COLOR_BUFFER_BIT);
    pressureBlurShader.setUniform("horizontal", false);
    glBindTexture(GL_TEXTURE_2D, colorTextures[1]);
    RenderFullscreenQuad();
    
    // Restore state
    glEnable(GL_DEPTH_TEST);
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    return colorTextures[0];
}

GLuint PostProcessor::ApplyVelocityBlur(GLuint velocityTexture, float maxVelocity, float blurRadius, float blurStrength)
{
    if (!initialized || !blurEnabled) return velocityTexture;
    
    // Store current viewport
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    
    glDisable(GL_DEPTH_TEST);
    glViewport(0, 0, screenWidth, screenHeight);
    
    // Use specialized velocity blur shader
    velocityBlurShader.use();
    velocityBlurShader.setUniform("velocityTexture", 0);
    velocityBlurShader.setUniform("texelSize", glm::vec2(1.0f / screenWidth, 1.0f / screenHeight));
    velocityBlurShader.setUniform("blurRadius", blurRadius);
    velocityBlurShader.setUniform("blurStrength", blurStrength);
    velocityBlurShader.setUniform("maxVelocity", maxVelocity);
    
    // Horizontal pass
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffers[1]);
    glClear(GL_COLOR_BUFFER_BIT);
    velocityBlurShader.setUniform("horizontal", true);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, velocityTexture);
    RenderFullscreenQuad();
    
    // Vertical pass
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffers[0]);
    glClear(GL_COLOR_BUFFER_BIT);
    velocityBlurShader.setUniform("horizontal", false);
    glBindTexture(GL_TEXTURE_2D, colorTextures[1]);
    RenderFullscreenQuad();
    
    // Restore state
    glEnable(GL_DEPTH_TEST);
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    return colorTextures[0];
}

void PostProcessor::PingPongBlur(GLuint inputTexture, float radius, float strength, int iterations)
{
    gaussianBlurShader.use();
    gaussianBlurShader.setUniform("inputTexture", 0);
    gaussianBlurShader.setUniform("texelSize", glm::vec2(1.0f / screenWidth, 1.0f / screenHeight));
    gaussianBlurShader.setUniform("blurRadius", radius);
    gaussianBlurShader.setUniform("blurStrength", strength);
    
    bool horizontal = true;
    bool firstIteration = true;
    
    for (int i = 0; i < iterations * 2; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffers[horizontal ? 1 : 0]);
        glClear(GL_COLOR_BUFFER_BIT);
        
        gaussianBlurShader.setUniform("horizontal", horizontal);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, firstIteration ? inputTexture : colorTextures[horizontal ? 0 : 1]);
        
        RenderFullscreenQuad();
        
        horizontal = !horizontal;
        if (firstIteration) firstIteration = false;
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PostProcessor::SetBlurParameters(float radius, float strength, int iterations)
{
    blurRadius = radius;
    blurStrength = strength;
    blurIterations = iterations;
}

void PostProcessor::RenderTexture(GLuint texture, float alpha)
{
    if (!initialized) return;
    
    // Save current state
    GLboolean depthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
    GLboolean blendEnabled = glIsEnabled(GL_BLEND);
    GLint blendSrcAlpha, blendDstAlpha;
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &blendSrcAlpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &blendDstAlpha);
    
    // Setup for texture rendering
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    textureShader.use();
    textureShader.setUniform("textureToRender", 0);
    textureShader.setUniform("alpha", alpha);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    
    RenderFullscreenQuad();
    
    // Restore state
    if (depthTestEnabled) glEnable(GL_DEPTH_TEST);
    if (!blendEnabled) glDisable(GL_BLEND);
    glBlendFunc(blendSrcAlpha, blendDstAlpha);
}
