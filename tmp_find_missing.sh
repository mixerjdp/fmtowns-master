#!/bin/bash
cd ~/fmtowns-master/3rdparty/asio/include/asio/detail
for f in $(ls *.hpp); do
  grep -oP '#\s*include "asio/detail/[^"]+"' $f 2>/dev/null
done | sort -u | sed 's/#include "asio\/detail\///' | sed 's/"//' > /tmp/asio_includes.txt
echo "All referenced detail files:"
cat /tmp/asio_includes.txt
echo ""
echo "=== MISSING FILES ==="
while IFS= read -r file; do
  if [ ! -f "$file" ]; then
    echo "$file"
  fi
done < /tmp/asio_includes.txt
