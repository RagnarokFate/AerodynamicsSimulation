#pragma once
#include <glm/glm.hpp>
#include <memory>
#include "AerodynamicsGrid.h"
#include "MeshModel.h"

using namespace glm;

// Simulation parameters
struct SimulationParams {
    float viscosity = 0.01f;        // Fluid viscosity
    float density = 1.0f;           // Fluid density
    vec3 windVelocity = vec3(1.0f, 0.0f, 0.0f);  // External wind force
    float timeStep = 0.016f;        // Time step (60 FPS)
    float gravity = -9.81f;         // Gravity acceleration
    bool enableGravity = false;     // Whether to apply gravity
    int maxIterations = 10;         // Max pressure solver iterations
    float pressureTolerance = 0.001f; // Pressure solver convergence tolerance
    
    SimulationParams() = default;
};

// Fluid simulation engine using Eulerian grid-based approach
class FluidSimulation {
public:
    FluidSimulation();
    FluidSimulation(std::shared_ptr<AerodynamicsGrid> grid);
    ~FluidSimulation();
    
    // Initialize simulation
    void initialize(std::shared_ptr<AerodynamicsGrid> grid, const SimulationParams& params) { Initialize(grid, params); }
    void Initialize(std::shared_ptr<AerodynamicsGrid> grid, const SimulationParams& params);
    
    // Main simulation step
    void step(float deltaTime) { Step(deltaTime); }
    void Step(float deltaTime);
    
    // Simulation phases
    void Advection(float deltaTime);        // Advect velocity field
    void Diffusion(float deltaTime);        // Apply viscosity
    void AddForces(float deltaTime);        // Apply external forces (wind, gravity)
    void PressureProjection(float deltaTime); // Make velocity field divergence-free
    
    // Boundary conditions
    void ApplyBoundaryConditions();
    void ApplyNoSlipConditions();
    
    // Parameter control methods
    void setViscosity(float viscosity) { params.viscosity = viscosity; }
    void setDensity(float density) { params.density = density; }
    void setWindVelocity(const vec3& wind) { params.windVelocity = wind; }
    void applyForce(const vec3& force, float deltaTime) { /* Apply force to simulation */ }
    
    // Force calculation methods
    vec3 calculateForces(const MeshModel& meshModel) const;
    
    // Simulation control
    void Start() { isRunning = true; }
    void Pause() { isRunning = false; }
    void Reset();
    bool IsRunning() const { return isRunning; }
    
    // Parameter access
    void SetParameters(const SimulationParams& params) { this->params = params; }
    const SimulationParams& GetParameters() const { return params; }
    
    // Grid access
    std::shared_ptr<AerodynamicsGrid> GetGrid() const { return grid; }
    
    // Statistics
    float GetCurrentTime() const { return currentTime; }
    int GetStepCount() const { return stepCount; }
    
    // Force calculation for aerodynamics
    vec3 CalculateDragForce(const MeshModel& meshModel) const;
    vec3 CalculateLiftForce(const MeshModel& meshModel) const;
    float CalculatePressureAt(const vec3& position) const;

private:
    std::shared_ptr<AerodynamicsGrid> grid;
    SimulationParams params;
    
    // Simulation state
    bool isRunning = false;
    float currentTime = 0.0f;
    int stepCount = 0;
    
    // Temporary storage for simulation steps
    std::vector<vec3> tempVelocity;     // Temporary velocity field
    std::vector<float> tempPressure;    // Temporary pressure field
    std::vector<float> divergence;      // Velocity divergence
    
    // Helper methods
    void AllocateTemporaryArrays();
    void ComputeDivergence();
    void SolvePressure();
    void SubtractPressureGradient();
    
    // Advection schemes
    vec3 SampleVelocity(const vec3& position) const;
    vec3 BacktraceVelocity(const vec3& position, float deltaTime) const;
    
    // Numerical methods
    float ComputeLaplacian(int x, int y, int z, const std::vector<float>& field) const;
    vec3 ComputeGradient(int x, int y, int z, const std::vector<float>& field) const;
    
    // Validation
    bool IsValidForSimulation() const;
};
