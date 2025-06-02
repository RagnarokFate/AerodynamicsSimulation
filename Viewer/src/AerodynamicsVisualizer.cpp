#include "AerodynamicsVisualizer.h"
#include <algorithm>
#include <iostream>

AerodynamicsVisualizer::AerodynamicsVisualizer() {
}

AerodynamicsVisualizer::~AerodynamicsVisualizer() {
    Cleanup();
}

void AerodynamicsVisualizer::Initialize() {
    if (isInitialized) return;
    
    LoadShaders();
    SetupVelocityBuffers();
    SetupPressureBuffers();
    SetupStreamlineBuffers();
    SetupParticleBuffers();
    SetupGridBuffers();
    
    isInitialized = true;
    std::cout << "AerodynamicsVisualizer initialized" << std::endl;
}

void AerodynamicsVisualizer::InitializePostProcessing(int width, int height)
{
    postProcessor.Initialize(width, height);
    postProcessingInitialized = true;
    std::cout << "AerodynamicsVisualizer: Post-processing initialized with Gaussian blur" << std::endl;
}

void AerodynamicsVisualizer::ResizePostProcessing(int width, int height)
{
    if (postProcessingInitialized) {
        postProcessor.Resize(width, height);
    }
}

void AerodynamicsVisualizer::SetBlurParameters(float radius, float strength, int iterations)
{
    blurRadius = radius;
    blurStrength = strength;
    blurIterations = iterations;
    
    if (postProcessingInitialized) {
        postProcessor.SetBlurParameters(radius, strength, iterations);
    }
}

void AerodynamicsVisualizer::EnablePressureBlur(bool enable)
{
    pressureBlurEnabled = enable;
}

void AerodynamicsVisualizer::EnableVelocityBlur(bool enable)
{
    velocityBlurEnabled = enable;
}

bool AerodynamicsVisualizer::IsPressureBlurEnabled() const
{
    return pressureBlurEnabled && postProcessingInitialized;
}

bool AerodynamicsVisualizer::IsVelocityBlurEnabled() const
{
    return velocityBlurEnabled && postProcessingInitialized;
}

void AerodynamicsVisualizer::Cleanup() {
    if (!isInitialized) return;
    
    // Clean up OpenGL resources
    if (velocityVAO != 0) {
        glDeleteVertexArrays(1, &velocityVAO);
        glDeleteBuffers(1, &velocityVBO);
    }
    
    if (pressureVAO != 0) {
        glDeleteVertexArrays(1, &pressureVAO);
        glDeleteBuffers(1, &pressureVBO);
    }
    
    if (streamlineVAO != 0) {
        glDeleteVertexArrays(1, &streamlineVAO);
        glDeleteBuffers(1, &streamlineVBO);
    }
    
    if (particleVAO != 0) {
        glDeleteVertexArrays(1, &particleVAO);
        glDeleteBuffers(1, &particleVBO);
    }
    
    if (gridVAO != 0) {
        glDeleteVertexArrays(1, &gridVAO);
        glDeleteBuffers(1, &gridVBO);
    }
    
    isInitialized = false;
}

void AerodynamicsVisualizer::Render(const FluidSimulation& simulation, const mat4& view, const mat4& projection) {
    if (!isInitialized) {
        Initialize();
    }
    
    auto grid = simulation.GetGrid();
    if (!grid) return;
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    if (settings.showVelocityVectors) {
        RenderVelocityVectors(*grid, view, projection);
    }
    
    if (settings.showPressure) {
        RenderPressureField(*grid, view, projection);
    }
    
    if (settings.showStreamlines) {
        RenderStreamlines(simulation, view, projection);
    }
    
    if (settings.showParticles) {
        RenderParticles(simulation, view, projection);
    }
    
    if (settings.showGrid) {
        RenderGrid(*grid, view, projection);
    }
    
    glDisable(GL_BLEND);
}

