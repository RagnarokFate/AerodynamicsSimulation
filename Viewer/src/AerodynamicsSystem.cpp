#include "AerodynamicsSystem.h"
#include <iostream>
#include <algorithm>
#include <chrono>

AerodynamicsSystem::AerodynamicsSystem()
    : m_initialized(false)
    , m_simulationRunning(false)
    , m_lastUpdateTime(0.0f)
{    // Initialize with default parameters (optimized for performance)
    m_params.gridResolution = 32;  // Reduced from 64 for better performance
    m_params.timeStep = 0.033f;    // ~30 FPS target for stability
    m_params.viscosity = 0.01f;
    m_params.density = 1.0f;
    m_params.windVelocity = glm::vec3(10.0f, 0.0f, 0.0f);
    m_params.gravity = glm::vec3(0.0f, -9.81f, 0.0f);

    m_visualizationParams.showVelocityVectors = true;
    m_visualizationParams.showPressureField = false;
    m_visualizationParams.showStreamlines = false;
    m_visualizationParams.showParticles = false;
    m_visualizationParams.showGrid = false;
    m_visualizationParams.velocityScale = 0.1f;
    m_visualizationParams.particleCount = 1000;
    m_visualizationParams.streamlineCount = 50;
    m_visualizationParams.maxStreamlineLength = 100;
}

AerodynamicsSystem::~AerodynamicsSystem()
{
    cleanup();
}

bool AerodynamicsSystem::initialize(std::shared_ptr<MeshModel> meshModel)
{
    if (!meshModel) {
        std::cerr << "AerodynamicsSystem: Invalid mesh model" << std::endl;
        return false;
    }

    m_meshModel = meshModel;    try {        
        // Initialize aerodynamics grid with performance optimizations
        m_grid = std::make_shared<AerodynamicsGrid>();
        
        // Calculate adaptive voxel size for performance
        float voxelSize = calculateOptimalVoxelSize(*meshModel);
        m_grid->InitializeFromMesh(*meshModel, voxelSize);
        
        // Log grid statistics for performance monitoring
        auto gridDim = m_grid->GetDimensions();
        int totalVoxels = gridDim.x * gridDim.y * gridDim.z;
        std::cout << "AerodynamicsSystem: Grid dimensions: " << gridDim.x << "x" << gridDim.y << "x" << gridDim.z 
                  << " (Total: " << totalVoxels << " voxels)" << std::endl;

        // Initialize fluid simulation
        m_fluidSim = std::make_shared<FluidSimulation>();
        SimulationParams simParams;
        simParams.viscosity = m_params.viscosity;
        simParams.density = m_params.density;
        simParams.windVelocity = m_params.windVelocity;
        simParams.timeStep = m_params.timeStep;
        m_fluidSim->Initialize(m_grid, simParams);        // Initialize visualizer
        m_visualizer = std::make_shared<AerodynamicsVisualizer>();
        m_visualizer->Initialize();
        
        // Note: Post-processing will be initialized later from main.cpp when viewport dimensions are available

        // Apply initial conditions
        applyInitialConditions();

        m_initialized = true;
        std::cout << "AerodynamicsSystem: Successfully initialized with grid resolution " 
                  << m_params.gridResolution << "^3" << std::endl;
        
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "AerodynamicsSystem: Exception during initialization: " << e.what() << std::endl;
        cleanup();
        return false;
    }
}

void AerodynamicsSystem::update(float deltaTime)
{
    if (!m_initialized || !m_simulationRunning) {
        return;
    }

    // Performance optimization: Frame skipping for heavy computations
    static float accumulatedTime = 0.0f;
    static int frameSkipCounter = 0;
    const int SIMULATION_SKIP_FRAMES = 3; // Only update simulation every 3 frames
    const float MAX_TIME_STEP = 0.05f; // Cap time step to prevent instability
    
    accumulatedTime += deltaTime;
    frameSkipCounter++;

    try {
        // Only update fluid simulation periodically for performance
        if (frameSkipCounter >= SIMULATION_SKIP_FRAMES) {
            float clampedTimeStep = std::min(accumulatedTime, MAX_TIME_STEP);
            m_fluidSim->step(clampedTimeStep);
            
            frameSkipCounter = 0;
            accumulatedTime = 0.0f;
        }

        // Apply external forces
        applyExternalForces();

        // Update visualization data
        updateVisualizationData();

        m_lastUpdateTime += deltaTime;
    }
    catch (const std::exception& e) {
        std::cerr << "AerodynamicsSystem: Exception during update: " << e.what() << std::endl;
        m_simulationRunning = false;
    }
}

