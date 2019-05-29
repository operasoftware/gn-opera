// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GN_JUMBO_FILE_LIST_GENERATOR_H_
#define GN_JUMBO_FILE_LIST_GENERATOR_H_

#include <map>

#include "base/macros.h"
#include "gn/source_dir.h"
#include "gn/source_file.h"
#include "gn/target.h"

/**
 * Helper class responsible for generating jumbo file list for target sources.
 */
class JumboFileListGenerator {
 public:
  JumboFileListGenerator(const Target* target,
                         Target::JumboFileList* jumbo_files,
                         Err* err);
  ~JumboFileListGenerator();

  // Fills |jumbo_files| list passed to constructor. Sets the error passed to
  // the constructor on failure.
  void Run();

 private:
  // Returns JumboSourceFile object for given |file_type| if it exists and is
  // suitable for adding more source files.
  Target::JumboSourceFile* FindJumboFile(SourceFile::Type file_type) const;

  // Creates a new JumboSourceFile object for given |file_type| and adds it to
  // |jumbo_files_|.
  Target::JumboSourceFile* CreateJumboFile(SourceFile::Type file_type);

  const Target* target_;

  // Parent directory for jumbo files of |target_|.
  SourceDir jumbo_files_dir_;

  // Generated list of jumbo files.
  Target::JumboFileList* jumbo_files_;

  // Recently used numbers for jumbo files. Numbering is separate for each
  // source file type.
  std::map<SourceFile::Type, int> jumbo_file_numbers_;

  // Recently used jumbo file and its type.
  Target::JumboSourceFile* recent_jumbo_file_;
  SourceFile::Type recent_jumbo_file_type_;

  Err* err_;

  DISALLOW_COPY_AND_ASSIGN(JumboFileListGenerator);
};

#endif  // GN_JUMBO_FILE_LIST_GENERATOR_H_
