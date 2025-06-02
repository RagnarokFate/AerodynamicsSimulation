#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "ShaderProgram.h"

class PostProcessor
{
public:
    PostProcessor();
    ~PostProcessor();
    
    // Initialize post-processing resources
    void Initialize(int width, int height);
    void Cleanup();
    
    // Resize render targets
    void Resize(int width, int height);
      // Gaussian blur operations
    void BeginFrame();
    void EndFrame();
    GLuint ApplyGaussianBlur(GLuint inputTexture, float blurRadius = 1.0f, float blurStrength = 0.5f, int iterations = 1);
    GLuint ApplyPressureBlur(GLuint pressureTexture, float minPressure, float maxPressure, float blurRadius = 1.0f, float blurStrength = 0.3f);
    GLuint ApplyVelocityBlur(GLuint velocityTexture, float maxVelocity, float blurRadius = 1.0f, float blurStrength = 0.3f);
    
    // Texture rendering
    void RenderTexture(GLuint texture, float alpha = 1.0f);
    
    // Bind for offscreen rendering
    void BindFramebuffer();
    void UnbindFramebuffer();
    
    // Get final processed texture
    GLuint GetFinalTexture() const { return colorTextures[0]; }
    
    // Configuration
    void SetBlurParameters(float radius, float strength, int iterations);
    void EnableBlur(bool enable) { blurEnabled = enable; }
    bool IsBlurEnabled() const { return blurEnabled; }

private:
    // Framebuffer resources
    GLuint framebuffers[2];  // Ping-pong framebuffers for multi-pass blur
    GLuint colorTextures[2]; // Color attachments for ping-pong
    GLuint depthBuffer;      // Shared depth buffer
    
    // Fullscreen quad for post-processing
    GLuint quadVAO, quadVBO;
      // Shaders
    ShaderProgram gaussianBlurShader;
    ShaderProgram pressureBlurShader;
    ShaderProgram velocityBlurShader;
    ShaderProgram textureShader;
    
    // State
    int screenWidth, screenHeight;
    bool initialized;
    bool blurEnabled;
    
    // Blur parameters
    float blurRadius;
    float blurStrength;
    int blurIterations;
    
    // Helper methods
    void CreateFramebuffers();
    void CreateFullscreenQuad();
    void LoadShaders();
    void RenderFullscreenQuad();
    
    // Ping-pong rendering for separable blur
    void PingPongBlur(GLuint inputTexture, float radius, float strength, int iterations);
};
