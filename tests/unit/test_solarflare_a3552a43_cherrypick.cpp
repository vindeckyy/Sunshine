/**
 * @file tests/unit/test_solarflare_a3552a43_cherrypick.cpp
 * @brief Regression guard for the round-3 cherry-pick of
 *        a3552a43 build(deps): fix building on Linux with DRM
 *        capture disabled.
 *
 * Upstream commit a3552a43 fixed a build error on Linux when
 * SUNSHINE_ENABLE_DRM=OFF. Before the fix, the cmake logic required
 * LIBDRM_FOUND *and* LIBCAP_FOUND as a single condition, so turning
 * DRM off also disabled libcap even when Wayland / KWin / Portal
 * still needed it. The fix:
 *
 *   1. libdrm is now searched for any of DRM / Wayland / Vulkan /
 *      KWin / Portal (not just DRM), but only LINKED when DRM is on.
 *   2. The DRM-specific blocks (SUNSHINE_BUILD_DRM, kmsgrab.cpp,
 *      EGL_NO_X11=1) are now nested inside 'if(SUNSHINE_ENABLE_DRM)'.
 *   3. libcap is now found for any LINUX build (not just DRM).
 *   4. The "Couldn't find either cuda, ..." FATAL_ERROR now only
 *      requires one of cuda / libdrm / libva / kwin / pipewire /
 *      portal / wayland / x11 (not the 'libdrm and libcap' pair).
 *
 * The cherry-pick was applied in round 3 with auto-merge (the fork
 * hadn't touched the affected file).
 *
 * These tests are build-time guards: if a future commit reverts the
 * 'if(SUNSHINE_ENABLE_DRM)' nesting (so DRM-off no longer skips
 * kmsgrab.cpp), the cherry-pick is broken and users with DRM off
 * (e.g. headless servers, X11-only setups) will fail to build.
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

  // Find the line containing `marker` and return the line's text, or
  // an empty string if not found.
  std::string find_line(const std::string &content, const std::string &marker) {
    size_t pos = content.find(marker);
    if (pos == std::string::npos) return "";
    size_t line_start = content.rfind('\n', pos);
    if (line_start == std::string::npos) line_start = 0; else line_start += 1;
    size_t line_end = content.find('\n', pos);
    if (line_end == std::string::npos) line_end = content.size();
    return content.substr(line_start, line_end - line_start);
  }

}  // namespace

// =============================================================================
// 1. The SUNSHINE_ENABLE_DRM conditional nests the DRM-only blocks.
//    Pre-a3552a43 the lines were at the top level of the LIBDRM_FOUND
//    block, so DRM-off would silently skip them but the build would
//    also try to compile kmsgrab.cpp which needs EGL_NO_X11=1.
// =============================================================================

TEST(SolarflareBuildDrmCherryPick, DrmBlocksNestedInEnableDrm) {
  const auto content = read_file("cmake/compile_definitions/linux.cmake");
  ASSERT_FALSE(content.empty())
    << "Could not read cmake/compile_definitions/linux.cmake.";

  // The cherry-pick nests the DRM-specific stuff inside an
  // 'if(SUNSHINE_ENABLE_DRM)' block. All three lines (DRM compile
  // def, kmsgrab.cpp target, EGL_NO_X11=1) must be inside that
  // block.
  const std::string drm_def_line = find_line(content, "SUNSHINE_BUILD_DRM");
  const std::string kmsgrab_line = find_line(content, "kmsgrab.cpp");
  const std::string egl_line = find_line(content, "EGL_NO_X11=1");

  // Use a windowed search to verify each DRM-only block is inside
  // an 'if(SUNSHINE_ENABLE_DRM)' gate. The 'if' may be on a previous
  // line (in the case of multi-line 'if/endif' blocks), so we look
  // at the 200 bytes before each marker.
  auto check_nested_in_drm = [&content](const std::string &marker,
                                      const char *marker_desc) {
    const size_t pos = content.find(marker);
    if (pos == std::string::npos) {
      return ::testing::AssertionFailure()
        << "Could not find '" << marker << "' in linux.cmake.";
    }
    const size_t window_start = (pos >= 200) ? pos - 200 : 0;
    const std::string window = content.substr(window_start, pos - window_start);
    if (window.find("if(${SUNSHINE_ENABLE_DRM})") == std::string::npos) {
      return ::testing::AssertionFailure()
        << marker_desc << " ('" << marker
        << "') is not nested inside an 'if(SUNSHINE_ENABLE_DRM)' "
           "block. The a3552a43 cherry-pick nested all DRM-specific "
           "code so it can be turned off via -DSUNSHINE_ENABLE_DRM=OFF. "
           "Re-apply the cherry-pick.\nContext (200 bytes before):\n"
        << window;
    }
    return ::testing::AssertionSuccess();
  };

  EXPECT_TRUE(check_nested_in_drm("add_compile_definitions(SUNSHINE_BUILD_DRM)",
                                    "The SUNSHINE_BUILD_DRM compile def"));
  EXPECT_TRUE(check_nested_in_drm("kmsgrab.cpp",
                                    "The kmsgrab.cpp target_files entry"));
  EXPECT_TRUE(check_nested_in_drm("EGL_NO_X11=1",
                                    "The EGL_NO_X11=1 compile def"));
}

// =============================================================================
// 2. libcap is found for any LINUX build, not just DRM. Pre-a3552a43
//    libcap was gated on SUNSHINE_ENABLE_DRM; the cherry-pick made
//    it gated on LINUX (the global LINUX flag).
// =============================================================================

TEST(SolarflareBuildDrmCherryPick, LibCapForAnyLinuxBuild) {
  const auto content = read_file("cmake/compile_definitions/linux.cmake");
  ASSERT_FALSE(content.empty())
    << "Could not read cmake/compile_definitions/linux.cmake.";

  // Find the LIBCAP block. After the cherry-pick, the
  // 'find_package(LIBCAP REQUIRED)' is preceded (on the previous
  // line) by 'if(LINUX)' (the global LINUX flag), not
  // 'if(SUNSHINE_ENABLE_DRM)'. Use a 100-byte window before
  // the find_package call to look for the if() gate.
  const size_t libcap_pos = content.find("find_package(LIBCAP REQUIRED)");
  ASSERT_NE(libcap_pos, std::string::npos)
    << "Could not find 'find_package(LIBCAP REQUIRED)' in the file.";
  const size_t window_start = (libcap_pos >= 100) ? libcap_pos - 100 : 0;
  const std::string window = content.substr(window_start, libcap_pos - window_start);
  EXPECT_TRUE(window.find("if(LINUX)") != std::string::npos)
    << "The 100 bytes before 'find_package(LIBCAP REQUIRED)' is:\n"
       << window << "\nThe a3552a43 cherry-pick should gate it on "
       "'if(LINUX)' so libcap is found for any Linux build, not "
       "just DRM. Re-apply the cherry-pick.";

  // The old "if(${SUNSHINE_ENABLE_DRM})" gating of libcap must be
  // gone. The old format was either "if(SUNSHINE_ENABLE_DRM)\n
  // find_package(LIBCAP" or the libcap find_package was inside the
  // DRM block. Either way, a SUNSHINE_ENABLE_DRM-gated libcap find
  // is wrong.
  EXPECT_FALSE(contains(content, "if(${SUNSHINE_ENABLE_DRM})\n    find_package(LIBCAP"))
    << "The old 'if(SUNSHINE_ENABLE_DRM) find_package(LIBCAP' "
       "chain is still in the file. The a3552a43 cherry-pick moved "
       "libcap to 'if(LINUX)'. Re-apply the cherry-pick.";
}

// =============================================================================
// 3. The "Couldn't find" FATAL_ERROR only requires ONE of cuda / libdrm
//    / libva / kwin / pipewire / portal / wayland / x11 (not the
//    old "(libdrm and libcap)" pair). The old condition required
//    both LIBDRM_FOUND AND LIBCAP_FOUND which was wrong because
//    libdrm is needed for Wayland/KWin/Portal too.
// =============================================================================

TEST(SolarflareBuildDrmCherryPick, FatalErrorLoosened) {
  const auto content = read_file("cmake/compile_definitions/linux.cmake");
  ASSERT_FALSE(content.empty())
    << "Could not read cmake/compile_definitions/linux.cmake.";

  EXPECT_TRUE(contains(content, "AND NOT ${LIBDRM_FOUND}"))
    << "The 'Couldn't find either ...' FATAL_ERROR is missing the "
       "'AND NOT ${LIBDRM_FOUND}' condition. The a3552a43 cherry-pick "
       "changed it from 'NOT (${LIBDRM_FOUND} AND ${LIBCAP_FOUND})' "
       "(required BOTH) to 'AND NOT ${LIBDRM_FOUND}' (just one). "
       "Re-apply the cherry-pick.";

  EXPECT_FALSE(contains(content, "AND NOT (${LIBDRM_FOUND} AND ${LIBCAP_FOUND})"))
    << "The 'Couldn't find either ...' FATAL_ERROR still has the "
       "old 'AND NOT (${LIBDRM_FOUND} AND ${LIBCAP_FOUND})' pair "
       "check. The a3552a43 cherry-pick loosened this to just "
       "'AND NOT ${LIBDRM_FOUND}'. Re-apply the cherry-pick.";

  // And the FATAL_ERROR message was updated to drop the 'and' between
  // libdrm and libcap.
  EXPECT_TRUE(contains(content, "cuda, libdrm, libva, kwin, pipewire, portal, wayland or x11"))
    << "The FATAL_ERROR message is missing the new wording "
       "'cuda, libdrm, libva, kwin, pipewire, portal, wayland or "
       "x11'. The a3552a43 cherry-pick updated the message to "
       "drop the 'and' between 'libdrm' and 'libva' (libcap is no "
       "longer required to be paired with libdrm). Re-apply the "
       "cherry-pick.";

  EXPECT_FALSE(contains(content, "cuda, (libdrm and libcap), libva, kwin, pipewire, portal, wayland or x11"))
    << "The FATAL_ERROR message still has the old wording "
       "'cuda, (libdrm and libcap), libva, kwin, pipewire, portal, "
       "wayland or x11'. The a3552a43 cherry-pick removed the "
       "'and libcap' parenthetical. Re-apply the cherry-pick.";
}