void AerodynamicsVisualizer::RenderVelocityVectors(const AerodynamicsGrid& grid, const mat4& view, const mat4& projection) {
    UpdateVelocityBuffers(grid);
    
    velocityShader.use();
    velocityShader.setUniform("uModel", mat4(1.0f));
    velocityShader.setUniform("uView", view);
    velocityShader.setUniform("uProjection", projection);
    velocityShader.setUniform("uVelocityScale", settings.velocityScale);
    
    float maxVelocity = GetMaxVelocity(grid);
    velocityShader.setUniform("uMaxVelocity", maxVelocity);
    
    if (IsVelocityBlurEnabled() && postProcessingInitialized) {
        // Enable smooth line rendering for blur effect
        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        // Render multiple passes with different line widths for blur effect
        glBindVertexArray(velocityVAO);
        
        // First pass - thickest lines with lowest opacity (background blur)
        glLineWidth(8.0f);
        glDrawArrays(GL_LINES, 0, velocityVertexData.size() / 6);
        
        // Second pass - medium lines 
        glLineWidth(5.0f);
        glDrawArrays(GL_LINES, 0, velocityVertexData.size() / 6);
        
        // Third pass - normal lines (main detail)
        glLineWidth(3.0f);
        glDrawArrays(GL_LINES, 0, velocityVertexData.size() / 6);
        
        glBindVertexArray(0);
        glDisable(GL_LINE_SMOOTH);
    } else {
        // Standard velocity vector rendering without blur
        glLineWidth(3.0f);
        glBindVertexArray(velocityVAO);
        glDrawArrays(GL_LINES, 0, velocityVertexData.size() / 6);
        glBindVertexArray(0);
    }
    
    // Reset line width
    glLineWidth(1.0f);
}

void AerodynamicsVisualizer::RenderPressureField(const AerodynamicsGrid& grid, const mat4& view, const mat4& projection) {
    UpdatePressureBuffers(grid);
    
    pressureShader.use();
    pressureShader.setUniform("view", view);
    pressureShader.setUniform("projection", projection);
    
    // Find pressure range for color mapping
    float minPressure = 0.0f, maxPressure = 0.0f;
    bool first = true;
    ivec3 dimensions = grid.GetDimensions();
    
    for (int z = 0; z < dimensions.z; z++) {
        for (int y = 0; y < dimensions.y; y++) {
            for (int x = 0; x < dimensions.x; x++) {
                const Voxel& voxel = grid.GetVoxel(x, y, z);
                if (!voxel.isSolid) {
                    if (first) {
                        minPressure = maxPressure = voxel.pressure;
                        first = false;
                    } else {
                        minPressure = std::min(minPressure, voxel.pressure);
                        maxPressure = std::max(maxPressure, voxel.pressure);
                    }
                }
            }
        }
    }
    
    pressureShader.setUniform("uMinPressure", minPressure);
    pressureShader.setUniform("uMaxPressure", maxPressure);    if (IsPressureBlurEnabled() && postProcessingInitialized) {
        // Enable blending for smooth blur effect
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        // Render multiple passes with different point sizes for blur effect
        glBindVertexArray(pressureVAO);
        
        // First pass - largest points (background blur)
        glPointSize(12.0f);
        glDrawArrays(GL_POINTS, 0, pressureVertices.size());
        
        // Second pass - medium points
        glPointSize(8.0f);
        glDrawArrays(GL_POINTS, 0, pressureVertices.size());
        
        // Third pass - normal points (main detail)
        glPointSize(5.0f);
        glDrawArrays(GL_POINTS, 0, pressureVertices.size());
        
        glBindVertexArray(0);
    } else {
        // Standard pressure field rendering without blur
        glPointSize(4.0f);
        glBindVertexArray(pressureVAO);
        glDrawArrays(GL_POINTS, 0, pressureVertices.size());
        glBindVertexArray(0);
    }
    
    // Reset to default point size
    glPointSize(1.0f);
}

