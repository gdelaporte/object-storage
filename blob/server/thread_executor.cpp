//
// Copyright 2019 Jean-Francois Smigielski
//
// This software is supplied under the terms of the MIT License, a
// copy of which should be located in the distribution where this
// file was obtained (LICENSE.txt). A copy of the license may also be
// found online at https://opensource.org/licenses/MIT.
//

#include "internals.hpp"

RequestExecutor::~RequestExecutor() {
  if (fd_wakeup >= 0) {
    ::close(fd_wakeup);
    fd_wakeup = -1;
  }
}

RequestExecutor::RequestExecutor(BlobServer *srv) :
  server{srv}, fd_tokens{-1}, fd_wakeup{-1}, latch(), queue() {
  fd_wakeup = ::eventfd(0, EFD_NONBLOCK);
  fd_tokens = ::eventfd(0, EFD_NONBLOCK|EFD_SEMAPHORE);
}

static void _configure_io_priority(int rt, int high) {
#if 0
  int io_class = IOPRIO_CLASS_BE;
  int io_value = 1;
  if (rt)
    io_class = IOPRIO_CLASS_RT;
  if (high)
    io_class = 2;
  const int prio = IOPRIO_PRIO_VALUE(io_class, io_value);
  syscall(SYS_ioprio_set, IOPRIO_WHO_PROCESS, 0, prio);
#else
  (void) rt, (void) high;
#endif
}

dill_coroutine void RequestExecutor::consume(int bundle) {
  int64_t evt{0};
  ssize_t r{-1};
  std::deque<HttpRequest*> pending;

  _configure_io_priority(ioprio_rt, ioprio_high);

  while (server->running()) {
    latch.lock();
    for (int i{0}; i < BATCH_EXECUTOR; ++i) {
      if (queue.empty())
        break;
      auto p = queue.front();
      queue.pop_front();
      pending.push_back(p);
    }
    latch.unlock();

    if (pending.empty()) {
      if (0 == ::dill_fdin(fd_wakeup, ::dill_now() + SLEEP_EXECUTOR)) {
        r = ::read(fd_wakeup, &evt, sizeof(evt));
        (void) r;
      }
    } else {
      while (!pending.empty()) {
        auto p = pending.front();
        pending.pop_front();
        dill_bundle_go(bundle, execute(p));
      }
      assert(pending.empty());
    }
  }
}

void RequestExecutor::receive(HttpRequest *req) {
  latch.lock();
  queue.push_back(req);
  latch.unlock();
  ssize_t w = ::write(fd_wakeup, &one64, 8);
  (void) w;
}

dill_coroutine void RequestExecutor::execute(HttpRequest *r) {
  do {
    std::unique_ptr<HttpRequest> req(r);
    HttpReply rep(req.get());
    server->service.execute(req.get(), &rep);
  } while (0);

  // Tell there is a worker available
  ssize_t w = ::write(fd_tokens, &one64, sizeof(one64));
  (void) w;
}
