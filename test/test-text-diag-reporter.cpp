// Copyright (C) 2020  Matthew "strager" Glazar
// See end of file for extended copyright information.

#include <cstring>
#include <gtest/gtest.h>
#include <quick-lint-js/cli/cli-location.h>
#include <quick-lint-js/cli/options.h>
#include <quick-lint-js/cli/text-diag-reporter.h>
#include <quick-lint-js/container/padded-string.h>
#include <quick-lint-js/fe/source-code-span.h>
#include <quick-lint-js/io/output-stream.h>
#include <quick-lint-js/port/char8.h>
#include <string>

using namespace std::literals::string_literals;

namespace quick_lint_js {
namespace {
class test_text_diag_reporter : public ::testing::Test {
 protected:
  text_diag_reporter make_reporter() {
    return text_diag_reporter(translator(), &this->stream_,
                              /*escape_errors=*/false);
  }

  text_diag_reporter make_reporter(padded_string_view input) {
    return this->make_reporter(input, /*escape_errors=*/false);
  }

  text_diag_reporter make_reporter(padded_string_view input,
                                   bool escape_errors) {
    text_diag_reporter reporter(translator(), &this->stream_,
                                /*escape_errors=*/escape_errors);
    reporter.set_source(input, this->file_path_);
    return reporter;
  }

  string8 get_output() {
    this->stream_.flush();
    return this->stream_.get_flushed_string8();
  }

  string8 create_escape_error_code(const char8* code) {
    return u8"\x1B]8;;https://quick-lint-js.com/errors/"s + code + u8"/\x1B\\" +
           code + u8"\x1B]8;;\x1B\\";
  }

