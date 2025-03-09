#if defined(__linux__) || (__APPLE__ && __aarch64__)
#define USING_COUNTERS
#endif
#include "event_counter.h"
#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctype.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <stdio.h>
#include <string>
#include <locale.h>

#include "fast_float/fast_float.h"

event_collector collector{};

std::chrono::high_resolution_clock::time_point t1, t2;

template <typename CharT, typename Value>
Value findmax_fastfloat(std::vector<std::basic_string<CharT>> &s,
#ifdef USING_COUNTERS
                        std::vector<event_count> &aggregate
#else
                        std::chrono::nanoseconds& time
#endif
) {
  Value answer = 0;
  Value x = 0;
  for (auto &st : s) {

#ifdef USING_COUNTERS
    collector.start();
#else
    t1 = std::chrono::high_resolution_clock::now();
#endif

    auto [p, ec] = fast_float::from_chars(st.data(), st.data() + st.size(), x);

#ifdef USING_COUNTERS
    aggregate.push_back(collector.end());
#else
    t2 = std::chrono::high_resolution_clock::now();
    time += t2 - t1;
#endif
    
    if (ec != std::errc{}) {
      throw std::runtime_error("bug in findmax_fastfloat");
    }
    answer = answer > x ? answer : x;
  }
  return answer;
}

#ifdef USING_COUNTERS
template <class T, class CharT>
std::vector<event_count>
time_it_ns(std::vector<std::basic_string<CharT>> &lines, T const &function,
           size_t repeat) {
  std::vector<event_count> aggregate;
  bool printed_bug = false;
  for (size_t i = 0; i < repeat; i++) {  
    double ts = function(lines, aggregate);
    if (ts == 0 && !printed_bug) {
      printf("bug\n");
      printed_bug = true;
    }
  }
  return aggregate;
}

void pretty_print(double volume, size_t number_of_floats, std::string name,
                  std::vector<event_count> events) {
  double volumeMB = volume / (1024. * 1024.);
  double average_ns{0};
  double min_ns{DBL_MAX};
  double cycles_min{DBL_MAX};
  double instructions_min{DBL_MAX};
  double cycles_avg{0};
  double instructions_avg{0};
  double branches_min{0};
  double branches_avg{0};
  double branch_misses_min{0};
  double branch_misses_avg{0};
  for (event_count e : events) {
    double ns = e.elapsed_ns();
    average_ns += ns;
    min_ns = min_ns < ns ? min_ns : ns;

    double cycles = e.cycles();
    cycles_avg += cycles;
    cycles_min = cycles_min < cycles ? cycles_min : cycles;

    double instructions = e.instructions();
    instructions_avg += instructions;
    instructions_min =
        instructions_min < instructions ? instructions_min : instructions;

    double branches = e.branches();
    branches_avg += branches;
    branches_min = branches_min < branches ? branches_min : branches;

    double branch_misses = e.missed_branches();
    branch_misses_avg += branch_misses;
    branch_misses_min =
        branch_misses_min < branch_misses ? branch_misses_min : branch_misses;
  }
  cycles_avg /= events.size();
  instructions_avg /= events.size();
  average_ns /= events.size();
  branches_avg /= events.size();
  printf("%-40s: %8.2f MB/s (+/- %.1f %%) ", name.data(),
         volumeMB * 1000000000 / min_ns,
         (average_ns - min_ns) * 100.0 / average_ns);
  printf("%8.2f Mfloat/s  ", number_of_floats * 1000 / min_ns);
  if (instructions_min > 0) {
    printf(" %8.2f i/B %8.2f i/f (+/- %.1f %%) ", instructions_min / volume,
           instructions_min / number_of_floats,
           (instructions_avg - instructions_min) * 100.0 / instructions_avg);

    printf(" %8.2f c/B %8.2f c/f (+/- %.1f %%) ", cycles_min / volume,
           cycles_min / number_of_floats,
           (cycles_avg - cycles_min) * 100.0 / cycles_avg);
    printf(" %8.2f i/c ", instructions_min / cycles_min);
    printf(" %8.2f b/f ", branches_avg / number_of_floats);
    printf(" %8.2f bm/f ", branch_misses_avg / number_of_floats);
    printf(" %8.2f GHz ", cycles_min / min_ns);
  }
  printf("\n");
}
#else
template <class T, class CharT>
std::pair<double, double>
time_it_ns(std::vector<std::basic_string<CharT>> &lines, T const &function,
           size_t repeat) {
  double average = 0;
  double min_value = DBL_MAX;
  bool printed_bug = false;
  for (size_t i = 0; i < repeat; i++) {
    std::chrono::nanoseconds time{};
    const auto ts = function(lines, time);
    if (ts == 0 && !printed_bug) {
      printf("bug\n");
      printed_bug = true;
    }
    double dif = std::chrono::duration_cast<std::chrono::nanoseconds>(time).count();
    average += dif;
    min_value = min_value < dif ? min_value : dif;
  }
  average /= repeat;
  return std::make_pair(min_value, average);
}

