#pragma once

#include <iostream>

#define LIBDMV_DBG_STREAM std::cerr

#include <atomic>
#include <map>
#include <mutex>
#include <ostream>
#include <set>
#include <sstream>
#include <stdio.h>
#include <string>
#include <vector>

#include "ILoggerObserver.h"

namespace libdmv {

class Logger {
public:
  Logger(int severity, int line, const char *filePath, int errnum = 0);
  ~Logger();

  inline std::ostream &stream() { return buf_; }

  static inline void setSeverityLevel(int level) { severityLevel_ = level; }

  static inline int severityLevel() { return severityLevel_; }

  static inline void setVerboseLogLevel(int level) { verboseLogLevel_ = level; }

  static inline int verboseLogLevel() { return verboseLogLevel_; }

  // This is constexpr so that the hash for a file name is computed at compile
  // time when used in the VLOG macros.
  // This way, there is no string comparison for matching VLOG modules,
  // only a comparison of pre-computed hashes.
  // No fancy hashing needed here. It's pretty inefficient (one character
  // at a time) but the strings are not large and it's not in the critical path.
  static constexpr uint64_t rol(uint64_t val, int amount) {
    return val << amount | val >> (63 - amount);
  }
  static constexpr uint64_t hash(const char *s) {
    uint64_t hash = hash_rec(s, 0);
    return hash & rol(0x41a0240682483014ull, hash & 63);
  }
  static constexpr uint64_t hash_rec(const char *s, int off) {
    // Random constants!
    return (!s[off] ? 57ull : (hash_rec(s, off + 1) * 293) ^ s[off]);
  }
  static constexpr const char *basename(const char *s, int off = 0) {
    return !s[off]         ? s
           : s[off] == '/' ? basename(&s[off + 1])
                           : basename(s, off + 1);
  }

  static void setVerboseLogModules(const std::vector<std::string> &modules);

  static inline uint64_t verboseLogModules() { return verboseLogModules_; }

  static void clearLoggerObservers() {
    std::lock_guard<std::mutex> g(loggerObserversMutex_);
    loggerObservers().clear();
  }

  static void addLoggerObserver(ILoggerObserver *observer);

  static void removeLoggerObserver(ILoggerObserver *observer);

  static void addLoggerObserverDevice(int64_t device);

  static void addLoggerObserverEventCount(int64_t count);

  static void setLoggerObserverTraceDurationMS(int64_t duration);

  static void setLoggerObserverTraceID(const std::string &tid);

  static void setLoggerObserverGroupTraceID(const std::string &gtid);

  static void addLoggerObserverDestination(const std::string &dest);

  static void setLoggerObserverOnDemand();

  static void addLoggerObserverAddMetadata(const std::string &key,
                                           const std::string &value);

private:
  std::stringstream buf_;
  std::ostream &out_;
  int errnum_;
  int messageSeverity_;
  static std::atomic_int severityLevel_;
  static std::atomic_int verboseLogLevel_;
  static std::atomic<uint64_t> verboseLogModules_;
  static std::set<libdmv::ILoggerObserver *> &loggerObservers() {
    static auto *inst = new std::set<libdmv::ILoggerObserver *>();
    return *inst;
  }
  static std::mutex loggerObserversMutex_;
};

class VoidLogger {
public:
  VoidLogger() {}
  void operator&(std::ostream &) {}
};

} // namespace libdmv

#ifdef LOG // Undefine in case these are already defined (quite likely)
#undef LOG
#undef LOG_IS_ON
#undef LOG_IF
#undef LOG_EVERY_N
#undef LOG_IF_EVERY_N
#undef DLOG
#undef DLOG_IF
#undef VLOG
#undef VLOG_IF
#undef VLOG_EVERY_N
#undef VLOG_IS_ON
#undef DVLOG
#undef LOG_FIRST_N
#undef CHECK
#undef DCHECK
#undef DCHECK_EQ
#undef PLOG
#undef PCHECK
#undef LOG_OCCURRENCES
#endif

#define LOG_IS_ON(severity) (severity >= libdmv::Logger::severityLevel())

