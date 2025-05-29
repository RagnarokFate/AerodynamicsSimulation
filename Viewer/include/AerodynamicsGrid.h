#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include "MeshModel.h"

using namespace glm;

// Represents a single voxel in the simulation grid
struct Voxel {
    vec3 velocity;      // Fluid velocity at this voxel
    float pressure;     // Pressure value
    float density;      // Fluid density
    bool isSolid;       // True if this voxel is inside the mesh (solid), false if fluid
    vec3 position;      // World position of voxel center
    
    Voxel() : velocity(0.0f), pressure(0.0f), density(1.0f), isSolid(false), position(0.0f) {}
};

// 3D grid for aerodynamic simulation
class AerodynamicsGrid {
public:
    AerodynamicsGrid();
    AerodynamicsGrid(const vec3& minBounds, const vec3& maxBounds, float voxelSize);
    ~AerodynamicsGrid();
    
    // Core initialization methods
    void initialize(const vec3& minBounds, const vec3& maxBounds, float voxelSize);
    void InitializeFromMesh(const MeshModel& meshModel, float voxelSize);
    
    // Voxelization - mark voxels as solid/fluid based on mesh
    void VoxelizeMesh(const MeshModel& meshModel);
    
    // Grid access
    Voxel& GetVoxel(int x, int y, int z);
    const Voxel& GetVoxel(int x, int y, int z) const;
    Voxel& GetVoxelAtPosition(const vec3& worldPos);
    
    // Grid properties
    ivec3 GetDimensions() const { return gridDimensions; }
    ivec3 getResolution() const { return gridDimensions; }
    vec3 GetMinBounds() const { return minBounds; }
    vec3 GetMaxBounds() const { return maxBounds; }
    float GetVoxelSize() const { return voxelSize; }
    vec3 GetGridCenter() const { return (minBounds + maxBounds) * 0.5f; }
    
    // Coordinate conversions
    vec3 GridToWorld(int x, int y, int z) const;
    ivec3 WorldToGrid(const vec3& worldPos) const;
    
    // Boundary conditions
    void ApplyBoundaryConditions();
    void applyBoundaryConditions() { ApplyBoundaryConditions(); }
    void ApplyNoSlipBoundaries();
    
    // Validation
    bool IsValidGridCoordinate(int x, int y, int z) const;
    bool IsInsideBounds(const vec3& worldPos) const;
    
    // Reset grid to initial state
    void Reset();
    
    // Get total number of voxels
    size_t GetTotalVoxels() const { return voxels.size(); }
      // Get voxel data for visualization
    const std::vector<Voxel>& GetVoxelData() const { return voxels; }
    
    // Helper functions (public for fluid simulation access)
    size_t GetIndex(int x, int y, int z) const;

private:
    std::vector<Voxel> voxels;      // 3D grid stored as 1D array
    ivec3 gridDimensions;           // Number of voxels in each dimension
    vec3 minBounds, maxBounds;      // World space bounds
    float voxelSize;                // Size of each voxel
    
    // Helper functions
    bool IsPointInsideMesh(const vec3& point, const MeshModel& meshModel) const;
    void InitializeVoxelPositions();
};
