# GLCrawler
![C++](https://img.shields.io/badge/C++-17-blue) ![OpenGL](https://img.shields.io/badge/OpenGL-4.0.0-green) [![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://github.com/sfdez0/GLCrawler/blob/main/LICENSE.md)

GLCrawler is a first-person 3D dungeon crawler built with **OpenGL** as a university project for Computer Graphics course at MUII.

It features different levels, real-time lighting with flickering torches, an A* powered enemy AI, collectable keys, and a particle system for visual effects. 

The logic is structured around a modular architecture where game entities (Doors, Torches, Keys, Enemies) are encapsulated in dedicated modules that manage their own shaders, textures, and logic.

---
  
## Features  
  
- [x] First-person movement (WASD) with mouse look and sprint (Shift)  
- [x] Z-Buffer 
- [x] BoundingBox collision detection against maze walls  
- [x] Texture mapping (walls, floor, ceiling)  
- [x] Pause menu and HUD via Dear ImGui  
- [x] Maze loading from `.txt` map files  
- [x] Dark-fog lighting (only areas near a light source are illuminated)  
- [x] Animated torches with flickering point lights  
- [x] Pseudo-Randomised light intensity  
- [x] Particle system
- [x] Enemy AI with A\* pathfinding
- [x] Key collection mechanic  
- [x] Win/loss state machine (escape portal or die)  
  
---  
  
## Technology Stack  
  
| Component | Library / Technology | 
|:----------------|:---------------|  
| Graphics API | OpenGL |
| Windowing/Input | GLFW |
| Math | GLM |
| Model Loading | Assimp |
| UI | Dear ImGui |
| Pathfinding | A\* ([astar](https://github.com/daancode/a-star)) |
  
---  
  
## Prerequisites  
  
1. **GPU drivers** with OpenGL 4.0 support.  
2. **C++17 compiler**  
   - Linux: GCC/G++ 
   - Windows: Visual Studio 2019/2022
   - macOS: Xcode
3. **CMake ≥ 3.7** accessible from the terminal.  
  
---  
  
## Building  

All dependencies (GLFW, GLM, Assimp, GLAD, ImGui, AStar) are bundled under `libs/`.  
  
### Linux  
  
```bash  
mkdir build && cd build  
cmake .. -DOpenGL_GL_PREFERENCE=GLVND  
make -j4
```

### Windows (Visual Studio)

Open the repository folder directly in Visual Studio 2019/2022 (CMake project
support) or run CMake-GUI to generate a .sln solution file, then build from the IDE.

---  

## Running

After building, launch the game from `build/bin/`:
```bash
./gpo_proyecto_final   # Linux / macOS  
gpo_proyecto_final.exe # Windows
```

---  

## Controls

| Key / Input | Action | 
|:------------|:-------|  
| W A S D | Move |
| Shift | Sprint |
| Mouse | Look around |
| ESC | Show menu |
| F11 | Toggle fullscreen mode |

---  

## Credits
- **Base framework:** [GitHub - gpo-framework](https://github.com/juanpebm/gpo-framework) by @juanpebm
- **[OpenGL](https://www.opengl.org/)**
- **[GLFW](https://www.glfw.org/)** - *License: zlib/libpng*
- **[GLM](glm.g-truc.net)** - *License: MIT*
- **[Assimp](https://www.assimp.org/)** - *License: BSD 3-clause*
- **[Dear ImGUI](https://github.com/ocornut/imgui)** by Omar Cornut - *License: BSD 3-clause*
- **Astar C++ algorithm:** [GitHub - a-star](https://github.com/daancode/a-star) by Damian Barczyński - *License: MIT*
- **3D Model conversion tool:** [Online 3D Viewer](https://3dviewer.net) by Viktor Kovacs - *(used as an external tool)*
- **Brick texture:** [Poly Haven - Castle Brick 02 Red](https://polyhaven.com/a/castle_brick_02_red) by Rob Tuytel - *License: CC0 1.0*
- **Concrete texture:** [Poly Haven - Concrete Floor Damaged 01](https://polyhaven.com/a/concrete_floor_damaged_01) by Rob Tuytel - *License: CC0 1.0*
- **Sand texture:** [Poly Haven - Sand 03](https://polyhaven.com/a/sand_03) by Charlotte Baglioni - *License: CC0 1.0*
- **Fire sprite:** [OpenGameArt - Animated Flame](https://opengameart.org/content/animated-flame) by @dorkster - *License: CC-BY-SA 3.0*
- **Door model:** [Sketchfab - Bunker Door](https://sketchfab.com/3d-models/bunker-door-153c7436da9b47e59f7539c1c54104f1) by @Skotchet - *License: CC BY 4.0*
- **Ghost model:** [Sketchfab - Cloth Ghost](https://sketchfab.com/3d-models/cloth-ghost-f4fc1cd448c04a809d106975cfa7011b) by @ANDRE - *License: CC BY 4.0*
- **Torch model:** [Sketchfab - Medieval Torch (worn)](https://sketchfab.com/3d-models/medieval-torch-worn-16e59c78217d42c195492089d7f398c5) by @reddification - *License: CC BY 4.0*
- **Key model:** [Sketchfab - Key](https://sketchfab.com/3d-models/key-99ee9ff58fd14cb19309da81d0b098c9) by @Gektark - *License: CC BY 4.0*

---  

## Authors
- [@sfdez0](https://github.com/sfdez0)
- [@TaigaSeven](https://github.com/TaigaSeven)
