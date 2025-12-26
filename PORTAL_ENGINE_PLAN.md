# Portal Engine Development Plan

## Overview
Build a portal/mirror system for a tunnel-based labyrinth game featuring:
- Proximity-based door mechanics (open/close)
- Portal rendering with view matrix transformations
- Transparent portal surfaces showing other locations
- Mirror surfaces with real-time reflections

## Architecture Overview

### Core Components
1. **Portal System** - Manages portal pairs and rendering
2. **Door System** - Handles proximity-based door animations
3. **Portal Renderer** - Renders portal views with matrix transformations
4. **Mirror System** - Handles reflection rendering
5. **Scene Manager** - Manages tunnel/labyrinth structure

---

## Phase 1: Foundation & Infrastructure

### Step 1.1: Portal Data Structures
**Goal**: Define core data structures for portals and doors

**Tasks**:
- [ ] Create `Portal.h` class with:
  - Position (vec3)
  - Normal vector (vec3)
  - Size (width, height)
  - Linked portal reference
  - View matrix for portal camera
  - Portal surface mesh/quad
  - Portal texture/framebuffer ID
- [ ] Create `Door.h` class with:
  - Position and orientation
  - Open/closed state
  - Animation state (0.0-1.0)
  - Open/close speed
  - Trigger distance threshold
  - Door mesh/model
- [ ] Create `PortalPair.h` to link two portals together
- [ ] Add portal/door enums and types

**Files to create**:
- `Engine/include/geometry/Portal.h`
- `Engine/include/geometry/Door.h`
- `Engine/include/geometry/PortalPair.h`
- `Engine/src/geometry/Portal.cpp`
- `Engine/src/geometry/Door.cpp`
- `Engine/src/geometry/PortalPair.cpp`

---

### Step 1.2: Framebuffer System for Portal Rendering
**Goal**: Create off-screen rendering system for portal views

**Tasks**:
- [ ] Create `PortalFramebuffer.h` class:
  - Framebuffer object (FBO)
  - Color texture attachment
  - Depth texture attachment
  - Resolution (width, height)
  - Bind/unbind methods
  - Clear methods
- [ ] Implement framebuffer creation and management
- [ ] Add framebuffer cleanup in destructor
- [ ] Test framebuffer rendering

**Files to create**:
- `Engine/include/render/PortalFramebuffer.h`
- `Engine/src/render/PortalFramebuffer.cpp`

**Technical Notes**:
- Use GL_RGBA8 for color texture
- Use GL_DEPTH_COMPONENT24 for depth
- Recommended resolution: 1024x1024 or 2048x2048
- Support multiple framebuffers for multiple portals

---

### Step 1.3: Portal Camera System
**Goal**: Calculate view matrices for portal rendering

**Tasks**:
- [ ] Create `PortalCamera.h` class:
  - Calculate view matrix from player position through portal
  - Handle portal-to-portal transformations
  - Clipping plane support (to prevent seeing through portal back)
- [ ] Implement matrix calculation:
  - Transform player position relative to source portal
  - Apply rotation based on portal orientations
  - Translate to destination portal position
- [ ] Add frustum culling for portal views
- [ ] Handle edge cases (player inside portal, etc.)

**Files to create**:
- `Engine/include/render/PortalCamera.h`
- `Engine/src/render/PortalCamera.cpp`

**Mathematical Formula**:
```
portalViewMatrix = destinationPortal.transform * 
                   sourcePortal.transform.inverse() * 
                   playerViewMatrix
```

---

## Phase 2: Door System Implementation

### Step 2.1: Door Proximity Detection
**Goal**: Detect when player is near doors

**Tasks**:
- [ ] Add proximity detection to `Door` class:
  - Calculate distance from player to door
  - Check if within trigger threshold
  - Update door state (opening/closing)
- [ ] Integrate with existing camera/player system
- [ ] Add configurable trigger distance
- [ ] Handle multiple doors simultaneously

**Implementation**:
```cpp
bool Door::checkProximity(const glm::vec3& playerPos, float threshold) {
    float distance = glm::distance(playerPos, position);
    return distance < threshold;
}
```

---

### Step 2.2: Door Animation System
**Goal**: Animate door opening/closing smoothly

**Tasks**:
- [ ] Implement door animation:
  - Linear or ease-in-out interpolation
  - Rotation-based opening (hinge simulation)
  - Or sliding door (translation)
- [ ] Update door transform matrix based on animation state
- [ ] Add animation speed control
- [ ] Handle door state transitions (closed -> opening -> open -> closing -> closed)

