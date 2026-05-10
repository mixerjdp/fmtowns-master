#!/bin/bash
cd ~/fmtowns-master/3rdparty/asio/include/asio/detail

create_file() {
  local f="$1"
  local content="$2"
  if [ ! -f "$f" ]; then
    echo "$content" > "$f"
    echo "Created $f"
  else
    echo "Exists: $f"
  fi
}

# Posix event
create_file "posix_event.hpp" '#ifndef ASIO_DETAIL_POSIX_EVENT_HPP
#define ASIO_DETAIL_POSIX_EVENT_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include "asio/detail/config.hpp"

#if defined(ASIO_HAS_PTHREADS)

#include <pthread.h>
#include "asio/detail/noncopyable.hpp"
#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

class posix_event : private noncopyable
{
public:
  posix_event() { pthread_cond_init(&cond_, 0); }
  ~posix_event() { pthread_cond_destroy(&cond_); }
  void signal() { pthread_cond_signal(&cond_); }
  void clear() {}
  void wait(pthread_mutex_t& m) { pthread_cond_wait(&cond_, &m); }
  bool wait_for_usec(pthread_mutex_t& m, long usec) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    ts.tv_sec += usec / 1000000;
    ts.tv_nsec += (usec % 1000000) * 1000;
    if (ts.tv_nsec >= 1000000000) { ts.tv_sec += 1; ts.tv_nsec -= 1000000000; }
    return pthread_cond_timedwait(&cond_, &m, &ts) == 0;
  }
private:
  pthread_cond_t cond_;
};

} // namespace detail
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif

#endif'

# Posix global
create_file "posix_global.hpp" '#ifndef ASIO_DETAIL_POSIX_GLOBAL_HPP
#define ASIO_DETAIL_POSIX_GLOBAL_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include "asio/detail/config.hpp"

#if defined(ASIO_HAS_PTHREADS)

#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

template <typename T>
struct posix_global_impl
{
  static T instance;
};
template <typename T>
T posix_global_impl<T>::instance = T();

template <typename T>
T& posix_global() { return posix_global_impl<T>::instance; }

} // namespace detail
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif

#endif'

# Posix mutex
create_file "posix_mutex.hpp" '#ifndef ASIO_DETAIL_POSIX_MUTEX_HPP
#define ASIO_DETAIL_POSIX_MUTEX_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include "asio/detail/config.hpp"

#if defined(ASIO_HAS_PTHREADS)

#include <pthread.h>
#include "asio/detail/noncopyable.hpp"
#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

class posix_mutex : private noncopyable
{
public:
  posix_mutex() { pthread_mutex_init(&mutex_, 0); }
  ~posix_mutex() { pthread_mutex_destroy(&mutex_); }
  void lock() { pthread_mutex_lock(&mutex_); }
  void unlock() { pthread_mutex_unlock(&mutex_); }
  pthread_mutex_t& native_handle() { return mutex_; }
private:
  pthread_mutex_t mutex_;
};

} // namespace detail
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif

#endif'

# Posix signal blocker
create_file "posix_signal_blocker.hpp" '#ifndef ASIO_DETAIL_POSIX_SIGNAL_BLOCKER_HPP
#define ASIO_DETAIL_POSIX_SIGNAL_BLOCKER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include "asio/detail/config.hpp"

#if defined(ASIO_HAS_PTHREADS)

#include <pthread.h>
#include <signal.h>
#include "asio/detail/noncopyable.hpp"
#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

class posix_signal_blocker : private noncopyable
{
public:
  posix_signal_blocker() {
    sigset_t new_mask;
    sigfillset(&new_mask);
    pthread_sigmask(SIG_BLOCK, &new_mask, &old_mask_);
  }
  ~posix_signal_blocker() { pthread_sigmask(SIG_SETMASK, &old_mask_, 0); }
private:
  sigset_t old_mask_;
};

} // namespace detail
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif

#endif'

# Posix static mutex
create_file "posix_static_mutex.hpp" '#ifndef ASIO_DETAIL_POSIX_STATIC_MUTEX_HPP
#define ASIO_DETAIL_POSIX_STATIC_MUTEX_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include "asio/detail/config.hpp"

#if defined(ASIO_HAS_PTHREADS)

#include <pthread.h>
#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

struct posix_static_mutex
{
  void init() { pthread_mutex_init(&mutex_, 0); }
  void lock() { pthread_mutex_lock(&mutex_); }
  void unlock() { pthread_mutex_unlock(&mutex_); }
  pthread_mutex_t mutex_;
};

#define ASIO_POSIX_STATIC_MUTEX_INIT { PTHREAD_MUTEX_INITIALIZER }

} // namespace detail
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif

#endif'

# Null event
create_file "null_event.hpp" '#ifndef ASIO_DETAIL_NULL_EVENT_HPP
#define ASIO_DETAIL_NULL_EVENT_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include "asio/detail/config.hpp"

#if !defined(ASIO_HAS_THREADS)

#include "asio/detail/noncopyable.hpp"
#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

