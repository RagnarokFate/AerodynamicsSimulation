#include "FluidSimulation.h"
#include <algorithm>
#include <cmath>
#include <iostream>

FluidSimulation::FluidSimulation() {
}

FluidSimulation::FluidSimulation(std::shared_ptr<AerodynamicsGrid> grid) 
    : grid(grid) {
    AllocateTemporaryArrays();
}

FluidSimulation::~FluidSimulation() {
}

void FluidSimulation::Initialize(std::shared_ptr<AerodynamicsGrid> grid, const SimulationParams& params) {
    this->grid = grid;
    this->params = params;
    
    AllocateTemporaryArrays();
    
    Reset();
    
    std::cout << "FluidSimulation initialized" << std::endl;
}

void FluidSimulation::Step(float deltaTime) {
    if (!IsValidForSimulation()) {
        return;
    }
    
    stepCount++;
    currentTime += deltaTime;
    
    // Use simulation time step, not frame delta time
    float simDeltaTime = params.timeStep;
    
    // Main simulation phases
    AddForces(simDeltaTime);
    Advection(simDeltaTime);
    Diffusion(simDeltaTime);
    PressureProjection(simDeltaTime);
    ApplyBoundaryConditions();
}

void FluidSimulation::Advection(float deltaTime) {
    // Semi-Lagrangian advection for velocity field
    size_t totalVoxels = grid->GetTotalVoxels();
    tempVelocity.assign(totalVoxels, vec3(0.0f));
    
    ivec3 dimensions = grid->GetDimensions();
    
    for (int z = 0; z < dimensions.z; z++) {
        for (int y = 0; y < dimensions.y; y++) {
            for (int x = 0; x < dimensions.x; x++) {
                const Voxel& voxel = grid->GetVoxel(x, y, z);
                
                if (voxel.isSolid) {
                    tempVelocity[grid->GetIndex(x, y, z)] = vec3(0.0f);
                    continue;
                }
                
                // Backtrace to find where this velocity came from
                vec3 backtracePos = voxel.position - voxel.velocity * deltaTime;
                
                // Sample velocity at the backtraced position
                vec3 advectedVelocity = SampleVelocity(backtracePos);
                
                size_t index = grid->GetIndex(x, y, z);
                tempVelocity[index] = advectedVelocity;
            }
        }
    }
    
    // Copy back to grid
    for (int z = 0; z < dimensions.z; z++) {
        for (int y = 0; y < dimensions.y; y++) {
            for (int x = 0; x < dimensions.x; x++) {
                Voxel& voxel = grid->GetVoxel(x, y, z);
                if (!voxel.isSolid) {
                    voxel.velocity = tempVelocity[grid->GetIndex(x, y, z)];
                }
            }
        }
    }
}

void FluidSimulation::Diffusion(float deltaTime) {
    // Simplified explicit diffusion (for stability, should use implicit methods)
    size_t totalVoxels = grid->GetTotalVoxels();
    tempVelocity.assign(totalVoxels, vec3(0.0f));
    
    ivec3 dimensions = grid->GetDimensions();
    float diffusionRate = params.viscosity * deltaTime;
    
    for (int z = 1; z < dimensions.z - 1; z++) {
        for (int y = 1; y < dimensions.y - 1; y++) {
            for (int x = 1; x < dimensions.x - 1; x++) {
                const Voxel& voxel = grid->GetVoxel(x, y, z);
                
                if (voxel.isSolid) {
                    tempVelocity[grid->GetIndex(x, y, z)] = vec3(0.0f);
                    continue;
                }
                
                // Calculate Laplacian (6-point stencil)
                vec3 laplacian = 
                    grid->GetVoxel(x+1, y, z).velocity +
                    grid->GetVoxel(x-1, y, z).velocity +
                    grid->GetVoxel(x, y+1, z).velocity +
                    grid->GetVoxel(x, y-1, z).velocity +
                    grid->GetVoxel(x, y, z+1).velocity +
                    grid->GetVoxel(x, y, z-1).velocity -
                    6.0f * voxel.velocity;
                
                // Apply diffusion
                vec3 newVelocity = voxel.velocity + diffusionRate * laplacian;
                tempVelocity[grid->GetIndex(x, y, z)] = newVelocity;
            }
        }
    }
    
    // Copy back to grid
    for (int z = 1; z < dimensions.z - 1; z++) {
        for (int y = 1; y < dimensions.y - 1; y++) {
            for (int x = 1; x < dimensions.x - 1; x++) {
                Voxel& voxel = grid->GetVoxel(x, y, z);
                if (!voxel.isSolid) {
                    voxel.velocity = tempVelocity[grid->GetIndex(x, y, z)];
                }
            }
        }
    }
}

