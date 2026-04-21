#pragma once

#include <system/Global.h>
#include <system/ByteArray.h>
#include <render/Texture.h>
#include <memory>
#include <map>

namespace omega {
namespace system {

typedef std::shared_ptr<render::Texture> TexturePtr;

class OMEGA_EXPORT TextureManager {
public:
  TextureManager() = default;

  auto load(std::string) -> TexturePtr;
  // Spec-aware load. The same source file can legitimately be loaded with
  // different specs (e.g. the AO jpg sampled sRGB for one material and
  // linear for another). Caching by filename alone would hand the wrong
  // GPU copy back on the second call, so this overload keys by
  // filename+spec-signature internally.
  auto load(std::string name, const render::TextureSpec& spec) -> TexturePtr;
  auto add(TexturePtr) -> bool;
  auto texture(std::string) -> TexturePtr;
  auto path(std::string) -> bool;

  inline auto verbose(bool) -> void { verbose_ = true; }

  static std::shared_ptr<TextureManager> instance();

private:
  auto locate(std::string) -> std::string;

private:
  std::map<std::string, TexturePtr> _textures;
  std::vector<std::string> _paths;
  bool verbose_{false};
};

}  // namespace fs
}  // namespace omega
