#include <system/TextureManager.h>
#include <render/Texture.h>
#include <fstream>
#include <filesystem>

using namespace omega::system;
using namespace omega::render;

static std::shared_ptr<TextureManager> _manager;

std::shared_ptr<TextureManager> TextureManager::instance() {
  if (!_manager) {
	_manager = std::make_shared<TextureManager>();
  }
  return _manager;
}

auto TextureManager::load(std::string name) -> TexturePtr {
  auto texture = std::make_shared<Texture>();

  auto filename = locate(name);
  if (filename.empty())
	return nullptr;

  if (!texture->load(filename, name))
	return nullptr;

  _textures[name] = texture;

  return texture;
}

auto TextureManager::load(std::string name, const render::TextureSpec& spec)
	-> TexturePtr {
  // Build a cache key that disambiguates the same source file loaded with
  // different specs. Only the fields that actually change the GPU resource
  // participate — (filename, colorspace, forceChannels, wrap, filter,
  // mipmaps, flipY). The spec is encoded as a short suffix on `name` so
  // legacy single-arg lookups via `texture(name)` still find entries loaded
  // through the old path, and spec-aware loads never collide with them.
  std::string key = name;
  key += "#cs=";
  key += (spec.colorSpace == render::TextureSpec::ColorSpace::sRGB) ? "srgb"
																	: "lin";
  key += "|fc=" + std::to_string(spec.forceChannels);
  key += "|ws=" + std::to_string(static_cast<int>(spec.wrapS));
  key += "|wt=" + std::to_string(static_cast<int>(spec.wrapT));
  key += "|mn=" + std::to_string(static_cast<int>(spec.minFilter));
  key += "|mg=" + std::to_string(static_cast<int>(spec.magFilter));
  key += "|mip=" + std::to_string(spec.generateMipmaps ? 1 : 0);
  key += "|fly=" + std::to_string(spec.flipY ? 1 : 0);

  if (auto it = _textures.find(key); it != _textures.end()) {
	return it->second;
  }

  auto filename = locate(name);
  if (filename.empty())
	return nullptr;

  auto texture = std::make_shared<Texture>();
  if (!texture->load(filename, spec, name))
	return nullptr;

  _textures[key] = texture;
  return texture;
}

auto TextureManager::add(TexturePtr texture) -> bool {
  if (texture->name().empty())
	return false;

  _textures[texture->name()] = texture;
  return true;
}

auto TextureManager::texture(std::string name) -> TexturePtr {
  return _textures.contains(name) ? _textures[name] : nullptr;
}

auto TextureManager::path(std::string path) -> bool {
  if (std::find(_paths.begin(), _paths.end(), path)==_paths.end()) {
	_paths.push_back(path);
	return true;
  }
  return false;
}

auto TextureManager::locate(std::string name) -> std::string {
  if (std::filesystem::exists(name))
	return name;

  if (name.starts_with(":/"))
	return name;

  for (auto path : _paths) {
	if (std::filesystem::exists(path + "/" + name))
	  return path + "/" + name;
  }
  return "";
}
