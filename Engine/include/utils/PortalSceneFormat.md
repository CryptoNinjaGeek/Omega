# Portal Scene JSON Format Specification

## Version 1.0

This document describes the JSON file format for portal scenes.

## File Structure

```json
{
  "version": "1.0",
  "scene": {
    "name": "Scene Name",
    "camera": {
      "position": [0.0, 1.5, 5.0],
      "yaw": -90.0,
      "pitch": 0.0
    },
    "ambient": [0.2, 0.2, 0.2, 1.0],
    "objects": [],
    "portals": [],
    "lights": [],
    "materials": {},
    "textures": {}
  }
}
```

## Scene Properties

### camera
Initial camera configuration:
- `position`: [x, y, z] - Starting camera position
- `yaw`: float - Initial yaw angle in degrees (default: -90.0)
- `pitch`: float - Initial pitch angle in degrees (default: 0.0)

### ambient
Ambient light color: [r, g, b, a] (default: [0.2, 0.2, 0.2, 1.0])

## Objects

Array of scene objects:

```json
{
  "type": "box|plane|container|mesh|dome",
  "name": "unique_object_name",
  "position": [x, y, z],
  "rotation": [x, y, z],  // Euler angles in degrees
  "scale": 1.0,
  "size": 0.5,  // For box/plane/container
  "mass": 1.0,
  "textures": ["texture_name1", "texture_name2"],
  "material": "material_name",
  "shader": "core|plain",
  "visible": true,
  "physics": {
    "enabled": true,
    "bodyType": "STATIC|DYNAMIC|KINEMATIC",
    "colliderType": "BOX|SPHERE|PLANE"
  }
}
```

### Object Types
- `box`: Cube/box primitive
- `plane`: Flat plane primitive
- `container`: Container mesh
- `mesh`: Custom mesh - can be either:
  - File-based: requires `meshFile` property (e.g., "models/room.obj")
  - Geometry-based: requires `vertices` and `indices` properties (supports per-face textures)
- `dome`: Sky dome

### Mesh Objects

#### File-based Mesh
For `type: "mesh"` with `meshFile` property:
- `meshFile`: Path to mesh file (e.g., "models/room.obj")

#### Custom Geometry Mesh
For `type: "mesh"` with `vertices` and `indices` properties, define geometry manually:

```json
{
  "type": "mesh",
  "name": "room_walls",
  "position": [0.0, 0.0, 0.0],
  "scale": 1.0,
  "textures": ["wall", "floor", "ceiling"],
  "material": "default",
  "vertices": [
    {
      "position": [0.0, 0.0, 0.0],
      "normal": [0.0, 1.0, 0.0],
      "uv": [0.0, 0.0]
    },
    {
      "position": [5.0, 0.0, 0.0],
      "normal": [0.0, 1.0, 0.0],
      "uv": [1.0, 0.0]
    },
    {
      "position": [5.0, 0.0, 5.0],
      "normal": [0.0, 1.0, 0.0],
      "uv": [1.0, 1.0]
    }
  ],
  "indices": [0, 1, 2, 0, 2, 3],
  "faces": [
    {
      "indices": [0, 1, 2, 0, 2, 3],
      "texture": "floor"
    },
    {
      "indices": [4, 5, 6, 4, 6, 7],
      "texture": "wall"
    }
  ],
  "physics": {
    "enabled": false
  }
}
```

**Custom Geometry Properties:**
- `vertices`: Array of vertex objects, each with:
  - `position`: [x, y, z] - Vertex position (required)
  - `normal`: [x, y, z] - Vertex normal (optional, defaults to [0, 1, 0])
  - `uv`: [u, v] - Texture coordinates (optional, defaults to [0, 0])
- `indices`: Array of unsigned integers defining triangles (3 indices per triangle)
- `faces`: Optional array of face groups, each with:
  - `indices`: Array of indices for this face group
  - `texture`: Texture name from textures array to use for this face
- `textures`: Array of texture names (used if `faces` not specified, or as fallback)

**Note:** If `faces` is specified, the geometry will be split into multiple objects (one per face group) to support different textures. Otherwise, all textures in the `textures` array will be applied to the single object.

## Portals

Array of portal definitions:

```json
{
  "id": "portal_a",
  "name": "Portal A",
  "position": [x, y, z],
  "normal": [0, 0, -1],  // Portal facing direction
  "up": [0, 1, 0],  // Optional, auto-calculated if omitted
  "width": 2.0,
  "height": 3.0,
  "linkedTo": "portal_b",  // ID of linked portal
  "enabled": true,
  "visible": true,
  "framebuffer": {
    "width": 1024,
    "height": 1024
  }
}
```

### Portal Properties
- `id`: Unique identifier (required for linking)
- `position`: Portal center position
- `normal`: Portal facing direction (normalized)
- `up`: Up vector (optional, auto-calculated)
- `width`: Portal width in world units
- `height`: Portal height in world units
- `linkedTo`: ID of portal to link with (creates PortalPair)
- `enabled`: Whether portal is active
- `visible`: Whether portal surface is visible
- `framebuffer`: Framebuffer resolution (optional, defaults to 1024x1024)

## Lights

Array of light sources:

### Directional Light
```json
{
  "type": "directional",
  "direction": [x, y, z],
  "ambient": [r, g, b],
  "diffuse": [r, g, b],
  "specular": [r, g, b],
  "enabled": true
}
```

### Point Light
```json
{
  "type": "point",
  "position": [x, y, z],
  "ambient": [r, g, b],
  "diffuse": [r, g, b],
  "specular": [r, g, b],
  "constant": 1.0,
  "linear": 0.09,
  "quadratic": 0.032,
  "enabled": true
}
```

### Spot Light
```json
{
  "type": "spot",
  "position": [x, y, z],
  "direction": [x, y, z],
  "ambient": [r, g, b],
  "diffuse": [r, g, b],
  "specular": [r, g, b],
  "cutOff": 12.5,
  "outerCutOff": 17.5,
  "constant": 1.0,
  "linear": 0.09,
  "quadratic": 0.032,
  "enabled": true
}
```

## Materials

Map of material definitions:

```json
{
  "material_name": {
    "shininess": 32.0,
    "diffuse": [r, g, b, a],
    "specular": [r, g, b, a]
  }
}
```

## Textures

Map of texture definitions:

```json
{
  "texture_name": {
    "file": "path/to/texture.png",
    "type": "diffuse|specular|normal|height"
  }
}
```

## Example Complete Scene

```json
{
  "version": "1.0",
  "scene": {
    "name": "Portal Room Demo",
    "camera": {
      "position": [0.0, 1.5, 5.0],
      "yaw": -90.0,
      "pitch": 0.0
    },
    "ambient": [0.2, 0.2, 0.2, 1.0],
    "textures": {
      "floor": {
        "file": ":/textures/container2.png",
        "type": "diffuse"
      },
      "wall": {
        "file": ":/textures/brickwall.jpg",
        "type": "diffuse"
      }
    },
    "materials": {
      "default": {
        "shininess": 32.0
      }
    },
    "objects": [
      {
        "type": "plane",
        "name": "floor",
        "position": [0, 0, 0],
        "size": 10.0,
        "textures": ["floor"],
        "material": "default",
        "physics": {
          "enabled": true,
          "bodyType": "STATIC",
          "colliderType": "PLANE"
        }
      }
    ],
    "portals": [
      {
        "id": "portal_left",
        "name": "Left Portal",
        "position": [-5.0, 1.5, 0.0],
        "normal": [1, 0, 0],
        "width": 2.0,
        "height": 3.0,
        "linkedTo": "portal_right",
        "enabled": true
      },
      {
        "id": "portal_right",
        "name": "Right Portal",
        "position": [5.0, 1.5, 0.0],
        "normal": [-1, 0, 0],
        "width": 2.0,
        "height": 3.0,
        "linkedTo": "portal_left",
        "enabled": true
      }
    ],
    "lights": [
      {
        "type": "directional",
        "direction": [-0.2, -1.0, -0.3],
        "ambient": [0.3, 0.3, 0.3],
        "diffuse": [0.8, 0.8, 0.8],
        "specular": [1.0, 1.0, 1.0],
        "enabled": true
      }
    ]
  }
}
```

## Future Extensions

The format is designed to be extensible. Future additions may include:
- Doors (proximity-activated, animated)
- Mirrors (reflection surfaces)
- Triggers (events, teleportation)
- Animated objects
- Particle systems
- Audio sources
- Scripts/behaviors

