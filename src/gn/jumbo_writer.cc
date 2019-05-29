// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gn/jumbo_writer.h"

#include <sstream>

#include "base/files/file_util.h"
#include "gn/build_settings.h"
#include "gn/filesystem_utils.h"
#include "gn/scheduler.h"
#include "gn/settings.h"

JumboWriter::JumboWriter(const Target* target)
    : target_(target),
      path_output_(GetBuildDirForTargetAsSourceDir(target, BuildDirType::GEN),
                   target_->settings()->build_settings()->root_path_utf8(),
                   ESCAPE_NONE) {}

JumboWriter::~JumboWriter() = default;

void JumboWriter::RunAndWriteFiles(const Target* target) {
  JumboWriter writer(target);
  writer.Run();
}

void JumboWriter::Run() const {
  if (target_->jumbo_files().empty())
    return;

  base::CreateDirectory(target_->settings()
                            ->build_settings()
                            ->GetFullPath(target_->jumbo_files()[0].first)
                            .DirName());

  for (const Target::JumboSourceFile& jumbo_file : target_->jumbo_files()) {
    if (!WriteJumboFile(jumbo_file))
      return;
  }
}

bool JumboWriter::WriteJumboFile(
    const Target::JumboSourceFile& jumbo_file) const {
  std::stringstream content;
  content << "/* This is a Jumbo file. Don't edit. "
          << "Generated with 'gn gen' command. */\n\n";

  for (const SourceFile* source_file : jumbo_file.second) {
    content << "#include \"";
    path_output_.WriteFile(content, *source_file);
    content << "\"\n";
  }

  Err err;
  if (!WriteFileIfChanged(
          target_->settings()->build_settings()->GetFullPath(jumbo_file.first),
          content.str(), &err)) {
    g_scheduler->FailWithError(err);
    return false;
  }

  return true;
}
