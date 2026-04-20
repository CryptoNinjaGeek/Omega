# Omega Completion Plan

Plan to close the gaps identified in the engine: clipping, culling, the doorway-redesign cutover, doors, traversal, world generation, physics, and tests. Phases are ordered so earlier work unblocks later work; within a phase the steps are roughly independent.

## Guiding principles

- **Finish the doorway cutover before adding new subsystems.** Doors, traversal, and tunnels all assume the new standalone-`Portal` model. Keep `PortalPair` only until the new path is proven, then delete it in one commit.
- **Visuals first, gameplay second.** A portal that renders wrong is worse than one you can't walk through. Clipping + culling + nested recursion must be correct before teleport.
- **Test the math, not the rendering.** View matrices, reflection, clipping-plane math, portal-plane crossing, and transform composition are all unit-testable without a GL context. Integration/visual correctness is checked by rendering small fixtures and diffing screenshots.
- **Every new class gets a header-level doc block** explaining its role, ownership, and lifetime — the current portal classes do this well and it should stay consistent.

## Phase 0 — Foundation & hygiene

**Goal: remove dev-state noise and make future work testable.**

### 0.1 Strip debug instrumentation from PortalRenderer

`Engine/src/geometry/PortalRenderer.cpp` has ~10 `static int` counter-gated `std::cout` blocks and repeated `glGetError` polls. Replace with a single, togglable logger (spdlog is already in the dependency graph).

Concretely:
- Add `Engine/include/system/Log.h` wrapping `spdlog::logger` with one logger per subsystem (`portal`, `scene`, `shader`).
- Replace `std::cout` / `std::cerr` in `PortalRenderer`, `PortalCamera`, `Scene`, `Shader` with `OMEGA_LOG_DEBUG("portal", ...)`. Default level = `info`; debug is compile-time or env-controlled.
- Keep `glGetError` polls but move them behind a single `OMEGA_GL_CHECK()` macro that no-ops in release.

### 0.2 Propagate camera parameters to portal camera

Kill the hardcoded `45°, 0.1, 100` in `PortalRenderer::renderPortalView`. Add FOV/near/far storage + accessors to `Camera`:

```cpp
// render/Camera.h
float fov() const { return fov_; }
float nearPlane() const { return near_; }
float farPlane() const { return far_; }
```

Then in `renderPortalView`, the portal camera inherits `playerCamera->fov()`, `nearPlane()`, `farPlane()` and only the aspect ratio comes from the framebuffer. Remove the comment-TODO on lines 175–184.

### 0.3 Make the portal shader a first-class resource

`PortalRenderer::renderPortalSurface` currently tries four file paths before giving up. Replace with a single resolution policy:

- Portal shaders live under `:/shaders/portal.{vs,fs}` inside `resources.zip`. Always.
- Loading fails loudly on missing shaders — no silent fallbacks to `core.vs`.
- The `defaultPortalShader` static is moved into `PortalRenderer` as a member and loaded once in `PortalRenderer()` constructor, not lazily.

### 0.4 Test scaffolding

Add Google Test via `FetchContent_Declare(googletest …)` to the top-level `CMakeLists.txt`. Create `Engine/tests/CMakeLists.txt` with an `oEngineTests` executable, wired to `add_test()` / CTest. Initial targets:
- `tests/math/PortalTransformTest.cpp`
- `tests/math/PortalCameraTest.cpp`
- `tests/math/ClippingPlaneTest.cpp`

These don't need GL — they test pure GLM math. CI (Phase 7) runs them on every push.

**Exit criteria for Phase 0:** no `std::cout` in engine code, portal camera inherits projection from player camera, shader loads from one path, `ctest` runs (even if it only has three green tests).

---

## Phase 1 — Correct portal rendering

**Goal: what you see through a portal is actually correct.**

### 1.1 Clipping plane plumbed into shaders

`PortalCamera::getClippingPlane` returns `vec4` but nothing uses it. Fix end-to-end:

