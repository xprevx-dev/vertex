# Vertex

Vertex is a Geometry Dash 2.2 Geode mod with a Dear ImGui control surface. The menu is opened with `Tab`; category panels and the HUD are independently draggable.

## Build

Install the Geode SDK and set `GEODE_SDK` to its checkout, then build with the Geode CLI:

```sh
geode build
```

The CMake project fetches Dear ImGui `1.92.4-docking` with CPM and compiles the renderer into the mod. The target binding is GD `2.2081` on desktop and mobile.

## Layout

- `src/Menu.*` contains only ImGui presentation and persistent toggle editing.
- `src/ImGuiLayer.*` owns the Cocos2d/OpenGL Dear ImGui backend and Tab/input bridge.
- `src/ModState.*` owns Geode-saved settings, CPS timestamps, session time, and best-run state.
- `src/main.cpp` contains the Player and PlayLayer hitbox hooks.
- `src/Hooks.cpp` contains Bypass, Creator/Editor, and rendering hooks.
