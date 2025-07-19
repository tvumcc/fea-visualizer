# Finite Element Visualizer 
![Hackatime Counter](https://hackatime-badge.hackclub.com/U091YQ9F6B0/fea-visualizer)

This project aims to showcase the time propogation to solutions of partial differential equations on triangularly meshed surfaces in 3D space. Users can adjust parameters with sliders, set initial conditions with the mouse, and create surfaces from drawings or .obj files. 

In particular, this project currently provides solvers for the following equations:
- [Heat Equation](https://en.wikipedia.org/wiki/Heat_equation)
- [Wave Equation](https://en.wikipedia.org/wiki/Wave_equation)
- [Advection-Diffusion Equation](https://en.wikipedia.org/wiki/Convection%E2%80%93diffusion_equation)
- [Gray-Scott Reaction-Diffusion Equation](https://groups.csail.mit.edu/mac/projects/amorphous/GrayScott/)

## Demo Video
The link to the demo video can be found [here]().

## Usage

### Initializing a Surface
Before any solver can be used a valid surface must be initialized first. There are two ways to initialize said surface:

#### Draw a PSLG (Planar Straight Line Graph)

1. By clicking the `Draw PSLG` button, the user enters into the PSLG drawing mode where left mouse clicks define a series of points connected by lines.
2. To close a loop with the final segment press Enter (to finish the whole PSLG and exit drawing) or Ctrl+Enter (to finish just that loop and continue drawing). 
3. Once there is at least one closed loop and none that are incomplete, holes can be designated in a region by clicking `Add Hole`.
4. Triangulate the PSLG using the `Init from PSLG` button to generate a valid surface. Regions containing a hole indicator will not be triangulated.

#### Import from a .obj 

Click the `Import from .obj` button and select a `.obj` file that contains vertex positions and normals. Some example meshes are provided in the `assets/fem_meshes` directory. In general, you need to use a mesh that does not have sharp edges like a cube does as that causes numerical instability. Tools like Blender can be used to smooth out vertices and possibly circumvent this issue. 

### Using the Solver

Once a surface is initialized, the application switches to brush mode where the user can hold left click and drag on the surface to set the values of the nodes in that region. Parameters for each equation's solver can be adjusted using the sliders at the bottom of the side panel. 

Be wary, however, of numerical instability that could arise when setting these parameters to be too extreme; this will trigger a pop up and clear the solver of its nodal values. In some cases, if using the brush causes numerical instability almost immediately, then try lowering the brush strength. Meshes with sharp edges and turns (around 90 degrees) can also cause problems so try to stick to those that are relatively smooth.

## Installation
Builds for Windows, macOS, and Linux are available in the [Releases](https://github.com/tvumcc/fea-visualizer/releases) tab.

## Building from Source
This project uses CMake for compilation, so make sure CMake is installed before proceeding.

```bash
git clone https://github.com/tvumcc/fea-visualizer.git --recursive
cmake -S . -B build -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Be sure to use the appropriate generator for the OS you are compiling on. Run `cmake -G` to see a list of generators. If you are using MSVC, use the argument `--config Release` when calling the build command to compile in Release mode.

When running the executable, make sure it is run from the same directory that contains the `shaders` and `assets` directories otherwise shaders and images will not be able to load.

## Attribution

### Libraries Used
- [glfw](https://github.com/glfw/glfw) - windowing and input handling
- [glad](https://gen.glad.sh/) - OpenGL function and enum loading
- [glm](https://github.com/g-truc/glm) - linear algebra library for use with OpenGL
- [eigen](https://gitlab.com/libeigen/eigen) - linear algebra library for solving linear systems
- [imgui](https://github.com/ocornut/imgui) - drawing the GUI
- [stb_image.h](https://github.com/nothings/stb/blob/master/stb_image.h) - image file loading
- [tinyobjloader](https://github.com/tinyobjloader/tinyobjloader) - .obj file loading
- [nativefiledialog](https://github.com/mlabbe/nativefiledialog) - cross platform interface to a file dialog
- [triangle](https://github.com/libigl/triangle) - Delaunay triangulation for PSLGs

### Online References Used
- Professor Qiqi Wang's Lectures on the Finite Element Method on his channel [AerodynamicCFD](https://www.youtube.com/@AeroCFD)
    - [2020 Lecture 12](https://www.youtube.com/playlist?list=PLcqHTXprNMIOEwNpmNo7HWx68FzBTxTh3)
    - [2020 Lecture 13](https://www.youtube.com/playlist?list=PLcqHTXprNMIPvSgBidAYOY1fIunDywInP)
    - [2020 Lecture 14](https://www.youtube.com/playlist?list=PLcqHTXprNMIN-YciJQ4gtVGrlrhG8bPQp)
    - [2020 Lecture 15](https://www.youtube.com/playlist?list=PLcqHTXprNMIMvURxGSkTe6ef3-DhfJxEn)
    - [2016 Lecture 16](https://www.youtube.com/playlist?list=PLcqHTXprNMIOhhcvwc5bWNs5CQfNKhpM-)
- [mbn010/Gray-Scott-reaction-diffusion-on-a-sphere](https://github.com/mbn010/Gray-Scott-reaction-diffusion-on-a-sphere)

### Inspiration
This project is largely inspired by the brilliant work done on [VisualPDE](https://visualpde.com/), an interactive sandbox for various PDEs that uses Finite Differences for spatial discretization instead of Finite Elements.