class null_event : private noncopyable
{
public:
  null_event() {}
  ~null_event() {}
  void signal() {}
  void clear() {}
};

} // namespace detail
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif

#endif'

# Null fenced block
create_file "null_fenced_block.hpp" '#ifndef ASIO_DETAIL_NULL_FENCED_BLOCK_HPP
#define ASIO_DETAIL_NULL_FENCED_BLOCK_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include "asio/detail/config.hpp"
#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

class null_fenced_block
{
public:
  explicit null_fenced_block(bool) {}
  ~null_fenced_block() {}
};

} // namespace detail
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif'

# Null global
create_file "null_global.hpp" '#ifndef ASIO_DETAIL_NULL_GLOBAL_HPP
#define ASIO_DETAIL_NULL_GLOBAL_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include "asio/detail/config.hpp"
#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

template <typename T>
struct null_global_impl { static T instance; };
template <typename T>
T null_global_impl<T>::instance = T();

template <typename T>
T& null_global() { return null_global_impl<T>::instance; }

} // namespace detail
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif'

# Null mutex
create_file "null_mutex.hpp" '#ifndef ASIO_DETAIL_NULL_MUTEX_HPP
#define ASIO_DETAIL_NULL_MUTEX_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include "asio/detail/config.hpp"
#include "asio/detail/noncopyable.hpp"
#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

class null_mutex : private noncopyable
{
public:
  null_mutex() {}
  ~null_mutex() {}
  void lock() {}
  void unlock() {}
};

} // namespace detail
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif'

# Null reactor
create_file "null_reactor.hpp" '#ifndef ASIO_DETAIL_NULL_REACTOR_HPP
#define ASIO_DETAIL_NULL_REACTOR_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include "asio/detail/config.hpp"
#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

class null_reactor
{
public:
  enum { implementation = 0 };
};

} // namespace detail
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif'

# Null signal blocker
create_file "null_signal_blocker.hpp" '#ifndef ASIO_DETAIL_NULL_SIGNAL_BLOCKER_HPP
#define ASIO_DETAIL_NULL_SIGNAL_BLOCKER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include "asio/detail/config.hpp"
#include "asio/detail/noncopyable.hpp"
#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

class null_signal_blocker : private noncopyable
{
public:
  null_signal_blocker() {}
  ~null_signal_blocker() {}
};

} // namespace detail
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif'

# Null static mutex
create_file "null_static_mutex.hpp" '#ifndef ASIO_DETAIL_NULL_STATIC_MUTEX_HPP
#define ASIO_DETAIL_NULL_STATIC_MUTEX_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include "asio/detail/config.hpp"
#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

struct null_static_mutex
{
  void init() {}
  void lock() {}
  void unlock() {}
};

#define ASIO_NULL_STATIC_MUTEX_INIT {}

} // namespace detail
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif'

# Null thread
create_file "null_thread.hpp" '#ifndef ASIO_DETAIL_NULL_THREAD_HPP
#define ASIO_DETAIL_NULL_THREAD_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include "asio/detail/config.hpp"
#include "asio/detail/noncopyable.hpp"
#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

class null_thread : private noncopyable
{
public:
  template <typename F> null_thread(F, unsigned int = 0) {}
  ~null_thread() {}
  void join() {}
  void detach() {}
  static unsigned int hardware_concurrency() { return 1; }
};

} // namespace detail
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif'

# Std event
create_file "std_event.hpp" '#ifndef ASIO_DETAIL_STD_EVENT_HPP
#define ASIO_DETAIL_STD_EVENT_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include "asio/detail/config.hpp"

#include <condition_variable>
#include <mutex>
#include "asio/detail/noncopyable.hpp"
#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

class std_event : private noncopyable
{
public:
  std_event() {}
  ~std_event() {}
  void signal() { cv_.notify_one(); }
  void clear() {}
private:
  std::condition_variable cv_;
};

} // namespace detail
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif'

# Std fenced block
create_file "std_fenced_block.hpp" '#ifndef ASIO_DETAIL_STD_FENCED_BLOCK_HPP
#define ASIO_DETAIL_STD_FENCED_BLOCK_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include "asio/detail/config.hpp"
#include <atomic>
#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

class std_fenced_block
{
public:
  enum half_t { half };
  enum full_t { full };
  explicit std_fenced_block(half_t) {}
  explicit std_fenced_block(full_t) { std::atomic_thread_fence(std::memory_order_acquire); }
  ~std_fenced_block() { std::atomic_thread_fence(std::memory_order_release); }
};

} // namespace detail
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif'

# Std global
create_file "std_global.hpp" '#ifndef ASIO_DETAIL_STD_GLOBAL_HPP
#define ASIO_DETAIL_STD_GLOBAL_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include "asio/detail/config.hpp"
#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

template <typename T>
struct std_global_impl { static T instance; };
template <typename T>
T std_global_impl<T>::instance = T();

template <typename T>
T& std_global() { return std_global_impl<T>::instance; }

} // namespace detail
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif'

