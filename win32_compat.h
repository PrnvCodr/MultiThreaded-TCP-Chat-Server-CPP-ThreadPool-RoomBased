#ifndef WIN32_COMPAT_H
#define WIN32_COMPAT_H

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif

#include <ctime>
#include <exception>
#include <functional>
#include <process.h>
#include <vector>
#include <windows.h>
#include <winsock2.h>


// Define missing console constant if needed
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

#ifndef ERROR_IO_PENDING
#define ERROR_IO_PENDING 997L
#endif

namespace w32 {

class Mutex {
public:
  Mutex() { InitializeCriticalSection(&cs); }
  ~Mutex() { DeleteCriticalSection(&cs); }
  void lock() { EnterCriticalSection(&cs); }
  void unlock() { LeaveCriticalSection(&cs); }
  PCRITICAL_SECTION native_handle() { return &cs; }

  // Prevent copy/move
  Mutex(const Mutex &) = delete;
  Mutex &operator=(const Mutex &) = delete;

private:
  CRITICAL_SECTION cs;
};

class ConditionVariable; // Forward declaration

class LockGuard {
public:
  explicit LockGuard(Mutex &m) : mutex(m) { mutex.lock(); }
  ~LockGuard() { mutex.unlock(); }
  // Prevent copy/move
  LockGuard(const LockGuard &) = delete;
  LockGuard &operator=(const LockGuard &) = delete;

  friend class ConditionVariable;

private:
  Mutex &mutex;
};

class ConditionVariable {
public:
  ConditionVariable() { InitializeConditionVariable(&cv); }
  ~ConditionVariable() { /* No cleanup needed for CVs */ }

  void wait(LockGuard &lock, std::function<bool()> predicate) {
    while (!predicate()) {
      SleepConditionVariableCS(&cv, lock.mutex.native_handle(), INFINITE);
    }
  }

  void notify_one() { WakeConditionVariable(&cv); }
  void notify_all() { WakeAllConditionVariable(&cv); }

  // Prevent copy/move
  ConditionVariable(const ConditionVariable &) = delete;
  ConditionVariable &operator=(const ConditionVariable &) = delete;

private:
  CONDITION_VARIABLE cv;
};

// Simple thread wrapper
class Thread {
public:
  Thread() : handle(NULL), thread_id(0) {}

  template <typename Function, typename... Args>
  explicit Thread(Function &&f, Args &&...args) {
    // Simple lambda-based thread start
    auto task = new std::function<void()>(
        std::bind(std::forward<Function>(f), std::forward<Args>(args)...));

    handle =
        (HANDLE)_beginthreadex(NULL, 0, StaticThreadStart, task, 0, &thread_id);
    if (handle == 0 || handle == (HANDLE)-1L) {
      delete task;
      handle = NULL;
    }
  }

  ~Thread() {
    if (joinable()) {
      // Should invoke join explicitly or it will leak handle/thread resource
      // conceptually if not waited
    }
    if (handle)
      CloseHandle(handle);
  }

  bool joinable() const { return handle != NULL; }

  void join() {
    if (joinable()) {
      WaitForSingleObject(handle, INFINITE);
      CloseHandle(handle);
      handle = NULL;
    }
  }

  void detach() {
    if (handle) {
      CloseHandle(handle);
      handle = NULL;
    }
  }

  // Prevent copy
  Thread(const Thread &) = delete;
  Thread &operator=(const Thread &) = delete;

  // Allow move
  Thread(Thread &&other) noexcept
      : handle(other.handle), thread_id(other.thread_id) {
    other.handle = NULL;
    other.thread_id = 0;
  }

  Thread &operator=(Thread &&other) noexcept {
    if (this != &other) {
      if (joinable())
        std::terminate(); // simplified
      handle = other.handle;
      thread_id = other.thread_id;
      other.handle = NULL;
      other.thread_id = 0;
    }
    return *this;
  }

private:
  HANDLE handle;
  unsigned thread_id;

  static unsigned __stdcall StaticThreadStart(void *arg) {
    auto *task = static_cast<std::function<void()> *>(arg);
    try {
      (*task)();
    } catch (...) {
    }
    delete task;
    return 0;
  }
};

// Time compatibility
inline void LocalTime(struct tm *result, const time_t *time) {
  // MSVCRT localtime uses TLS so it is thread safe to return pointer,
  // but we want a copy in result.
  struct tm *t = localtime(time);
  if (t && result)
    *result = *t;
}

} // namespace w32

#endif // WIN32_COMPAT_H
