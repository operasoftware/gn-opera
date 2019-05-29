// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/jumbo_writer.h"

#include <string>
#include <utility>
#include <vector>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "gn/build_settings.h"
#include "gn/source_dir.h"
#include "gn/target.h"
#include "gn/test_with_scope.h"
#include "util/test/test.h"

namespace {

std::vector<std::string> ReadFileLines(const base::FilePath& file_path) {
  std::string contents;
  base::ReadFileToString(file_path, &contents);
  return base::SplitString(contents, "\n", base::TRIM_WHITESPACE,
                           base::SPLIT_WANT_NONEMPTY);
}

// Returns #include line for |file_path|. As a side effect, prevents presubmit
// check from complaining about relative includes in this test.
std::string MakeIncludeLine(const char* file_path) {
  return std::string("#include \"") + file_path + '\"';
}

}  // namespace

TEST(JumboWriter, WriteJumboFiles) {
  TestWithScope setup;

  // Make a real directory for writing the files.
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  setup.build_settings()->SetRootPath(temp_dir.GetPath());

  Target target(setup.settings(), Label(SourceDir("foo"), "bar"));

  SourceFile file_a("//foo/a.cc");
  SourceFile file_b("//foo/subdir/b.cc");
  SourceFile file_c("//foo/c.cc");

  target.jumbo_files().assign(
      {Target::JumboSourceFile(SourceFile("//out/Debug/gen/foo/jumbo_cc_1.cc"),
                               {&file_a, &file_b}),
       Target::JumboSourceFile(SourceFile("//out/Debug/gen/foo/jumbo_cc_2.cc"),
                               {&file_c})});

  JumboWriter::RunAndWriteFiles(&target);

  auto lines = ReadFileLines(
      temp_dir.GetPath().AppendASCII("out/Debug/gen/foo/jumbo_cc_1.cc"));
  ASSERT_EQ(3u, lines.size());
  // First line is a comment.
  EXPECT_TRUE(base::StartsWith(lines[0], "/*", base::CompareCase::SENSITIVE));
  EXPECT_TRUE(base::EndsWith(lines[0], "*/", base::CompareCase::SENSITIVE));
  EXPECT_EQ(MakeIncludeLine("../../../../foo/a.cc"), lines[1]);
  EXPECT_EQ(MakeIncludeLine("../../../../foo/subdir/b.cc"), lines[2]);

  lines = ReadFileLines(
      temp_dir.GetPath().AppendASCII("out/Debug/gen/foo/jumbo_cc_2.cc"));
  ASSERT_EQ(2u, lines.size());
  // First line is a comment.
  EXPECT_TRUE(base::StartsWith(lines[0], "/*", base::CompareCase::SENSITIVE));
  EXPECT_TRUE(base::EndsWith(lines[0], "*/", base::CompareCase::SENSITIVE));
  EXPECT_EQ(MakeIncludeLine("../../../../foo/c.cc"), lines[1]);
}