void AerodynamicsVisualizer::RenderStreamlines(const FluidSimulation& simulation, const mat4& view, const mat4& projection) {
    // Generate streamlines if needed
    if (streamlines.empty()) {
        GenerateStreamlines(simulation);
    }
    
    UpdateStreamlineBuffers();
    
    streamlineShader.use();
    streamlineShader.setUniform("view", view);
    streamlineShader.setUniform("projection", projection);
    streamlineShader.setUniform("color", settings.streamlineColor);
    
    glBindVertexArray(streamlineVAO);
    glDrawArrays(GL_LINE_STRIP, 0, streamlineVertices.size());
    glBindVertexArray(0);
}

void AerodynamicsVisualizer::RenderParticles(const FluidSimulation& simulation, const mat4& view, const mat4& projection) {
    UpdateParticleBuffers();
    
    particleShader.use();
    particleShader.setUniform("view", view);
    particleShader.setUniform("projection", projection);
    particleShader.setUniform("color", settings.particleColor);
    particleShader.setUniform("pointSize", settings.particleSize);
    
    // Optimize particle rendering with hardware-level point size
    // This provides better performance and larger particles for improved visibility
    float hardwarePointSize = settings.particleSize * 100.0f; // Scale up for visibility
    hardwarePointSize = std::max(2.0f, std::min(hardwarePointSize, 10.0f)); // Clamp between 2-10 pixels
    glPointSize(hardwarePointSize);
    
    glBindVertexArray(particleVAO);
    glDrawArrays(GL_POINTS, 0, particles.size());
    glBindVertexArray(0);
    
    // Reset to default point size
    glPointSize(1.0f);
}

void AerodynamicsVisualizer::RenderGrid(const AerodynamicsGrid& grid, const mat4& view, const mat4& projection) {
    UpdateGridBuffers(grid);
    
    gridShader.use();
    gridShader.setUniform("view", view);
    gridShader.setUniform("projection", projection);
    vec4 gridColorWithAlpha = vec4(settings.gridColor, settings.gridOpacity);
    gridShader.setUniform("color", gridColorWithAlpha);
    
    glBindVertexArray(gridVAO);
    glDrawArrays(GL_LINES, 0, gridVertices.size());
    glBindVertexArray(0);
}

void AerodynamicsVisualizer::RenderSlicePlane(const AerodynamicsGrid& grid, const mat4& view, const mat4& projection) {
    // TODO: Implement slice plane visualization
}

void AerodynamicsVisualizer::UpdateParticles(const FluidSimulation& simulation, float deltaTime) {
    auto grid = simulation.GetGrid();
    if (!grid) return;
    
    vec3 minBounds = grid->GetMinBounds();
    vec3 maxBounds = grid->GetMaxBounds();
    vec3 domainSize = maxBounds - minBounds;
    
    // Update existing particles
    for (auto& particle : particles) {
        if (particle.life <= 0.0f) {
            // Reset particle to inlet area with better distribution
            float inletWidth = domainSize.x * 0.15f;
            particle.position = vec3(
                minBounds.x + 0.05f + static_cast<float>(rand()) / RAND_MAX * inletWidth,
                minBounds.y + 0.1f * domainSize.y + static_cast<float>(rand()) / RAND_MAX * 0.8f * domainSize.y,
                minBounds.z + 0.1f * domainSize.z + static_cast<float>(rand()) / RAND_MAX * 0.8f * domainSize.z
            );
            particle.life = particle.maxLife;
        }
        
        // Get velocity at particle position
        if (grid->IsInsideBounds(particle.position)) {
            ivec3 gridPos = grid->WorldToGrid(particle.position);
            const Voxel& voxel = grid->GetVoxel(gridPos.x, gridPos.y, gridPos.z);
            
            // Only use velocity if not in solid voxel
            if (!voxel.isSolid) {
                particle.velocity = voxel.velocity;
            } else {
                // If particle entered solid, respawn it
                particle.life = 0.0f;
                continue;
            }
        } else {
            // Particle left domain, respawn it
            particle.life = 0.0f;
            continue;
        }
        
        // Update position with velocity
        particle.position += particle.velocity * deltaTime;
        particle.life -= deltaTime;
        
        // Check if particle has left domain bounds
        if (particle.position.x > maxBounds.x || 
            particle.position.y < minBounds.y || particle.position.y > maxBounds.y ||
            particle.position.z < minBounds.z || particle.position.z > maxBounds.z) {
            particle.life = 0.0f; // Force respawn
        }
    }
}