void AerodynamicsSystem::render(const glm::mat4& view, const glm::mat4& projection)
{
    if (!m_initialized || !m_visualizer) {
        return;
    }

    glm::mat4 model = glm::mat4(1.0f);

    try {        // Render velocity vectors
        if (m_visualizationParams.showVelocityVectors) {
            m_visualizer->renderVelocityVectors(*m_grid, view, projection);
        }

        // Render pressure field
        if (m_visualizationParams.showPressureField) {
            m_visualizer->renderPressureField(*m_grid, view, projection);
        }

        // Render streamlines
        if (m_visualizationParams.showStreamlines) {
            m_visualizer->renderStreamlines(*m_fluidSim, view, projection);
        }

        // Render particles
        if (m_visualizationParams.showParticles) {
            m_visualizer->renderParticles(*m_fluidSim, view, projection);
        }

        // Render grid wireframe
        if (m_visualizationParams.showGrid) {
            m_visualizer->renderGrid(*m_grid, view, projection);
        }
    }
    catch (const std::exception& e) {
        std::cerr << "AerodynamicsSystem: Exception during rendering: " << e.what() << std::endl;
    }
}

void AerodynamicsSystem::startSimulation()
{
    if (!m_initialized) {
        std::cerr << "AerodynamicsSystem: Cannot start simulation - system not initialized" << std::endl;
        return;
    }

    m_simulationRunning = true;
    std::cout << "AerodynamicsSystem: Simulation started" << std::endl;
}

void AerodynamicsSystem::stopSimulation()
{
    m_simulationRunning = false;
    std::cout << "AerodynamicsSystem: Simulation stopped" << std::endl;
}

void AerodynamicsSystem::resetSimulation()
{
    if (!m_initialized) {
        return;
    }

    m_simulationRunning = false;
      // Reset grid and simulation state
    if (m_grid && m_fluidSim) {
        m_grid->Reset();
        applyInitialConditions();
        updateVisualizationData();
    }

    m_lastUpdateTime = 0.0f;
    std::cout << "AerodynamicsSystem: Simulation reset" << std::endl;
}

void AerodynamicsSystem::updateParameters(const SimulationParameters& params)
{
    m_params = params;
    
    if (m_initialized && m_fluidSim) {
        // Update fluid simulation parameters
        m_fluidSim->setViscosity(m_params.viscosity);
        m_fluidSim->setDensity(m_params.density);
          // If grid resolution changed, reinitialize
        if (m_grid) {
            auto currentRes = m_grid->getResolution();
            if (currentRes.x != m_params.gridResolution || 
                currentRes.y != m_params.gridResolution || 
                currentRes.z != m_params.gridResolution) {
                reinitialize();
            }
        }
    }
}

void AerodynamicsSystem::updateVisualizationParameters(const VisualizationParameters& params)
{
    m_visualizationParams = params;
    
    if (m_initialized && m_visualizer) {
        // Update particle count if changed
        if (params.particleCount != m_visualizationParams.particleCount) {
            m_visualizer->setParticleCount(params.particleCount);
            m_visualizationParams.particleCount = params.particleCount;
        }
        
        // Update streamline parameters
        m_visualizer->setStreamlineParameters(params.streamlineCount, params.maxStreamlineLength);
        
        // Force update of visualization data to apply changes immediately
        updateVisualizationData();
    }
}

SimulationStatistics AerodynamicsSystem::getStatistics() const
{
    SimulationStatistics stats = {};
    static auto lastTime = std::chrono::high_resolution_clock::now();
    static float frameCounter = 0;
    static float accumulatedFrameTime = 0.0f;
    
    // Update frame rate calculation
    auto currentTime = std::chrono::high_resolution_clock::now();
    float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
    lastTime = currentTime;
    
    frameCounter++;
    accumulatedFrameTime += deltaTime;
    
    // Update frame rate every second
    if (accumulatedFrameTime >= 1.0f) {
        stats.frameRate = frameCounter / accumulatedFrameTime;
        frameCounter = 0;
        accumulatedFrameTime = 0.0f;
    }
    
    if (!m_initialized || !m_fluidSim) {
        return stats;
    }
    
    // Calculate forces on the mesh
    if (m_meshModel) {
        stats.dragForce = m_fluidSim->CalculateDragForce(*m_meshModel);
        stats.liftForce = m_fluidSim->CalculateLiftForce(*m_meshModel);
    }
    
    // Get simulation metrics
    stats.averageVelocity = calculateAverageVelocity();
    stats.maxVelocity = calculateMaxVelocity();
    stats.averagePressure = calculateAveragePressure();
    stats.simulationTime = m_lastUpdateTime;
    stats.isRunning = m_simulationRunning;
    stats.gridResolution = m_grid ? m_grid->getResolution().x : 0;
    
    // Performance statistics
    if (m_grid) {
        auto resolution = m_grid->getResolution();
        stats.totalVoxels = resolution.x * resolution.y * resolution.z;
        
        int fluidVoxels = 0;
        for (int i = 0; i < resolution.x; ++i) {
            for (int j = 0; j < resolution.y; ++j) {
                for (int k = 0; k < resolution.z; ++k) {
                    if (!m_grid->GetVoxel(i, j, k).isSolid) {
                        fluidVoxels++;
                    }
                }
            }
        }
        stats.activeVoxels = fluidVoxels;
    }
    
    return stats;
}