void FluidSimulation::AddForces(float deltaTime) {
    ivec3 dimensions = grid->GetDimensions();
    
    for (int z = 0; z < dimensions.z; z++) {
        for (int y = 0; y < dimensions.y; y++) {
            for (int x = 0; x < dimensions.x; x++) {
                Voxel& voxel = grid->GetVoxel(x, y, z);
                
                if (voxel.isSolid) {
                    continue;
                }
                
                // Add wind force
                voxel.velocity += params.windVelocity * deltaTime * 0.1f;
                
                // Add gravity
                if (params.enableGravity) {
                    voxel.velocity.y += params.gravity * deltaTime;
                }
            }
        }
    }
}

void FluidSimulation::PressureProjection(float deltaTime) {
    // Make velocity field divergence-free
    ComputeDivergence();
    SolvePressure();
    SubtractPressureGradient();
}

void FluidSimulation::ApplyBoundaryConditions() {
    grid->ApplyBoundaryConditions();
    grid->ApplyNoSlipBoundaries();
}

void FluidSimulation::ApplyNoSlipConditions() {
    grid->ApplyNoSlipBoundaries();
}

void FluidSimulation::Reset() {
    if (!grid) return;
    
    grid->Reset();
    currentTime = 0.0f;
    stepCount = 0;
    isRunning = false;
    
    AllocateTemporaryArrays();
    
    std::cout << "FluidSimulation reset" << std::endl;
}

vec3 FluidSimulation::CalculateDragForce(const MeshModel& meshModel) const {
    // Simplified drag calculation based on pressure and velocity around the mesh
    vec3 totalDragForce(0.0f);
    
    if (!grid) return totalDragForce;
    
    ivec3 dimensions = grid->GetDimensions();
    
    // Sum pressure forces on solid boundaries
    for (int z = 1; z < dimensions.z - 1; z++) {
        for (int y = 1; y < dimensions.y - 1; y++) {
            for (int x = 1; x < dimensions.x - 1; x++) {
                const Voxel& voxel = grid->GetVoxel(x, y, z);
                
                if (voxel.isSolid) {
                    // Check neighboring fluid voxels
                    if (!grid->GetVoxel(x+1, y, z).isSolid) {
                        float pressureDiff = grid->GetVoxel(x+1, y, z).pressure - voxel.pressure;
                        totalDragForce.x += pressureDiff;
                    }
                    if (!grid->GetVoxel(x-1, y, z).isSolid) {
                        float pressureDiff = voxel.pressure - grid->GetVoxel(x-1, y, z).pressure;
                        totalDragForce.x += pressureDiff;
                    }
                }
            }
        }
    }
    
    return totalDragForce * grid->GetVoxelSize() * grid->GetVoxelSize();
}

vec3 FluidSimulation::CalculateLiftForce(const MeshModel& meshModel) const {
    // Simplified lift calculation
    vec3 totalLiftForce(0.0f);
    
    if (!grid) return totalLiftForce;
    
    ivec3 dimensions = grid->GetDimensions();
    
    // Calculate lift based on pressure differences above and below the object
    for (int z = 1; z < dimensions.z - 1; z++) {
        for (int y = 1; y < dimensions.y - 1; y++) {
            for (int x = 1; x < dimensions.x - 1; x++) {
                const Voxel& voxel = grid->GetVoxel(x, y, z);
                
                if (voxel.isSolid) {
                    // Check neighboring fluid voxels above and below
                    if (!grid->GetVoxel(x, y+1, z).isSolid) {
                        float pressureDiff = grid->GetVoxel(x, y+1, z).pressure - voxel.pressure;
                        totalLiftForce.y += pressureDiff;
                    }
                    if (!grid->GetVoxel(x, y-1, z).isSolid) {
                        float pressureDiff = voxel.pressure - grid->GetVoxel(x, y-1, z).pressure;
                        totalLiftForce.y += pressureDiff;
                    }
                }
            }
        }
    }
    
    return totalLiftForce * grid->GetVoxelSize() * grid->GetVoxelSize();
}

