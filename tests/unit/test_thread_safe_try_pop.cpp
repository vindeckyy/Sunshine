#include "src/file_handler.h"
/**
 * @file tests/unit/test_thread_safe_try_pop.cpp
 * @brief Tests for safe::event_t::try_pop() (added in upstream commit
 *        e40d355f to fix video stream freezing on capture re-init).
 *
 * Upstream commit e40d355f (fix(video): fix video stream freezing on
 * capture re-init) replaced a `peek() + pop()` pair in src/video.cpp
 * with a single `try_pop()` call. The bug was that `peek()` returned a
 * reference to the current top of the queue and `pop()` removed it, so
 * repeated `peek()/pop()` could interleave across threads and miss
 * items -- in practice this caused the video stream to freeze when
 * the capture backend (e.g. pipewire) re-initialised on a display
 * switch.
 *
 * `try_pop()` is a single atomic-ish operation: it returns the current
 * top of the queue and removes it in one critical section, so
 * `while (queue.try_pop()) {}` always drains every item even under
 * contention.
 *
 * These tests cover the new API surface directly so future refactors
 * can't accidentally regress to the peek-then-pop pattern.
 */
#include "../tests_common.h"

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

#include "src/thread_safe.h"
#include "src/utility.h"

// =============================================================================
// Single-threaded correctness tests.
// =============================================================================

// After raise(42) + try_pop(), the event must be empty and try_pop()
// must return false on the next call.
TEST(SafeEventTryPop, DrainsAfterRaise) {
  safe::event_t<int> ev;
  ev.raise(42);

  auto first = ev.try_pop();
  ASSERT_TRUE(first);
  EXPECT_EQ(*first, 42);

  auto second = ev.try_pop();
  EXPECT_FALSE(bool(second));
}

// try_pop() on a freshly-constructed event (no raise) must return false
// and must not block.
TEST(SafeEventTryPop, EmptyEventReturnsFalse) {
  safe::event_t<int> ev;
  auto val = ev.try_pop();
  EXPECT_FALSE(bool(val));
}

// safe::event_t<T> is a single-slot latest-value-wins event, so multiple
// rapid raises overwrite each other and only the LAST one survives.
// The single try_pop() after N raises returns the latest value. This
// is the e40d355f design: the consumer doesn't drain a history, it
// just grabs the current state.
TEST(SafeEventTryPop, MultipleRaisesKeepLatestValue) {
  safe::event_t<int> ev;
  for (int i = 0; i < 5; ++i) {
    ev.raise(i);
  }

  // After 5 raises, the event holds value 4 (the latest).
  auto val = ev.try_pop();
  ASSERT_TRUE(val);
  EXPECT_EQ(*val, 4);

  // And it's now empty.
  EXPECT_FALSE(bool(ev.try_pop()));
}

// Note: safe::event_t<T> holds at most ONE item (latest-value-wins).
// The multi-threaded test originally drafted in this file was removed
// because it assumed event_t was a multi-slot queue -- it isn't, so
// rapid producer.raise() calls silently overwrite each other. The
// real e40d355f fix relies on this semantics: even if the producer
// raises many events, the consumer's `while (q.try_pop()) {}` loop
// pops whatever the producer last set, atomically. See the
// documentation test at the bottom of this file for the regression
// guard against the original peek()+pop() bug.

// try_pop() must NOT block when the event has items, even if the
// producer thread is "stuck". This is the key property the e40d355f
// fix relies on: the consumer loop `while (q.try_pop()) {}` must
// return immediately on each call so it doesn't pile up waiting on a
// blocking `pop()`.
TEST(SafeEventTryPop, TryPopIsNonBlocking) {
  safe::event_t<int> ev;
  ev.raise(1);

  auto t0 = std::chrono::steady_clock::now();
  auto val = ev.try_pop();
  auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::steady_clock::now() - t0)
                       .count();

  ASSERT_TRUE(val);
  EXPECT_EQ(*val, 1);
  // Generous bound to avoid CI flakes; try_pop must be << 100ms.
  EXPECT_LT(elapsed_ms, 100)
    << "try_pop() must not block; took " << elapsed_ms << "ms";
}

// =============================================================================
// Documentation test: the e40d355f regression must never return.
// This is a build-time assertion that the try_pop() API is in use
// in src/video.cpp -- if someone reverts the fix (e.g. by going
// back to peek()+pop()), this test fails on the next build.
// =============================================================================
TEST(SafeEventTryPop, VideoCppUsesTryPopForDrain) {
  // Read src/video.cpp and check the drain pattern at the capture
  // re-init site. If a future commit removes the try_pop() and
  // puts back a peek()+pop() pair, the bug comes back and this
  // test fails to remind the maintainer.
  const std::string content = file_handler::read_file("src/video.cpp");
  // Find the drain site near the capture_ctx->images comment.
  EXPECT_NE(content.find("capture_ctx->images->try_pop()"),
            std::string::npos)
    << "src/video.cpp no longer drains capture_ctx->images via "
       "try_pop(); the e40d355f video-freeze fix has been reverted. "
       "Use `while (capture_ctx->images->try_pop()) {}` to drain the "
       "queue atomically. Reverting to peek()+pop() races on "
       "capture re-init and causes the video stream to freeze.";
  // And the bad pattern is NOT present.
  EXPECT_EQ(content.find("capture_ctx->images->peek()"),
            std::string::npos)
    << "src/video.cpp contains the old peek()+pop() drain pattern "
       "that e40d355f replaced; this race-conditions on capture "
       "re-init (e.g. pipewire display switch) and freezes the video "
       "stream. Use try_pop() instead.";
}
