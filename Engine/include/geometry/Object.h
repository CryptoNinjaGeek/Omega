#pragma once

#include <system/Global.h>
#include <render/Material.h>
#include <interface/Entity.h>
#include <interface/Light.h>
#include <system/PhysicsObject.h>
#include <memory>
#include <vector>
#include <optional>

#include "glm/gtc/matrix_transform.hpp"
#include <reactphysics3d/reactphysics3d.h>

namespace omega {
namespace render {
class Camera;
class Shader;
class Texture;
}  // namespace render
namespace geometry {

enum class ObjectType { Elements, Array };

// Local-space bounding sphere used for coarse view-frustum culling.
//
// The sphere is stored in the object's LOCAL (model-space) coordinates —
// Scene::render transforms it by the object's model matrix at cull time.
// This matches the pattern mesh generators can populate once at construction
// time (when vertex data is still in scope) without being invalidated when
// the object is later translated/rotated/scaled via `model_`.
struct BoundingSphere {
  glm::vec3 center{0.0f};
  float radius{0.0f};
};

class OMEGA_EXPORT Object : public interface::Entity {
public:
  Object(unsigned int vao, unsigned int vbo, unsigned int cnt,
		 ObjectType type = ObjectType::Array);
  Object();

  virtual void render(std::shared_ptr<render::Camera>);
  virtual auto setupPhysics(reactphysics3d::PhysicsWorld *, reactphysics3d::PhysicsCommon *) -> void;
  virtual auto process() -> void;

  auto debug(bool) -> void;

  void setName(std::string name) { name_ = name; }
  void setMaterial(render::Material material) { material_ = material; }
  // Attach an ordered list of Materials — used by multi-material meshes
  // (e.g. a splat-blended terrain with one Material per surface band).
  // Each Material binds itself to the shader under the uniform name
  // `materials[<i>]` via Material::apply(), so the bound shader has to
  // declare a matching `uniform MaterialSlot materials[N];` array. When
  // `materials_` is non-empty it overrides the legacy `material_` /
  // `textures_` binding path; both are still supported for existing
  // call sites (Box, Plane, Container, PortalSceneLoader, Castle demo)
  // that expect the old flat "material.*" + per-unit `texture0..N`
  // convention.
  void addMaterial(render::Material material) {
    materials_.push_back(std::move(material));
  }
  void setMaterials(std::vector<render::Material> materials) {
    materials_ = std::move(materials);
  }
  void clearMaterials() { materials_.clear(); }
  const std::vector<render::Material>& getMaterials() const {
    return materials_;
  }
  void setModel(glm::mat4x4 mat) { model_ = mat; }
  void setShader(std::shared_ptr<render::Shader> shader) { shader_ = shader; }
  // Read-only access to the bound shader, used by Scene::render to push
  // frame-wide uniforms (e.g. the portal clipping plane) before the object
  // sets its own model/view/projection and draws.
  std::shared_ptr<render::Shader> shader() const { return shader_; }
  void addTexture(std::shared_ptr<render::Texture> texture) {
	textures_.push_back(texture);
  };
  void setTextures(std::vector<std::shared_ptr<render::Texture>> textures) {
	textures_ = textures;
  };

  void affectedByLights(std::vector<std::shared_ptr<interface::Light>> lights) {
	lights_ = lights;
  }

  glm::vec3 entityPosition() { return glm::vec3(model_[3]); }

  auto name() -> std::string { return name_; }
  auto scale(float) -> void;
  auto position(glm::vec3) -> void;
  auto physics(physics::PhysicsObject physicsObject) -> void { physicsObject_ = physicsObject; }
  inline auto visible(bool val) -> void { visible_ = val; }
  inline auto visible() -> bool { return visible_; }
  
  // Getters for portal rendering
  unsigned int getVAO() const { return vao_; }
  unsigned int getCount() const { return count_; }
  ObjectType getType() const { return type_; }
  glm::mat4 getModel() const { return model_; }

  // Additional accessors used by demos to clone a loaded mesh into many
  // instances that share VAO/VBO but carry per-instance transforms. Object
  // does not own its GL handles (no destructor frees them), so multiple
  // Objects may safely point at the same VAO. See Demo/Basic forest.
  unsigned int getVBO() const { return vbo_; }
  const std::vector<std::shared_ptr<render::Texture>>& getTextures() const {
    return textures_;
  }
  std::optional<render::Material> getMaterial() const { return material_; }

  // Bounding-sphere API used by Scene::render for per-object frustum culling.
  //
  // Populated by mesh generators (Box, Plane, Container, Mesh, …) immediately
  // after the vertex data is uploaded to the GPU — the sphere is derived from
  // those same vertices and stored in local space. Objects without a sphere
  // set (SkyBox, loaded assimp sub-meshes that we haven't measured yet) are
  // never culled, which is the conservative default.
  void setBoundingSphere(const glm::vec3& center, float radius) {
    boundingSphere_ = BoundingSphere{center, radius};
  }
  void setBoundingSphere(const BoundingSphere& sphere) {
    boundingSphere_ = sphere;
  }
  void clearBoundingSphere() { boundingSphere_.reset(); }
  const std::optional<BoundingSphere>& boundingSphere() const {
    return boundingSphere_;
  }

  // Return the bounding sphere transformed into world space by `model_`.
  // The radius is scaled by the longest of the three scale factors encoded
  // in the model matrix columns — a conservative choice that keeps the
  // sphere fully enclosing under non-uniform scale (bounded by the largest
  // axis). Returns std::nullopt if no local sphere was set.
  std::optional<BoundingSphere> worldBoundingSphere() const;

protected:
  std::string name_;
  unsigned int vao_;
  unsigned int vbo_;
  unsigned int count_;
  ObjectType type_{ObjectType::Array};

  reactphysics3d::RigidBody *body_{nullptr};
  physics::PhysicsObject physicsObject_;
  reactphysics3d::Collider *collider_{nullptr};

  bool visible_{true};
  glm::mat4 model_;
  std::optional<render::Material> material_;
  // Multi-material slots. Emptiness is the switch between paths:
  //   empty         → legacy binding: `material_` → flat `material.*` uniforms,
  //                   `textures_` → per-unit `texture0..N` samplers.
  //   non-empty     → each entry calls Material::apply under prefix
  //                   "materials[<i>]", starting at texture unit 0 and
  //                   advancing by Material::kApplyUnitCount per slot. The
  //                   bound shader must declare a matching array.
  std::vector<render::Material> materials_;
  std::shared_ptr<render::Shader> shader_;
  std::vector<std::shared_ptr<render::Texture>> textures_;
  std::vector<std::shared_ptr<interface::Light>> lights_;
  std::optional<BoundingSphere> boundingSphere_{};
};
}  // namespace geometry
}  // namespace omega
