// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/gn/jumbo_file_list_generator.h"

#include "base/strings/string_number_conversions.h"
#include "tools/gn/filesystem_utils.h"
#include "tools/gn/label.h"
#include "tools/gn/source_dir.h"
#include "tools/gn/source_file.h"
#include "tools/gn/test_with_scope.h"
#include "util/test/test.h"

namespace {

class JumboFileListGeneratorTest : public testing::Test {
 public:
  JumboFileListGeneratorTest()
      : target_(setup_.settings(), Label(SourceDir("foo"), "bar")) {
    target_.set_jumbo_allowed(true);
  }

 protected:
  TestWithScope setup_;
  Target target_;
  Err err_;
};

}  // namespace

TEST_F(JumboFileListGeneratorTest, BasicList) {
  target_.set_jumbo_file_merge_limit(3);
  target_.sources().assign({
      SourceFile("a.cc"), SourceFile("a.h"), SourceFile("b.cc"),
      SourceFile("A/c.cc"), SourceFile("B/d.cc"), SourceFile("A/B/e.cc"),
      SourceFile("A/B/e.h"),
  });

  Target::JumboFileList jumbo_files;
  JumboFileListGenerator generator(&target_, &jumbo_files, &err_);
  generator.Run();

  EXPECT_FALSE(err_.has_error());
  ASSERT_EQ(2u, jumbo_files.size());
  ASSERT_EQ(3u, jumbo_files[0].second.size());
  EXPECT_EQ("a.cc", jumbo_files[0].second[0]->value());
  EXPECT_EQ("b.cc", jumbo_files[0].second[1]->value());
  EXPECT_EQ("A/c.cc", jumbo_files[0].second[2]->value());
  ASSERT_EQ(2u, jumbo_files[1].second.size());
  EXPECT_EQ("B/d.cc", jumbo_files[1].second[0]->value());
  EXPECT_EQ("A/B/e.cc", jumbo_files[1].second[1]->value());
}

TEST_F(JumboFileListGeneratorTest, DefaultFileMergeLimit) {
  // Default limit is 50. Add many source files.
  for (int i = 0; i < 105; ++i) {
    target_.sources().push_back(SourceFile(base::NumberToString(i) + ".cc"));
  }

  Target::JumboFileList jumbo_files;
  JumboFileListGenerator generator(&target_, &jumbo_files, &err_);
  generator.Run();

  EXPECT_FALSE(err_.has_error());
  ASSERT_EQ(3u, jumbo_files.size());
  EXPECT_EQ(50u, jumbo_files[0].second.size());
  EXPECT_EQ(50u, jumbo_files[1].second.size());
  EXPECT_EQ(5u, jumbo_files[2].second.size());
}

TEST_F(JumboFileListGeneratorTest, ExcludedSources) {
  target_.set_jumbo_file_merge_limit(2);
  target_.sources().assign({
      SourceFile("a.cc"), SourceFile("b.cc"), SourceFile("c.cc"),
      SourceFile("d.cc"), SourceFile("e.cc"),
  });
  target_.jumbo_excluded_sources().assign(
      {SourceFile("b.cc"), SourceFile("d.cc")});

  Target::JumboFileList jumbo_files;
  JumboFileListGenerator generator(&target_, &jumbo_files, &err_);
  generator.Run();

  EXPECT_FALSE(err_.has_error());
  ASSERT_EQ(2u, jumbo_files.size());
  ASSERT_EQ(2u, jumbo_files[0].second.size());
  EXPECT_EQ("a.cc", jumbo_files[0].second[0]->value());
  EXPECT_EQ("c.cc", jumbo_files[0].second[1]->value());
  ASSERT_EQ(1u, jumbo_files[1].second.size());
  EXPECT_EQ("e.cc", jumbo_files[1].second[0]->value());
}

