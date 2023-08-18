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

#pragma once

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <unistd.h>
#include <functional>
#include <string>
#include <string_view>
#include <thread>
#include "kosak/coding/delegate.h"
#include "kosak/coding/failures.h"

namespace kosak::coding {

namespace nsunix {  // ugh, "unix" is a macro

class FileCloser {
public:
  FileCloser() = default;
  explicit FileCloser(int fd) : fd_(fd) {}
  DISALLOW_COPY_AND_ASSIGN(FileCloser);
  FileCloser(FileCloser &&other) noexcept : fd_(other.fd_) { other.fd_ = -1; }
  FileCloser &operator=(FileCloser &&other) noexcept;
  ~FileCloser();

  bool tryClose(const kosak::coding::FailFrame &ff);

  int release() {
    auto result = fd_;
    fd_ = -1;
    return result;
  }

  [[nodiscard]]
  int get() const { return fd_; }

  bool closed() const {
    return fd_ < 0;
  }

private:
  int fd_ = -1;
};

class PipeCloser {
public:
  PipeCloser() = default;
  explicit PipeCloser(FILE *pipe) : pipe_(pipe) {}
  DISALLOW_COPY_AND_ASSIGN(PipeCloser);
  DEFINE_MOVE_COPY_AND_ASSIGN(PipeCloser);
  ~PipeCloser();

  bool tryClose(const kosak::coding::FailFrame &ff);

  [[nodiscard]]
  FILE *get() const { return pipe_.get(); }
private:
  UnownedPtr<FILE> pipe_;
};

bool tryMakeFile(const std::string &filename, mode_t mode, std::string_view contents,
    const kosak::coding::FailFrame &ff);
bool tryMakeFileOfSize(const std::string &filename, mode_t mode, size_t size,
    const kosak::coding::FailFrame &ff);

bool tryMakeDirectory(const std::string &dirname, mode_t mode, const kosak::coding::FailFrame &ff);
bool tryEnsureBaseExists(const std::string &pathname, mode_t mode, const kosak::coding::FailFrame &ff);

bool tryOpen(const std::string &filename, int flags, mode_t mode, FileCloser *fc, const kosak::coding::FailFrame &ff);
bool tryRead(int fd, void *buffer, size_t bufferSize, size_t *bytesRead, const kosak::coding::FailFrame &ff);
bool tryReadAll(int fd, void *buffer, size_t bufferSize, const kosak::coding::FailFrame &ff);
bool tryReadAll(const std::string &filename, std::string *result, const kosak::coding::FailFrame &ff);
bool tryWrite(int fd, const void *buf, size_t count, ssize_t *result, const kosak::coding::FailFrame &ff);
bool tryWriteAll(int fd, const char *data, size_t size, const kosak::coding::FailFrame &ff);
bool tryWriteAll(const std::string &filename, std::string_view text, const kosak::coding::FailFrame &ff);
bool tryWritevAll(int fd, struct iovec *iovecs, size_t numIovecs, const kosak::coding::FailFrame &ff);
bool tryClose(int fd, const kosak::coding::FailFrame &ff);
bool tryTruncate(const std::string &filename, size_t size, const kosak::coding::FailFrame &ff);
bool tryFstat(int fd, struct stat *stat, const kosak::coding::FailFrame &ff);
bool tryLseek(int fd, off_t offset, int whence, off_t *result, const kosak::coding::FailFrame &ff);
bool tryExists(const std::string &filename, bool *exists, const kosak::coding::FailFrame &ff);
bool trySync(int fd, const kosak::coding::FailFrame &ff);
bool tryFork(pid_t *result, const kosak::coding::FailFrame &ff);

bool tryDup2(int oldFd, int newFd, const kosak::coding::FailFrame &ff);
bool tryPipe2(int flags, FileCloser *fd0, FileCloser *fd1, const kosak::coding::FailFrame &ff);
bool trySocketpair(int domain, int type, int protocol, FileCloser *fd0, FileCloser *fd1,
    const kosak::coding::FailFrame &ff);

bool tryPopen(const char *command, const char *type, PipeCloser *pipe,
    const kosak::coding::FailFrame &ff);
bool tryFread(void *ptr, size_t size, size_t nmemb, FILE *f, size_t *bytesRead,
    const kosak::coding::FailFrame &ff);

bool tryGetNextLine(std::string_view *src, std::string_view *result,
    const kosak::coding::FailFrame &ff);

bool tryLink(const std::string &oldPath, const std::string &newPath, const kosak::coding::FailFrame &ff);
bool tryUnlink(const std::string &path, const kosak::coding::FailFrame &ff);
bool tryRename(const std::string &src, const std::string &dest, const kosak::coding::FailFrame &ff);

// callback signature:
// bool callback(std::string_view fullPath, bool isDir, const FailFrame &ff)
bool tryEnumerateFilesAndDirsRecursively(const std::string &root,
    const kosak::coding::Delegate<bool, std::string_view, bool, const kosak::coding::FailFrame &> &cb,
    const kosak::coding::FailFrame &ff);

bool tryGetHostname(std::string *result, const kosak::coding::FailFrame &ff);

inline size_t numCores() {
  return std::thread::hardware_concurrency();
}
bool tryGetTotalMemory(size_t *result, const kosak::coding::FailFrame &ff);
bool trySetThreadName(std::thread *thread, const std::string &name, const kosak::coding::FailFrame &ff);

bool tryWait(int *statusp, const kosak::coding::FailFrame &ff);
bool tryWaitPid(pid_t pid, int *statusp, int options, pid_t *pidResult,
    const kosak::coding::FailFrame &ff);
bool tryExecve(const std::string &pathname, const std::vector<std::string> &args,
    const std::vector<std::string> &envs, const kosak::coding::FailFrame &ff);
}  // namespace nsunix

}  // namespace kosak::coding