void AerodynamicsVisualizer::ResetParticles(const AerodynamicsGrid& grid) {
    particles.clear();
    particles.reserve(settings.numParticles);
    
    // Use grid bounds as fallback if no mesh model is available
    vec3 minBounds = grid.GetMinBounds();
    vec3 maxBounds = grid.GetMaxBounds();
    
    // Position particles in a more realistic inlet area (left side of domain)
    // This creates a more natural flow visualization where particles enter from upwind
    vec3 inletSize = maxBounds - minBounds;
    float inletWidth = inletSize.x * 0.15f; // Use 15% of domain width for inlet
    
    for (int i = 0; i < settings.numParticles; i++) {
        FlowParticle particle;
        
        // Position particles in the inlet zone (upwind area)
        particle.position = vec3(
            minBounds.x + 0.05f + static_cast<float>(rand()) / RAND_MAX * inletWidth,
            minBounds.y + 0.1f * inletSize.y + static_cast<float>(rand()) / RAND_MAX * 0.8f * inletSize.y,
            minBounds.z + 0.1f * inletSize.z + static_cast<float>(rand()) / RAND_MAX * 0.8f * inletSize.z
        );
        particle.life = particle.maxLife;
        particles.push_back(particle);
    }
}

void AerodynamicsVisualizer::GenerateStreamlines(const FluidSimulation& simulation) {
    auto grid = simulation.GetGrid();
    if (!grid) return;
    
    streamlines.clear();
    vec3 minBounds = grid->GetMinBounds();
    vec3 maxBounds = grid->GetMaxBounds();
    
    for (int i = 0; i < settings.numStreamlines; i++) {
        vec3 startPos = vec3(
            minBounds.x + 0.1f,
            minBounds.y + static_cast<float>(i) / settings.numStreamlines * (maxBounds.y - minBounds.y),
            minBounds.z + 0.5f * (maxBounds.z - minBounds.z)
        );
        
        std::vector<vec3> streamline = TraceStreamline(startPos, simulation);
        if (!streamline.empty()) {
            streamlines.push_back(streamline);
        }
    }
}

std::vector<vec3> AerodynamicsVisualizer::TraceStreamline(const vec3& startPos, const FluidSimulation& simulation) const {
    std::vector<vec3> streamline;
    auto grid = simulation.GetGrid();
    if (!grid) return streamline;
    
    vec3 currentPos = startPos;
    float stepSize = grid->GetVoxelSize() * 0.5f;
    int maxSteps = static_cast<int>(settings.streamlineLength / stepSize);
    
    for (int step = 0; step < maxSteps; step++) {
        if (!grid->IsInsideBounds(currentPos)) {
            break;
        }
        
        streamline.push_back(currentPos);
        
        // Get velocity at current position
        ivec3 gridPos = grid->WorldToGrid(currentPos);
        const Voxel& voxel = grid->GetVoxel(gridPos.x, gridPos.y, gridPos.z);
        
        if (voxel.isSolid || length(voxel.velocity) < 0.001f) {
            break;
        }
        
        // Move to next position
        currentPos += normalize(voxel.velocity) * stepSize;
    }
    
    return streamline;
}

void AerodynamicsVisualizer::UpdateMeshPressureColors(MeshModel& meshModel, const FluidSimulation& simulation) {
    // TODO: Update mesh vertex colors based on pressure values
    // This would require access to mesh vertex data and the ability to modify it
}

