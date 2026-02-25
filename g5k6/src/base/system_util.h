#ifndef BASE_SYSTEM_UTIL_H_
#define BASE_SYSTEM_UTIL_H_

#include <cstdint>
#include <string>
#include <vector>

#include "base/file_path.h"

namespace base {
// Options for launching a subprocess that are passed to LaunchProcess().
// The default constructor constructs the object with default options.
struct LaunchOptions {
  LaunchOptions();
  LaunchOptions(const LaunchOptions&);
  ~LaunchOptions();

  // If true, wait for the process to complete.
  bool wait = false;

  // If not empty, change to this directory before executing the new process.
  FilePath current_directory;

//   // Remap file descriptors according to the mapping of src_fd->dest_fd to
//   // propagate FDs into the child process.
//   FileHandleMappingVector fds_to_remap;

//   // Set/unset environment variables. These are applied on top of the parent
//   // process environment.  Empty (the default) means to inherit the same
//   // environment. See internal::AlterEnvironment().
//   EnvironmentMap environment;

  // Clear the environment for the new process before processing changes from
  // |environment|.
  bool clear_environment = false;
  // If non-zero, start the process using clone(), using flags as provided.
  // Unlike in clone, clone_flags may not contain a custom termination signal
  // that is sent to the parent when the child dies. The termination signal will
  // always be set to SIGCHLD.
  int clone_flags = 0;

  // By default, child processes will have the PR_SET_NO_NEW_PRIVS bit set. If
  // true, then this bit will not be set in the new child process.
  bool allow_new_privs = false;

  // Sets parent process death signal to SIGKILL.
  bool kill_on_parent_death = false;

  // If not empty, launch the specified executable instead of
  // cmdline.GetProgram(). This is useful when it is necessary to pass a custom
  // argv[0].
  FilePath real_path;


  // Each element is an RLIMIT_* constant that should be raised to its
  // rlim_max.  This pointer is owned by the caller and must live through
  // the call to LaunchProcess().
  const std::vector<int>* maximize_rlimits = nullptr;

  // If true, start the process in a new process group, instead of
  // inheriting the parent's process group.  The pgid of the child process
  // will be the same as its pid.
  bool new_process_group = true;
};

int LaunchProcess(const std::vector<std::string>& argv,
                  const LaunchOptions& options);

int LaunchProcess(const std::string& command_line, bool wait = true);
};

#endif  // BASE_SYSTEM_UTIL_H_
