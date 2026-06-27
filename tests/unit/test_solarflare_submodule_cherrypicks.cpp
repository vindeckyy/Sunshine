/**
 * @file tests/unit/test_solarflare_submodule_cherrypicks.cpp
 * @brief Regression guard for the round-6 submodule-pointer
 *        cherry-picks.
 */
#include "../tests_common.h"

#include "src/file_handler.h"

#include <string>
#include <sys/stat.h>

namespace {

  std::string read_file(const std::string &path) {
    return file_handler::read_file(path.c_str());
  }

  bool contains(const std::string &haystack, const std::string &needle) {
    return haystack.find(needle) != std::string::npos;
  }

  // Walk up the directory tree until we find a .gitmodules file.
  // The test binary's CWD is build/tests/ (2 levels deep), so we
  // need to walk up twice to reach the source root.
  std::string find_source_root() {
    std::string path = ".";
    while (true) {
      if (!read_file(path + "/.gitmodules").empty()) {
        return path;
      }
      // Go up: strip the last "/" to drop the current dir.
      auto slash = path.find_last_of('/');
      if (slash == std::string::npos) {
        // We started at "." and there's no "/" — try "/" and stop.
        return read_file("/.gitmodules").empty() ? "" : "/";
      }
      if (slash == 0) {
        // We're at "/" already.
        return read_file("/.gitmodules").empty() ? "" : "/";
      }
      path = path.substr(0, slash);
      if (path.empty()) path = "/";
    }
  }

  bool is_lower_hex_40(const std::string &s) {
    if (s.size() < 40) return false;
    for (int i = 0; i < 40; ++i) {
      char c = s[i];
      if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
    }
    return true;
  }

  // Follow the .git file's "gitdir:" line to the submodule's
  // HEAD file. HEAD is a 40-char lowercase hex SHA (possibly with
  // or without a trailing newline) when the submodule is on a
  // detached HEAD, or a "ref: refs/heads/<branch>" line if it's
  // on a branch.
  std::string read_submodule_head_sha(const std::string &source_root,
                                      const std::string &path) {
    const std::string dot_git = source_root + "/" + path + "/.git";
    const std::string dot_git_contents = read_file(dot_git);
    if (dot_git_contents.empty()) return "";
    const std::string marker = "gitdir: ";
    const size_t pos = dot_git_contents.find(marker);
    if (pos == std::string::npos) return "";
    size_t eol = dot_git_contents.find('\n', pos);
    if (eol == std::string::npos) eol = dot_git_contents.size();
    std::string gitdir = dot_git_contents.substr(pos + marker.size(),
                                                  eol - pos - marker.size());
    // gitdir is relative to the submodule root; resolve it.
    size_t slash = path.find_last_of('/');
    const std::string parent = (slash == std::string::npos) ? "." : path.substr(0, slash);
    const std::string head_path = source_root + "/" + parent + "/" + gitdir + "/HEAD";
    const std::string head = read_file(head_path);
    if (head.empty()) return "";
    // The HEAD file may or may not have a trailing newline.
    if (is_lower_hex_40(head)) return head.substr(0, 40);
    return "";
  }

  struct SubmodulePointer {
    const char *name;
    const char *path;
    const char *round6_sha_prefix;  // first 7 chars of the bumped SHA
  };

  constexpr SubmodulePointer kSubmodules[] = {
    {"lizardbyte-common",   "third-party/lizardbyte-common",   "8d7dcc9"},
    {"moonlight-common-c",  "third-party/moonlight-common-c",  "47b4d33"},
    {"nvapi",               "third-party/nvapi",               "cd6918f"},
  };

}  // namespace

TEST(SolarflareSubmoduleCherryPicks, OnDiskShasAreRound6Bumped) {
  const std::string source_root = find_source_root();
  EXPECT_FALSE(source_root.empty())
    << "Could not find .gitmodules by walking up from the test CWD. "
       "Make sure the test is run from a build/ subdir of the source root.";

  for (const auto &sm : kSubmodules) {
    const std::string sha = read_submodule_head_sha(source_root, sm.path);
    EXPECT_FALSE(sha.empty())
      << "Could not read HEAD SHA for submodule " << sm.name
      << " (path=" << sm.path << "). The submodule is either not "
         "checked out or the .git file is malformed. Run 'git "
         "submodule update --init --recursive'.";
    if (!sha.empty()) {
      EXPECT_TRUE(contains(sha, sm.round6_sha_prefix))
        << "Submodule " << sm.name
        << " is at SHA '" << sha << "', which does not start "
           "with the expected round-6 prefix '"
        << sm.round6_sha_prefix
        << "'. The submodule pointer has been reverted. Re-apply "
           "the round-6 cherry-pick.";
    }
  }
}

TEST(SolarflareSubmoduleCherryPicks, SubmoduleDirectoriesPresent) {
  const std::string source_root = find_source_root();
  EXPECT_FALSE(source_root.empty());

  for (const auto &sm : kSubmodules) {
    const std::string full = source_root + "/" + sm.path;
    struct stat sb;
    EXPECT_EQ(stat(full.c_str(), &sb), 0)
      << "Submodule directory " << full
      << " does not exist on disk. Run 'git submodule update "
         "--init --recursive' to check it out.";
  }
}