- Add `uniform vec4 clippingPlane; uniform bool enableClipping;` to `core.vs` (the main scene shader used when rendering into a portal FBO).
- In the vertex shader, emit `gl_ClipDistance[0] = enableClipping ? dot(vec4(worldPos, 1.0), clippingPlane) : 1.0;`
- Before `scene->render(portalCamera)` in `renderPortalView`: `glEnable(GL_CLIP_DISTANCE0)`, set both uniforms on the mesh shader, then render, then `glDisable(GL_CLIP_DISTANCE0)`.
- The clipping plane is the destination portal's plane with the normal facing *into* the destination space (so objects behind the destination portal get clipped).

### 1.2 Real frustum culling against portal corners

`PortalCamera::isInViewFrustum` is a dot-product + distance heuristic. Replace with a proper 6-plane frustum test:

- Add `Camera::getFrustumPlanes()` returning `std::array<glm::vec4, 6>` extracted from `projection * view` (Gribb-Hartmann).
- `PortalCamera::isInViewFrustum(portal, camera)` tests all four portal corners against all six planes; if all corners are outside one plane → culled.

### 1.3 Object-level culling through portal bounds

Currently `scene->render(portalCamera)` draws everything. For each object we need a cheap bounds test against the portal camera's frustum.

- Add `Object::boundingSphere()` (center + radius) computed lazily from mesh vertices.
- `Scene::render(camera)` iterates objects and skips those whose bounding sphere is fully outside the camera's frustum.
- This benefits *all* rendering, not just portal rendering, so it lives in `Scene::render`, not in `PortalRenderer`.

### 1.4 Nested-portal rendering strategy — decide and implement

Two options, pick one:

**A. FBO-per-recursion (current approach, finished).** Each recursion level renders into its own FBO. Requires an FBO pool keyed on (portal, depth). Deepest level renders without portals (base case). Simpler, fits existing code, reasonable perf up to depth 3.

**B. Stencil-based.** Clear stencil, draw portal quad to stencil, render destination scene with stencil test, repeat recursively with increment/decrement. Much better perf, single FBO, but requires rewriting the render loop.

**Recommendation: A**, because the FBO infrastructure already exists, depth limit is 3, and correctness is easier to verify. Revisit B only if performance becomes a problem.

Implementation for A:
- `PortalRenderer` gains `std::vector<std::shared_ptr<PortalFramebuffer>> fboPool_` sized to `maxRecursionDepth_ * N_portals`.
- `renderPortalViewRecursive` picks the next free FBO by `(portal, depth)` and recurses, clipping applied at each level.

### 1.5 Mirror overlay in fragment shader

`Portal::hasMirrorOverlay_` and `mirrorIntensity_` are stored but `portal.fs` ignores them. Extend:

```glsl
uniform bool  hasMirrorOverlay;
uniform float mirrorIntensity;
uniform vec3  mirrorTint;  // default (1,1,1)

void main() {
    vec4 base = texture(portalTexture, TexCoords);
    if (hasMirrorOverlay) {
        base.rgb = mix(base.rgb, base.rgb * mirrorTint, mirrorIntensity);
    }
    FragColor = base;
}
```

`PortalRenderer::renderPortalSurface` sets the uniforms from the `Portal`.

### 1.6 Scene-loader parity

`PortalSceneLoader` must read: `destinationId`, `isOpen`, `isPassable`, `hasMirrorOverlay`, `mirrorIntensity`. Update `PortalSceneFormat.md` to document the new fields. Update `tunnel_scene.json` and `rooms_scene.json` to use the new schema — delete any `portalPairs` section once standalone works.

**Exit criteria for Phase 1:** walking up to a portal in the Rooms demo shows a correctly clipped, correctly framed view of the destination room; a self-linked portal renders as a mirror with optional tint; nested portals up to depth 3 render without visual artifacts.

---

## Phase 2 — Retire the legacy path

**Goal: one portal model in the codebase.**

### 2.1 Delete PortalPair

