#!/bin/bash
# Format C/C++ files
# Don't format vendor files
find . -type d -name "vendor" -prune -o -iname *.c -print -o -iname *.h -print | xargs clang-format -i

# Format Python files using black
black . --extend-exclude src/vendor
