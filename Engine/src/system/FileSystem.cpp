#include <system/FileSystem.h>
#include <system/Log.h>
#include <fstream>
#include <optional>

using namespace omega::fs;

static std::shared_ptr<FileSystem> _manager;

std::shared_ptr<FileSystem> omega::fs::instance() {
  if (!_manager) {
	_manager = std::make_shared<FileSystem>();
  }
  return _manager;
}

auto FileSystem::add(std::string record) -> bool {
  if (record.ends_with(".zip"))
	return addZipFile(record);
  else
	_paths.push_back(record);
  return true;
}
auto FileSystem::addZipFile(std::string file) -> bool {
  struct zip *za;
  struct zip_file *zf;
  struct zip_stat sb;
  char buf[100];
  int err;

  if ((za = zip_open(file.c_str(), 0, &err))==NULL) {
	OMEGA_LOG_ERROR("fs", "Unable to add file => {}", file);
	return false;
  }

  if (verbose_)
	OMEGA_LOG_DEBUG("fs", "==== zip contents ({}) ====", file);
  for (int i = 0; i < zip_get_num_entries(za, 0); i++) {
	if (zip_stat_index(za, i, 0, &sb)==0 &&
		!std::string(sb.name).ends_with("/")) {
	  if (verbose_) {
		OMEGA_LOG_DEBUG("fs", "Name=[:/{}] Size={} mtime={}", sb.name, sb.size,
						static_cast<unsigned int>(sb.mtime));
	  }

	  _zipFiles[std::string(":/") + sb.name] =
		  ZipFile{.za = za, .index = i, .size = (unsigned int)sb.size};
	}
  }
  return true;
}

namespace {

// Try to read a file from disk at `path`. Returns an empty optional if the
// file doesn't exist, can't be opened, or is empty — an empty file is
// indistinguishable from "missing" for shader loading and we'd rather keep
// searching candidate paths than silently load empty source.
std::optional<std::string> readDiskFile(const std::string &path) {
  std::ifstream in(path);
  if (!in.good()) return std::nullopt;
  std::string s((std::istreambuf_iterator<char>(in)),
                std::istreambuf_iterator<char>());
  if (s.empty()) return std::nullopt;
  return s;
}

}  // namespace

auto FileSystem::string(std::string file) -> std::string {
  std::string result;

  if (file.starts_with(":/")) {
	// Disk overlay: the shipping `resources.zip` is not always in sync with
	// the source tree (e.g. portal shaders are missing and core.vs lags).
	// Let disk files *override* the zip so developers can iterate on shaders
	// (and other assets) without rebundling a 400 MB archive. Zip remains
	// the fallback when no overlay exists.
	//
	// Layouts tried for `:/shaders/foo.vs`:
	//   ./shaders/foo.vs            (preserved relative path)
	//   ./resources/shaders/foo.vs  (older bin/resources/shaders/ layout)
	//   ./foo.vs                    (flat bin/foo.vs layout — matches how
	//                                Demo/Portal CMake copies portal.vs)
	const std::string stripped = file.substr(2);  // drop leading ":/"
	std::string basename = stripped;
	if (auto slash = stripped.find_last_of('/');
	    slash != std::string::npos) {
	  basename = stripped.substr(slash + 1);
	}
	const std::string candidates[] = {
	    stripped,
	    "resources/" + stripped,
	    basename,
	};
	for (const auto &c : candidates) {
	  if (auto disk = readDiskFile(c)) {
	    return *disk;
	  }
	}

	if (!_zipFiles.contains(file))
	  return {};
	auto zip = _zipFiles[file];
	auto zf = zip_fopen_index(zip.za, zip.index, 0);
	if (!zf) {
	  return {};
	}

	char *buf = new char[zip.size + 1];
	memset(buf, 0, zip.size + 1);
	auto len = zip_fread(zf, buf, zip.size);
	if (len < 0) {
	  delete[] buf;
	  return {};
	}
	result = std::string(buf);
	delete[] buf;
	zip_fclose(zf);
  } else {
	std::ifstream in(file);
	if (!in.good())
	  return {};
	std::string str((std::istreambuf_iterator<char>(in)),
					std::istreambuf_iterator<char>());
	result = str;
  }
  return result;
}

unsigned int FileSystem::filesize(std::string filename) {
  std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
  if (!in.good())
	return 0;
  return (unsigned int)in.tellg();
}

auto FileSystem::extension(std::string fileName) -> std::string {
  return fileName.substr(fileName.find_last_of(".") + 1);
}

auto FileSystem::data(std::string file)
-> omega::system::ByteArray<unsigned char> {
  omega::system::ByteArray<unsigned char> result;

  if (file.starts_with(":/")) {
	if (!_zipFiles.contains(file))
	  return {};
	auto zip = _zipFiles[file];
	auto zf = zip_fopen_index(zip.za, zip.index, 0);
	if (!zf) {
	  return {};
	}

	result.setSize(zip.size);
	auto len = zip_fread(zf, result.data(), zip.size);
	if (len < 0) {
	  return {};
	}
	zip_fclose(zf);
  } else {
	auto size = filesize(file);
	if (!size)
	  return {};
	result.setSize(size);
	std::fstream fin{file, std::ifstream::binary | std::ifstream::in};
	fin.read(reinterpret_cast<char *>(result.data()), result.size());
  }
  return result;
}
