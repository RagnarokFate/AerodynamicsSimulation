#include "AerodynamicsGrid.h"
#include <algorithm>
#include <cmath>
#include <iostream>

AerodynamicsGrid::AerodynamicsGrid() 
    : gridDimensions(0), minBounds(0.0f), maxBounds(0.0f), voxelSize(0.1f) {
}

AerodynamicsGrid::AerodynamicsGrid(const vec3& minBounds, const vec3& maxBounds, float voxelSize)
    : minBounds(minBounds), maxBounds(maxBounds), voxelSize(voxelSize) {
    
    // Calculate grid dimensions based on bounds and voxel size
    vec3 size = maxBounds - minBounds;
    gridDimensions.x = static_cast<int>(std::ceil(size.x / voxelSize));
    gridDimensions.y = static_cast<int>(std::ceil(size.y / voxelSize));
    gridDimensions.z = static_cast<int>(std::ceil(size.z / voxelSize));
    
    // Ensure minimum grid size
    gridDimensions = max(gridDimensions, ivec3(1));
    
    // Allocate voxel array
    size_t totalVoxels = gridDimensions.x * gridDimensions.y * gridDimensions.z;
    voxels.resize(totalVoxels);
    
    // Initialize voxel positions
    InitializeVoxelPositions();
    
    std::cout << "AerodynamicsGrid initialized: " << gridDimensions.x << "x" 
              << gridDimensions.y << "x" << gridDimensions.z << " voxels" << std::endl;
}

void AerodynamicsGrid::initialize(const vec3& minBounds, const vec3& maxBounds, float voxelSize) {
    this->minBounds = minBounds;
    this->maxBounds = maxBounds;
    this->voxelSize = voxelSize;
    
    // Calculate grid dimensions based on bounds and voxel size
    vec3 size = maxBounds - minBounds;
    gridDimensions.x = static_cast<int>(std::ceil(size.x / voxelSize));
    gridDimensions.y = static_cast<int>(std::ceil(size.y / voxelSize));
    gridDimensions.z = static_cast<int>(std::ceil(size.z / voxelSize));
    
    // Ensure minimum grid size
    gridDimensions = max(gridDimensions, ivec3(1));
    
    // Allocate voxel array
    size_t totalVoxels = gridDimensions.x * gridDimensions.y * gridDimensions.z;
    voxels.resize(totalVoxels);
    
    // Initialize voxel positions
    InitializeVoxelPositions();
    
    std::cout << "AerodynamicsGrid initialized: " << gridDimensions.x << "x" 
              << gridDimensions.y << "x" << gridDimensions.z << " voxels" << std::endl;
}

AerodynamicsGrid::~AerodynamicsGrid() {
}

void AerodynamicsGrid::InitializeFromMesh(const MeshModel& meshModel, float voxelSize) {
    this->voxelSize = voxelSize;
      // Get mesh bounding box (these are public members calculated in MeshModel constructor)
    minBounds = meshModel.MinPoints;
    maxBounds = meshModel.MaxPoints;
    
    // Expand bounds slightly to ensure mesh is completely contained
    vec3 expansion = vec3(voxelSize * 2.0f);
    minBounds -= expansion;
    maxBounds += expansion;
    
    // Calculate grid dimensions
    vec3 size = maxBounds - minBounds;
    gridDimensions.x = static_cast<int>(std::ceil(size.x / voxelSize));
    gridDimensions.y = static_cast<int>(std::ceil(size.y / voxelSize));
    gridDimensions.z = static_cast<int>(std::ceil(size.z / voxelSize));
    
    // Ensure minimum grid size
    gridDimensions = max(gridDimensions, ivec3(1));
    
    // Allocate and initialize voxels
    size_t totalVoxels = gridDimensions.x * gridDimensions.y * gridDimensions.z;
    voxels.resize(totalVoxels);
    
    InitializeVoxelPositions();
    
    // Perform voxelization
    VoxelizeMesh(meshModel);
    
    std::cout << "Grid initialized from mesh. Dimensions: " << gridDimensions.x 
              << "x" << gridDimensions.y << "x" << gridDimensions.z 
              << " (Total: " << totalVoxels << " voxels)" << std::endl;
}

void AerodynamicsGrid::VoxelizeMesh(const MeshModel& meshModel) {
    int solidCount = 0;
    
    // For each voxel, determine if it's inside the mesh
    for (int z = 0; z < gridDimensions.z; z++) {
        for (int y = 0; y < gridDimensions.y; y++) {
            for (int x = 0; x < gridDimensions.x; x++) {
                Voxel& voxel = GetVoxel(x, y, z);
                
                // Check if voxel center is inside the mesh
                voxel.isSolid = IsPointInsideMesh(voxel.position, meshModel);
                
                if (voxel.isSolid) {
                    solidCount++;
                    // Solid voxels have zero velocity
                    voxel.velocity = vec3(0.0f);
                    voxel.density = 2.0f; // Higher density for solid
                }
            }
        }
    }
    
    std::cout << "Voxelization complete. Solid voxels: " << solidCount 
              << " / " << GetTotalVoxels() << std::endl;
}

Voxel& AerodynamicsGrid::GetVoxel(int x, int y, int z) {
    return voxels[GetIndex(x, y, z)];
}

const Voxel& AerodynamicsGrid::GetVoxel(int x, int y, int z) const {
    return voxels[GetIndex(x, y, z)];
}

Voxel& AerodynamicsGrid::GetVoxelAtPosition(const vec3& worldPos) {
    ivec3 gridPos = WorldToGrid(worldPos);
    return GetVoxel(gridPos.x, gridPos.y, gridPos.z);
}