void pretty_print(double volume, size_t number_of_floats, std::string name,
                  std::pair<double, double> result) {
  double volumeMB = volume / (1024. * 1024.);
  printf("%-40s: %8.2f MB/s (+/- %.1f %%) ", name.data(),
         volumeMB * 1'000'000'000 / result.first,
         (result.second - result.first) * 100.0 / result.second);
  printf("%8.2f Mfloat/s  ", number_of_floats * 1000 / result.first);
  printf(" %8.2f ns/f \n", double(result.first) / number_of_floats);
}
#endif

// this is okay, all chars are ASCII
inline std::u16string widen(std::string line) {
  std::u16string u16line;
  u16line.resize(line.size());
  for (size_t i = 0; i < line.size(); ++i) {
    u16line[i] = char16_t(line[i]);
  }
  return u16line;
}

std::vector<std::u16string> widen(const std::vector<std::string> &lines) {
  std::vector<std::u16string> u16lines;
  u16lines.reserve(lines.size());
  for (auto const &line : lines) {
    u16lines.push_back(widen(line));
  }
  return u16lines;
}

void process(std::vector<std::string> &lines, size_t volume) {
  size_t repeat = 1000;
  double volumeMB = volume / (1024. * 1024.);
  std::cout << "ASCII volume = " << volumeMB << " MB " << std::endl;
  pretty_print(volume, lines.size(), "fastfloat (64)",
               time_it_ns(lines, findmax_fastfloat<char, double>, repeat));
  pretty_print(volume, lines.size(), "fastfloat (32)",
               time_it_ns(lines, findmax_fastfloat<char, float>, repeat));

  std::vector<std::u16string> lines16 = widen(lines);
  volume = 2 * volume;
  volumeMB = volume / (1024. * 1024.);
  std::cout << "UTF-16 volume = " << volumeMB << " MB " << std::endl;
  pretty_print(volume, lines.size(), "fastfloat (64)",
               time_it_ns(lines16, findmax_fastfloat<char16_t, double>, repeat));
  pretty_print(volume, lines.size(), "fastfloat (32)",
               time_it_ns(lines16, findmax_fastfloat<char16_t, float>, repeat));
}

void fileload(std::string filename) {
  std::ifstream inputfile(filename);
  if (!inputfile) {
    std::cerr << "can't open " << filename << std::endl;
    return;
  }
  std::cout << "#### " << std::endl;
  std::cout << "# reading " << filename << std::endl;
  std::cout << "#### " << std::endl;
  std::string line;
  std::vector<std::string> lines;
  lines.reserve(10000); // let us reserve plenty of memory.
  size_t volume = 0;
  while (getline(inputfile, line)) {
#ifdef FASTFLOAT_ONLY_POSITIVE_C_NUMBER_WO_INF_NAN
    /* This code is a simple parser emulator */
    for (size_t n = 0; n < line.size(); ++n) {
      if ((line[n] >= '0' && line[n] <= '9'))
      {
        /* in the real parser we don't check anything else
         and call the from_chars function immediately */
        const auto s = n;
        for (++n; n < line.size() &&
                  ((line[n] >= '0' && line[n] <= '9') ||
                    line[n] == 'e' || line[n] == 'E' ||
                    line[n] == '.' ||
                    line[n] == '-' || line[n] == '+'
                    /* last line for exponent sign*/
                   );
           ++n)
        {
        
        }
        /*~ in the real parser we don't check anything else
           and call the from_chars function immediately */

        volume += lines.emplace_back(line.substr(s, n)).size();
      }
      else
      {
        /* for the test we simplify skipped all other symbols,
           in real application this should be a full parser,
           that parse also any mathematical operations like + and -
           and this is the reason why we don't need to check a sign
           when FASTFLOAT_ONLY_POSITIVE_C_NUMBER_WO_INF_NAN is enabled. */
        ++n;
        continue;
      }
    }
  // in the real parser this part of code should return end token
#else
    volume += lines.emplace_back(line).size();
#endif
  }
  std::cout << "# read " << lines.size() << " lines " << std::endl;
  process(lines, volume);
}

int main(int argc, char **argv) {
  if (collector.has_events()) {
    std::cout << "# Using hardware counters" << std::endl;
  } else {
#if defined(__linux__) || (__APPLE__ && __aarch64__)
    std::cout << "# Hardware counters not available, try to run in privileged "
                 "mode (e.g., sudo)."
              << std::endl;
#endif
  }
  fileload(std::string(BENCHMARK_DATA_DIR) + "/canada.txt");
  fileload(std::string(BENCHMARK_DATA_DIR) + "/mesh.txt");
}
