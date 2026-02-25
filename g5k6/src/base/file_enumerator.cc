#include "base/file_enumerator.h"

#include "base/file_path.h"
#include "base/logging.h"
#include "logging.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <string>
#include <vector>
#include <unistd.h>

#include <dirent.h>
#include <errno.h>

namespace {

inline bool IsTypeMatched(const FilePath& path, const std::vector<std::string>& ext_filter) {
    if (ext_filter.empty()) {
        return true;
    }

    const auto& path_ext = path.Extension();
    for (const auto& ext : ext_filter) {
        if (ext == path_ext) {
            return true;
        }
    }
    return false;
}

}

std::vector<std::string> FileEnumerator::FetchAll(const std::string& root_path,
        const std::vector<std::string>& ext_filter,
        bool recursion) {
    std::vector<std::string> directory_entries;
    do {
        DIR* dir = opendir(root_path.c_str());
        if (!dir) {
            return directory_entries;
        }

        struct dirent* dent = NULL;
        while (errno = 0, dent = readdir(dir)) {
          if (!IsTypeMatched(std::string(dent->d_name), ext_filter)) {
            continue;
          }
          std::string file_path = root_path + "/" + dent->d_name;
          int fd = open(file_path.data(), O_RDONLY);
          uint64_t size = 0;
          if (fd) {
            size = FilePath::Size(fd);
            close(fd);
          }

          if (size > 0) {
              directory_entries.emplace_back(file_path);
          } else {
            remove(file_path.data());
          }

        //   const bool is_dir = info.IsDirectory();

        //   // Recursive mode: schedule traversal of a directory if either
        //   // SHOW_SYM_LINKS is on or we haven't visited the directory yet.
        //   if (recursive_ && is_dir &&
        //       (!ShouldTrackVisitedDirectories(file_type_) ||
        //        MarkVisited(info.stat_))) {
        //     pending_paths_.push(std::move(full_path));
        //   }

        //   if (is_pattern_matched && IsTypeMatched(is_dir)) {
        //     directory_entries.push_back(std::move(info));
        //   }
        }
        closedir(dir);
    } while (false);

    return directory_entries;
}
