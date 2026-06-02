# 3D Anime Character Modelling | OpenGL

An interactive 3D character modelling project developed using C++, OpenGL, GLU, and the Win32 API. The project features a fully animated anime-inspired warrior monk character named **Jian Feng (剑枫)**, designed with modular body parts, hierarchical animation, texture customization, and real-time user interaction.

## Project Overview

This project was developed as a Graphics Programming assignment to demonstrate 3D modelling, rendering, animation, and user interaction using the OpenGL fixed-function pipeline.

The character is assembled from individual mesh components including:

- Body
- Arms
- Fists
- Legs
- Shoes
- Clothes
- Cloak
- Hat
- Hair
- Knee Guards
- Weapon

Each component is modelled separately and connected through hierarchical transformations to enable realistic character movement and animation. :contentReference[oaicite:1]{index=1}

## Features

### Character Animation

- Walking Animation
- Idle Animation
- Attack Animation
- Spinning Animation
- Play / Pause Animation

### Character Controls

- Independent Arm Movement
- Independent Leg Movement
- Knee Joint Bending
- Fist Rotation
- Hat Tilting

### Camera Controls

- Mouse Rotation
- Zoom In / Out
- Camera Height Adjustment
- Perspective Projection
- Orthographic Projection

### Visual Effects

- Ambient Lighting
- Diffuse Lighting
- Spotlight Effects
- Depth Testing
- Face Culling
- Ground Grid

### Texture Customization

- Multiple Shirt Textures
- Multiple Hat Textures
- Multiple Cloak Textures

### Environment Settings

- Day Sky
- Night Sky
- Sunset Sky
- Midnight Sky

## Technologies Used

- C++
- OpenGL
- GLU
- Win32 API
- Visual Studio 2022
- Blender
- BMP Texture Mapping

## Development Tools

| Tool | Purpose |
|--------|----------|
| Visual Studio 2022 | Development Environment |
| OpenGL | 3D Rendering |
| GLU | Utility Functions and Projection |
| Win32 API | Window Management and Input |
| Blender | Character Modelling |
| BMP Textures | Surface Texturing |

## Character Design

The character, **Jian Feng (剑枫)**, is inspired by East Asian warrior monk aesthetics. The design combines traditional monk robes with combat-oriented armor pieces, creating a balance between spirituality and martial strength. 

## Model Statistics

### Polygon Count

| Part | Triangles |
|--------|----------|
| Belt | 680 |
| Body | 1117 |
| Cloak | 316 |
| Clothes | 422 |
| Hair | 612 |
| Hat | 242 |
| Knee Cover (Left) | 786 |
| Knee Cover (Right) | 786 |
| Left Arm | 257 |
| Left Fist | 401 |
| Left Hand Cover | 497 |
| Lower Leg (Left) | 561 |
| Lower Leg (Right) | 561 |
| Low Waist | 160 |
| Pants | 544 |
| Right Arm | 207 |
| Right Fist | 339 |
| Right Hand Cover | 497 |
| Shoe (Left) | 656 |
| Shoe (Right) | 656 |
| Weapon | 398 |
| Upper Left Leg | 550 |
| Upper Right Leg | 530 |

### Total Polygons

```text
11,775 Triangles
23 Mesh Components
```

## Project Structure

```text
3D-Anime-Character-Modelling/
│
├── main.cpp
│
├── onlybody.h
├── onlybelt.h
├── onlycloak.h
├── onlyclothes.h
├── onlyhair.h
├── onlyhat.h
├── onlykneecoverleft.h
├── onlykneecoverright.h
├── onlyleftarm.h
├── onlyleftfist.h
├── onlylefthandcover.h
├── onlylowerlegleft.h
├── onlylowerlegright.h
├── onlylowwaist.h
├── onlypants.h
├── onlyrightarm.h
├── onlyrightfist.h
├── onlyrighthandcover.h
├── onlyshoeleft.h
├── onlyshoeright.h
├── onlyweapon.h
├── upperleftleg.h
├── uprightleg.h
│
├── textures/
│
└── README.md
```

## Controls

### Mouse Controls

| Key | Function |
|------|----------|
| Left Mouse Drag | Rotate Camera |
| Mouse Wheel | Zoom In / Out |
| Z / X | Adjust Camera Height |

### Animation Controls

| Key | Function |
|------|----------|
| 1 | Walk Animation |
| 2 | Idle Animation |
| 3 | Attack Animation |
| 4 | Spin Animation |
| P | Play / Pause |

### Camera & Environment

| Key | Function |
|------|----------|
| F | Change Sky Color |
| O | Toggle Projection Mode |
| L | Toggle Lighting |
| R | Toggle Ground Grid |
| + | Increase Animation Speed |
| - | Decrease Animation Speed |

### Body Controls

| Key | Function |
|------|----------|
| T / G | Right Arm |
| Y / H | Left Arm |
| I / K | Right Fist |
| U / J | Left Fist |
| V / B | Right Leg |
| N / M | Left Leg |
| ; | Right Knee |
| [ ] | Left Knee |
| Q / E | Hat Tilt |

### Texture Customization

| Key | Function |
|------|----------|
| F1 / F2 / F3 | Shirt Textures |
| F4 / F5 / F6 | Hat Textures |
| F7 / F8 / F9 | Cloak Textures |

## How to Run

### 1. Open Project

Open:

```text
Graphics Programming Assignment.sln
```

using Visual Studio 2022.

### 2. Build Project

```text
Build → Build Solution
```

### 3. Run Project

```text
Debug → Start Without Debugging
```

or press:

```text
Ctrl + F5
```

## Learning Outcomes

- OpenGL Graphics Programming
- Hierarchical Character Animation
- 3D Transformations
- Camera Systems
- Texture Mapping
- Lighting Techniques
- Event Handling with Win32 API
- Interactive User Controls
- Character Modelling Integration
- Real-Time Rendering

## Author

Ngoh Jia Ying
