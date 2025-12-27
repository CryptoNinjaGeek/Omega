# Portal System Redesign: Doorway-Based Architecture

## Conceptual Model

Portals are **doorways/windows** between locations in space. They provide a view into another location and optionally allow passage through them.

## Rendering Strategy Summary

### Key Principles

1. **Skip Closed/Invisible Portals**: If a portal is closed or outside the view frustum, don't render its content
2. **Back Rendering**: When a portal is visible, render from behind it (through the portal) showing what's on the other side
3. **Portal Culling**: Only render objects that are visible through the portal (culled by portal bounds)
4. **Forward Rendering**: After rendering portal views, render portal surfaces forward with framebuffer textures
5. **Recursion Limiting**: Track recursion depth and active portals to prevent infinite loops

### Rendering Pipeline

```
Frame Render:
  1. Forward Render: Render main scene (normal objects)
  2. Back Render: For each visible, open portal:
     - Bind portal framebuffer
     - Calculate view through portal
     - Render scene from portal perspective (back rendering)
     - Apply portal clipping (only show what's visible through portal)
     - Recursively render nested portals (with depth limit)
  3. Forward Render: Render portal surfaces with framebuffer textures
```

### Culling Checks (in order)

Before rendering a portal's content:
1. ✅ Portal is open (`isOpen()`)
2. ✅ Portal is enabled (`isEnabled()`)
3. ✅ Portal is visible from camera (facing camera)
4. ✅ Portal is in view frustum
5. ✅ Recursion depth < max depth
6. ✅ Portal not already being rendered (prevent loops)

### Core Concepts

1. **Portal as Doorway**: A portal is a doorway or window that connects two locations
2. **View Transformation**: The portal's transform matrix defines what you see when looking through it
3. **Mirror Mode**: If a portal connects back to itself with a 180° rotation, it acts as a mirror
4. **Optional Overlay**: Portals can have an optional mirror overlay effect for visual enhancement
5. **Passability**: Portals can be configured to allow or prevent passage through them
6. **Universal Application**: Any door in a house, window, or opening can be a portal

## Architecture

### Portal Properties

```cpp
class Portal {
  // Location and orientation
  glm::vec3 position;
  glm::mat4 transform;  // Defines the doorway's orientation and what you see
  
  // Connection
  std::shared_ptr<Portal> destination;  // Where this portal leads to
  
  // Visual properties
  bool hasMirrorOverlay;  // Optional mirror effect overlay
  float mirrorIntensity;  // 0.0 = transparent, 1.0 = full mirror
  
  // Physical properties
  bool isPassable;  // Can entities walk through this portal?
  bool isOpen;      // Is the doorway open? (for doors)
  
  // Rendering
  std::shared_ptr<PortalFramebuffer> framebuffer;  // For rendering the view
  glm::mat4 viewMatrix;  // Calculated view matrix through this portal
};
```

### Key Differences from Current Implementation

**Current Model:**
- Portals are paired (PortalPair)
- View is calculated from relative transform between portals
- Focus on portal-to-portal transformation

**New Model:**
- Portal has a direct destination (can be itself for mirrors)
- Portal's transform matrix directly defines the view
- Portal can be standalone (mirror) or connected (doorway)
- Optional mirror overlay for visual effect
- Passability control for gameplay

## View Matrix Calculation

### For Connected Portals (Doorway)

When looking through a portal to another location:

```cpp
glm::mat4 calculateDoorwayView(const Camera& playerCamera, const Portal& portal) {
  // Get the destination portal
  auto dest = portal.getDestination();
  
  // Calculate relative transform: from portal space to destination space
  glm::mat4 relativeTransform = dest->getTransform() * glm::inverse(portal.getTransform());
  
  // Transform player camera through the portal
  glm::mat4 playerWorld = glm::inverse(playerCamera.viewMatrix());
  glm::mat4 transformedWorld = relativeTransform * playerWorld;
  
  // Update position
  glm::vec3 transformedPos = transformPosition(playerCamera.position(), portal, dest);
  transformedWorld[3] = glm::vec4(transformedPos, 1.0f);
  
  return glm::inverse(transformedWorld);
}
```

### For Mirror Portals (Self-Connected)

When a portal connects to itself with 180° rotation:

