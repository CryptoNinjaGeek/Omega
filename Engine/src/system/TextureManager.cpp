#include <system/TextureManager.h>
#include <render/Texture.h>
#include <iostream>
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
  if (std::find(_paths.begin(), _paths.end(), path)!=_paths.end()) {
	_paths.push_back(path);
	return true;
  }
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
