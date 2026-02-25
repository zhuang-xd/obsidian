#include "base/file_path.h"

#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <string>
#include <unistd.h>
#include <vector>

#include <dirent.h>
#include <errno.h>


namespace {

using StringType = FilePath::StringType;

inline StringType::size_type FinalExtensionSeparatorPosition(const StringType& path) {
  // Special case "." and ".."
  if (path == FilePath::kCurrentDirectory ||
      path == FilePath::kParentDirectory) {
    return StringType::npos;
  }

  return path.rfind(FilePath::kExtensionSeparator);
}


// Same as above, but allow a second extension component of up to 4
// characters when the rightmost extension component is a common double
// extension (gz, bz2, Z).  For example, foo.tar.gz or foo.tar.Z would have
// extension components of '.tar.gz' and '.tar.Z' respectively.
StringType::size_type ExtensionSeparatorPosition(const StringType& path) {
  const StringType::size_type last_dot = FinalExtensionSeparatorPosition(path);

  // No extension, or the extension is the whole filename.
  if (last_dot == StringType::npos || last_dot == 0U) {
    return last_dot;
  }

  const StringType::size_type penultimate_dot =
      path.rfind(FilePath::kExtensionSeparator, last_dot - 1);
  const StringType::size_type last_separator = path.find_last_of(
      FilePath::kSeparators, last_dot - 1, FilePath::kSeparatorsLength - 1);

  if (penultimate_dot == StringType::npos ||
      (last_separator != StringType::npos &&
       penultimate_dot < last_separator)) {
    return last_dot;
  }

  return last_dot;
}

}

constexpr FilePath::CharType FilePath::kSeparators[];
constexpr FilePath::CharType FilePath::kCurrentDirectory[];
constexpr FilePath::CharType FilePath::kParentDirectory[];

//     // The special path component meaning "the parent directory."
// static constexpr FilePath CharType FilePath::kParentDirectory[3] = "..";

FilePath::FilePath(const std::string& path)
  : path_(path) {

}

FilePath::StringType FilePath::Extension(void) const {
  FilePath base(BaseName());
  const StringType::size_type dot = ExtensionSeparatorPosition(base.path_);
  if (dot == StringType::npos) {
    return StringType();
  }

  return base.path_.substr(dot, StringType::npos);
}

FilePath::StringType FilePath::BaseName(void) const {
  FilePath new_path(path_);

  // The drive letter, if any, is always stripped.
  // StringType::size_type letter = FindDriveLetter(new_path.path_);
  // if (letter != StringType::npos) {
  //   new_path.path_.erase(0, letter + 1);
  // }

  // Keep everything after the final separator, but if the pathname is only
  // one character and it's a separator, leave it alone.
  StringType::size_type last_separator = new_path.path_.find_last_of(
      FilePath::kSeparators, StringType::npos, kSeparatorsLength - 1);
  if (last_separator != StringType::npos &&
      last_separator < new_path.path_.length() - 1) {
    new_path.path_.erase(0, last_separator + 1);
  }
  return new_path.path_;
}

int64_t FilePath::Size(int fd) {
  struct stat file_stat;
  int64_t size = -1;

  if (fstat(fd, &file_stat) != -1) {
    size = file_stat.st_size;
  }

  return size;
}