```cpp
glm::mat4 calculateMirrorView(const Camera& playerCamera, const Portal& portal) {
  // Mirror: reflect camera through portal plane
  glm::mat4 portalTransform = portal.getTransform();
  glm::mat4 reflection = calculateReflectionMatrix(portalTransform);
  
  glm::mat4 playerWorld = glm::inverse(playerCamera.viewMatrix());
  glm::mat4 mirroredWorld = reflection * playerWorld;
  
  // Reflect position
  glm::vec3 mirroredPos = reflectPosition(playerCamera.position(), portal);
  mirroredWorld[3] = glm::vec4(mirroredPos, 1.0f);
  
  return glm::inverse(mirroredWorld);
}
```

## Portal Types

### 1. Doorway Portal
- Connects two different locations
- Shows view of destination location
- Can be passable or non-passable
- Example: Door between rooms

### 2. Mirror Portal
- Connects to itself with 180° rotation
- Shows reflected view of current location
- Optional mirror overlay effect
- Example: Mirror on wall

### 3. Window Portal
- Connects to another location
- Non-passable (view only)
- Example: Window showing another room

## Rendering Strategy

### Rendering Order

1. **Forward Rendering (Main Scene)**
   - Render all regular scene objects
   - Use normal camera view
   - Standard depth testing and culling

