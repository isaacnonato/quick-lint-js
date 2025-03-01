// Copyright (C) 2020  Matthew "strager" Glazar
// See end of file for extended copyright information.

#include <benchmark/benchmark.h>
#include <quick-lint-js/cli/vim-location.h>
#include <quick-lint-js/container/padded-string.h>
#include <quick-lint-js/fe/source-code-span.h>
#include <quick-lint-js/generate-code.h>
#include <quick-lint-js/port/char8.h>
#include <quick-lint-js/util/narrow-cast.h>

namespace quick_lint_js {
namespace {
void benchmark_location_scale_of_long_line(::benchmark::State &state) {
  int line_length = 10'000;
  padded_string line(string8(narrow_cast<std::size_t>(line_length), u8'x'));
  for (auto _ : state) {
    vim_locator l(&line);
    for (int i = 0; i < line_length; ++i) {
      vim_source_position p = l.position(&line[i]);
      ::benchmark::DoNotOptimize(p);
    }
  }
}
BENCHMARK(benchmark_location_scale_of_long_line);

void benchmark_location_scale_of_empty_lines(::benchmark::State &state) {
  int line_count = 10'000;
  padded_string lines(string8(narrow_cast<std::size_t>(line_count), u8'\n'));
  for (auto _ : state) {
    vim_locator l(&lines);
    for (int i = 0; i < line_count; ++i) {
      vim_source_position p = l.position(&lines[i]);
      ::benchmark::DoNotOptimize(p);
    }
  }
}
BENCHMARK(benchmark_location_scale_of_empty_lines);

void benchmark_range_scale_of_empty_lines(::benchmark::State &state) {
  int line_length = 10'000;
  int span_length = 5;
  padded_string line(string8(narrow_cast<std::size_t>(line_length), u8'\n'));
  for (auto _ : state) {
    vim_locator l(&line);
    for (int i = 0; i < line_length - span_length; i += span_length) {
      source_code_span span(&line[i], &line[i + span_length]);
      vim_source_range r = l.range(span);
      ::benchmark::DoNotOptimize(r);
    }
  }
}
BENCHMARK(benchmark_range_scale_of_empty_lines);

void benchmark_location_realisticish(::benchmark::State &state) {
  int line_count = 10'000;
  int span_count = narrow_cast<int>(state.range(0));
  source_code_with_spans code = make_realisticish_code(
      /*line_count=*/line_count, /*span_count=*/span_count);

  for (auto _ : state) {
    vim_locator l(code.source.get());
    for (const source_code_span &span : code.spans) {
      vim_source_range r = l.range(span);
      ::benchmark::DoNotOptimize(r);
    }
  }
}
BENCHMARK(benchmark_location_realisticish)->Arg(1)->Arg(50);
}
}

// quick-lint-js finds bugs in JavaScript programs.
// Copyright (C) 2020  Matthew "strager" Glazar
//
// This file is part of quick-lint-js.
//
// quick-lint-js is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// quick-lint-js is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with quick-lint-js.  If not, see <https://www.gnu.org/licenses/>.
