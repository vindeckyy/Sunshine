/**
 * @file tests/unit/test_solarflare_fdf13632_cherrypick.cpp
 * @brief Regression guard for the round-3 cherry-pick of
 *        fdf13632 feat(linux/kwin): log object serial when available.
 *
 * Upstream commit fdf13632 added an info-level log message at the
 * point the kwin screen-cast stream is created. Before the fix, the
 * log was just '[kwingrab] stream created, PipeWire node <id>'. After
 * the fix, the log shows the pipewire OBJECT SERIAL (when available)
 * as well as the legacy node id, which is what Sunshine passes to
 * pw_stream_connect for the new object-serial connect path.
 *
 * The cherry-pick was applied in round 3 with auto-merge (the fork
 * hadn't touched kwingrab.cpp).
 *
 * These tests are build-time guards: if a future commit drops the
 * object-serial log line, these tests fail with a clear error
 * message pointing at the round-3 cherry-pick so the maintainer
 * can re-apply the fix.
 */
#include "../tests_common.h"

#include "src/file_handler.h"

#include <string>

namespace {

  std::string read_file(const std::string &path) {
    return file_handler::read_file(path.c_str());
  }

  bool contains(const std::string &haystack, const std::string &needle) {
    return haystack.find(needle) != std::string::npos;
  }

}  // namespace

// =============================================================================
// 1. The new log line at the point of stream creation distinguishes
//    between the "no object serial available" and "object serial
//    available" cases. Pre-fdf13632 the log was unconditional.
// =============================================================================

TEST(SolarflareKwinLogCherryPick, NewLogLineDistinguishesObjectSerialCases) {
  const auto content = read_file("src/platform/linux/kwingrab.cpp");
  ASSERT_FALSE(content.empty())
    << "Could not read src/platform/linux/kwingrab.cpp.";

  // The new code path is:
  //   if ((out_objectserial & SPA_ID_INVALID) == SPA_ID_INVALID) {
  //     BOOST_LOG(info) << "...: node=" << out_node_id;
  //   } else {
  //     BOOST_LOG(info) << "...: objectserial=" << out_objectserial
  //                     << " (node=" << out_node_id << ")";
  //   }
  // Both branches must be present.
  EXPECT_TRUE(contains(content, "Pipewire stream created: node="))
    << "src/platform/linux/kwingrab.cpp is missing the "
       "'Pipewire stream created: node=' log line (the "
       "no-object-serial-available branch). The fdf13632 "
       "cherry-pick added this branch. Re-apply the cherry-pick.";
  EXPECT_TRUE(contains(content, "Pipewire stream created: objectserial="))
    << "src/platform/linux/kwingrab.cpp is missing the "
       "'Pipewire stream created: objectserial=' log line (the "
       "object-serial-available branch). The fdf13632 cherry-pick "
       "added this so operators can see which connect path "
       "pw_stream_connect will use. Re-apply the cherry-pick.";
}

// =============================================================================
// 2. The conditional gates the two branches. The conditional must
//    test whether out_objectserial is SPA_ID_INVALID; the SPA_ID_INVALID
//    value signals "no object serial" (kwin-6+ may not populate it).
// =============================================================================

TEST(SolarflareKwinLogCherryPick, ConditionalChecksObjectSerialInvalid) {
  const auto content = read_file("src/platform/linux/kwingrab.cpp");
  ASSERT_FALSE(content.empty())
    << "Could not read src/platform/linux/kwingrab.cpp.";

  EXPECT_TRUE(contains(content, "(out_objectserial & SPA_ID_INVALID) == SPA_ID_INVALID"))
    << "src/platform/linux/kwingrab.cpp is missing the "
       "'(out_objectserial & SPA_ID_INVALID) == SPA_ID_INVALID' "
       "conditional. The fdf13632 cherry-pick uses this to decide "
       "whether to include the objectserial= prefix in the log "
       "line. Re-apply the cherry-pick.";
}

// =============================================================================
// 3. The old unconditional log line is GONE. Pre-fdf13632 the line
//    was just '[kwingrab] stream created, PipeWire node <id>' with
//    no object-serial mention. The new code replaces this entirely.
//    The new log lines are 'Pipewire stream created: ...' (note the
//    capital W in 'Pipewire' and the absence of the trailing ',')
// =============================================================================

TEST(SolarflareKwinLogCherryPick, OldUnconditionalLogLineRemoved) {
  const auto content = read_file("src/platform/linux/kwingrab.cpp");
  ASSERT_FALSE(content.empty())
    << "Could not read src/platform/linux/kwingrab.cpp.";

  // The old log line had '[kwingrab] stream created, PipeWire node '
  // (capital P, lowercase w in 'stream created', trailing comma).
  // The new log lines start with '[kwingrab] Pipewire stream created:'
  // (capital P, capital W, no comma).
  EXPECT_FALSE(contains(content, "[kwingrab] stream created, PipeWire node "))
    << "src/platform/linux/kwingrab.cpp still contains the old "
       "unconditional log line '[kwingrab] stream created, "
       "PipeWire node <id>'. The fdf13632 cherry-pick replaced "
       "this with the two-branch log that includes the object "
       "serial when available. Re-apply the cherry-pick.";
}