TEST_F(JumboFileListGeneratorTest, MixedSourceFileTypes) {
  target_.set_jumbo_file_merge_limit(2);
  target_.sources().assign({
      SourceFile("a.cc"), SourceFile("1.mm"), SourceFile("2.mm"),
      SourceFile("3.mm"), SourceFile("b.cc"), SourceFile("c.cc"),
      SourceFile("d.cc"), SourceFile("4.mm"), SourceFile("5.mm"),
      SourceFile("e.cc"),
  });

  Target::JumboFileList jumbo_files;
  JumboFileListGenerator generator(&target_, &jumbo_files, &err_);
  generator.Run();

  EXPECT_FALSE(err_.has_error());
  ASSERT_EQ(6u, jumbo_files.size());
  EXPECT_EQ("cc", FindExtension(&jumbo_files[0].first.value()));
  ASSERT_EQ(2u, jumbo_files[0].second.size());
  EXPECT_EQ("a.cc", jumbo_files[0].second[0]->value());
  EXPECT_EQ("b.cc", jumbo_files[0].second[1]->value());
  EXPECT_EQ("mm", FindExtension(&jumbo_files[1].first.value()));
  ASSERT_EQ(2u, jumbo_files[1].second.size());
  EXPECT_EQ("1.mm", jumbo_files[1].second[0]->value());
  EXPECT_EQ("2.mm", jumbo_files[1].second[1]->value());
  EXPECT_EQ("mm", FindExtension(&jumbo_files[2].first.value()));
  ASSERT_EQ(2u, jumbo_files[2].second.size());
  EXPECT_EQ("3.mm", jumbo_files[2].second[0]->value());
  EXPECT_EQ("4.mm", jumbo_files[2].second[1]->value());
  EXPECT_EQ("cc", FindExtension(&jumbo_files[3].first.value()));
  ASSERT_EQ(2u, jumbo_files[3].second.size());
  EXPECT_EQ("c.cc", jumbo_files[3].second[0]->value());
  EXPECT_EQ("d.cc", jumbo_files[3].second[1]->value());
  EXPECT_EQ("mm", FindExtension(&jumbo_files[4].first.value()));
  ASSERT_EQ(1u, jumbo_files[4].second.size());
  EXPECT_EQ("5.mm", jumbo_files[4].second[0]->value());
  EXPECT_EQ("cc", FindExtension(&jumbo_files[5].first.value()));
  ASSERT_EQ(1u, jumbo_files[5].second.size());
  EXPECT_EQ("e.cc", jumbo_files[5].second[0]->value());
}

TEST_F(JumboFileListGeneratorTest, SupportedSourceFileTypes) {
  target_.sources().assign({
      SourceFile("x.cc"), SourceFile("x.cpp"), SourceFile("x.cxx"),
      SourceFile("x.h"), SourceFile("x.hpp"), SourceFile("x.hxx"),
      SourceFile("x.hh"), SourceFile("x.c"), SourceFile("x.m"),
      SourceFile("x.mm"), SourceFile("x.rc"), SourceFile("x.S"),
      SourceFile("x.s"), SourceFile("x.asm"), SourceFile("x.o"),
      SourceFile("x.obj"), SourceFile("x.def"),
  });

  Target::JumboFileList jumbo_files;
  JumboFileListGenerator generator(&target_, &jumbo_files, &err_);
  generator.Run();

  EXPECT_FALSE(err_.has_error());
  ASSERT_EQ(3u, jumbo_files.size());
  EXPECT_EQ("cc", FindExtension(&jumbo_files[0].first.value()));
  ASSERT_EQ(3u, jumbo_files[0].second.size());
  EXPECT_EQ("c", FindExtension(&jumbo_files[1].first.value()));
  ASSERT_EQ(1u, jumbo_files[1].second.size());
  EXPECT_EQ("mm", FindExtension(&jumbo_files[2].first.value()));
  ASSERT_EQ(1u, jumbo_files[2].second.size());
}