float FluidSimulation::CalculatePressureAt(const vec3& position) const {
    if (!grid || !grid->IsInsideBounds(position)) {
        return 0.0f;
    }
    
    ivec3 gridPos = grid->WorldToGrid(position);
    const Voxel& voxel = grid->GetVoxel(gridPos.x, gridPos.y, gridPos.z);
    return voxel.pressure;
}

void FluidSimulation::AllocateTemporaryArrays() {
    if (!grid) return;
    
    size_t totalVoxels = grid->GetTotalVoxels();
    tempVelocity.resize(totalVoxels);
    tempPressure.resize(totalVoxels);
    divergence.resize(totalVoxels);
    
    // Initialize to zero
    std::fill(tempVelocity.begin(), tempVelocity.end(), vec3(0.0f));
    std::fill(tempPressure.begin(), tempPressure.end(), 0.0f);
    std::fill(divergence.begin(), divergence.end(), 0.0f);
}

void FluidSimulation::ComputeDivergence() {
    ivec3 dimensions = grid->GetDimensions();
    
    for (int z = 1; z < dimensions.z - 1; z++) {
        for (int y = 1; y < dimensions.y - 1; y++) {
            for (int x = 1; x < dimensions.x - 1; x++) {
                const Voxel& voxel = grid->GetVoxel(x, y, z);
                
                if (voxel.isSolid) {
                    divergence[grid->GetIndex(x, y, z)] = 0.0f;
                    continue;
                }
                
                // Central difference for divergence
                float div = 
                    (grid->GetVoxel(x+1, y, z).velocity.x - grid->GetVoxel(x-1, y, z).velocity.x) +
                    (grid->GetVoxel(x, y+1, z).velocity.y - grid->GetVoxel(x, y-1, z).velocity.y) +
                    (grid->GetVoxel(x, y, z+1).velocity.z - grid->GetVoxel(x, y, z-1).velocity.z);
                
                div *= 0.5f / grid->GetVoxelSize();
                divergence[grid->GetIndex(x, y, z)] = div;
            }
        }
    }
}

void FluidSimulation::SolvePressure() {
    // Gauss-Seidel iteration to solve pressure Poisson equation
    ivec3 dimensions = grid->GetDimensions();
    
    // Initialize pressure to zero
    for (int z = 0; z < dimensions.z; z++) {
        for (int y = 0; y < dimensions.y; y++) {
            for (int x = 0; x < dimensions.x; x++) {
                Voxel& voxel = grid->GetVoxel(x, y, z);
                voxel.pressure = 0.0f;
            }
        }
    }
    
    // Iterative solver
    for (int iter = 0; iter < params.maxIterations; iter++) {
        float maxChange = 0.0f;
        
        for (int z = 1; z < dimensions.z - 1; z++) {
            for (int y = 1; y < dimensions.y - 1; y++) {
                for (int x = 1; x < dimensions.x - 1; x++) {
                    Voxel& voxel = grid->GetVoxel(x, y, z);
                    
                    if (voxel.isSolid) {
                        continue;
                    }
                    
                    float oldPressure = voxel.pressure;
                    
                    // Solve: ∇²p = ρ∇·v / Δt
                    float neighborSum = 
                        grid->GetVoxel(x+1, y, z).pressure +
                        grid->GetVoxel(x-1, y, z).pressure +
                        grid->GetVoxel(x, y+1, z).pressure +
                        grid->GetVoxel(x, y-1, z).pressure +
                        grid->GetVoxel(x, y, z+1).pressure +
                        grid->GetVoxel(x, y, z-1).pressure;
                    
                    float rhs = -params.density * divergence[grid->GetIndex(x, y, z)] / params.timeStep;
                    float newPressure = (neighborSum + rhs * grid->GetVoxelSize() * grid->GetVoxelSize()) / 6.0f;
                    
                    voxel.pressure = newPressure;
                    maxChange = std::max(maxChange, std::abs(newPressure - oldPressure));
                }
            }
        }
        
        // Check for convergence
        if (maxChange < params.pressureTolerance) {
            break;
        }
    }
}

