#pragma once
#include <glm/glm.hpp>
#include <glad/glad.h>
#include <vector>
#include <memory>
#include "AerodynamicsGrid.h"
#include "FluidSimulation.h"
#include "ShaderProgram.h"
#include "PostProcessor.h"

using namespace glm;

// Visualization settings
struct VisualizationSettings {
    // Velocity field visualization
    bool showVelocityVectors = true;
    float velocityScale = 1.0f;
    float vectorLength = 0.1f;
    vec3 velocityColor = vec3(0.0f, 1.0f, 0.0f);
    
    // Pressure visualization
    bool showPressure = true;
    float pressureScale = 1.0f;
    vec3 pressureColorLow = vec3(0.0f, 0.0f, 1.0f);   // Blue for low pressure
    vec3 pressureColorHigh = vec3(1.0f, 0.0f, 0.0f);  // Red for high pressure
    
    // Streamlines
    bool showStreamlines = false;
    int numStreamlines = 50;
    float streamlineLength = 2.0f;
    vec3 streamlineColor = vec3(1.0f, 1.0f, 0.0f);
    
    // Particles
    bool showParticles = false;
    int numParticles = 1000;
    float particleSize = 0.01f;
    vec3 particleColor = vec3(1.0f, 0.5f, 0.0f);
    
    // Grid visualization
    bool showGrid = false;
    float gridOpacity = 0.3f;
    vec3 gridColor = vec3(0.5f, 0.5f, 0.5f);
    
    // Slicing planes
    bool showSlicePlane = false;
    vec3 slicePlaneNormal = vec3(0.0f, 0.0f, 1.0f);
    float slicePlanePosition = 0.0f;
    
    VisualizationSettings() = default;
};

// Gaussian blur settings for post-processing
struct BlurSettings {
    bool enablePressureBlur = true;
    bool enableVelocityBlur = true;
    bool enableParticleBlur = false;
    float blurRadius = 1.5f;
    float blurStrength = 0.4f;
    int blurIterations = 2;
    bool adaptiveBlur = true;  // Adjust blur based on simulation data
    float minBlurThreshold = 0.1f;  // Minimum change required for blur
    
    BlurSettings() = default;
};

// Particle for flow visualization
struct FlowParticle {
    vec3 position;
    vec3 velocity;
    float life;
    float maxLife;
    
    FlowParticle() : position(0.0f), velocity(0.0f), life(0.0f), maxLife(5.0f) {}
    FlowParticle(const vec3& pos, float lifeTime) 
        : position(pos), velocity(0.0f), life(lifeTime), maxLife(lifeTime) {}
};

// Handles rendering of aerodynamic simulation data
class AerodynamicsVisualizer {
public:
    AerodynamicsVisualizer();
    ~AerodynamicsVisualizer();
    
    // Initialize OpenGL resources
    void initialize() { Initialize(); }
    void Initialize();
    void Cleanup();
    
    // Main rendering function
    void Render(const FluidSimulation& simulation, const mat4& view, const mat4& projection);
    
    // Individual rendering components
    void renderVelocityVectors(const AerodynamicsGrid& grid, const mat4& view, const mat4& projection) { RenderVelocityVectors(grid, view, projection); }
    void renderPressureField(const AerodynamicsGrid& grid, const mat4& view, const mat4& projection) { RenderPressureField(grid, view, projection); }
    void renderStreamlines(const FluidSimulation& simulation, const mat4& view, const mat4& projection) { RenderStreamlines(simulation, view, projection); }
    void renderParticles(const FluidSimulation& simulation, const mat4& view, const mat4& projection) { RenderParticles(simulation, view, projection); }
    void renderGrid(const AerodynamicsGrid& grid, const mat4& view, const mat4& projection) { RenderGrid(grid, view, projection); }
    
    void RenderVelocityVectors(const AerodynamicsGrid& grid, const mat4& view, const mat4& projection);
    void RenderPressureField(const AerodynamicsGrid& grid, const mat4& view, const mat4& projection);
    void RenderStreamlines(const FluidSimulation& simulation, const mat4& view, const mat4& projection);
    void RenderParticles(const FluidSimulation& simulation, const mat4& view, const mat4& projection);
    void RenderGrid(const AerodynamicsGrid& grid, const mat4& view, const mat4& projection);
    void RenderSlicePlane(const AerodynamicsGrid& grid, const mat4& view, const mat4& projection);
    
