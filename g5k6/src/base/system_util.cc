#include "base/system_util.h"
#include "base/eintr_wrapper.h"
#include "base/logging.h"
#include "base/string_util.h"

#include <fcntl.h>
#include <sched.h>
#include <setjmp.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <sys/prctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace base {

LaunchOptions::LaunchOptions() = default;

LaunchOptions::LaunchOptions(const LaunchOptions& other) = default;

LaunchOptions::~LaunchOptions() = default;

// Set the calling thread's signal mask to new_sigmask and return
// the previous signal mask.
sigset_t SetSignalMask(const sigset_t& new_sigmask) {
  sigset_t old_sigmask;
  pthread_sigmask(SIG_SETMASK, &new_sigmask, &old_sigmask);
  return old_sigmask;
}

int LaunchProcess(const std::string& command_line, bool wait) {
    LaunchOptions options;
  options.wait = wait;
  std::vector<std::string> argv = SplitString(command_line, ' ');
  return LaunchProcess(argv, options);
}

int LaunchProcess(const std::vector<std::string>& argv,
                  const LaunchOptions& options) {
  std::vector<char*> argv_cstr;
  argv_cstr.reserve(argv.size() + 1);
  for (const auto& arg : argv)
    argv_cstr.push_back(const_cast<char*>(arg.c_str()));
  argv_cstr.push_back(nullptr);

  // std::unique_ptr<char* []> new_environ;
  // char* const empty_environ = nullptr;
  // char* const* old_environ = GetEnvironment();
  // if (options.clear_environment)
  //   old_environ = &empty_environ;
  // if (!options.environment.empty())
  //   new_environ = internal::AlterEnvironment(old_environ, options.environment);

  sigset_t full_sigset;
  sigfillset(&full_sigset);
  const sigset_t orig_sigmask = SetSignalMask(full_sigset);

  const char* current_directory = nullptr;
  if (!options.current_directory.empty()) {
    current_directory = options.current_directory.value().c_str();
  }

  pid_t pid;
//   base::TimeTicks before_fork = TimeTicks::Now();
  pid = fork();

  // Always restore the original signal mask in the parent.
  if (pid != 0) {
    // base::TimeTicks after_fork = TimeTicks::Now();
    SetSignalMask(orig_sigmask);
  }

  if (pid < 0) {
    // DPLOG(ERROR) << "fork";
    return pid;
  }
  if (pid == 0) {
    // Child process

    // DANGER: no calls to malloc or locks are allowed from now on:
    // http://crbug.com/36678

    // DANGER: fork() rule: in the child, if you don't end up doing exec*(),
    // you call _exit() instead of exit(). This is because _exit() does not
    // call any previously-registered (in the parent) exit handlers, which
    // might do things like block waiting for threads that don't even exist
    // in the child.

    // // See comments on the ResetFDOwnership() declaration in
    // // base/files/scoped_file.h regarding why this is called early here.
    // subtle::ResetFDOwnership();

    {
      // If a child process uses the readline library, the process block
      // forever. In BSD like OSes including OS X it is safe to assign /dev/null
      // as stdin. See http://crbug.com/56596.
      int null_fd = HANDLE_EINTR(open("/dev/null", O_RDONLY));
      if (null_fd == -1) {
        // LOGE("Failed to open /dev/null");
        _exit(127);
      }

      int new_fd = HANDLE_EINTR(dup2(null_fd, STDIN_FILENO));
      if (new_fd != STDIN_FILENO) {
        LOGE("Failed to dup /dev/null for stdin");
        _exit(127);
      }
    }

    if (options.new_process_group) {
      // Instead of inheriting the process group ID of the parent, the child
      // starts off a new process group with pgid equal to its process ID.
      if (setpgid(0, 0) < 0) {
        LOGE("setpgid failed");
        _exit(127);
      }
    }

    if (options.maximize_rlimits) {
      // Some resource limits need to be maximal in this child.
      for (auto resource : *options.maximize_rlimits) {
        struct rlimit limit;
        if (getrlimit(resource, &limit) < 0) {
          LOGW("getrlimit failed");
        } else if (limit.rlim_cur < limit.rlim_max) {
          limit.rlim_cur = limit.rlim_max;
          if (setrlimit(resource, &limit) < 0) {
            LOGW("setrlimit failed");
          }
        }
      }
    }

    // ResetChildSignalHandlersToDefaults();
    SetSignalMask(orig_sigmask);

    // // Cannot use STL iterators here, since debug iterators use locks.
    // // NOLINTNEXTLINE(modernize-loop-convert)
    // for (size_t i = 0; i < options.fds_to_remap.size(); ++i) {
    //   const FileHandleMappingVector::value_type& value =
    //       options.fds_to_remap[i];
    //   fd_shuffle1.push_back(InjectionArc(value.first, value.second, false));
    //   fd_shuffle2.push_back(InjectionArc(value.first, value.second, false));
    // }

    // if (!options.environment.empty() || options.clear_environment)
      // SetEnvironment(new_environ.get());

    // // fd_shuffle1 is mutated by this call because it cannot malloc.
    // if (!ShuffleFileDescriptors(&fd_shuffle1))
    //   _exit(127);

    // CloseSuperfluousFds(fd_shuffle2);

    // Set NO_NEW_PRIVS by default. Since NO_NEW_PRIVS only exists in kernel
    // 3.5+, do not check the return value of prctl here.
#ifndef PR_SET_NO_NEW_PRIVS
#define PR_SET_NO_NEW_PRIVS 38
#endif
    if (!options.allow_new_privs) {
      if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) && errno != EINVAL) {
        // Only log if the error is not EINVAL (i.e. not supported).
        LOGE("prctl(PR_SET_NO_NEW_PRIVS) failed");
      }
    }

    if (options.kill_on_parent_death) {
      if (prctl(PR_SET_PDEATHSIG, SIGKILL) != 0) {
        LOGE("prctl(PR_SET_PDEATHSIG) failed");
        _exit(127);
      }
    }

    if (current_directory != nullptr) {
      chdir(current_directory);
    }

    // if (options.pre_exec_delegate != nullptr) {
    //   options.pre_exec_delegate->RunAsyncSafe();
    // }

    const char* executable_path = !options.real_path.empty() ?
        options.real_path.value().c_str() : argv_cstr[0];

    execvp(executable_path, argv_cstr.data());

    LOGE("LaunchProcess: failed to execvp:");
    // LOGE(argv_cstr[0]);
    _exit(127);
  } else {
    // Parent process
    if (options.wait) {
      // While this isn't strictly disk IO, waiting for another process to
      // finish is the sort of thing ThreadRestrictions is trying to prevent.
    //   ScopedBlockingCall scoped_blocking_call(FROM_HERE,
    //                                           BlockingType::MAY_BLOCK);
      HANDLE_EINTR(waitpid(pid, nullptr, 0));
    //   DPCHECK(ret > 0);
    }
  }

  return pid;
}

}  // namespace base
