//
// detail/keyword_tss_ptr.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2024 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_KEYWORD_TSS_PTR_HPP
#define ASIO_DETAIL_KEYWORD_TSS_PTR_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include "asio/detail/config.hpp"

#if defined(ASIO_HAS_THREAD_KEYWORD_EXTENSION)

#include "asio/detail/noncopyable.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

template <typename T>
class keyword_tss_ptr
  : private noncopyable
{
public:
  keyword_tss_ptr()
  {
  }

  ~keyword_tss_ptr()
  {
  }

  operator T*() const
  {
    return value_;
  }

  void operator=(T* val)
  {
    value_ = val;
  }

private:
  static __thread T* value_;
};

template <typename T>
__thread T* keyword_tss_ptr<T>::value_ = 0;

} // namespace detail
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif

#endif