void FluidSimulation::SubtractPressureGradient() {
    ivec3 dimensions = grid->GetDimensions();
    
    for (int z = 1; z < dimensions.z - 1; z++) {
        for (int y = 1; y < dimensions.y - 1; y++) {
            for (int x = 1; x < dimensions.x - 1; x++) {
                Voxel& voxel = grid->GetVoxel(x, y, z);
                
                if (voxel.isSolid) {
                    continue;
                }
                
                // Compute pressure gradient
                vec3 pressureGradient;
                pressureGradient.x = (grid->GetVoxel(x+1, y, z).pressure - grid->GetVoxel(x-1, y, z).pressure) / (2.0f * grid->GetVoxelSize());
                pressureGradient.y = (grid->GetVoxel(x, y+1, z).pressure - grid->GetVoxel(x, y-1, z).pressure) / (2.0f * grid->GetVoxelSize());
                pressureGradient.z = (grid->GetVoxel(x, y, z+1).pressure - grid->GetVoxel(x, y, z-1).pressure) / (2.0f * grid->GetVoxelSize());
                
                // Subtract gradient to make velocity field divergence-free
                voxel.velocity -= (params.timeStep / params.density) * pressureGradient;
            }
        }
    }
}

vec3 FluidSimulation::SampleVelocity(const vec3& position) const {
    if (!grid->IsInsideBounds(position)) {
        return vec3(0.0f);
    }
    
    // Simple nearest neighbor sampling (can be improved with trilinear interpolation)
    ivec3 gridPos = grid->WorldToGrid(position);
    const Voxel& voxel = grid->GetVoxel(gridPos.x, gridPos.y, gridPos.z);
    return voxel.velocity;
}

vec3 FluidSimulation::BacktraceVelocity(const vec3& position, float deltaTime) const {
    return position - SampleVelocity(position) * deltaTime;
}

float FluidSimulation::ComputeLaplacian(int x, int y, int z, const std::vector<float>& field) const {
    if (!grid) return 0.0f;
    
    ivec3 dimensions = grid->GetDimensions();
    
    if (x <= 0 || x >= dimensions.x - 1 ||
        y <= 0 || y >= dimensions.y - 1 ||
        z <= 0 || z >= dimensions.z - 1) {
        return 0.0f;
    }
    
    size_t centerIndex = grid->GetIndex(x, y, z);
    size_t rightIndex = grid->GetIndex(x+1, y, z);
    size_t leftIndex = grid->GetIndex(x-1, y, z);
    size_t upIndex = grid->GetIndex(x, y+1, z);
    size_t downIndex = grid->GetIndex(x, y-1, z);
    size_t frontIndex = grid->GetIndex(x, y, z+1);
    size_t backIndex = grid->GetIndex(x, y, z-1);
    
    float laplacian = field[rightIndex] + field[leftIndex] + 
                     field[upIndex] + field[downIndex] + 
                     field[frontIndex] + field[backIndex] - 
                     6.0f * field[centerIndex];
    
    float voxelSize = grid->GetVoxelSize();
    return laplacian / (voxelSize * voxelSize);
}

vec3 FluidSimulation::ComputeGradient(int x, int y, int z, const std::vector<float>& field) const {
    if (!grid) return vec3(0.0f);
    
    ivec3 dimensions = grid->GetDimensions();
    
    if (x <= 0 || x >= dimensions.x - 1 ||
        y <= 0 || y >= dimensions.y - 1 ||
        z <= 0 || z >= dimensions.z - 1) {
        return vec3(0.0f);
    }
    
    vec3 gradient;
    float voxelSize = grid->GetVoxelSize();
    
    gradient.x = (field[grid->GetIndex(x+1, y, z)] - field[grid->GetIndex(x-1, y, z)]) / (2.0f * voxelSize);
    gradient.y = (field[grid->GetIndex(x, y+1, z)] - field[grid->GetIndex(x, y-1, z)]) / (2.0f * voxelSize);
    gradient.z = (field[grid->GetIndex(x, y, z+1)] - field[grid->GetIndex(x, y, z-1)]) / (2.0f * voxelSize);
    
    return gradient;
}

vec3 FluidSimulation::calculateForces(const MeshModel& meshModel) const {
    // This is a simplified version - combine drag and lift into total force
    vec3 dragForce = CalculateDragForce(meshModel);
    vec3 liftForce = CalculateLiftForce(meshModel);
    return dragForce + liftForce;
}

bool FluidSimulation::IsValidForSimulation() const {
    return grid != nullptr && grid->GetTotalVoxels() > 0;
}