2. **Back Rendering (Portal Views)**
   - For each visible, open portal:
     - Bind portal framebuffer
     - Calculate view through portal
     - Render scene from portal's perspective
     - Apply portal clipping (only show what's visible through portal)
     - Limit recursion depth

3. **Forward Rendering (Portal Surfaces)**
   - Render portal quads with framebuffer textures
   - Apply mirror overlay if enabled
   - Blend with main scene

### Culling Strategy

**Portal Visibility Check:**
```cpp
bool isPortalVisible(Portal* portal, Camera* camera) {
  // Check if portal is facing camera
  glm::vec3 toPortal = portal->getPosition() - camera->position();
  float dot = glm::dot(glm::normalize(toPortal), portal->getNormal());
  
  if (dot > 0.0f) {
    return false;  // Portal is facing away from camera
  }
  
  // Check if portal is in view frustum
  if (!isInViewFrustum(portal, camera)) {
    return false;
  }
  
  // Check distance (optional LOD)
  float distance = glm::length(toPortal);
  if (distance > MAX_PORTAL_VIEW_DISTANCE) {
    return false;
  }
  
  return true;
}
```

**Portal Clipping:**
- Only render objects that are visible through the portal
- Use clipping plane at destination portal (prevents seeing through portal back)
- Frustum cull based on portal bounds (only render what's in portal's view)
- Depth test against portal plane
- Portal acts as a window - only objects behind it (through it) are visible

**Clipping Plane Setup:**
```cpp
void setupPortalClipping(Portal* source, Portal* dest) {
  // Calculate clipping plane at destination portal
  // This prevents seeing through the back of the destination portal
  glm::vec3 destNormal = dest->getNormal();
  glm::vec3 destPos = dest->getPosition();
  
  // Clipping plane: normal points away from portal (opposite of portal normal)
  glm::vec4 clippingPlane = glm::vec4(-destNormal, glm::dot(-destNormal, destPos));
  
  // Transform clipping plane to view space
  glm::mat4 viewMatrix = portalCamera->viewMatrix();
  clippingPlane = glm::inverse(glm::transpose(viewMatrix)) * clippingPlane;
  
  // Set clipping plane in shader (GL_CLIP_DISTANCE0)
  // Objects behind the clipping plane will be clipped
}
```

### Recursion Limiting

**Prevent Infinite Loops:**
- Track recursion depth for each portal render
- Maximum depth: 3-4 levels (configurable)
- Skip portals that would cause infinite recursion
- Mark portals being rendered in current recursion path

```cpp
class PortalRenderContext {
  std::set<Portal*> renderingPortals_;  // Portals currently being rendered
  int maxDepth_{3};
  
  bool canRenderPortal(Portal* portal, int depth) {
    if (depth >= maxDepth_) {
      return false;  // Max depth reached
    }
    
    if (renderingPortals_.count(portal) > 0) {
      return false;  // Already rendering this portal (would cause loop)
    }
    
    return true;
  }
  
  void enterPortal(Portal* portal) {
    renderingPortals_.insert(portal);
  }
  
  void exitPortal(Portal* portal) {
    renderingPortals_.erase(portal);
  }
};
```

## Implementation Plan

### Phase 1: Refactor Portal Class

**Changes to `Portal.h`:**
- Add `destination_` (can be self for mirrors)
- Add `isPassable_` flag
- Add `hasMirrorOverlay_` and `mirrorIntensity_`
- Add `isOpen_` for door state
- Remove dependency on PortalPair (portals can be standalone)

**New Methods:**
```cpp
void setDestination(std::shared_ptr<Portal> dest);
std::shared_ptr<Portal> getDestination() const;

void setPassable(bool passable) { isPassable_ = passable; }
bool isPassable() const { return isPassable_; }

void setMirrorOverlay(bool enabled, float intensity = 0.5f);
bool hasMirrorOverlay() const { return hasMirrorOverlay_; }
float getMirrorIntensity() const { return mirrorIntensity_; }

void setOpen(bool open) { isOpen_ = open; }
bool isOpen() const { return isOpen_; }

bool isMirror() const;  // Returns true if destination == this

// Visibility and culling
bool isVisibleFrom(const glm::vec3& position) const;
bool isInViewFrustum(const Camera& camera) const;
glm::vec4 getClippingPlane() const;  // For portal culling
```

### Phase 2: Update PortalCamera

**Add Culling Support:**
```cpp
class PortalCamera {
  // Calculate clipping plane for portal culling
  static glm::vec4 getClippingPlane(const Portal& portal);
  
  // Check if portal is visible from camera
  static bool isPortalVisible(const Portal& portal, const Camera& camera);
  
  // Check if portal is in view frustum
  static bool isInViewFrustum(const Portal& portal, const Camera& camera);
};
```

**New Methods:**
```cpp
// Calculate view for doorway portal
static glm::mat4 calculateDoorwayView(
    const Camera& playerCamera,
    const Portal& portal);

// Calculate view for mirror portal
static glm::mat4 calculateMirrorView(
    const Camera& playerCamera,
    const Portal& portal);

// Unified interface
static glm::mat4 calculatePortalView(
    const Camera& playerCamera,
    const Portal& portal) {
  if (portal.isMirror()) {
    return calculateMirrorView(playerCamera, portal);
  } else {
    return calculateDoorwayView(playerCamera, portal);
  }
}
```

### Phase 3: Update PortalRenderer

**Changes:**
- Remove PortalPair dependency (portals are standalone)
- Check `portal->getDestination()` instead of PortalPair
- Handle mirror overlay rendering
- Respect `isOpen()` for doors
- Implement view frustum culling
- Implement portal culling (only render if portal is visible)
- Limit recursion depth to prevent infinite loops

**Rendering Order and Culling:**

1. **Visibility Check**: Only render portal content if:
   - Portal is open (`isOpen()`)
   - Portal is enabled (`isEnabled()`)
   - Portal is visible from camera (`isPortalVisible()`)
   - Portal is in view frustum

2. **Rendering Flow**:
   ```
   For each frame:
     1. Render main scene (forward rendering)
     2. For each visible portal:
         a. Check if portal is in view and open
         b. If yes, render portal view (back rendering through portal)
         c. Apply portal culling (only render what's visible through portal)
         d. Limit recursion depth
     3. Render portal surfaces (forward rendering with framebuffer textures)
   ```

**New Rendering Flow:**
```cpp
void renderFrame(Scene* scene, Camera* playerCamera) {
  // Phase 1: Render main scene (forward)
  scene->render(playerCamera);
  
  // Phase 2: Render portal views (back rendering, culled by portal)
  for (auto& portal : scene->getPortals()) {
    if (shouldRenderPortal(portal, playerCamera)) {
      renderPortalView(portal, scene, playerCamera, 0);  // recursionDepth = 0
    }
  }
  
  // Phase 3: Render portal surfaces (forward, with framebuffer textures)
  for (auto& portal : scene->getPortals()) {
    if (isPortalVisible(portal, playerCamera)) {
      renderPortalSurface(portal, playerCamera);
    }
  }
}

bool shouldRenderPortal(Portal* portal, Camera* camera) {
  // Check if portal should be rendered
  if (!portal->isOpen() || !portal->isEnabled()) {
    return false;  // Portal is closed or disabled
  }
  
  if (!isPortalVisible(portal, camera)) {
    return false;  // Portal is not in view
  }
  
  if (!isInViewFrustum(portal, camera)) {
    return false;  // Portal is outside view frustum
  }
  
  return true;
}

void renderPortalView(Portal* portal, Scene* scene, Camera* playerCamera, int recursionDepth) {
  // Prevent infinite recursion
  const int MAX_RECURSION_DEPTH = 3;
  if (recursionDepth >= MAX_RECURSION_DEPTH) {
    return;  // Stop recursion
  }
  
  auto dest = portal->getDestination();
  if (!dest) {
    return;  // No destination
  }
  
  // Bind portal framebuffer
  portal->getFramebuffer()->bind();
  
  // Calculate view matrix through portal
  glm::mat4 portalView = PortalCamera::calculatePortalView(*playerCamera, *portal);
  
  // Create portal camera
  auto portalCamera = std::make_shared<PortalViewCamera>(playerCamera, portalView);
  
  // Set up portal culling (only render what's visible through portal)
  setupPortalClipping(portal, dest);
  
  // Render scene from portal's perspective (back rendering)
  // This renders what's BEHIND the portal (through it)
  scene->render(portalCamera, portal);  // Pass portal for culling
  
  // Recursively render portals visible through this portal
  for (auto& nestedPortal : scene->getPortals()) {
    if (nestedPortal != portal && nestedPortal != dest) {  // Avoid self-reference
      if (shouldRenderPortal(nestedPortal, portalCamera.get())) {
        renderPortalView(nestedPortal, scene, portalCamera.get(), recursionDepth + 1);
      }
    }
  }
  
  // Unbind framebuffer
  portal->getFramebuffer()->unbind();
}

void setupPortalClipping(Portal* source, Portal* dest) {
  // Set up clipping plane to prevent seeing through portal back
  // Only render what's in front of the destination portal
  glm::vec4 clippingPlane = PortalCamera::getClippingPlane(*dest);
  
  // Enable clipping plane in shader
  // glEnable(GL_CLIP_DISTANCE0);
  // Set clipping plane uniform in shader
}
```

### Phase 4: Portal Renderer with Culling and Recursion Control

**New PortalRenderer Structure:**
```cpp
class PortalRenderer {
private:
  struct RenderContext {
    std::set<Portal*> activePortals_;  // Portals being rendered in current frame
    int maxRecursionDepth_{3};
    int currentDepth_{0};
  };
  
  RenderContext renderContext_;
  
public:
  void renderPortals(Scene* scene, Camera* playerCamera) {
    renderContext_.activePortals_.clear();
    renderContext_.currentDepth_ = 0;
    
    // Phase 1: Render main scene
    scene->render(playerCamera);
    
    // Phase 2: Render portal views (back rendering)
    for (auto& portal : scene->getPortals()) {
      if (shouldRenderPortal(portal, playerCamera)) {
        renderPortalViewRecursive(portal, scene, playerCamera);
      }
    }
    
    // Phase 3: Render portal surfaces (forward)
    for (auto& portal : scene->getPortals()) {
      if (isPortalVisible(portal, playerCamera)) {
        renderPortalSurface(portal, playerCamera);
      }
    }
  }
  
private:
  bool shouldRenderPortal(Portal* portal, Camera* camera) {
    // Check if portal is closed
    if (!portal->isOpen()) {
      return false;
    }
    
    // Check if portal is enabled
    if (!portal->isEnabled()) {
      return false;
    }
    
    // Check if portal is visible
    if (!isPortalVisible(portal, camera)) {
      return false;
    }
    
    // Check if portal is in view frustum
    if (!isInViewFrustum(portal, camera)) {
      return false;
    }
    
    // Check recursion limit
    if (renderContext_.currentDepth_ >= renderContext_.maxRecursionDepth_) {
      return false;
    }
    
    // Check for infinite loops
    if (renderContext_.activePortals_.count(portal) > 0) {
      return false;  // Already rendering this portal
    }
    
    return true;
  }
  
  void renderPortalViewRecursive(Portal* portal, Scene* scene, Camera* playerCamera) {
    // Mark portal as being rendered
    renderContext_.activePortals_.insert(portal);
    renderContext_.currentDepth_++;
    
    // Render portal view
    renderPortalView(portal, scene, playerCamera);
    
    // Unmark portal
    renderContext_.activePortals_.erase(portal);
    renderContext_.currentDepth_--;
  }
  
  void renderPortalView(Portal* portal, Scene* scene, Camera* playerCamera) {
    auto dest = portal->getDestination();
    if (!dest) {
      return;
    }
    
    // Bind framebuffer
    portal->getFramebuffer()->bind();
    
    // Calculate view through portal
    glm::mat4 portalView = PortalCamera::calculatePortalView(*playerCamera, *portal);
    auto portalCamera = std::make_shared<PortalViewCamera>(playerCamera, portalView);
    
    // Set up portal clipping
    glm::vec4 clippingPlane = PortalCamera::getClippingPlane(*dest);
    setupClippingPlane(clippingPlane);
    
    // Render scene from portal perspective (back rendering)
    // Only render what's visible through the portal
    scene->render(portalCamera.get(), portal);  // Pass portal for culling
    
    // Recursively render nested portals
    for (auto& nestedPortal : scene->getPortals()) {
      // Skip source and destination portals to avoid immediate loops
      if (nestedPortal == portal || nestedPortal == dest) {
        continue;
      }
      
      // Check if we can render this nested portal
      if (shouldRenderPortal(nestedPortal, portalCamera.get())) {
        renderPortalViewRecursive(nestedPortal, scene, portalCamera.get());
      }
    }
    
    // Clean up clipping
    disableClippingPlane();
    
    // Unbind framebuffer
    portal->getFramebuffer()->unbind();
  }
};
```

### Phase 5: Mirror Overlay Shader

**New Shader: `mirror_overlay.fs`:**
```glsl
#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D portalTexture;
uniform float mirrorIntensity;
uniform bool useMirrorOverlay;

void main() {
    vec4 portalColor = texture(portalTexture, TexCoords);
    
    if (useMirrorOverlay) {
        // Add mirror-like reflection effect
        vec3 mirrorTint = vec3(0.9, 0.95, 1.0);  // Slight blue tint
        portalColor.rgb = mix(portalColor.rgb, portalColor.rgb * mirrorTint, mirrorIntensity);
        
        // Add slight darkening for mirror effect
        portalColor.rgb *= (1.0 - mirrorIntensity * 0.1);
    }
    
    FragColor = portalColor;
}
```

### Phase 6: Physics/Teleportation

**New Component: PortalTrigger**
```cpp
class PortalTrigger {
  void checkCollision(Entity* entity) {
    if (portal_->isPassable() && portal_->isOpen()) {
      auto dest = portal_->getDestination();
      if (dest) {
        teleportEntity(entity, portal_, dest);
      }
    }
  }
  
  void teleportEntity(Entity* entity, Portal* source, Portal* dest) {
    // Transform entity position through portal
    glm::vec3 newPos = transformPosition(entity->position(), source, dest);
    entity->setPosition(newPos);
    
    // Transform entity orientation
    glm::mat4 relativeTransform = dest->getTransform() * glm::inverse(source->getTransform());
    // Apply rotation to entity...
  }
};
```

## JSON Scene Format Updates

```json
{
  "portals": [
    {
      "id": "doorway_kitchen_living",
      "type": "doorway",
      "position": [5.0, 1.0, 0.0],
      "transform": {
        "rotation": [0, 90, 0],
        "scale": [1, 1, 1]
      },
      "destination": "doorway_living_kitchen",
      "passable": true,
      "open": true,
      "mirrorOverlay": false
    },
    {
      "id": "mirror_bathroom",
      "type": "mirror",
      "position": [0.0, 1.5, -2.0],
      "transform": {
        "rotation": [0, 0, 0],
        "scale": [1, 1, 1]
      },
      "destination": "mirror_bathroom",  // Self-reference
      "passable": false,
      "open": true,
      "mirrorOverlay": true,
      "mirrorIntensity": 0.7
    }
  ]
}
```

## Benefits of This Approach

1. **Simpler Mental Model**: Portals are just doorways/windows
2. **More Flexible**: Can represent doors, windows, mirrors, portals
3. **Easier to Understand**: Transform matrix directly defines what you see
4. **Better for Level Design**: Any opening can be a portal
5. **Mirror Support**: Built-in mirror mode with optional overlay
6. **Gameplay Control**: Passability and open/closed states

## Migration Path

1. Keep current PortalPair system working
2. Add new doorway-based methods alongside existing ones
3. Update scene loader to support both formats
4. Gradually migrate to new system
5. Remove PortalPair dependency once migration complete