**Animation Types**:
- **Hinged Door**: Rotate around hinge axis (Y-axis typically)
- **Sliding Door**: Translate along track axis
- **Double Door**: Two doors opening in opposite directions

**State Machine**:
```
CLOSED -> (proximity detected) -> OPENING -> OPEN -> (proximity lost) -> CLOSING -> CLOSED
```

---

### Step 2.3: Door Rendering Integration
**Goal**: Render animated doors in the scene

**Tasks**:
- [ ] Add door mesh/model loading
- [ ] Update door rendering in scene render loop
- [ ] Apply animation transform to door model matrix
- [ ] Add door materials/textures
- [ ] Ensure doors don't clip through walls

**Integration Points**:
- Add doors to `Scene` class
- Update door transforms each frame
- Render doors with appropriate shader

---

## Phase 3: Portal Rendering System

### Step 3.1: Portal Surface Rendering
**Goal**: Render portal quad/surface in the scene

**Tasks**:
- [ ] Create portal surface mesh (quad):
  - 4 vertices forming a rectangle
  - UV coordinates
  - Normal vector
- [ ] Render portal surface with:
  - Portal framebuffer texture
  - Transparency/alpha blending
  - Stencil buffer for portal masking
- [ ] Add portal edge/rim rendering (optional visual enhancement)
- [ ] Handle portal culling (don't render if not visible)

**Rendering Order**:
1. Render scene normally
2. Render portal surface with stencil
3. Render portal view into framebuffer
4. Composite portal texture onto surface

---

### Step 3.2: Portal View Rendering
**Goal**: Render the view through portals

**Tasks**:
- [ ] For each portal pair:
  - Bind portal framebuffer
  - Calculate portal camera view matrix
  - Render scene from portal camera perspective
  - Unbind framebuffer
- [ ] Handle recursive portals (portals showing portals)
- [ ] Add depth limit to prevent infinite recursion
- [ ] Optimize: Only render portals that are visible

**Rendering Pipeline**:
```
For each visible portal:
  1. Bind portal framebuffer
  2. Clear framebuffer
  3. Calculate portal camera transform
  4. Render scene from portal camera (excluding portal surface itself)
  5. Unbind framebuffer
  6. Render portal surface with framebuffer texture
```

---

### Step 3.3: Portal View Matrix Calculation
**Goal**: Correctly transform view through portals

**Tasks**:
- [ ] Implement portal-to-portal transformation:
  - Transform player position relative to source portal
  - Apply portal rotation difference
  - Translate to destination portal
- [ ] Handle portal orientation differences
- [ ] Add clipping plane to prevent seeing portal backside
- [ ] Test with various portal orientations

**Matrix Calculation**:
```cpp
glm::mat4 calculatePortalView(const Portal& source, const Portal& dest, 
                               const glm::vec3& playerPos, 
                               const glm::mat4& playerView) {
    // Transform player position to source portal space
    glm::mat4 sourceTransform = source.getTransform();
    glm::mat4 destTransform = dest.getTransform();
    
    // Calculate relative transform
    glm::mat4 relativeTransform = destTransform * glm::inverse(sourceTransform);
    
    // Apply to player view
    return relativeTransform * playerView;
}
```

---

### Step 3.4: Portal Transparency & Blending
**Goal**: Make portals look like windows to another location

**Tasks**:
- [ ] Enable alpha blending for portal surfaces
- [ ] Render portal texture with transparency
- [ ] Add portal edge effects (optional):
  - Rim lighting
  - Distortion effects
  - Particle effects
- [ ] Handle depth testing correctly (portals should occlude objects behind them)

**Blending Setup**:
```cpp
glEnable(GL_BLEND);
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
glBlendEquation(GL_FUNC_ADD);
```

---

## Phase 4: Mirror System

### Step 4.1: Mirror Surface Rendering
**Goal**: Create mirror surfaces that reflect the scene

**Tasks**:
- [ ] Create `Mirror.h` class:
  - Position and normal
  - Mirror surface mesh
  - Mirror framebuffer
  - Reflection matrix calculation
- [ ] Implement reflection matrix:
  - Reflect camera position across mirror plane
  - Reflect camera orientation
- [ ] Render mirror view to framebuffer
- [ ] Apply mirror texture to mirror surface

**Reflection Matrix**:
```cpp
glm::mat4 calculateReflectionMatrix(const glm::vec3& normal, const glm::vec3& point) {
    glm::mat4 reflection = glm::mat4(1.0f);
    reflection[0][0] = 1.0f - 2.0f * normal.x * normal.x;
    reflection[1][1] = 1.0f - 2.0f * normal.y * normal.y;
    reflection[2][2] = 1.0f - 2.0f * normal.z * normal.z;
    // ... (full reflection matrix calculation)
    return reflection;
}
```

---

### Step 4.2: Mirror Integration
**Goal**: Integrate mirrors into rendering pipeline

**Tasks**:
- [ ] Render mirrors in scene:
  - Bind mirror framebuffer
  - Calculate reflection view matrix
  - Render scene from reflected camera
  - Unbind framebuffer
  - Render mirror surface with mirror texture
- [ ] Handle mirror visibility culling
- [ ] Add mirror materials (slightly reflective surface)
- [ ] Optimize: Only update mirrors when camera moves

**Rendering Order**:
1. Render scene normally
2. For each visible mirror:
   - Render reflection to framebuffer
   - Render mirror surface with reflection texture

---

## Phase 5: Scene Integration & Optimization

### Step 5.1: Tunnel/Labyrinth Structure
**Goal**: Create tunnel system with portals and doors

**Tasks**:
- [ ] Design tunnel layout system:
  - Tunnel segments
  - Junction points
  - Portal placement points
  - Door placement points
- [ ] Create tunnel mesh generation:
  - Cylindrical or rectangular tunnels
  - Wall, floor, ceiling geometry
  - Texture mapping
- [ ] Add tunnel navigation system
- [ ] Place portals and doors in tunnel network

**Tunnel Structure**:
- Tunnel segments (straight, curves, junctions)
- Portal nodes (where portals can be placed)
- Door nodes (where doors are placed)
- Room nodes (larger spaces)

---

### Step 5.2: Portal-Door Interaction
**Goal**: Ensure portals and doors work together

**Tasks**:
- [ ] Handle portals through open doors
- [ ] Handle portals blocked by closed doors
- [ ] Update portal visibility based on door state
- [ ] Test edge cases:
  - Portal on door surface
  - Portal showing through closing door
  - Multiple portals in same area

---

### Step 5.3: Performance Optimization
**Goal**: Optimize portal rendering performance

**Tasks**:
- [ ] Portal culling:
  - Only render portals in camera frustum
  - Skip portals behind camera
  - Skip portals too far away
- [ ] Framebuffer optimization:
  - Use lower resolution for distant portals
  - Share framebuffers between portals (if not visible simultaneously)
  - Reduce framebuffer updates (only when scene changes)
- [ ] LOD system:
  - Lower detail for portal views
  - Reduce draw distance in portal views
- [ ] Occlusion culling:
  - Don't render portals occluded by walls
- [ ] Portal recursion limits:
  - Maximum recursion depth (e.g., 2-3 levels)
  - Skip rendering if recursion limit reached

**Optimization Strategies**:
- Portal view resolution scaling based on distance
- Temporal caching (reuse portal framebuffer if scene unchanged)
- Frustum culling for portal cameras
- Early exit if portal not visible

---

## Phase 6: Polish & Effects

### Step 6.1: Portal Visual Effects
**Goal**: Enhance portal appearance

**Tasks**:
- [ ] Portal edge effects:
  - Rim lighting/shader
  - Distortion shader
  - Particle effects around edges
- [ ] Portal transition effects:
  - Warp/distortion when looking through
  - Color tinting
  - Screen-space effects
- [ ] Portal sound effects (if audio system exists)
- [ ] Portal interaction feedback

---

### Step 6.2: Door Visual Effects
**Goal**: Enhance door appearance and feedback

**Tasks**:
- [ ] Door opening/closing sound effects
- [ ] Door animation easing (smooth acceleration/deceleration)
- [ ] Door visual feedback:
  - Highlight when in range
  - Particle effects
  - Light changes
- [ ] Door interaction prompts (optional UI)

---

### Step 6.3: Mirror Polish
**Goal**: Enhance mirror realism

**Tasks**:
- [ ] Mirror surface shader:
  - Slight reflection blur
  - Fresnel effect
  - Surface imperfections
- [ ] Mirror frame/edge rendering
- [ ] Mirror interaction (optional: breakable mirrors)

---

## Phase 7: Testing & Debugging

### Step 7.1: Unit Testing
**Goal**: Test individual components

**Tasks**:
- [ ] Test portal matrix calculations
- [ ] Test door proximity detection
- [ ] Test door animations
- [ ] Test framebuffer rendering
- [ ] Test portal view rendering

---

### Step 7.2: Integration Testing
**Goal**: Test complete system

**Tasks**:
- [ ] Test portal pairs in simple scene
- [ ] Test doors in tunnel
- [ ] Test portals and doors together
- [ ] Test mirrors in scene
- [ ] Test edge cases:
  - Player inside portal
  - Portal showing portal
  - Multiple portals visible
  - Doors blocking portals

---

### Step 7.3: Performance Testing
**Goal**: Ensure acceptable performance

**Tasks**:
- [ ] Profile portal rendering
- [ ] Measure framebuffer overhead
- [ ] Test with multiple portals
- [ ] Test with multiple mirrors
- [ ] Optimize bottlenecks

---

## Implementation Priority

### High Priority (Core Functionality)
1. Portal data structures (Step 1.1)
2. Framebuffer system (Step 1.2)
3. Portal camera system (Step 1.3)
4. Portal surface rendering (Step 3.1)
5. Portal view rendering (Step 3.2)

### Medium Priority (Essential Features)
6. Door proximity detection (Step 2.1)
7. Door animation (Step 2.2)
8. Portal view matrix calculation (Step 3.3)
9. Portal transparency (Step 3.4)
10. Tunnel structure (Step 5.1)

### Low Priority (Polish)
11. Mirror system (Phase 4)
12. Visual effects (Phase 6)
13. Advanced optimizations (Step 5.3)

---

## Technical Considerations

### Rendering Order
1. Render main scene
2. Render portal views (to framebuffers)
3. Render mirror views (to framebuffers)
4. Render portal surfaces (with framebuffer textures)
5. Render mirror surfaces (with framebuffer textures)
6. Render UI/overlays

### Stencil Buffer Usage
- Use stencil buffer to mask portal surfaces
- Prevents rendering outside portal bounds
- Improves visual quality

### Depth Handling
- Portal surfaces need proper depth testing
- Portal views need correct depth values
- Handle depth conflicts between portal and main scene

### Coordinate Systems
- World space: Main scene coordinates
- Portal space: Relative to portal surface
- Portal view space: Transformed view through portal

---

## Dependencies

### Existing Engine Components
- `Camera` class (for view matrices)
- `Shader` class (for rendering)
- `Texture` class (for framebuffer textures)
- `Object` class (for portal/door meshes)
- `Scene` class (for scene management)

### New Components Needed
- Framebuffer management
- Portal camera calculations
- Door animation system
- Portal rendering pipeline

---

## Notes for AI Agent

### When Implementing:
1. **Start with simple test cases**: Single portal pair in empty scene
2. **Test incrementally**: Add one feature at a time
3. **Use debug rendering**: Draw portal normals, bounds, etc.
4. **Profile early**: Identify performance bottlenecks
5. **Handle edge cases**: Player inside portal, portal behind camera, etc.

### Common Pitfalls:
- **Incorrect matrix math**: Double-check portal transformations
- **Framebuffer leaks**: Always clean up framebuffers
- **Depth conflicts**: Ensure proper depth testing setup
- **Performance**: Multiple portals can be expensive, optimize early
- **Recursive portals**: Set recursion limits to prevent infinite loops

### Debugging Tips:
- Render portal camera frustum for debugging
- Display portal framebuffer texture on screen (debug view)
- Log portal transformations
- Visualize portal bounds/normals
- Profile framebuffer operations

---

## Success Criteria

### Minimum Viable Product (MVP)
- [ ] Single portal pair working
- [ ] Portal shows correct view of other location
- [ ] Doors open/close based on proximity
- [ ] Basic tunnel structure

### Complete System
- [ ] Multiple portal pairs working
- [ ] Recursive portals (portals showing portals)
- [ ] Smooth door animations
- [ ] Mirror system working
- [ ] Performance optimized
- [ ] Visual polish applied

---

## Estimated Timeline

- **Phase 1** (Foundation): 2-3 days
- **Phase 2** (Doors): 1-2 days
- **Phase 3** (Portals): 3-4 days
- **Phase 4** (Mirrors): 1-2 days
- **Phase 5** (Integration): 2-3 days
- **Phase 6** (Polish): 2-3 days
- **Phase 7** (Testing): 1-2 days

**Total**: ~12-19 days for complete implementation

---

## References & Resources

### Portal Rendering Techniques
- Portal rendering in Source engine
- Stencil buffer portal techniques
- Recursive portal rendering

### Matrix Mathematics
- 3D transformations
- Reflection matrices
- View matrix calculations

### OpenGL Techniques
- Framebuffer objects (FBOs)
- Stencil buffer usage
- Depth testing
- Alpha blending

---

*Last Updated: [Current Date]*
*Version: 1.0*