 private:
  memory_output_stream stream_;
  static constexpr const char* file_path_ = "FILE";
};

TEST_F(test_text_diag_reporter, change_source) {
  text_diag_reporter reporter = this->make_reporter();

  padded_string input_1(u8"aaaaaaaa"_sv);
  reporter.set_source(&input_1, /*file_name=*/"hello.js");
  reporter.report(diag_assignment_to_const_global_variable{
      source_code_span::unit(&input_1[4 - 1])});

  padded_string input_2(u8"bbbbbbbb"_sv);
  reporter.set_source(&input_2, /*file_name=*/"world.js");
  reporter.report(diag_assignment_to_const_global_variable{
      source_code_span::unit(&input_2[5 - 1])});

  EXPECT_EQ(
      this->get_output(),
      u8"hello.js:1:4: error: assignment to const global variable [E0002]\n"
      u8"world.js:1:5: error: assignment to const global variable [E0002]\n");
}

TEST_F(test_text_diag_reporter, assignment_before_variable_declaration) {
  padded_string input(u8"x=0;let x;"_sv);
  source_code_span assignment_span(&input[1 - 1], &input[1 + 1 - 1]);
  ASSERT_EQ(assignment_span.string_view(), u8"x"_sv);
  source_code_span declaration_span(&input[9 - 1], &input[9 + 1 - 1]);
  ASSERT_EQ(declaration_span.string_view(), u8"x"_sv);

  this->make_reporter(&input).report(
      diag_assignment_before_variable_declaration{
          .assignment = assignment_span,
          .declaration = declaration_span,
      });
  EXPECT_EQ(
      this->get_output(),
      u8"FILE:1:1: error: variable assigned before its declaration [E0001]\n"
      u8"FILE:1:9: note: variable declared here [E0001]\n");
}

TEST_F(test_text_diag_reporter, assignment_to_const_global_variable) {
  padded_string input(u8"to Infinity and beyond"_sv);
  source_code_span infinity_span(&input[4 - 1], &input[11 + 1 - 1]);
  ASSERT_EQ(infinity_span.string_view(), u8"Infinity"_sv);

  this->make_reporter(&input).report(
      diag_assignment_to_const_global_variable{infinity_span});
  EXPECT_EQ(this->get_output(),
            u8"FILE:1:4: error: assignment to const global variable [E0002]\n");
}

TEST_F(test_text_diag_reporter, expected_parenthesis_around_if_condition) {
  padded_string input(u8"if cond) {}"_sv);
  source_code_span parenthesis_span(&input[4 - 1], &input[4 - 1]);

  this->make_reporter(&input).report(
      diag_expected_parenthesis_around_if_condition{
          .where = parenthesis_span,
          .token = '(',
      });
  EXPECT_EQ(this->get_output(),
            u8"FILE:1:4: error: if statement is missing '(' around condition "
            u8"[E0018]\n");
}

TEST_F(test_text_diag_reporter, redeclaration_of_variable) {
  padded_string input(u8"let myvar; let myvar;"_sv);
  source_code_span original_declaration_span(&input[5 - 1], &input[9 + 1 - 1]);
  ASSERT_EQ(original_declaration_span.string_view(), u8"myvar"_sv);
  source_code_span redeclaration_span(&input[16 - 1], &input[20 + 1 - 1]);
  ASSERT_EQ(redeclaration_span.string_view(), u8"myvar"_sv);

  this->make_reporter(&input).report(diag_redeclaration_of_variable{
      redeclaration_span, original_declaration_span});
  EXPECT_EQ(this->get_output(),
            u8"FILE:1:16: error: redeclaration of variable: myvar [E0034]\n"
            u8"FILE:1:5: note: variable already declared here [E0034]\n");
}

TEST_F(test_text_diag_reporter, unexpected_hash_character) {
  padded_string input(u8"#"_sv);
  source_code_span hash_span(&input[1 - 1], &input[1 + 1 - 1]);
  ASSERT_EQ(hash_span.string_view(), u8"#"_sv);

  this->make_reporter(&input).report(diag_unexpected_hash_character{hash_span});
  EXPECT_EQ(this->get_output(), u8"FILE:1:1: error: unexpected '#' [E0052]\n");
}

TEST_F(test_text_diag_reporter, use_of_undeclared_variable) {
  padded_string input(u8"myvar;"_sv);
  source_code_span myvar_span(&input[1 - 1], &input[5 + 1 - 1]);
  ASSERT_EQ(myvar_span.string_view(), u8"myvar"_sv);

  this->make_reporter(&input).report(
      diag_use_of_undeclared_variable{myvar_span});
  EXPECT_EQ(this->get_output(),
            u8"FILE:1:1: warning: use of undeclared variable: myvar [E0057]\n");
}

TEST_F(test_text_diag_reporter, use_of_undeclared_variable_escaped_error) {
  padded_string input(u8"myvar;"_sv);
  source_code_span myvar_span(&input[1 - 1], &input[5 + 1 - 1]);
  ASSERT_EQ(myvar_span.string_view(), u8"myvar"_sv);

  this->make_reporter(&input, /*escape_errors=*/true)
      .report(diag_use_of_undeclared_variable{myvar_span});
  EXPECT_EQ(this->get_output(),
            u8"FILE:1:1: warning: use of undeclared variable: myvar [" +
                this->create_escape_error_code(u8"E0057") + u8"]\n");
}

TEST_F(test_text_diag_reporter, string8_view_parameter) {
  padded_string input(u8"<a . b></c>;"_sv);
  source_code_span open_span(&input[2 - 1], &input[6 + 1 - 1]);
  ASSERT_EQ(open_span.string_view(), u8"a . b"_sv);
  source_code_span close_span(&input[10 - 1], &input[10 + 1 - 1]);
  ASSERT_EQ(close_span.string_view(), u8"c"_sv);

  this->make_reporter(&input).report(diag_mismatched_jsx_tags{
      .opening_tag_name = open_span,
      .closing_tag_name = close_span,
      .opening_tag_name_pretty = u8"a.b"_sv,
  });
  EXPECT_EQ(
      this->get_output(),
      u8"FILE:1:10: error: mismatched JSX tags; expected '</a.b>' [E0187]\n"
      u8"FILE:1:2: note: opening '<a.b>' tag here [E0187]\n");
}
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