Once every demo uses standalone portals with `destinationId`, delete `PortalPair.h/cpp` and every reference:
- `PortalRenderer::addPortalPair`, `portalPairs_` member, both render loops that iterate it.
- `CMakeLists.txt` entries.
- `Portal::linkedPortal_` and the legacy `setLinkedPortal`/`getLinkedPortal`/`calculatePortalView(src, dst)` helpers.

One PR, one commit.

### 2.2 Consolidate PortalCamera

After 2.1, `PortalCamera::calculatePortalView(source, dest)` becomes unused. Inline what's worth keeping into `calculateDoorwayView` and delete the rest. The public surface becomes:

- `calculatePortalViewUnified(camera, portal)` — the only entry point
- `getClippingPlane(portal)`
- `isPortalVisible(portal, camera)` + `isInViewFrustum(portal, camera)`

Everything else is private.

**Exit criteria:** no grep hit for `PortalPair`, `linkedPortal`, or the legacy triple-arg `calculatePortalView` anywhere in the tree.

---

## Phase 3 — Doors

**Goal: portals animate open/closed, triggered by proximity.**

A door = a `Portal` with (a) a visible `Object` (the door mesh) whose rotation is driven by the portal's open state, and (b) a proximity trigger. No separate `Door` class — the doorway redesign folds this into `Portal`.

### 3.1 Animated state on Portal

Add to `Portal`:

```cpp
void setOpenTarget(bool open, float durationSeconds = 0.5f);
void update(float deltaSeconds);  // drives openAmount_
float openAmount() const { return openAmount_; }  // 0 = closed, 1 = open
```

Internal state: `openAmount_`, `openTarget_`, `openSpeed_`. `isOpen()` returns `openAmount_ > 0.99f`, so renderer culling sees the transition instantly when the player triggers it.

### 3.2 Door visual — attached Object

`Portal` gets an optional `std::shared_ptr<Object> doorObject_` + a rotation axis/angle range. In `Portal::update`, the door object's `model_` is rotated by `lerp(closedAngle, openAngle, openAmount_)` around the hinge.

In `Scene::process(dt)`, every portal gets `portal->update(dt)`.

### 3.3 Proximity triggers

Add `Engine/include/geometry/Trigger.h`:

```cpp
class Trigger {
  glm::vec3 center_;
  float radius_;
  std::function<void(bool entered)> callback_;
  bool wasInside_{false};
public:
  void update(const glm::vec3& playerPos);
};
```

`Scene` owns a `std::vector<Trigger>`. A door registers a trigger in its constructor that flips `portal->setOpenTarget(true, 0.5f)` on enter and `false` on exit.

### 3.4 JSON schema for doors

Extend scene JSON:

```json
{
  "portals": [{
    "id": "door_01",
    "destinationId": "door_02",
    "door": {
      "mesh": ":/models/door.obj",
      "hinge": [0, 0, -1],
      "axis": [0, 1, 0],
      "closedAngle": 0,
      "openAngle": 90,
      "trigger": { "radius": 2.0 }
    }
  }]
}
```

`PortalSceneLoader` parses this and wires up the door object + trigger.

**Exit criteria:** in the Rooms demo, walking toward a door smoothly swings it open; walking away closes it; the portal is un-renderable while the door is not fully open (optional — gameplay choice).

---

## Phase 4 — Traversal / teleport

**Goal: the player actually walks through the portal.**

### 4.1 Portal-plane crossing detection

Per frame, for each open + passable portal, compare the player's previous-frame position against this-frame position against the portal plane. If the player crossed from the front hemisphere to the back hemisphere AND the crossing point lies within the portal's width/height → the player has traversed.

Add `Portal::didCross(const glm::vec3& prev, const glm::vec3& curr, glm::vec3* intersection)`.

### 4.2 Camera teleport through destination

On crossing, apply the portal-to-destination transform to the camera:

