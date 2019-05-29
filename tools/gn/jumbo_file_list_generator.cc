// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/gn/jumbo_file_list_generator.h"

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

#include "base/logging.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/stringprintf.h"
#include "tools/gn/filesystem_utils.h"

namespace {

// Constructs the file name for a jumbo file.
std::string GetJumboFileName(const std::string& target_name,
                             SourceFile::Type file_type,
                             int file_number) {
  std::string_view extension;
  switch (file_type) {
    case SourceFile::SOURCE_C:
      extension = "c";
      break;
    case SourceFile::SOURCE_CPP:
      extension = "cc";
      break;
    case SourceFile::SOURCE_MM:
      extension = "mm";
      break;
    default:
      return std::string();
  }

  return base::StringPrintf("%s_jumbo_%s_%d.%s", target_name.c_str(),
                            extension.data(), file_number, extension.data());
}

}  // namespace

JumboFileListGenerator::JumboFileListGenerator(
    const Target* target,
    Target::JumboFileList* jumbo_files,
    Err* err)
    : target_(target),
      jumbo_files_dir_(
          GetBuildDirForTargetAsSourceDir(target, BuildDirType::GEN)),
      jumbo_files_(jumbo_files),
      recent_jumbo_file_(nullptr),
      recent_jumbo_file_type_(SourceFile::SOURCE_UNKNOWN),
      err_(err) {}

JumboFileListGenerator::~JumboFileListGenerator() = default;

void JumboFileListGenerator::Run() {
  const Target::FileList& excluded_sources = target_->jumbo_excluded_sources();

  for (const SourceFile& input : target_->sources()) {
    SourceFile::Type file_type = input.type();
    if (file_type != SourceFile::SOURCE_C &&
        file_type != SourceFile::SOURCE_CPP &&
        file_type != SourceFile::SOURCE_MM) {
      continue;
    }

    if (std::find(excluded_sources.begin(), excluded_sources.end(), input) !=
        excluded_sources.end()) {
      continue;
    }

    Target::JumboSourceFile* jumbo_file = FindJumboFile(file_type);
    if (!jumbo_file)
      jumbo_file = CreateJumboFile(file_type);
    if (!jumbo_file) {
      if (err_->has_error())
        return;
      else
        continue;  // Source file type not supported by jumbo. Just skip it.
    }

    jumbo_file->second.push_back(&input);

    recent_jumbo_file_ = jumbo_file;
    recent_jumbo_file_type_ = file_type;
  }
}

Target::JumboSourceFile* JumboFileListGenerator::FindJumboFile(
    SourceFile::Type file_type) const {
  // Return recently used file if suitable.
  if (recent_jumbo_file_type_ == file_type) {
    return base::checked_cast<int>(recent_jumbo_file_->second.size()) <
                   target_->jumbo_file_merge_limit()
               ? recent_jumbo_file_
               : nullptr;
  }

  // Return immediately if we don't have any files for |file_type|.
  if (jumbo_file_numbers_.count(file_type) == 0)
    return nullptr;

  // Search for file on |jumbo_files_| list.
  for (auto it = jumbo_files_->rbegin(); it != jumbo_files_->rend(); ++it) {
    if (it->first.type() == file_type) {
      return base::checked_cast<int>(it->second.size()) <
                     target_->jumbo_file_merge_limit()
                 ? &(*it)
                 : nullptr;
    }
  }

  NOTREACHED();
  return nullptr;
}

Target::JumboSourceFile* JumboFileListGenerator::CreateJumboFile(
    SourceFile::Type file_type) {
  const auto it = jumbo_file_numbers_.find(file_type);
  int file_number = it != jumbo_file_numbers_.end() ? it->second + 1 : 0;
  jumbo_file_numbers_[file_type] = file_number;

  std::string file_name =
      GetJumboFileName(target_->label().name(), file_type, file_number);
  if (file_name.empty())
    return nullptr;

  SourceFile source_file =
      jumbo_files_dir_.ResolveRelativeFile(Value(nullptr, file_name), err_);
  if (source_file.is_null())
    return nullptr;

  jumbo_files_->push_back(
      Target::JumboSourceFile(source_file, std::vector<const SourceFile*>()));
  Target::JumboSourceFile* jumbo_file = &jumbo_files_->back();
  jumbo_file->second.reserve(target_->jumbo_file_merge_limit());
  return jumbo_file;
}
