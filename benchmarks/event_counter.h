#ifndef __EVENT_COUNTER_H
#define __EVENT_COUNTER_H

#include <cctype>
#ifndef _MSC_VER
#include <dirent.h>
#endif
#include <cinttypes>

#include <cstring>

#include <chrono>
#include <array>

#include "linux-perf-events.h"
#ifdef __linux__
#include <libgen.h>
#endif

#if (defined(__APPLE__) && __APPLE__) && (defined(__aarch64__) && __aarch64__)
#include "apple_arm_events.h"
#endif

struct event_count {
  // The types of counters (so we can read the getter more easily)
  enum event_counter_types {
    CPU_CYCLES = 0,
    INSTRUCTIONS = 1,
    BRANCHES = 2,
    MISSED_BRANCHES = 3,
    event_counter_types_size = 4
  };

  std::chrono::duration<double> elapsed;
  std::array<unsigned long long, event_counter_types_size> event_counts;

  event_count() : elapsed(0), event_counts{0, 0, 0, 0} {}

  event_count(const std::chrono::duration<double> &_elapsed,
              const std::array<unsigned long long, event_counter_types_size>
                  &_event_counts)
      : elapsed(_elapsed), event_counts(_event_counts) {}

  event_count(const event_count &other)
      : elapsed(other.elapsed), event_counts(other.event_counts) {}

  double elapsed_sec() const {
    return std::chrono::duration<double>(elapsed).count();
  }

  double elapsed_ns() const {
    return std::chrono::duration<double, std::nano>(elapsed).count();
  }

  double cycles() const {
    return static_cast<double>(event_counts[CPU_CYCLES]);
  }

  double instructions() const {
    return static_cast<double>(event_counts[INSTRUCTIONS]);
  }

  double branches() const {
    return static_cast<double>(event_counts[BRANCHES]);
  }

  double missed_branches() const {
    return static_cast<double>(event_counts[MISSED_BRANCHES]);
  }

  event_count &operator=(const event_count &other) {
    this->elapsed = other.elapsed;
    this->event_counts = other.event_counts;
    return *this;
  }

  event_count operator+(const event_count &other) const {
    return event_count(elapsed + other.elapsed,
                       {
                           event_counts[0] + other.event_counts[0],
                           event_counts[1] + other.event_counts[1],
                           event_counts[2] + other.event_counts[2],
                           event_counts[3] + other.event_counts[3],
                       });
  }

  void operator+=(const event_count &other) { *this = *this + other; }
};

struct event_aggregate {
  bool has_events = false;
  int iterations = 0;
  event_count total{};
  event_count best{};
  event_count worst{};

  event_aggregate() = default;

  void operator<<(const event_count &other) {
    if (iterations == 0 || other.elapsed < best.elapsed) {
      best = other;
    }
    if (iterations == 0 || other.elapsed > worst.elapsed) {
      worst = other;
    }
    iterations++;
    total += other;
  }

  double elapsed_sec() const { return total.elapsed_sec() / iterations; }

  double elapsed_ns() const { return total.elapsed_ns() / iterations; }

  double cycles() const { return total.cycles() / iterations; }

  double instructions() const { return total.instructions() / iterations; }

  double branches() const { return total.branches() / iterations; }

  double missed_branches() const {
    return total.missed_branches() / iterations;
  }
};

struct event_collector {
  event_count count{};
  std::chrono::time_point<std::chrono::steady_clock> start_clock{};

#if defined(__linux__)
  LinuxEvents<PERF_TYPE_HARDWARE> linux_events;

  event_collector()
      : linux_events(std::array<unsigned long long, 4 /*event_counter_types_size*/>{
            PERF_COUNT_HW_CPU_CYCLES, PERF_COUNT_HW_INSTRUCTIONS,
            PERF_COUNT_HW_BRANCH_INSTRUCTIONS, // Retired branch instructions
            PERF_COUNT_HW_BRANCH_MISSES}) {}

  bool has_events() { return linux_events.is_working(); }
#elif __APPLE__ && __aarch64__
  performance_counters diff;

  event_collector() : diff(0) { setup_performance_counters(); }

  bool has_events() { return setup_performance_counters(); }
#else
  event_collector() = default;

  bool has_events() { return false; }
#endif

  inline void start() {
#if defined(__linux)
    linux_events.start();
#elif __APPLE__ && __aarch64__
    if (has_events()) {
      diff = get_counters();
    }
#endif
    start_clock = std::chrono::steady_clock::now();
  }

  inline event_count &end() {
    const auto end_clock = std::chrono::steady_clock::now();
#if defined(__linux)
    linux_events.end(count.event_counts);
#elif __APPLE__ && __aarch64__
    if (has_events()) {
      performance_counters end = get_counters();
      diff = end - diff;
    }
    count.event_counts[0] = diff.cycles;
    count.event_counts[1] = diff.instructions;
    count.event_counts[2] = diff.branches;
    count.event_counts[3] = diff.missed_branches;
#endif
    count.elapsed = end_clock - start_clock;
    return count;
  }
};

#endif