    // Data update methods
    void updateVelocityData(const AerodynamicsGrid& grid) { UpdateVelocityBuffers(grid); }
    void updatePressureData(const AerodynamicsGrid& grid) { UpdatePressureBuffers(grid); }
    void updateStreamlines(const FluidSimulation& simulation) { GenerateStreamlines(simulation); }
    void updateParticles(const FluidSimulation& simulation, float deltaTime) { UpdateParticles(simulation, deltaTime); }
    
    // Parameter control methods
    void setParticleCount(int count);
    void setStreamlineParameters(int count, int maxLength);    // Settings
    void SetVisualizationSettings(const VisualizationSettings& settings) { this->settings = settings; }
    const VisualizationSettings& GetVisualizationSettings() const { return settings; }
    void SetBlurSettings(const BlurSettings& blurSettings) { this->blurSettings = blurSettings; }
    const BlurSettings& GetBlurSettings() const { return blurSettings; }
    
    // Post-processing controls
    void InitializePostProcessing(int width, int height);
    void ResizePostProcessing(int width, int height);
    void SetBlurParameters(float radius, float strength, int iterations);
    void EnablePressureBlur(bool enable);
    void EnableVelocityBlur(bool enable);
    bool IsPressureBlurEnabled() const;
    bool IsVelocityBlurEnabled() const;
    
    // Particle system
    void UpdateParticles(const FluidSimulation& simulation, float deltaTime);
    void ResetParticles(const AerodynamicsGrid& grid);
    
    // Streamline generation
    void GenerateStreamlines(const FluidSimulation& simulation);
    std::vector<vec3> TraceStreamline(const vec3& startPos, const FluidSimulation& simulation) const;
    
    // Pressure mapping on mesh vertices
    void UpdateMeshPressureColors(MeshModel& meshModel, const FluidSimulation& simulation);
      // Utility functions
    vec3 ColorMapPressure(float pressure, float minPressure, float maxPressure) const;
    vec3 ColorMapVelocity(float velocityMagnitude, float maxVelocity) const;
    float GetMaxVelocity(const AerodynamicsGrid& grid) const;

private:    VisualizationSettings settings;
    BlurSettings blurSettings;
    
    // OpenGL resources for velocity vectors
    GLuint velocityVAO, velocityVBO;
    std::vector<vec3> velocityVertices;
    std::vector<float> velocityVertexData; // Interleaved position and velocity data (6 floats per vertex)
    ShaderProgram velocityShader;
    
    // OpenGL resources for pressure visualization
    GLuint pressureVAO, pressureVBO;
    std::vector<vec3> pressureVertices;
    std::vector<vec3> pressureColors;
    ShaderProgram pressureShader;
    
    // OpenGL resources for streamlines
    GLuint streamlineVAO, streamlineVBO;
    std::vector<vec3> streamlineVertices;
    std::vector<std::vector<vec3>> streamlines;
    ShaderProgram streamlineShader;
    
    // OpenGL resources for particles
    GLuint particleVAO, particleVBO;
    std::vector<FlowParticle> particles;
    ShaderProgram particleShader;
      // OpenGL resources for grid
    GLuint gridVAO, gridVBO;
    std::vector<vec3> gridVertices;
    ShaderProgram gridShader;
    
    // Post-processing
    PostProcessor postProcessor;
    bool postProcessingInitialized = false;
    bool pressureBlurEnabled = true;
    bool velocityBlurEnabled = true;
    float blurRadius = 1.5f;
    float blurStrength = 0.4f;
    int blurIterations = 2;
    
    // Helper methods
    void SetupVelocityBuffers();
    void SetupPressureBuffers();
    void SetupStreamlineBuffers();
    void SetupParticleBuffers();
    void SetupGridBuffers();
    
    void UpdateVelocityBuffers(const AerodynamicsGrid& grid);
    void UpdatePressureBuffers(const AerodynamicsGrid& grid);
    void UpdateStreamlineBuffers();
    void UpdateParticleBuffers();
    void UpdateGridBuffers(const AerodynamicsGrid& grid);
    
    // Shader loading
    void LoadShaders();
    
    // Initialization state
    bool isInitialized = false;
};