#define LOG_IF(severity, condition)                                            \
  !(LOG_IS_ON(severity) && (condition))                                        \
      ? (void)0                                                                \
      : libdmv::VoidLogger() &                                                 \
            libdmv::Logger(severity, __LINE__, __FILE__).stream()

#define LOG(severity) LOG_IF(severity, true)

#define LOCAL_VARNAME_CONCAT(name, suffix) _##name##suffix##_

#define LOCAL_VARNAME(name) LOCAL_VARNAME_CONCAT(name, __LINE__)

#define LOG_OCCURRENCES LOCAL_VARNAME(log_count)

#define LOG_EVERY_N(severity, rate)                                            \
  static int LOG_OCCURRENCES = 0;                                              \
  LOG_IF(severity, LOG_OCCURRENCES++ % rate == 0)                              \
      << "(x" << LOG_OCCURRENCES << ") "

template <uint64_t n> struct __to_constant__ { static const uint64_t val = n; };
#define FILENAME_HASH                                                          \
  __to_constant__<libdmv::Logger::hash(libdmv::Logger::basename(__FILE__))>::val
#define VLOG_IS_ON(verbosity)                                                  \
  (libdmv::Logger::verboseLogLevel() >= verbosity &&                           \
   (libdmv::Logger::verboseLogModules() & FILENAME_HASH) == FILENAME_HASH)

#define VLOG_IF(verbosity, condition)                                          \
  LOG_IF(VERBOSE, VLOG_IS_ON(verbosity) && (condition))

#define VLOG(verbosity) VLOG_IF(verbosity, true)

#define VLOG_EVERY_N(verbosity, rate)                                          \
  static int LOG_OCCURRENCES = 0;                                              \
  VLOG_IF(verbosity, LOG_OCCURRENCES++ % rate == 0)                            \
      << "(x" << LOG_OCCURRENCES << ") "

#define PLOG(severity)                                                         \
  libdmv::Logger(severity, __LINE__, __FILE__, errno).stream()

#define SET_LOG_SEVERITY_LEVEL(level) libdmv::Logger::setSeverityLevel(level)

#define SET_LOG_VERBOSITY_LEVEL(level, modules)                                \
  libdmv::Logger::setVerboseLogLevel(level);                                   \
  libdmv::Logger::setVerboseLogModules(modules)

// Logging the set of devices the trace is collect on.
#define LOGGER_OBSERVER_ADD_DEVICE(device_count)                               \
  libdmv::Logger::addLoggerObserverDevice(device_count)

// Incrementing the number of events collected by this trace.
#define LOGGER_OBSERVER_ADD_EVENT_COUNT(count)                                 \
  libdmv::Logger::addLoggerObserverEventCount(count)

// Record duration of trace in milliseconds.
#define LOGGER_OBSERVER_SET_TRACE_DURATION_MS(duration)                        \
  libdmv::Logger::setLoggerObserverTraceDurationMS(duration)

// Record the trace id when given.
#define LOGGER_OBSERVER_SET_TRACE_ID(tid)                                      \
  libdmv::Logger::setLoggerObserverTraceID(tid)

// Record the group trace id when given.
#define LOGGER_OBSERVER_SET_GROUP_TRACE_ID(gtid)                               \
  libdmv::Logger::setLoggerObserverGroupTraceID(gtid)

// Log the set of destinations the trace is sent to.
#define LOGGER_OBSERVER_ADD_DESTINATION(dest)                                  \
  libdmv::Logger::addLoggerObserverDestination(dest)

// Record this was triggered by On-Demand.
#define LOGGER_OBSERVER_SET_TRIGGER_ON_DEMAND()                                \
  libdmv::Logger::setLoggerObserverOnDemand()

// Record this was triggered by On-Demand.
#define LOGGER_OBSERVER_ADD_METADATA(key, value)                               \
  libdmv::Logger::addLoggerObserverAddMetadata(key, value)

// UST Logger Semantics to describe when a stage is complete.
#define UST_LOGGER_MARK_COMPLETED(stage)                                       \
  LOG(libdmv::LoggerOutputType::STAGE) << "Completed Stage: " << stage