vec3 AerodynamicsVisualizer::ColorMapPressure(float pressure, float minPressure, float maxPressure) const {
    if (maxPressure <= minPressure) {
        return vec3(0.0f, 0.0f, 1.0f); // Blue for neutral pressure
    }
    
    float normalized = (pressure - minPressure) / (maxPressure - minPressure);
    normalized = std::max(0.0f, std::min(1.0f, normalized));
    
    // Scientific pressure color mapping for aerodynamics
    // Blue (low pressure) -> Green (neutral) -> Yellow -> Orange -> Red (high pressure)
    if (normalized < 0.25f) {
        // Low pressure: Deep blue to light blue
        float t = normalized / 0.25f;
        return mix(vec3(0.0f, 0.0f, 0.8f), vec3(0.2f, 0.6f, 1.0f), t);
    } else if (normalized < 0.5f) {
        // Medium-low pressure: Light blue to green
        float t = (normalized - 0.25f) / 0.25f;
        return mix(vec3(0.2f, 0.6f, 1.0f), vec3(0.0f, 1.0f, 0.0f), t);
    } else if (normalized < 0.75f) {
        // Medium-high pressure: Green to yellow
        float t = (normalized - 0.5f) / 0.25f;
        return mix(vec3(0.0f, 1.0f, 0.0f), vec3(1.0f, 1.0f, 0.0f), t);
    } else {
        // High pressure: Yellow to red
        float t = (normalized - 0.75f) / 0.25f;
        return mix(vec3(1.0f, 1.0f, 0.0f), vec3(1.0f, 0.0f, 0.0f), t);
    }
}

vec3 AerodynamicsVisualizer::ColorMapVelocity(float velocityMagnitude, float maxVelocity) const {
    if (maxVelocity <= 0.0f) {
        return vec3(0.7f, 0.85f, 1.0f); // Light blue for still air
    }
    
    float normalized = std::max(0.0f, std::min(1.0f, velocityMagnitude / maxVelocity));
    
    // Enhanced fluid color mapping for natural air flow visualization
    if (normalized < 0.15f) {
        // Very still air - almost transparent light blue
        float t = normalized / 0.15f;
        return mix(vec3(0.9f, 0.95f, 1.0f), vec3(0.7f, 0.85f, 1.0f), t);
    } else if (normalized < 0.4f) {
        // Gentle movement - light blue to medium blue
        float t = (normalized - 0.15f) / 0.25f;
        return mix(vec3(0.7f, 0.85f, 1.0f), vec3(0.4f, 0.7f, 1.0f), t);
    } else if (normalized < 0.65f) {
        // Medium flow - blue to cyan with more saturation
        float t = (normalized - 0.4f) / 0.25f;
        return mix(vec3(0.4f, 0.7f, 1.0f), vec3(0.1f, 0.8f, 1.0f), t);
    } else if (normalized < 0.85f) {
        // Fast flow - cyan to bright cyan-white
        float t = (normalized - 0.65f) / 0.2f;
        return mix(vec3(0.1f, 0.8f, 1.0f), vec3(0.6f, 0.95f, 1.0f), t);
    } else {
        // Very fast/turbulent flow - bright cyan-white to pure white
        float t = (normalized - 0.85f) / 0.15f;
        return mix(vec3(0.6f, 0.95f, 1.0f), vec3(0.95f, 0.98f, 1.0f), t);
    }
}

void AerodynamicsVisualizer::SetupVelocityBuffers() {
    glGenVertexArrays(1, &velocityVAO);
    glGenBuffers(1, &velocityVBO);
    
    glBindVertexArray(velocityVAO);
    glBindBuffer(GL_ARRAY_BUFFER, velocityVBO);
    
    // Interleaved vertex attributes: position (3 floats) + velocity (3 floats) = 6 floats per vertex
    size_t stride = 6 * sizeof(float);
    
    // Position attribute (location = 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    
    // Velocity attribute (location = 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindVertexArray(0);
}

