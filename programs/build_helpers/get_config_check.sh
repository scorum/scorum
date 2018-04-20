#!/bin/bash
file1=libraries/protocol/get_config.cpp
file2=libraries/protocol/include/scorum/protocol/config.hpp
diff -u \
   <(cat $file1 | grep 'result[[]".*"' | cut -d '"' -f 2 | sort | uniq) \
   <(cat $file2 | grep '[#]define\s\+[A-Z0-9_]\+\s' | cut -d ' ' -f 2 | sort | uniq)
if [[ $? -ne 0 ]]; then
    echo error: Check for conformity $file1 and $file2
fi
exit $?


