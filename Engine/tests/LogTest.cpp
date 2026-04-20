// LogTest — smoke coverage for the logging subsystem introduced in Phase 0.1.
// We don't try to capture output (that would couple to spdlog internals);
// instead we verify that the public API is idempotent and thread-safe enough
// to survive repeated calls, which is the contract the rest of the engine
// relies on.

#include <gtest/gtest.h>

#include <system/Log.h>

#include <thread>
#include <vector>

using omega::system::Log;

TEST(LogTest, RepeatedGetReturnsSameLogger) {
  auto a = Log::get("test-subsystem");
  auto b = Log::get("test-subsystem");
  EXPECT_EQ(a.get(), b.get())
      << "Log::get should reuse existing loggers by name";
}

TEST(LogTest, DifferentNamesReturnDifferentLoggers) {
  auto a = Log::get("alpha");
  auto b = Log::get("beta");
  EXPECT_NE(a.get(), b.get());
}

TEST(LogTest, MacrosCompileAndDoNotCrash) {
  // Just verify the macros actually expand and run. If spdlog's default
  // sink were misconfigured this would segfault.
  OMEGA_LOG_TRACE("unit-test", "trace {}", 1);
  OMEGA_LOG_DEBUG("unit-test", "debug {}", 2);
  OMEGA_LOG_INFO("unit-test", "info {}", 3);
  OMEGA_LOG_WARN("unit-test", "warn {}", 4);
  OMEGA_LOG_ERROR("unit-test", "error {}", 5);
  SUCCEED();
}

TEST(LogTest, ConcurrentGetIsRaceFree) {
  // Spawn threads that all request the same logger; Log::get must serialize
  // internally. If the mutex is missing, TSan (or Address Sanitizer) should
  // catch it, otherwise we verify all threads saw the same pointer.
  constexpr int kThreads = 16;
  std::vector<std::thread> ts;
  std::vector<std::shared_ptr<spdlog::logger>> loggers(kThreads);
  for (int i = 0; i < kThreads; ++i) {
    ts.emplace_back([i, &loggers]() { loggers[i] = Log::get("racy"); });
  }
  for (auto &t : ts) t.join();
  for (int i = 1; i < kThreads; ++i) {
    EXPECT_EQ(loggers[0].get(), loggers[i].get());
  }
}
