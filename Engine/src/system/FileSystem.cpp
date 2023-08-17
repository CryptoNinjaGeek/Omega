#include <system/FileSystem.h>
#include <iostream>

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
  struct zip* za;
  struct zip_file* zf;
  struct zip_stat sb;
  char buf[100];
  int err;

  if ((za = zip_open(file.c_str(), 0, &err)) == NULL) {
    std::cout << "Unable to add file => " << file << std::endl;
    return false;
  }

  std::cout << "==================" << std::endl;
  for (int i = 0; i < zip_get_num_entries(za, 0); i++) {
    if (zip_stat_index(za, i, 0, &sb) == 0 &&
        !std::string(sb.name).ends_with("/")) {
      std::cout << "Name: [:/" << sb.name << "] ";
      std::cout << "Size: " << sb.size << "] ";
      std::cout << "mtime: [" << (unsigned int)sb.mtime << "] " << std::endl;

      _zipFiles[std::string(":/") + sb.name] =
          ZipFile{.za = za, .index = i, .size = (unsigned int)sb.size};
    }
  }
}

auto FileSystem::string(std::string file) -> std::string {
  std::string result;

  if (file.starts_with(":/")) {
    if (!_zipFiles.contains(file)) return {};
    auto zip = _zipFiles[file];
    auto zf = zip_fopen_index(zip.za, zip.index, 0);
    if (!zf) {
      return {};
    }

    char* buf = new char[zip.size + 1];
    memset(buf, 0, zip.size + 1);
    auto len = zip_fread(zf, buf, zip.size);
    if (len < 0) {
      delete[] buf;
      return {};
    }
    result = std::string(buf);
    delete[] buf;
    zip_fclose(zf);
  }
  return result;
}

auto FileSystem::data(std::string file)
    -> omega::system::ByteArray<unsigned char> {
  omega::system::ByteArray<unsigned char> result;

  if (file.starts_with(":/")) {
    if (!_zipFiles.contains(file)) return {};
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
  }
  return result;
}
