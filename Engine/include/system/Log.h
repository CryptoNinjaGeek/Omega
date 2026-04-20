#pragma once

#include <system/Global.h>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <memory>
#include <string>

namespace omega {
namespace system {

/**
 * Log - Central logging subsystem.
 *
 * Provides per-subsystem spdlog loggers (portal, scene, shader, window, loader).
 * Default level is "info"; set OMEGA_LOG_LEVEL env var to "debug"/"trace"/"warn"
 * to change it, or call Log::setLevel() at runtime.
 *
 * Use the OMEGA_LOG_* macros rather than calling into spdlog directly so that
 * we keep consistent formatting and can redirect output later if needed.
 */
class OMEGA_EXPORT Log {
 public:
  static std::shared_ptr<spdlog::logger> get(const std::string& name);
  static void setLevel(spdlog::level::level_enum level);
  static void init();

 private:
  static bool initialized_;
};

}  // namespace system
}  // namespace omega

// Convenience macros - cheap to call, format lazily.
// Subsystem is a string literal (e.g. "portal", "scene", "shader").
#define OMEGA_LOG_TRACE(subsystem, ...) \
  ::omega::system::Log::get(subsystem)->trace(__VA_ARGS__)
#define OMEGA_LOG_DEBUG(subsystem, ...) \
  ::omega::system::Log::get(subsystem)->debug(__VA_ARGS__)
#define OMEGA_LOG_INFO(subsystem, ...) \
  ::omega::system::Log::get(subsystem)->info(__VA_ARGS__)
#define OMEGA_LOG_WARN(subsystem, ...) \
  ::omega::system::Log::get(subsystem)->warn(__VA_ARGS__)
#define OMEGA_LOG_ERROR(subsystem, ...) \
  ::omega::system::Log::get(subsystem)->error(__VA_ARGS__)

// GL error check - no-op in release, logs in debug.
// Usage: OMEGA_GL_CHECK("portal/renderPortalView: after scene render");
#ifdef NDEBUG
#define OMEGA_GL_CHECK(context) ((void)0)
#else
#define OMEGA_GL_CHECK(context)                                                \
  do {                                                                         \
    GLenum _err = glGetError();                                                \
    if (_err != GL_NO_ERROR) {                                                 \
      OMEGA_LOG_ERROR("gl", "GL error 0x{:x} at {}", _err, context);           \
    }                                                                          \
  } while (0)
#endif
