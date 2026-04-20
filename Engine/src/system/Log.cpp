#include <system/Log.h>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <cstdlib>
#include <mutex>
#include <string>
#include <unordered_map>

namespace omega {
namespace system {

bool Log::initialized_ = false;

namespace {

std::mutex& loggers_mutex() {
  static std::mutex m;
  return m;
}

std::unordered_map<std::string, std::shared_ptr<spdlog::logger>>& loggers() {
  static std::unordered_map<std::string, std::shared_ptr<spdlog::logger>> m;
  return m;
}

spdlog::level::level_enum level_from_env() {
  const char* env = std::getenv("OMEGA_LOG_LEVEL");
  if (!env) return spdlog::level::info;
  std::string v(env);
  if (v == "trace") return spdlog::level::trace;
  if (v == "debug") return spdlog::level::debug;
  if (v == "info") return spdlog::level::info;
  if (v == "warn" || v == "warning") return spdlog::level::warn;
  if (v == "error") return spdlog::level::err;
  if (v == "off") return spdlog::level::off;
  return spdlog::level::info;
}

}  // namespace

void Log::init() {
  std::lock_guard<std::mutex> lock(loggers_mutex());
  if (initialized_) return;
  spdlog::set_pattern("[%H:%M:%S.%e] [%^%l%$] [%n] %v");
  spdlog::set_level(level_from_env());
  initialized_ = true;
}

void Log::setLevel(spdlog::level::level_enum level) {
  spdlog::set_level(level);
  std::lock_guard<std::mutex> lock(loggers_mutex());
  for (auto& [_, logger] : loggers()) {
    logger->set_level(level);
  }
}

std::shared_ptr<spdlog::logger> Log::get(const std::string& name) {
  if (!initialized_) init();
  std::lock_guard<std::mutex> lock(loggers_mutex());
  auto it = loggers().find(name);
  if (it != loggers().end()) return it->second;

  // spdlog registers loggers globally by name; reuse if already registered.
  auto existing = spdlog::get(name);
  if (existing) {
    loggers()[name] = existing;
    return existing;
  }
  auto logger = spdlog::stdout_color_mt(name);
  logger->set_level(level_from_env());
  loggers()[name] = logger;
  return logger;
}

}  // namespace system
}  // namespace omega
