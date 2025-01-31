# OpenVTT
*An open-source virtual tabletop implementation*

## Todo: descriptive stuff

## Dependencies
Dependencies are managed via [Conan](https://conan.io/). 
As long as you have Conan installed, you should be able to just load the CMake project, which will download the necessary dependencies.
An explicit list:
- `antlr4-runtime` (runtime for the parser)
- `antlr4` (ensures we can build the parser from ANTLR to C++ source)
- `stb` (loading images as textures)
- `assimp` (loading models)
- `whereami` (finding the asset folder)
- `glm` (math library)
- `imgui` (UI library)
- `glfw` (windowing library)
- `opengl` (rendering)

Additionally, you need to have some Java version (to run the ANTLR parser generator) and C++23 or higher (development environment is `gcc version 14.2.1 20240910 (GCC) `).

### Quick building instructions
```shell
mkdir build
cd build
cmake ..
cmake --build openvtt 
```
Alternatively, the project can be loaded in CLion, which will automatically configure everything for you.

### CMake targets
- `openvtt` (the main executable)
- `docs` (generates documentation using Doxygen)
- `map_spec` (builds the parser using ANTLR; `openvtt` depends on this target)

## Documentation
The code is documented using [Doxygen](https://www.doxygen.nl/index.html)-style comments.
Additionally, the CMake project exposes a documentation target (which will generate both HTML pages and LaTeX files):
```shell
cmake --build docs
```