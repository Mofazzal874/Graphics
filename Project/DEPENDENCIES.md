# Third-party dependencies

The game builds without references to the lab branch or a machine-specific SDK folder.

| Component | Version / origin | Included files | License |
| --- | --- | --- | --- |
| GLFW | [3.4 official Windows x64 archive](https://github.com/glfw/glfw/releases/tag/3.4) | Headers, Visual C++ 2022 import library and runtime DLL | `vendor/glfw/LICENSE.md` |
| GLM | [1.0.1](https://github.com/g-truc/glm/releases/tag/1.0.1) | Header-only mathematics library | `vendor/glm/copying.txt` |
| GLAD | 0.1.36, generated for OpenGL 3.3 core on 2025-12-16 | Original course installation's generated C loader and headers | `vendor/glad/LICENSE` and Khronos notice in `khrplatform.h` |
| stb_image | Version in `stb_image.h` | Original project's image decoder | License included at the end of the header |

GLFW is dynamically linked (`GLFW_DLL`); the build copies `glfw3.dll` beside the executable. The included binaries target Windows x64. OpenGL itself is supplied by Windows and the graphics driver.

Textures and submission media are preserved from the supplied project. Their inclusion does not grant a new license to those assets.
