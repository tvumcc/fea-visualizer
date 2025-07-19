# Finite Element Visualizer
https://hackatime-badge.hackclub.com/U091YQ9F6B0/fea-visualizer

## Demo Video
The link to the demo video can be found [here]().

## Usage

### Initializing a Surface

### Using the Solver

## Installation
Builds for Windows, macOS, and Linux are available in the [Releases](https://github.com/tvumcc/fea-visualizer/releases) tab.

## Building from Source
This project uses CMake for compilation, so make sure you have CMake installed before proceeding.

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
This project is largely inspired by the brilliant work done on [VisualPDE](https://visualpde.com/), another interactive sandbox for various PDEs that uses Finite Differences for spatial discretization instead of Finite Elements.