- Position: `destPortal->getPosition() + destRotation * (sourceRotation^-1 * (playerPos - sourcePortal->getPosition()))` — exactly the math already in `calculatePortalView`, extracted into a helper `Portal::transformPoint(const glm::vec3&)`.
- Orientation: apply the same relative rotation to the camera's yaw/pitch. For `CameraFPS`, this needs to decompose the rotation back into yaw/pitch rather than just overwriting a quaternion.

Important gotcha: the player needs to be nudged slightly *past* the destination portal's plane after teleport, or next frame's crossing test fires again and they teleport back.

### 4.3 Hook into input loop

`Scene::process(dt)` iterates portals, calls `didCross` against the current camera, and teleports on the first match. Disable teleport on portals where `isPassable_` is false — those render the view but don't let you through.

### 4.4 Mirror portals don't teleport

A portal whose destination is itself (`isMirror()`) never teleports, regardless of `isPassable_`. Explicit check in the crossing loop.

**Exit criteria:** player walks through a doorway in Rooms demo → seamless traversal, camera ends up correctly positioned + oriented in the destination room, no "teleport loop" stutter.

---

## Phase 5 — Tunnel / labyrinth generator

**Goal: procedurally build a connected multi-room space.**

### 5.1 Grid-graph generator

Add `Engine/include/utils/LabyrinthGenerator.h`:

```cpp
struct RoomCell { int x, z; std::array<bool, 4> doors; };  // N, E, S, W
class LabyrinthGenerator {
  std::vector<RoomCell> generate(int width, int height, uint32_t seed);
};
```

Algorithm: randomized DFS maze generator on a `width × height` grid. Each cell becomes a room; removed walls become door connections.

### 5.2 RoomBuilder — cell → geometry

Given a vector of `RoomCell`, emit:
- One floor quad + one ceiling quad per cell.
- One wall quad per cell edge that doesn't have a door.
- One `Portal` per cell edge that *does* have a door, with `destinationId` pointing at the adjacent cell's portal.

Output is a `Scene` directly, or a serialized JSON consumable by `PortalSceneLoader`.

### 5.3 Demo integration

Replace `Demo/Rooms/rooms_scene.json` generation step with a `--generate width height seed` CLI flag on the demo that calls `LabyrinthGenerator` + `RoomBuilder` at startup.

**Exit criteria:** running `./bin/Rooms --generate 5 5 42` produces a walkable 5×5 labyrinth with doors, consistent across runs with the same seed.

---

## Phase 6 — Physics integration

**Goal: reactphysics3d actually drives something.**

### 6.1 Collider component on Object

Add `Engine/include/system/Collider.h` — a thin wrapper around `reactphysics3d::CollisionBody` + a shape (box/sphere/mesh). `Object` gains `std::shared_ptr<Collider> collider_`.

Scene JSON schema gains optional `"collider": { "type": "box", "size": [1,1,1] }` per object.

### 6.2 Scene physics step

`Scene::process(float dt)` calls `physics_world_->update(dt)` and then syncs each collider's transform back to its `Object::model_`.

### 6.3 Player capsule

The player (camera) gets a capsule collider. `CameraFPS` movement becomes: compute desired velocity from input → apply to rigid body → let physics resolve → read back world-space position.

This gives you wall collision for free, which is what you want for the labyrinth.

### 6.4 Portal-aware physics

When a rigid body crosses a portal, its physics body teleports too. Same math as camera teleport in 4.2, applied to `rp3d::RigidBody::setTransform`.

**Exit criteria:** player can't walk through walls in the Rooms demo; can still walk through passable portals; physics runs at 60 Hz without spikes.

---

## Phase 7 — Tests, CI, polish

**Goal: regressions get caught, the code is release-grade.**

### 7.1 Unit tests (no GL context)

- `PortalTransformTest`: `getTransform()` produces a right-handed basis; `getCorners()` returns four coplanar points at expected offsets.
- `PortalCameraTest`: `calculateDoorwayView` composed with `calculateDoorwayView` of the destination portal ≈ identity (round-trip invariant); `calculateMirrorView` applied twice ≈ identity.
- `ClippingPlaneTest`: plane equation math for known portal positions/normals.
- `TraversalTest`: `didCross` returns true for straight-through crossings and false for parallel movement.
- `LabyrinthGeneratorTest`: same seed → same output; every cell reachable from every other cell.

