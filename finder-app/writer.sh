#!/bin/bash

if [ $# -lt 2 ]; then
  echo "invalid argument, should receive filesdir and searchstr"
  exit 1
fi

if test -d "$1"; then
  echo "$1 is a directory"
  exit 1
fi

mkdir -p "$(dirname $1)"
echo "$2" > "$1"