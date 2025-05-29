# Aerodynamics Simulation Viewer

A high-performance 3D visualization tool for aerodynamics and computational fluid dynamics (CFD) simulation data. Built with OpenGL, ImGui, and modern C++ for real-time interactive visualization of complex fluid flow patterns.

![Aerodynamics Simulation](https://img.shields.io/badge/OpenGL-4.0+-blue.svg)
![Build Status](https://img.shields.io/badge/Build-Passing-success.svg)
![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey.svg)

## ✨ Features

### 🎯 **Core Visualization**
- **Real-time 3D Mesh Rendering**: High-performance wireframe and solid mesh visualization
- **RGB Coordinate Axes**: Color-coded orientation indicators (X=Red, Y=Green, Z=Blue)
- **Interactive Camera Controls**: Mouse-based rotation, panning, and zoom
- **Multiple View Modes**: Wireframe, solid, and hybrid rendering options

### 🌊 **Fluid Dynamics Visualization**
- **Pressure Heatmaps**: Scientific color-coded pressure field visualization
  - 🔵 Blue: Low pressure zones
  - 🟢 Green: Neutral pressure  
  - 🟡 Yellow: Medium pressure
  - 🟠 Orange: High pressure
  - 🔴 Red: Maximum pressure
- **Velocity Vectors**: Realistic fluid flow representation
  - 🟦 Dark Blue: Slow air movement
  - 🔷 Light Blue: Medium air flow
  - ⚪ Light Cyan: Fast air flow
  - 💙 Bright Cyan: Maximum velocity
- **Particle System**: Enhanced particle rendering with pressure-based coloring
- **Streamlines**: Advanced flow path visualization

### 🎮 **User Interface**
- **ImGui Integration**: Modern, responsive user interface
- **Real-time Controls**: Dynamic parameter adjustment
- **Performance Monitoring**: Live FPS and performance metrics
- **File Management**: Native file dialogs for mesh loading

### ⚡ **Performance Optimizations**
- **Hardware Acceleration**: Full OpenGL 4.0+ support
- **Optimized Shaders**: High-performance GLSL implementations
- **Efficient Rendering**: Advanced culling and batching techniques
- **Memory Management**: Optimized buffer handling

## 🚀 Quick Start

### Prerequisites
- **CMake** 3.16 or higher
- **C++17** compatible compiler
- **OpenGL 4.0+** support
- **Git** for cloning

### Windows (Visual Studio)
```bash
git clone https://github.com/ragnarokfate/AerodynamicsSimulation.git
cd AerodynamicsSimulation
mkdir Build && cd Build
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Release
```

### Linux/macOS
```bash
git clone https://github.com/ragnarokfate/AerodynamicsSimulation.git
cd AerodynamicsSimulation
mkdir Build && cd Build
cmake ..
make -j$(nproc)
```

### Running the Application
```bash
# Windows
./Build/bin/Release/MeshViewer.exe

# Linux/macOS  
./Build/bin/MeshViewer
```

## 🎯 Usage

### Camera Controls
- **Mouse Wheel**: Zoom in/out
- **Left Click + Drag**: Rotate camera around target
- **Right Click + Drag**: Pan camera view
- **Mouse Hover**: UI interaction when over interface elements

### Loading Models
1. **File Menu** → **Open Mesh**
2. Select `.obj` files from the Data directory
3. Models include: `bunny.obj`, `teapot.obj`, `cow.obj`, and more

### Visualization Options
- **Wireframe Mode**: Toggle mesh wireframe display
- **Pressure Visualization**: Enable scientific pressure heatmaps
- **Velocity Vectors**: Show fluid flow directions
- **Particle System**: Display pressure-based particle effects
- **RGB Axes**: Show coordinate system orientation

## 📁 Project Structure

```
AerodynamicsSimulation/
├── Viewer/                 # Main application source
│   ├── include/           # Header files
│   ├── src/              # C++ source files
│   └── shaders/          # GLSL shader files
├── ThirdParty/           # External dependencies
│   ├── glfw/            # Window management
│   ├── glad/            # OpenGL loading
│   ├── glm/             # Mathematics library
│   ├── imgui/           # User interface
│   └── nativefiledialog/ # File dialogs
├── Data/                # Sample mesh models
├── Build/               # Build output (generated)
└── CMakeLists.txt       # Build configuration
```

## 🔧 Technical Details

### Core Technologies
- **OpenGL 4.0+**: Modern graphics pipeline
- **GLFW**: Cross-platform window management
- **GLM**: Mathematics library for 3D operations
- **ImGui**: Immediate mode GUI framework
- **GLAD**: OpenGL extension loader

### Rendering Pipeline
1. **Mesh Loading**: OBJ file parsing and buffer creation
2. **Shader Compilation**: Dynamic GLSL shader management
3. **Uniform Updates**: Real-time parameter passing
4. **Multi-pass Rendering**: Optimized draw call batching
5. **Post-processing**: Advanced visual effects

### Shader Programs
- `vshader.glsl` / `fshader.glsl`: Main mesh rendering
- `particle_vertex.glsl` / `particle_fragment.glsl`: Particle system
- `velocity_vertex.glsl` / `velocity_fragment.glsl`: Vector field
- `pressure_vertex.glsl` / `pressure_fragment.glsl`: Pressure visualization

## 🎨 Visualization Examples

### Pressure Field Analysis
The application provides industry-standard CFD visualization with:
- **Scientific Color Mapping**: Standard pressure heatmap colors
- **Smooth Transitions**: Interpolated color gradients
- **Real-time Updates**: Dynamic pressure field changes

### Fluid Flow Visualization
- **Vector Fields**: Directional flow indicators
- **Streamlines**: Particle-based flow paths
- **Velocity Magnitude**: Color-coded speed representation

## 🛠️ Development

### Building from Source
The project uses CMake for cross-platform building. All dependencies are included in the `ThirdParty/` directory.

### Adding New Features
1. **Mesh Formats**: Extend `MeshModel` class for new file types
2. **Visualization Modes**: Add new shaders in `Viewer/shaders/`
3. **UI Components**: Integrate new ImGui panels in `main.cpp`

### Performance Profiling
- Built-in FPS monitoring
- GPU timing analysis
- Memory usage tracking

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.



### Development Guidelines
- Follow C++17 standards
- Use consistent naming conventions
- Add comments for complex algorithms
- Test on multiple platforms when possible


---