# Std mutex
create_file "std_mutex.hpp" '#ifndef ASIO_DETAIL_STD_MUTEX_HPP
#define ASIO_DETAIL_STD_MUTEX_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include "asio/detail/config.hpp"
#include <mutex>
#include "asio/detail/noncopyable.hpp"
#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

class std_mutex : private noncopyable
{
public:
  std_mutex() {}
  ~std_mutex() {}
  void lock() { m_.lock(); }
  void unlock() { m_.unlock(); }
private:
  std::mutex m_;
};

} // namespace detail
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif'

# Std static mutex
create_file "std_static_mutex.hpp" '#ifndef ASIO_DETAIL_STD_STATIC_MUTEX_HPP
#define ASIO_DETAIL_STD_STATIC_MUTEX_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include "asio/detail/config.hpp"
#include <mutex>
#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

struct std_static_mutex
{
  void init() {}
  void lock() { m_.lock(); }
  void unlock() { m_.unlock(); }
  std::mutex m_;
};

#define ASIO_STD_STATIC_MUTEX_INIT {}

} // namespace detail
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif'

# Std thread
create_file "std_thread.hpp" '#ifndef ASIO_DETAIL_STD_THREAD_HPP
#define ASIO_DETAIL_STD_THREAD_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include "asio/detail/config.hpp"
#include <thread>
#include "asio/detail/noncopyable.hpp"
#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

class std_thread : private noncopyable
{
public:
  template <typename F> std_thread(F f, unsigned int = 0) : t_(f) {}
  ~std_thread() {}
  void join() { if (t_.joinable()) t_.join(); }
  void detach() { if (t_.joinable()) t_.detach(); }
  static unsigned int hardware_concurrency() { return std::thread::hardware_concurrency(); }
private:
  std::thread t_;
};

} // namespace detail
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif'

# Old win SDK compat
create_file "old_win_sdk_compat.hpp" '#ifndef ASIO_DETAIL_OLD_WIN_SDK_COMPAT_HPP
#define ASIO_DETAIL_OLD_WIN_SDK_COMPAT_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#endif'

# Pipe select interrupter
create_file "pipe_select_interrupter.hpp" '#ifndef ASIO_DETAIL_PIPE_SELECT_INTERRUPTER_HPP
#define ASIO_DETAIL_PIPE_SELECT_INTERRUPTER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include "asio/detail/config.hpp"

#if defined(ASIO_HAS_PTHREADS)

#include <unistd.h>
#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

class pipe_select_interrupter
{
public:
  pipe_select_interrupter() { pipe(fds_); }
  ~pipe_select_interrupter() { close(fds_[0]); close(fds_[1]); }
  void interrupt() { write(fds_[1], "", 1); }
  void reset() { char c; while (read(fds_[0], &c, 1) > 0) {} }
  int read_descriptor() const { return fds_[0]; }
private:
  int fds_[2];
};

} // namespace detail
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif

#endif'

# Socket select interrupter
create_file "socket_select_interrupter.hpp" '#ifndef ASIO_DETAIL_SOCKET_SELECT_INTERRUPTER_HPP
#define ASIO_DETAIL_SOCKET_SELECT_INTERRUPTER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include "asio/detail/config.hpp"

#if (defined(ASIO_HAS_PTHREADS) && !defined(ASIO_DISABLE_PIPE_SELECT_INTERRUPTER)) || !defined(ASIO_HAS_PTHREADS)

#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

class socket_select_interrupter
{
public:
  socket_select_interrupter() {}
  ~socket_select_interrupter() {}
};

} // namespace detail
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif

#endif'

# Eventfd select interrupter
create_file "eventfd_select_interrupter.hpp" '#ifndef ASIO_DETAIL_EVENTFD_SELECT_INTERRUPTER_HPP
#define ASIO_DETAIL_EVENTFD_SELECT_INTERRUPTER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include "asio/detail/config.hpp"

#if defined(ASIO_HAS_EVENTFD)

#include <sys/eventfd.h>
#include <unistd.h>
#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

class eventfd_select_interrupter
{
public:
  eventfd_select_interrupter() { fd_ = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC); }
  ~eventfd_select_interrupter() { close(fd_); }
  void interrupt() { uint64_t v = 1; write(fd_, &v, sizeof(v)); }
  void reset() { uint64_t v; read(fd_, &v, sizeof(v)); }
  int read_descriptor() const { return fd_; }
private:
  int fd_;
};

} // namespace detail
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif

#endif'

# Socket types
create_file "socket_types.hpp" '#ifndef ASIO_DETAIL_SOCKET_TYPES_HPP
#define ASIO_DETAIL_SOCKET_TYPES_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include "asio/detail/config.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>

#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

typedef int socket_type;
typedef struct sockaddr socket_addr_type;
typedef struct sockaddr_in socket_addr_ipv4_type;
typedef struct sockaddr_in6 socket_addr_ipv6_type;

const int socket_error_connection_refused = ECONNREFUSED;
const int socket_error_connection_aborted = ECONNABORTED;

} // namespace detail
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif'

echo "=== All done ==="