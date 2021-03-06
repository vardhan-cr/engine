// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/at_exit.h"
#include "base/basictypes.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/edk/embedder/simple_platform_support.h"
#include "sky/shell/shell.h"
#include "sky/shell/switches.h"
#include "sky/shell/testing/testing.h"

int main(int argc, const char* argv[]) {
  base::AtExitManager exit_manager;
  base::CommandLine::Init(argc, argv);

  base::CommandLine& command_line = *base::CommandLine::ForCurrentProcess();

  if (command_line.HasSwitch(sky::shell::switches::kHelp)) {
    sky::shell::switches::PrintUsage("sky_shell");
    return 0;
  }

  base::MessageLoop message_loop;

  mojo::embedder::Init(std::unique_ptr<mojo::embedder::PlatformSupport>(
      new mojo::embedder::SimplePlatformSupport()));

  sky::shell::Shell::InitStandalone();

  if (!sky::shell::InitForTesting()) {
    sky::shell::switches::PrintUsage("sky_shell");
    return 1;
  }

  message_loop.Run();
  return 0;
}
