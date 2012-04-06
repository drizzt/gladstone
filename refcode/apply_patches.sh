#!/bin/sh

if [ -f patches.stamp ]; then
  exit 0
fi

for file in patches/*
do
  patch -p0 < $file
  echo "$file" > patches.stamp
done
