#pragma once

#include <system/Global.h>
#include <system/ByteArray.h>
#include <memory>
#include <zip.h>
#include <map>

namespace omega {
namespace fs {

struct ZipFile {
  struct zip *za;
  int index;
  unsigned int size;
};

class OMEGA_EXPORT FileSystem {
public:
  FileSystem() = default;

  auto add(std::string) -> bool;
  auto string(std::string) -> std::string;
  auto data(std::string) -> omega::system::ByteArray<unsigned char>;
  unsigned int filesize(std::string);
  auto extension(std::string) -> std::string;
  inline auto verbose(bool) -> void { verbose_ = true; }
private:
  auto addZipFile(std::string) -> bool;

private:
  std::map<std::string, ZipFile> _zipFiles;
  std::vector<std::string> _paths;
  bool verbose_{false};
};

OMEGA_EXPORT std::shared_ptr<FileSystem> instance();

}  // namespace fs
}  // namespace omega