### 7.2 Headless scene-loader tests

Mock the GL calls (or run against a hidden GLFW context). Load each demo scene JSON, assert expected portal counts, destinations, object counts.

### 7.3 Golden-image rendering tests

Create `Engine/tests/visual/` with small fixtures. Render to an offscreen FBO, read pixels, compare against a stored reference PNG with a tolerance (e.g. `libpng` + per-pixel diff ≤ 2%). Regenerate references with a `--update-golden` flag.

### 7.4 CI

`.github/workflows/ci.yml` builds on Linux + macOS, runs `ctest`. Add `clang-format --dry-run --Werror` using the existing `.clang-format`.

### 7.5 Documentation refresh

- Replace the current `PORTAL_ENGINE_STATUS.md` with auto-generated status from this plan's phase completion.
- Add `Engine/README.md` documenting how to link against `oEngine` in another project.
- Add architecture diagram for the portal rendering flow (Mermaid inside the README).

**Exit criteria:** `ctest` green on both platforms in CI, golden-image tests pass, `clang-format` is clean.

---

## Dependency graph

```
Phase 0 (foundation)
  ├── Phase 1 (rendering correctness)
  │     └── Phase 2 (retire legacy)
  │           ├── Phase 3 (doors)
  │           │     └── Phase 4 (traversal)
  │           │           └── Phase 6 (physics)
  │           └── Phase 5 (labyrinth)
  └── Phase 7 (tests, CI) — runs in parallel from 0.4 onward
```

Phase 7's test scaffolding (0.4) comes early so every later phase contributes tests as it lands.

## Estimated effort

Rough ranges assuming one focused engineer:

| Phase | Work | Rough effort |
|-------|------|---------------|
| 0     | Logging, projection propagation, shader cleanup, test setup | 2–3 days |
| 1     | Clipping, frustum, object culling, nested FBO, mirror overlay | 5–7 days |
| 2     | Legacy removal | 1 day |
| 3     | Door animation + triggers | 3–4 days |
| 4     | Traversal + teleport | 2–3 days |
| 5     | Labyrinth generator + room builder | 3–4 days |
| 6     | Physics integration | 4–6 days |
| 7     | Tests, CI, polish | 3–5 days |

Total: ~4–5 weeks of solo work to go from current state to "feature-complete 3D portal engine."

## Risk register

- **Nested clipping correctness.** The 6-plane frustum + clipping plane interaction is subtle. Mitigation: thorough golden-image tests on nested-portal scenes before declaring Phase 1 done.
- **Camera-orientation teleport math.** Decomposing an arbitrary rotation back into `CameraFPS` yaw/pitch loses roll. Mitigation: constrain portals to vertical planes initially (no ceiling-floor portals). Revisit for full 6-DOF later.
- **Physics-portal seams.** Rigid bodies straddling a portal plane during a physics substep can tunnel or duplicate. Mitigation: use reactphysics3d's continuous collision detection; teleport only between physics steps, not mid-step.
- **Labyrinth × portal performance.** A 10×10 labyrinth has up to 180 doors. Even with culling, nested-portal rendering can blow recursion budgets. Mitigation: raise `maxRecursionDepth_` only when justified; add per-portal LOD (skip rendering destination view past distance X, show static texture).

## What this plan deliberately does *not* cover

- **Stencil-based portal rendering.** Documented as an option in 1.4 but not planned. Revisit only if FBO approach hits a perf ceiling.
- **Shadow mapping through portals.** Portals currently don't affect lights — a shadow-casting light visible through a portal won't cast into the destination room. Noted as a potential future phase.
- **Networked multiplayer.** Out of scope.
- **Audio propagation through portals.** `audeo` is a dependency but unused. A separate future phase.
- **Editor tooling.** Portal placement is JSON-authored. A visual editor is a separate project.
