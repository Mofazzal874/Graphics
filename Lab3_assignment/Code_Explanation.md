# Project Code Explanation

This document provides a detailed explanation of the source code files (`assignment.cpp`, `Bus.h`, `Primitives.h`) for the OpenGL Bus Simulation project.

## 1. `Primitives.h`
This header file defines the basic 3D geometric shapes used to construct the bus and scene. It abstracts the OpenGL buffers (VAO, VBO) and drawing calls for clean reusability.

*   **`Cube` Class**:
    *   Defines a standard 1x1x1 cube centered at (0,0,0).
    *   `init()`: Sets up the vertices and normals for all 6 faces (36 vertices).
    *   `draw()`: Binds the VAO and issues the `glDrawArrays` call.
*   **`Cylinder` Class**:
    *   Generates a cylinder parametrically.
    *   `init(int sectors)`: Creates a cylinder with specified smoothness (sectors). Generates side triangles and top/bottom caps.
    *   Used for wheels, steering column, fan hubs, seat legs, etc.
*   **`Torus` Class**:
    *   Generates a donut shape.
    *   `init(mainRadius, tubeRadius, ...)`: Creates vertices using parametric equations for a torus.
    *   Used specifically for the steering wheel.

## 2. `Bus.h`
This file contains the `Bus` class, which encapsulates the entire logic for rendering and animating the bus. It uses the primitives defined in `Primitives.h`.

*   **State Variables**:
    *   `frontDoorAngle`, `windowOpenAmount[]`: Track animation states for interactive parts.
    *   `fanRotation`, `wheelRotation`, `steeringAngle`: Physics and animation states.
    *   `lightOn`: Toggle for interior lighting.
*   **Drawing Methods**:
    *   `draw(shader, parentTransform)`: Main draw method that calls sub-methods.
    *   `drawExterior(shader, parent)`:
        *   Renders the main body (coach style), roof, windows, and lights.
        *   **Wheels**: Draws 4 wheels. The front wheels incorporate `steeringAngle` to visually turn. All wheels rotate based on `wheelRotation`.
        *   **Door**: Renders the front door with rotation logic.
    *   `drawInterior(shader, parent)`: 
        *   Renders the floor, aisle carpet, and realistic coach passenger seats (cushions, headrests, armrests).
        *   Renders the detailed driver area (dashboard, steering wheel, seat).
        *   Renders ceiling features: LED strips, dome lights, and 2 animated ceiling fans.
*   **Update/Interaction Methods**:
    *   `updateWheels(speed)`: Updates `wheelRotation` based on bus movement speed.
    *   `updateFan(dt, spinning)`: Updates fan rotation if turned on.
    *   `toggle...()`: Helper methods to toggle doors, windows, and lights.

## 3. `assignment.cpp`
This is the main application file containing the entry point (`main`), render loop, and input handling.

*   **Global State**:
    *   **Camera**: Manages position (`cameraPos`), orientation (`pitch`, `yaw`, `roll`), and mode (Free Fly vs Orbit vs Chase).
    *   **Driving Sim**: `isDrivingMode`, `busPosition`, `busYaw`, `busSpeed`, `busSteerAngle`.
*   **`main()` Function**:
    *   Initializes GLFW, GLAD, and creates the window.
    *   Loads shaders and initializes the `Bus` object.
    *   **Render Loop**:
        *   Calculates `deltaTime` for smooth animations.
        *   Calls `processInput()` for controls.
        *   Updates bus mechanics (fans, wheels).
        *   Sets up View and Projection matrices.
        *   Draws the scene.
*   **`processInput(window)`**:
    *   **Driving Mode (`K` key)**:
        *   Implements physics-based movement (acceleration, deceleration).
        *   Calculates turn radius based on steering angle and speed.
        *   Updates `busPosition` and `busYaw`.
        *   Implements **Chase Camera**: Automatically positions camera behind and above the driving bus.
    *   **Free Fly Mode**:
        *   Standard "flying camera" controls: `W/S` (forward/back), `A/D` (strafe), `E/R` (up/down).
        *   Camera rotation: `X/Y/Z` keys.
    *   **Orbit Mode (`F` key)**:
        *   Rotates the camera in a perfect circle around the current bus position.
*   **`key_callback()`**:
    *   Handles one-time key presses for toggles:
        *   `1-8`: Doors and windows.
        *   `G`: Fans. `L`: Lights.
        *   `2`: Teleport inside bus. `9`: Driver seat view. `0`: Exit bus.
        *   `K`: Toggle Driving Mode.

## Controls Summary
*   **Driving**: `K` to toggle. `W/S` to drive, `A/D` to steer.
*   **Camera**: `W/S/A/D/E/R` to fly (when not driving). `X/Y/Z` to rotate view. `F` to orbit.
*   **Bus**: `1` (Door), `G` (Fans), `L` (Lights), `3-8` (Windows).
