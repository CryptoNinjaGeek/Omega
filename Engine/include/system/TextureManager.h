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
