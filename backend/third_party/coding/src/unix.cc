// Copyright 2023 Corey Kosak
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "kosak/coding/unix.h"

#include <dirent.h>
#include <fcntl.h>
#include <fts.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <functional>
#include <string>
#include "kosak/coding/coding.h"
#include "kosak/coding/failures.h"
#include "kosak/coding/mystream.h"

#define HERE KOSAK_CODING_HERE

using kosak::coding::BufferStream;

namespace kosak::coding::nsunix {

namespace {
class PrettyError {
public:
  explicit PrettyError(int errnum);

  const char *data() const { return data_; }

private:
  char data_[256];

  friend std::ostream &operator<<(std::ostream &s, const PrettyError &o) {
    return s << o.data_;
  }
};
}  // namespace

FileCloser &FileCloser::operator=(FileCloser &&other) noexcept {
  FailRoot fr(true);
  (void)tryClose(fr.nest(HERE));
  fd_ = other.fd_;
  other.fd_ = -1;
  return *this;
}

FileCloser::~FileCloser() {
  FailRoot fr(true);
  (void)tryClose(fr.nest(HERE));
}

bool FileCloser::tryClose(const FailFrame &ff) {
  if (fd_ < 0) {
    // Not an error to close an already closed FileCloser.
    return true;
  }
  auto fdToUse = fd_;
  fd_ = -1;
  return nsunix::tryClose(fdToUse, ff.nest(HERE));
}

PipeCloser::~PipeCloser() {
  FailRoot fr(true);
  (void)tryClose(fr.nest(HERE));
}

bool PipeCloser::tryClose(const FailFrame &/*ff*/) {
  if (pipe_ == nullptr) {
    // not an error to not have a pipe
    return true;
  }
  pclose(pipe_.release());
  return true;
}

bool tryMakeFile(const std::string &filename, mode_t mode, const std::string_view contents,
    const FailFrame &ff) {
  FileCloser fc;
  return tryOpen(filename, O_RDWR | O_CREAT | O_TRUNC, mode, &fc, ff.nest(HERE)) &&
      tryWriteAll(fc.get(), contents.data(), contents.size(), ff.nest(HERE)) &&
      fc.tryClose(ff.nest(HERE));
}

bool tryMakeFileOfSize(const std::string &filename, mode_t mode, size_t size, const FailFrame &ff) {
  FileCloser fc;
  if (!tryOpen(filename, O_RDWR | O_CREAT | O_TRUNC, mode, &fc, ff.nest(HERE))) {
    return false;
  }

  if (size == 0) {
    return fc.tryClose(ff.nest(HERE));
  }

  char ch = 0;
  off_t dummy;
  return (tryLseek(fc.get(), off_t(size - 1), SEEK_SET, &dummy, ff.nest(HERE)) &&
    tryWriteAll(fc.get(), &ch, 1, ff.nest(HERE))) &&
    fc.tryClose(ff.nest(HERE));
}

bool tryMakeDirectory(const std::string &dirName, mode_t mode, const FailFrame &ff) {
  int result = mkdir(dirName.data(), mode);
  if (result != 0) {
    return ff.failf(HERE, R"(mkdir("%o",%o) failed, errno=%o)", dirName, mode, errno);
  }
  return true;
}

bool tryEnsureBaseExists(const std::string &pathname, mode_t mode, const FailFrame &ff) {
  size_t searchPos = 0;
  while (true) {
    auto firstSlash = pathname.find_first_of('/', searchPos);
    if (firstSlash == std::string::npos) {
      return true;
    }
    searchPos = firstSlash + 1;
    if (firstSlash == 0) {
      // Ignore filesystem root.
      continue;
    }
    auto baseDir = pathname.substr(0, firstSlash);
    if (baseDir.empty()) {
      continue;
    }
    bool exists;
    if (!tryExists(baseDir, &exists, ff.nest(HERE))) {
      return false;
    }
    if (exists) {
      continue;
    }
    if (!tryMakeDirectory(baseDir, mode, ff.nest(HERE))) {
      return false;
    }
  }
}

bool tryOpen(const std::string &filename, int flags, mode_t mode, FileCloser *fc,
    const FailFrame &ff) {

  int fd = open(filename.data(), flags, mode);
  if (fd < 0) {
    return ff.failf(HERE, "open(\"%o\",%o,%o) failed, error=%o", filename, flags, mode, PrettyError(errno));
  }
  *fc = FileCloser(fd);
  return true;
}

bool tryRead(int fd, void *buffer, size_t bufferSize, size_t *bytesRead, const FailFrame &ff) {
  auto retval = read(fd, buffer, bufferSize);
  if (retval < 0) {
    return ff.failf(HERE, "read(%o,%o,%o) failed, errno=%o", fd, buffer, bufferSize,
        PrettyError(errno));
  }
  *bytesRead = (size_t)retval;
  return true;
}

bool tryReadAll(const std::string &filename, std::string *result, const FailFrame &ff) {
  FileCloser fc;
  struct stat st {};
  if (!tryOpen(filename, O_RDONLY, 0, &fc, ff.nest(HERE)) ||
    !tryFstat(fc.get(), &st, ff.nest(HERE))) {
    return false;
  }
  result->resize(st.st_size);
  return tryReadAll(fc.get(), result->data(), result->size(), ff.nest(HERE));
}

bool tryReadAll(int fd, void *buffer, size_t bufferSize, const FailFrame &ff) {
  auto remaining = bufferSize;
  while (remaining != 0) {
    size_t bytesRead;
    if (!tryRead(fd, buffer, remaining, &bytesRead, ff.nest(HERE))) {
      return false;
    }
    if (bytesRead == 0) {
      return ff.failf(HERE, "Short read: requested %o, got %o", bufferSize, bufferSize - remaining);
    }
    buffer = static_cast<void*>(static_cast<char*>(buffer) + bytesRead);
    remaining -= bytesRead;
  }
  return true;
}

bool tryGetNextLine(std::string_view *src, std::string_view *result, const FailFrame &ff) {
  if (src->empty()) {
    return ff.fail(HERE, "No more lines");
  }

  auto curr = src->begin();
  auto lineEnd = src->end();  // default value
  while (curr != src->end()) {
    char ch = *curr;
    // optimistic
    lineEnd = curr++;

    // if \n
    if (ch == '\n') {
      break;
    }
    // \r or \r\n
    if (ch == '\r') {
      if (curr != src->end() && *curr == '\n') {
        ++curr;
      }
      break;
    }
  }

  auto lineSize = lineEnd - src->begin();
  auto srcAdvance = curr - src->begin();
  *result = std::string_view(src->data(), lineSize);
  *src = src->substr(srcAdvance);
  return true;
}

bool tryWrite(int fd, const void *buf, size_t count, ssize_t *result, const FailFrame &ff) {
  *result = write(fd, buf, count);
  if (*result < 0) {
    return ff.failf(HERE, "write(%o,%o,%o) failed, errno=%o", fd, buf, count, errno);
  }
  return true;
}

bool tryWriteAll(int fd, const char *data, size_t size, const FailFrame &ff) {
  while (size > 0) {
    ssize_t result;
    if (!tryWrite(fd, data, size, &result, ff.nest(HERE))) {
      return false;
    }
    data += result;
    size -= result;
  }
  return true;
}

bool tryWriteAll(const std::string &filename, std::string_view text, const FailFrame &ff) {
  FileCloser fc;
  return tryOpen(filename, O_CREAT | O_TRUNC | O_WRONLY, 0664, &fc, ff.nest(HERE)) &&
      tryWriteAll(fc.get(), text.data(), text.size(), ff.nest(HERE));
}

bool tryWritevAll(int fd, struct iovec *iovecs, size_t numIovecs, const FailFrame &ff) {
  size_t remaining = 0;
  for (size_t i = 0; i < numIovecs; ++i) {
    remaining += iovecs[i].iov_len;
  }

  while (remaining != 0) {
    ssize_t result = writev(fd, iovecs, numIovecs);
    if (result < 0) {
      return ff.failf(HERE, "writev(%o,%o,%o) failed, errno=%o", fd, iovecs, numIovecs, errno);
    }
    size_t uresult = result;
    if (uresult > remaining) {
      return ff.fail(HERE, "Impossible: I wrote more than was requested");
    }
    remaining -= result;

    for (size_t i = 0; i < numIovecs; ++i) {
      if (result == 0) {
        break;
      }
      auto &iov = iovecs[i];
      auto amountToAdvance = std::min(uresult, iov.iov_len);
      iov.iov_base = static_cast<void*>(static_cast<char*>(iov.iov_base) + amountToAdvance);
      iov.iov_len -= amountToAdvance;
      result -= amountToAdvance;
    }

    if (result != 0) {
      return ff.failf(HERE, "Impossible: I had an amount left that I couldn't distribute");
    }
  }
  return true;
}

bool tryClose(int fd, const FailFrame &ff) {
  if (close(fd) < 0) {
    return ff.failf(HERE, "close(%o) failed, errno=%o", fd, errno);
  }
  return true;
}

bool tryTruncate(const std::string &filename, size_t size, const FailFrame &ff) {
  if (truncate(filename.data(), (off_t)size) < 0) {
    return ff.failf(HERE, R"(truncate("%o","%o") failed, errno=%o)", filename, size, errno);
  }
  return true;
}


bool tryFstat(int fd, struct stat *stat, const FailFrame &ff) {
  if (fstat(fd, stat) < 0) {
    return ff.failf(HERE, "fstat failed, errno=%o", errno);
  }
  return true;
}

bool tryLseek(int fd, off_t offset, int whence, off_t *result, const FailFrame &ff) {
  *result = lseek(fd, offset, whence);
  if (*result == (off_t)-1) {
    return ff.failf(HERE, "lseek(%o,%o,%o) failed, errno=%o", fd, offset, whence, errno);
  }
  return true;
}

bool tryExists(const std::string &filename, bool *exists, const FailFrame &ff) {
  struct stat statBuf {};
  auto result = stat(filename.data(), &statBuf);
  if (result == 0) {
    *exists = true;
    return true;
  }
  *exists = false;
  if (errno == ENOENT) {
    return true;
  }
  return ff.failf(HERE, "stat(\"%o\") failed, errno=%o", filename, errno);
}

bool trySync(int fd, const FailFrame &ff) {
  if (fsync(fd) < 0) {
    return ff.failf(HERE, "fsync(%o) failed, errno=%o", fd, errno);
  }
  return true;
}

bool tryFork(pid_t *result, const FailFrame &ff) {
  *result = fork();
  if (*result < 0) {
    return ff.failf(HERE, "fork() failed, errno=%o", errno);
  }
  return true;
}

bool tryDup2(int oldFd, int newFd, const FailFrame &ff) {
  auto rc = dup2(oldFd, newFd);
  if (rc < 0) {
    return ff.failf(HERE, "dup2(%o,%o) failed, errno=%o", oldFd, newFd, errno);
  }
  return true;
}

bool tryPipe2(int flags, FileCloser *fd0, FileCloser *fd1, const FailFrame &ff) {
  int pipeFd[2];
  auto result = pipe2(pipeFd, flags);
  if (result < 0) {
    return ff.failf(HERE, "pipe2(%o) failed, errno=%o", flags, errno);
  }
  *fd0 = FileCloser(pipeFd[0]);
  *fd1 = FileCloser(pipeFd[1]);
  return true;
}

bool trySocketpair(int domain, int type, int protocol, FileCloser *fd0, FileCloser *fd1,
    const FailFrame &ff) {
  int fd[2];
  auto result = socketpair(domain, type, protocol, fd);
  if (result < 0) {
    return ff.failf(HERE, "socketpair(%o,%o,%o) failed, errno=%o", domain, type, protocol, errno);
  }
  *fd0 = FileCloser(fd[0]);
  *fd1 = FileCloser(fd[1]);
  return true;
}

bool tryPopen(const char *command, const char *type, PipeCloser *pipe, const FailFrame &ff) {
  FILE *result = popen(command, type);
  if (result == nullptr) {
    return ff.failf(HERE, R"(popen("%o","%o") failed, errno=%o)", command, type);
  }
  *pipe = PipeCloser(result);
  return true;
}

bool tryFread(void *ptr, size_t size, size_t nmemb, FILE *f, size_t *bytesRead, const FailFrame &ff) {
  *bytesRead = fread(ptr, size, nmemb, f);
  if (*bytesRead == 0 && ferror(f)) {
    return ff.failf(HERE, "tryFread failed, errno=%o", errno);
  }
  return true;
}

bool tryLink(const std::string &oldPath, const std::string &newPath, const FailFrame &ff) {
  if (link(oldPath.data(), newPath.data()) < 0) {
    return ff.failf(HERE, "link(%o,%o) failed, errno=%o", oldPath, newPath, errno);
  }
  return true;
}

bool tryUnlink(const std::string &path, const FailFrame &ff) {
  if (unlink(path.data()) < 0) {
    return ff.failf(HERE, R"xxx(unlink("%o") failed, error=%o (%o))xxx", path, errno, strerror(errno));
  }
  return true;
}

bool tryRename(const std::string &oldPath, const std::string &newPath, const FailFrame &ff) {
  if (rename(oldPath.data(), newPath.data()) < 0) {
    return ff.failf(HERE, "rename(%o,%o) failed, errno=%o", oldPath, newPath, errno);
  }
  return true;
}

// Adapted from https://stackoverflow.com/a/27808574
bool tryEnumerateFilesAndDirsRecursively(const std::string &root,
    const Delegate<bool, std::string_view, bool, const FailFrame &> &callback, const FailFrame &ff) {
  char *files[] = { const_cast<char*>(root.data()), nullptr };
  auto *fts = fts_open(files, FTS_NOCHDIR | FTS_PHYSICAL | FTS_XDEV, nullptr);
  if (fts == nullptr) {
    return ff.failf(HERE, "fts_open(%o) failed: %o (%o)", root, errno, strerror(errno));
  }

  while (true) {
    auto *next = fts_read(fts);
    if (next == nullptr) {
      break;
    }
    switch (next->fts_info) {
      case FTS_NS:
      case FTS_DNR:
      case FTS_ERR:
        return ff.failf(HERE, "fts_read() failed at %o: %o (%o)", next->fts_accpath, errno, strerror(errno));

      case FTS_DC:
      case FTS_DOT:
      case FTS_NSOK:
        notreached;
        break;

      case FTS_D:
        // Preorder directory: do nothing.
        break;

      case FTS_DP:
        // Postorder directory: call back with "isDir" flag set to true.
        if (!callback(next->fts_path, true, ff.nest(HERE))) {
          return false;
        }
        break;

      case FTS_F:
      case FTS_SL:
      case FTS_SLNONE:
      case FTS_DEFAULT:
        // Some normal-looking thing: call back with "isDir" flag set to false.
        if (!callback(next->fts_path, false, ff.nest(HERE))) {
          return false;
        }
        break;
    }
  }
  return true;
}

bool tryGetHostname(std::string *result, const FailFrame &ff) {
  char hostname[HOST_NAME_MAX + 1];
  if (gethostname(hostname, STATIC_ARRAYSIZE(hostname)) < 0) {
    return ff.failf(HERE, "gethostname() failed, errno=%o", errno);
  }
  *result = hostname;
  return true;
}

bool tryGetTotalMemory(size_t *result, const FailFrame &ff) {
  struct sysinfo info = {};
  if (sysinfo(&info) < 0) {
    return ff.failf(HERE, "sysinfo() failed, errno=%o", errno);
  }
  *result = info.mem_unit * info.totalram;
  return true;
}

bool trySetThreadName(std::thread *thread, const std::string &name, const FailFrame &ff) {
  pthread_t pthread = thread->native_handle();
  auto error = pthread_setname_np(pthread, name.data());
  if (error != 0) {
    return ff.failf(HERE, R"(pthread_setname_np("%o") failed, errno=%o)", name, error);
  }
  return true;
}

bool tryWait(int *statusp, const FailFrame &ff) {
  auto pid = wait(statusp);
  if (pid < 0) {
    return ff.failf(HERE, "wait() failed, errno=%o", errno);
  }
  return true;
}

bool tryWaitPid(pid_t pid, int *statusp, int options, pid_t *pidResult, const FailFrame &ff) {
  *pidResult = waitpid(pid, statusp, options);
  if (*pidResult < 0) {
    return ff.failf(HERE, "wait() failed, errno=%o", errno);
  }
  return true;
}

bool tryExecve(const std::string &pathname, const std::vector<std::string> &args,
    const std::vector<std::string> &envs, const FailFrame &ff) {
  // All this copying because I have const char *, not char *.
  std::vector<std::string> argsCopy(args);
  std::vector<std::string> envsCopy(envs);

  std::vector<char *> argsToUse;
  std::vector<char *> envsToUse;
  argsToUse.reserve(args.size() + 1);
  envsToUse.reserve(envs.size() + 1);
  for (auto &arg : argsCopy) {
    argsToUse.push_back(arg.data());
  }
  for (auto &env : envsCopy) {
    envsToUse.push_back(env.data());
  }
  argsToUse.push_back(nullptr);
  envsToUse.push_back(nullptr);
  (void)execve(pathname.data(), argsToUse.data(), envsToUse.data());
  // Only returns if there is an error
  (void)ff.failf(HERE, R"(execve("%o", ...) failed, errno=%o)", pathname, errno);
  return false;
}

namespace {
PrettyError::PrettyError(int errnum) {
  char error[256];
  const char *errorText = strerror_r(errnum, error, STATIC_ARRAYSIZE(error));

  static_assert(STATIC_ARRAYSIZE(data_) != 0);
  BufferStream bs(data_, STATIC_ARRAYSIZE(data_) - 1);
  streamf(bs, "%o (%o)", errnum, errorText);
  *bs.current() = 0;
}
}  // namespace
}  // namespace kosak::coding::nsunix
