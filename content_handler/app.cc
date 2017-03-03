// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/content_handler/app.h"

#include <thread>
#include <utility>

#include "apps/tracing/lib/trace/provider.h"
#include "flutter/common/settings.h"
#include "flutter/common/threads.h"
#include "flutter/runtime/runtime_init.h"
#include "flutter/sky/engine/platform/fonts/fuchsia/FontCacheFuchsia.h"
#include "lib/ftl/macros.h"
#include "lib/ftl/tasks/task_runner.h"
#include "lib/mtl/tasks/message_loop.h"
#include "apps/icu_data/lib/icu_data.h"

namespace flutter_runner {
namespace {

void QuitMessageLoop() {
  mtl::MessageLoop::GetCurrent()->QuitNow();
}

}  // namespace

App::App() {
  context_ = app::ApplicationContext::CreateFromStartupInfo();

  tracing::InitializeTracer(context_.get(), {});

  gpu_thread_ = std::make_unique<Thread>();
  io_thread_ = std::make_unique<Thread>();

  FTL_CHECK(gpu_thread_->IsValid()) << "Must be able to create the GPU thread";
  FTL_CHECK(io_thread_->IsValid()) << "Must be able to create the IO thread";

  auto ui_task_runner = mtl::MessageLoop::GetCurrent()->task_runner();
  auto gpu_task_runner = gpu_thread_->TaskRunner();
  auto io_task_runner = io_thread_->TaskRunner();

  // Notice that the Platform and UI threads are actually the same.
  blink::Threads::Set(blink::Threads(ui_task_runner,   // Platform
                                     gpu_task_runner,  // GPU
                                     ui_task_runner,   // UI
                                     io_task_runner    // IO
                                     ));
  blink::Settings settings;
  settings.enable_observatory = true;
  blink::Settings::Set(settings);
  blink::InitRuntime();

  blink::SetFontProvider(
      context_->ConnectToEnvironmentService<fonts::FontProvider>());

  if (!icu_data::Initialize(context_->environment_services().get())) {
    FTL_LOG(ERROR) << "Could not initialize ICU data.";
  }

  context_->outgoing_services()->AddService<app::ApplicationRunner>(
      [this](fidl::InterfaceRequest<app::ApplicationRunner> request) {
        runner_bindings_.AddBinding(this, std::move(request));
      });
}

App::~App() {
  icu_data::Release();
  blink::Threads::Gpu()->PostTask(QuitMessageLoop);
  blink::Threads::IO()->PostTask(QuitMessageLoop);
}

void App::StartApplication(
    app::ApplicationPackagePtr application,
    app::ApplicationStartupInfoPtr startup_info,
    fidl::InterfaceRequest<app::ApplicationController> controller) {
  std::unique_ptr<ApplicationControllerImpl> impl =
      std::make_unique<ApplicationControllerImpl>(this, std::move(application),
                                                  std::move(startup_info),
                                                  std::move(controller));
  ApplicationControllerImpl* key = impl.get();
  controllers_.emplace(key, std::move(impl));
}

void App::Destroy(ApplicationControllerImpl* controller) {
  auto it = controllers_.find(controller);
  if (it == controllers_.end())
    return;
  controllers_.erase(it);
}

}  // namespace flutter_runner