void AerodynamicsSystem::applyInitialConditions()
{
    if (!m_grid || !m_fluidSim) {
        return;
    }

    // Set initial wind velocity
    m_fluidSim->setWindVelocity(m_params.windVelocity);
    
    // Apply boundary conditions
    m_grid->applyBoundaryConditions();
    
    std::cout << "AerodynamicsSystem: Initial conditions applied" << std::endl;
}

void AerodynamicsSystem::applyExternalForces()
{
    if (!m_fluidSim) {
        return;
    }    // Apply gravity
    m_fluidSim->applyForce(m_params.gravity, m_params.timeStep);
    
    // Apply wind force
    m_fluidSim->setWindVelocity(m_params.windVelocity);
}

void AerodynamicsSystem::updateVisualizationData()
{
    if (!m_visualizer || !m_grid) {
        return;
    }    // Update velocity vectors
    if (m_visualizationParams.showVelocityVectors) {
        m_visualizer->updateVelocityData(*m_grid);
    }

    // Update pressure field
    if (m_visualizationParams.showPressureField) {
        m_visualizer->updatePressureData(*m_grid);
    }

    // Update streamlines
    if (m_visualizationParams.showStreamlines) {
        m_visualizer->updateStreamlines(*m_fluidSim);
    }

    // Update particles
    if (m_visualizationParams.showParticles) {
        m_visualizer->updateParticles(*m_fluidSim, m_params.timeStep);
    }
}

float AerodynamicsSystem::calculateAverageVelocity() const
{
    if (!m_grid) {
        return 0.0f;
    }    float totalVelocity = 0.0f;
    int fluidVoxelCount = 0;
    auto resolution = m_grid->getResolution();

    for (int i = 0; i < resolution.x; ++i) {
        for (int j = 0; j < resolution.y; ++j) {
            for (int k = 0; k < resolution.z; ++k) {
                const auto& voxel = m_grid->GetVoxel(i, j, k);
                if (!voxel.isSolid) {
                    totalVelocity += glm::length(voxel.velocity);
                    fluidVoxelCount++;
                }
            }
        }
    }

    return fluidVoxelCount > 0 ? totalVelocity / fluidVoxelCount : 0.0f;
}

float AerodynamicsSystem::calculateMaxVelocity() const
{
    if (!m_grid) {
        return 0.0f;
    }    float maxVelocity = 0.0f;
    auto resolution = m_grid->getResolution();

    for (int i = 0; i < resolution.x; ++i) {
        for (int j = 0; j < resolution.y; ++j) {
            for (int k = 0; k < resolution.z; ++k) {
                const auto& voxel = m_grid->GetVoxel(i, j, k);
                if (!voxel.isSolid) {
                    maxVelocity = std::max(maxVelocity, glm::length(voxel.velocity));
                }
            }
        }
    }

    return maxVelocity;
}

float AerodynamicsSystem::calculateAveragePressure() const
{
    if (!m_grid) {
        return 0.0f;
    }    float totalPressure = 0.0f;
    int fluidVoxelCount = 0;
    auto resolution = m_grid->getResolution();

    for (int i = 0; i < resolution.x; ++i) {
        for (int j = 0; j < resolution.y; ++j) {
            for (int k = 0; k < resolution.z; ++k) {
                const auto& voxel = m_grid->GetVoxel(i, j, k);
                if (!voxel.isSolid) {
                    totalPressure += voxel.pressure;
                    fluidVoxelCount++;
                }
            }
        }
    }

    return fluidVoxelCount > 0 ? totalPressure / fluidVoxelCount : 0.0f;
}

void AerodynamicsSystem::reinitialize()
{
    if (!m_meshModel) {
        return;
    }

    cleanup();
    initialize(m_meshModel);
}

void AerodynamicsSystem::cleanup()
{
    m_simulationRunning = false;
    m_initialized = false;
    
    m_grid.reset();
    m_fluidSim.reset();
    m_visualizer.reset();
}

float AerodynamicsSystem::calculateOptimalVoxelSize(const MeshModel& meshModel) const
{
    // Use mesh bounding box from MeshModel
    glm::vec3 minBounds = meshModel.MinPoints;
    glm::vec3 maxBounds = meshModel.MaxPoints;
    
    // Calculate mesh dimensions
    glm::vec3 meshSize = maxBounds - minBounds;
    float maxDimension = std::max({meshSize.x, meshSize.y, meshSize.z});
    
    // Adaptive voxel size based on mesh complexity and target performance
    // Target: ~100K-200K total voxels for good performance
    float targetVoxels = 150000.0f; // Sweet spot for performance
    int targetResolution = static_cast<int>(std::cbrt(targetVoxels)); // Cube root for 3D grid
    
    // Clamp resolution to reasonable bounds for performance
    targetResolution = std::max(24, std::min(targetResolution, 48));
    
    float voxelSize = maxDimension / targetResolution;
    
    std::cout << "AerodynamicsSystem: Adaptive voxel size: " << voxelSize 
              << " (target resolution: ~" << targetResolution << ")" << std::endl;
    
    return voxelSize;
}
