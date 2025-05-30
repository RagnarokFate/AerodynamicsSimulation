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
    
    // Update existing particles
    for (auto& particle : particles) {
        if (particle.life <= 0.0f) {
            // Reset particle to inlet
            particle.position = grid->GetMinBounds() + vec3(0.1f, 
                static_cast<float>(rand()) / RAND_MAX * (grid->GetMaxBounds().y - grid->GetMinBounds().y),
                static_cast<float>(rand()) / RAND_MAX * (grid->GetMaxBounds().z - grid->GetMinBounds().z));
            particle.life = particle.maxLife;
        }
        
        // Get velocity at particle position
        if (grid->IsInsideBounds(particle.position)) {
            ivec3 gridPos = grid->WorldToGrid(particle.position);
            const Voxel& voxel = grid->GetVoxel(gridPos.x, gridPos.y, gridPos.z);
            particle.velocity = voxel.velocity;
        }
        
        // Update position
        particle.position += particle.velocity * deltaTime;
        particle.life -= deltaTime;
    }
}

void AerodynamicsVisualizer::ResetParticles(const AerodynamicsGrid& grid) {
    particles.clear();
    particles.reserve(settings.numParticles);
    
    vec3 minBounds = grid.GetMinBounds();
    vec3 maxBounds = grid.GetMaxBounds();
    
    for (int i = 0; i < settings.numParticles; i++) {
        FlowParticle particle;
        particle.position = vec3(
            minBounds.x + 0.1f,
            minBounds.y + static_cast<float>(rand()) / RAND_MAX * (maxBounds.y - minBounds.y),
            minBounds.z + static_cast<float>(rand()) / RAND_MAX * (maxBounds.z - minBounds.z)
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
        return settings.pressureColorLow;
    }
      float t = (pressure - minPressure) / (maxPressure - minPressure);
    t = std::max(0.0f, std::min(1.0f, t));
    
    return mix(settings.pressureColorLow, settings.pressureColorHigh, t);
}

vec3 AerodynamicsVisualizer::ColorMapVelocity(float velocityMagnitude, float maxVelocity) const {
    if (maxVelocity <= 0.0f) {
        return settings.velocityColor;
    }
      float t = std::max(0.0f, std::min(1.0f, velocityMagnitude / maxVelocity));
    return settings.velocityColor * t;
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
    
    // Sample every nth voxel to avoid too many vectors
    int skipFactor = std::max(1, dimensions.x / 20);
    
    for (int z = 0; z < dimensions.z; z += skipFactor) {
        for (int y = 0; y < dimensions.y; y += skipFactor) {
            for (int x = 0; x < dimensions.x; x += skipFactor) {
                const Voxel& voxel = grid.GetVoxel(x, y, z);
                
                if (voxel.isSolid || length(voxel.velocity) < 0.001f) {
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
    particles.resize(count);
    // Reinitialize particles if already set up
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
