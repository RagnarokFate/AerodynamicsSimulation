#pragma once
#include <glm/glm.hpp>
#include <memory>
#include "AerodynamicsGrid.h"
#include "FluidSimulation.h" 
#include "AerodynamicsVisualizer.h"
#include "MeshModel.h"

// Extended simulation parameters with aerodynamics-specific settings
struct SimulationParameters {
    int gridResolution = 64;
    float timeStep = 0.016f;  // ~60 FPS
    float viscosity = 0.01f;
    float density = 1.0f;
    glm::vec3 windVelocity = glm::vec3(10.0f, 0.0f, 0.0f);
    glm::vec3 gravity = glm::vec3(0.0f, -9.81f, 0.0f);
};

// Extended visualization parameters 
struct VisualizationParameters {
    bool showVelocityVectors = true;
    bool showPressureField = false;
    bool showStreamlines = false;
    bool showParticles = false;
    bool showGrid = false;
    float velocityScale = 0.1f;
    int particleCount = 1000;
    int streamlineCount = 50;
    int maxStreamlineLength = 100;
};

// Statistics for simulation analysis
struct SimulationStatistics {
    bool isRunning = false;
    int gridResolution = 0;
    glm::vec3 dragForce = glm::vec3(0.0f);
    glm::vec3 liftForce = glm::vec3(0.0f);
    float averageVelocity = 0.0f;
    float maxVelocity = 0.0f;
    float averagePressure = 0.0f;
    float minPressure = 0.0f;
    float maxPressure = 0.0f;
    int activeVoxels = 0;
    float simulationTime = 0.0f;
    
    // Performance metrics
    int totalVoxels = 0;
    float frameRate = 0.0f;
    float updateTime = 0.0f;
};

// Main aerodynamics system that coordinates all components
class AerodynamicsSystem {
public:
    AerodynamicsSystem();
    ~AerodynamicsSystem();
    
    // System initialization
    bool initialize(std::shared_ptr<MeshModel> meshModel);
    void cleanup();
    
    // Simulation control
    void startSimulation();
    void stopSimulation();
    void resetSimulation();
    void update(float deltaTime);
    void reinitialize();
    
    // Rendering
    void render(const glm::mat4& view, const glm::mat4& projection);
    
    // Parameter management
    void updateParameters(const SimulationParameters& params);
    void updateVisualizationParameters(const VisualizationParameters& params);
    
    // Statistics and analysis
    SimulationStatistics getStatistics() const;
    
    // System state
    bool isInitialized() const { return m_initialized; }
    bool isSimulationRunning() const { return m_simulationRunning; }
    
    // Parameter access
    const SimulationParameters& getParameters() const { return m_params; }
    const VisualizationParameters& getVisualizationParameters() const { return m_visualizationParams; }
    
    // Component access
    std::shared_ptr<AerodynamicsGrid> getGrid() const { return m_grid; }
    std::shared_ptr<FluidSimulation> getFluidSimulation() const { return m_fluidSim; }
    std::shared_ptr<AerodynamicsVisualizer> getVisualizer() const { return m_visualizer; }
    std::shared_ptr<MeshModel> getMeshModel() const { return m_meshModel; }

private:
    // Core components
    std::shared_ptr<AerodynamicsGrid> m_grid;
    std::shared_ptr<FluidSimulation> m_fluidSim;
    std::shared_ptr<AerodynamicsVisualizer> m_visualizer;
    std::shared_ptr<MeshModel> m_meshModel;
    
    // System state
    bool m_initialized;
    bool m_simulationRunning;
    float m_lastUpdateTime;
    
    // Parameters
    SimulationParameters m_params;
    VisualizationParameters m_visualizationParams;
    
    // Helper methods
    void applyInitialConditions();
    void applyExternalForces();
    void updateVisualizationData();
    float calculateAverageVelocity() const;
    float calculateMaxVelocity() const; 
    float calculateAveragePressure() const;
    
    // Performance optimization methods
    float calculateOptimalVoxelSize(const MeshModel& meshModel) const;
};
