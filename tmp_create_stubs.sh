#!/bin/bash
cd ~/fmtowns-master/3rdparty/asio/include/asio/detail
for f in epoll_reactor kqueue_reactor dev_poll_reactor io_uring_service; do
  fn="${f}.hpp"
  if [ ! -f "$fn" ]; then
    name=$(echo $f | tr 'a-z' 'A-Z')
    cat > "$fn" << EOF2
#ifndef ASIO_DETAIL_${name}_HPP
#define ASIO_DETAIL_${name}_HPP

#include "asio/detail/config.hpp"

#if defined(ASIO_HAS_PTHREADS)

#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

class ${f}
{
public:
  enum { implementation = 0 };
};

} // namespace detail
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif

#endif
EOF2
    echo "Created $fn"
  else
    echo "Exists: $fn"
  fi
done