void AerodynamicsVisualizer::SetupPressureBuffers() {
    glGenVertexArrays(1, &pressureVAO);
    glGenBuffers(1, &pressureVBO);
    
    glBindVertexArray(pressureVAO);
    glBindBuffer(GL_ARRAY_BUFFER, pressureVBO);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 2 * sizeof(vec3), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 2 * sizeof(vec3), (void*)sizeof(vec3));
    glEnableVertexAttribArray(1);
    
    glBindVertexArray(0);
}

void AerodynamicsVisualizer::SetupStreamlineBuffers() {
    glGenVertexArrays(1, &streamlineVAO);
    glGenBuffers(1, &streamlineVBO);
    
    glBindVertexArray(streamlineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, streamlineVBO);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindVertexArray(0);
}

void AerodynamicsVisualizer::SetupParticleBuffers() {
    glGenVertexArrays(1, &particleVAO);
    glGenBuffers(1, &particleVBO);
    
    glBindVertexArray(particleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, particleVBO);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindVertexArray(0);
}

void AerodynamicsVisualizer::SetupGridBuffers() {
    glGenVertexArrays(1, &gridVAO);
    glGenBuffers(1, &gridVBO);
    
    glBindVertexArray(gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindVertexArray(0);
}

void AerodynamicsVisualizer::UpdateVelocityBuffers(const AerodynamicsGrid& grid) {
    velocityVertices.clear();
    velocityVertexData.clear(); // Clear the interleaved data
    
    ivec3 dimensions = grid.GetDimensions();
    
    // Adaptive sampling - higher density near boundaries and flow regions
    int baseSkipFactor = std::max(1, dimensions.x / 25);
    
    for (int z = 0; z < dimensions.z; z += baseSkipFactor) {
        for (int y = 0; y < dimensions.y; y += baseSkipFactor) {
            for (int x = 0; x < dimensions.x; x += baseSkipFactor) {
                const Voxel& voxel = grid.GetVoxel(x, y, z);
                
                // Skip solid voxels and very low velocity areas
                if (voxel.isSolid || length(voxel.velocity) < 0.001f) {
                    continue;
                }
                
                // Enhanced sampling near solid boundaries (around mesh)
                bool nearBoundary = false;
                int checkRadius = 2;
                for (int dz = -checkRadius; dz <= checkRadius && !nearBoundary; dz++) {
                    for (int dy = -checkRadius; dy <= checkRadius && !nearBoundary; dy++) {
                        for (int dx = -checkRadius; dx <= checkRadius && !nearBoundary; dx++) {
                            int nx = x + dx, ny = y + dy, nz = z + dz;
                            if (nx >= 0 && nx < dimensions.x && 
                                ny >= 0 && ny < dimensions.y && 
                                nz >= 0 && nz < dimensions.z) {
                                if (grid.GetVoxel(nx, ny, nz).isSolid) {
                                    nearBoundary = true;
                                }
                            }
                        }
                    }
                }
                
                // Sample at higher density near boundaries (where interesting flow occurs)
                int currentSkip = nearBoundary ? std::max(1, baseSkipFactor / 2) : baseSkipFactor;
                if ((x % currentSkip != 0) || (y % currentSkip != 0) || (z % currentSkip != 0)) {
                    continue;
                }
                
                vec3 start = voxel.position;
                vec3 end = start + voxel.velocity * settings.vectorLength * settings.velocityScale;
                vec3 velocity = voxel.velocity;
                
                // Add line start vertex: position + velocity
                velocityVertexData.push_back(start.x);
                velocityVertexData.push_back(start.y);
                velocityVertexData.push_back(start.z);
                velocityVertexData.push_back(velocity.x);
                velocityVertexData.push_back(velocity.y);
                velocityVertexData.push_back(velocity.z);
                
                // Add line end vertex: position + velocity (same velocity for the vector)
                velocityVertexData.push_back(end.x);
                velocityVertexData.push_back(end.y);
                velocityVertexData.push_back(end.z);
                velocityVertexData.push_back(velocity.x);
                velocityVertexData.push_back(velocity.y);
                velocityVertexData.push_back(velocity.z);
                
                // Keep the old data for compatibility
                velocityVertices.push_back(start);
                velocityVertices.push_back(end);
            }
        }
    }
    
    // Upload the interleaved vertex data
    glBindBuffer(GL_ARRAY_BUFFER, velocityVBO);
    glBufferData(GL_ARRAY_BUFFER, velocityVertexData.size() * sizeof(float), 
                 velocityVertexData.data(), GL_DYNAMIC_DRAW);
}

void AerodynamicsVisualizer::UpdatePressureBuffers(const AerodynamicsGrid& grid) {
    pressureVertices.clear();
    pressureColors.clear();
    
    ivec3 dimensions = grid.GetDimensions();
    
    // Find pressure range for color mapping
    float minPressure = 0.0f, maxPressure = 0.0f;
    bool first = true;
    
    for (int z = 0; z < dimensions.z; z++) {
        for (int y = 0; y < dimensions.y; y++) {
            for (int x = 0; x < dimensions.x; x++) {
                const Voxel& voxel = grid.GetVoxel(x, y, z);
                if (!voxel.isSolid) {
                    if (first) {
                        minPressure = maxPressure = voxel.pressure;
                        first = false;
                    } else {
                        minPressure = std::min(minPressure, voxel.pressure);
                        maxPressure = std::max(maxPressure, voxel.pressure);
                    }
                }
            }
        }
    }
    
    // Sample every nth voxel
    int skipFactor = std::max(1, dimensions.x / 15);
    
    for (int z = 0; z < dimensions.z; z += skipFactor) {
        for (int y = 0; y < dimensions.y; y += skipFactor) {
            for (int x = 0; x < dimensions.x; x += skipFactor) {
                const Voxel& voxel = grid.GetVoxel(x, y, z);
                
                if (voxel.isSolid) {
                    continue;
                }
                
                pressureVertices.push_back(voxel.position);
                vec3 color = ColorMapPressure(voxel.pressure, minPressure, maxPressure);
                pressureColors.push_back(color);
            }
        }
    }
    
    // Interleave position and color data
    std::vector<float> interleavedData;
    interleavedData.reserve(pressureVertices.size() * 6);
    
    for (size_t i = 0; i < pressureVertices.size(); i++) {
        interleavedData.push_back(pressureVertices[i].x);
        interleavedData.push_back(pressureVertices[i].y);
        interleavedData.push_back(pressureVertices[i].z);
        interleavedData.push_back(pressureColors[i].x);
        interleavedData.push_back(pressureColors[i].y);
        interleavedData.push_back(pressureColors[i].z);
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, pressureVBO);
    glBufferData(GL_ARRAY_BUFFER, interleavedData.size() * sizeof(float), 
                 interleavedData.data(), GL_DYNAMIC_DRAW);
}

void AerodynamicsVisualizer::UpdateStreamlineBuffers() {
    streamlineVertices.clear();
    
    for (const auto& streamline : streamlines) {
        for (const vec3& point : streamline) {
            streamlineVertices.push_back(point);
        }
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, streamlineVBO);
    glBufferData(GL_ARRAY_BUFFER, streamlineVertices.size() * sizeof(vec3), 
                 streamlineVertices.data(), GL_DYNAMIC_DRAW);
}

void AerodynamicsVisualizer::UpdateParticleBuffers() {
    std::vector<vec3> particlePositions;
    particlePositions.reserve(particles.size());
    
    for (const auto& particle : particles) {
        if (particle.life > 0.0f) {
            particlePositions.push_back(particle.position);
        }
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, particleVBO);
    glBufferData(GL_ARRAY_BUFFER, particlePositions.size() * sizeof(vec3), 
                 particlePositions.data(), GL_DYNAMIC_DRAW);
}

void AerodynamicsVisualizer::UpdateGridBuffers(const AerodynamicsGrid& grid) {
    gridVertices.clear();
    
    vec3 minBounds = grid.GetMinBounds();
    vec3 maxBounds = grid.GetMaxBounds();
    ivec3 dimensions = grid.GetDimensions();
    
    // Draw grid lines
    float voxelSize = grid.GetVoxelSize();
    
    // X-direction lines
    for (int y = 0; y <= dimensions.y; y += std::max(1, dimensions.y / 10)) {
        for (int z = 0; z <= dimensions.z; z += std::max(1, dimensions.z / 10)) {
            vec3 start = minBounds + vec3(0, y * voxelSize, z * voxelSize);
            vec3 end = minBounds + vec3((dimensions.x - 1) * voxelSize, y * voxelSize, z * voxelSize);
            gridVertices.push_back(start);
            gridVertices.push_back(end);
        }
    }
    
    // Y-direction lines
    for (int x = 0; x <= dimensions.x; x += std::max(1, dimensions.x / 10)) {
        for (int z = 0; z <= dimensions.z; z += std::max(1, dimensions.z / 10)) {
            vec3 start = minBounds + vec3(x * voxelSize, 0, z * voxelSize);
            vec3 end = minBounds + vec3(x * voxelSize, (dimensions.y - 1) * voxelSize, z * voxelSize);
            gridVertices.push_back(start);
            gridVertices.push_back(end);
        }
    }
    
    // Z-direction lines
    for (int x = 0; x <= dimensions.x; x += std::max(1, dimensions.x / 10)) {
        for (int y = 0; y <= dimensions.y; y += std::max(1, dimensions.y / 10)) {
            vec3 start = minBounds + vec3(x * voxelSize, y * voxelSize, 0);
            vec3 end = minBounds + vec3(x * voxelSize, y * voxelSize, (dimensions.z - 1) * voxelSize);
            gridVertices.push_back(start);
            gridVertices.push_back(end);
        }
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    glBufferData(GL_ARRAY_BUFFER, gridVertices.size() * sizeof(vec3), 
                 gridVertices.data(), GL_DYNAMIC_DRAW);
}

void AerodynamicsVisualizer::LoadShaders() {
    // For now, we'll assume these shaders exist or will be created
    // In a full implementation, these would need to be proper shader files
    
    // Basic vertex/fragment shaders for visualization
    // The actual shader loading would depend on the existing ShaderProgram implementation
    
    std::cout << "Aerodynamics visualization shaders loaded (placeholder)" << std::endl;
}

// Parameter control methods
void AerodynamicsVisualizer::setParticleCount(int count) {
    settings.numParticles = count;
    
    // Immediately resize and reinitialize particles
    particles.clear();
    particles.resize(count);
    
    // Initialize all particles with default values
    for (int i = 0; i < count; i++) {
        particles[i] = FlowParticle();
        particles[i].life = 0.0f; // Force respawn on next update
    }
    
    // Reinitialize particle buffers if already set up
    if (isInitialized) {
        SetupParticleBuffers();
    }
}

void AerodynamicsVisualizer::setStreamlineParameters(int count, int maxLength) {
    settings.numStreamlines = count;
    settings.streamlineLength = static_cast<float>(maxLength);
    streamlines.clear();
    streamlines.resize(count);
    // Regenerate streamlines if visualization is active
    if (isInitialized && settings.showStreamlines) {
        SetupStreamlineBuffers();
    }
}

// Utility method to calculate maximum velocity in grid for shader normalization
float AerodynamicsVisualizer::GetMaxVelocity(const AerodynamicsGrid& grid) const {
    float maxVelocity = 0.0f;
    ivec3 dimensions = grid.GetDimensions();
    
    for (int z = 0; z < dimensions.z; z++) {
        for (int y = 0; y < dimensions.y; y++) {
            for (int x = 0; x < dimensions.x; x++) {
                const Voxel& voxel = grid.GetVoxel(x, y, z);
                if (!voxel.isSolid) {
                    float velocityMagnitude = length(voxel.velocity);
                    maxVelocity = std::max(maxVelocity, velocityMagnitude);
                }
            }
        }
    }
    
    return maxVelocity > 0.0f ? maxVelocity : 1.0f; // Avoid division by zero
}