vec3 AerodynamicsGrid::GridToWorld(int x, int y, int z) const {
    return minBounds + vec3(x, y, z) * voxelSize + vec3(voxelSize * 0.5f);
}

ivec3 AerodynamicsGrid::WorldToGrid(const vec3& worldPos) const {
    vec3 localPos = worldPos - minBounds;
    ivec3 gridPos = ivec3(localPos / voxelSize);
    
    // Clamp to grid bounds
    gridPos.x = std::max(0, std::min(gridPos.x, gridDimensions.x - 1));
    gridPos.y = std::max(0, std::min(gridPos.y, gridDimensions.y - 1));
    gridPos.z = std::max(0, std::min(gridPos.z, gridDimensions.z - 1));
    
    return gridPos;
}

void AerodynamicsGrid::ApplyBoundaryConditions() {
    // Apply boundary conditions to grid edges
    for (int z = 0; z < gridDimensions.z; z++) {
        for (int y = 0; y < gridDimensions.y; y++) {
            for (int x = 0; x < gridDimensions.x; x++) {
                // Check if this is a boundary voxel
                bool isBoundary = (x == 0 || x == gridDimensions.x - 1 ||
                                  y == 0 || y == gridDimensions.y - 1 ||
                                  z == 0 || z == gridDimensions.z - 1);
                
                if (isBoundary) {
                    Voxel& voxel = GetVoxel(x, y, z);
                    
                    // Inlet boundary (left side) - set wind velocity
                    if (x == 0) {
                        voxel.velocity = vec3(1.0f, 0.0f, 0.0f);
                        voxel.pressure = 0.0f;
                    }
                    // Outlet boundary (right side) - zero pressure gradient
                    else if (x == gridDimensions.x - 1) {
                        voxel.pressure = 0.0f;
                    }
                    // Wall boundaries (top, bottom, front, back) - no-slip
                    else {
                        if (!voxel.isSolid) {
                            voxel.velocity = vec3(0.0f);
                        }
                    }
                }
            }
        }
    }
}

void AerodynamicsGrid::ApplyNoSlipBoundaries() {
    // Apply no-slip boundary condition at solid-fluid interfaces
    for (int z = 1; z < gridDimensions.z - 1; z++) {
        for (int y = 1; y < gridDimensions.y - 1; y++) {
            for (int x = 1; x < gridDimensions.x - 1; x++) {
                Voxel& voxel = GetVoxel(x, y, z);
                
                // If this is a fluid voxel next to a solid voxel
                if (!voxel.isSolid) {
                    bool nextToSolid = false;
                    
                    // Check all 6 neighbors
                    if (GetVoxel(x-1, y, z).isSolid || GetVoxel(x+1, y, z).isSolid ||
                        GetVoxel(x, y-1, z).isSolid || GetVoxel(x, y+1, z).isSolid ||
                        GetVoxel(x, y, z-1).isSolid || GetVoxel(x, y, z+1).isSolid) {
                        nextToSolid = true;
                    }
                    
                    if (nextToSolid) {
                        // Apply no-slip condition (reduce velocity near solid boundary)
                        voxel.velocity *= 0.5f;
                    }
                }
            }
        }
    }
}

bool AerodynamicsGrid::IsValidGridCoordinate(int x, int y, int z) const {
    return (x >= 0 && x < gridDimensions.x &&
            y >= 0 && y < gridDimensions.y &&
            z >= 0 && z < gridDimensions.z);
}

bool AerodynamicsGrid::IsInsideBounds(const vec3& worldPos) const {
    return (worldPos.x >= minBounds.x && worldPos.x <= maxBounds.x &&
            worldPos.y >= minBounds.y && worldPos.y <= maxBounds.y &&
            worldPos.z >= minBounds.z && worldPos.z <= maxBounds.z);
}

void AerodynamicsGrid::Reset() {
    for (auto& voxel : voxels) {
        if (!voxel.isSolid) {
            voxel.velocity = vec3(0.0f);
            voxel.pressure = 0.0f;
            voxel.density = 1.0f;
        }
    }
}

size_t AerodynamicsGrid::GetIndex(int x, int y, int z) const {
    return static_cast<size_t>(z * gridDimensions.x * gridDimensions.y + 
                              y * gridDimensions.x + x);
}

bool AerodynamicsGrid::IsPointInsideMesh(const vec3& point, const MeshModel& meshModel) const {
    // Simple ray casting algorithm to determine if point is inside mesh
    // Cast a ray from the point in the positive X direction and count intersections
    int intersectionCount = 0;
    
    // We'll use a simplified approach for now
    // In a full implementation, this would require proper mesh ray intersection
      // For demonstration, we'll use a simple distance-based approach
    // Check if point is close to the mesh center and within reasonable bounds
    vec3 meshCenter = (meshModel.MinPoints + meshModel.MaxPoints) * 0.5f;
    vec3 meshSize = meshModel.MaxPoints - meshModel.MinPoints;
    
    // Simple box-based inside test (can be improved with actual mesh intersection)
    vec3 relativePos = abs(point - meshCenter);
    vec3 halfSize = meshSize * 0.4f; // Make it smaller than actual bounding box
    
    return (relativePos.x < halfSize.x && 
            relativePos.y < halfSize.y && 
            relativePos.z < halfSize.z);
}

void AerodynamicsGrid::InitializeVoxelPositions() {
    for (int z = 0; z < gridDimensions.z; z++) {
        for (int y = 0; y < gridDimensions.y; y++) {
            for (int x = 0; x < gridDimensions.x; x++) {
                Voxel& voxel = GetVoxel(x, y, z);
                voxel.position = GridToWorld(x, y, z);
            }
        }
    }
}
