#!/bin/bash

if [ $# -lt 2 ]; then
  echo "invalid argument, should receive filesdir and searchstr"
  exit 1
fi

if ! test -d "$1"; then
  echo "$1 does not exist."
  exit 1
fi

X=$(grep -rl $2 $1/* | wc -l)
Y=$(grep -r $2 $1/* | wc -l)

echo "The number of files are $X and the number of matching lines are $Y"
