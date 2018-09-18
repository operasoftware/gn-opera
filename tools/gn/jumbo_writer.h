// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_JUMBO_WRITER_H_
#define TOOLS_GN_JUMBO_WRITER_H_

#include <iosfwd>
#include <vector>

#include "base/macros.h"
#include "tools/gn/path_output.h"
#include "tools/gn/target.h"

// Helper class responsible for writing jumbo files.
class JumboWriter {
 public:
  explicit JumboWriter(const Target* target);
  ~JumboWriter();

  // Writes jumbo files for given |target|.
  static void RunAndWriteFiles(const Target* target);

 private:
  void Run() const;
  bool WriteJumboFile(const Target::JumboSourceFile& jumbo_file) const;

  const Target* target_;
  const PathOutput path_output_;

  DISALLOW_COPY_AND_ASSIGN(JumboWriter);
};

#endif  // TOOLS_GN_JUMBO_WRITER_H_
