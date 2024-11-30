#!/bin/bash

tasks=(
  #  "04-ext2-read-file"
  #  "05-ext2-read-dir"
  #  "06-ext2-walk-path"
  #  "08-ext2-read-sparse-file"
  #  "14-ext2-blkiter"
  "10-ext2-fuse"
)

base_url="http://185.106.102.104:9091/results/OgurtsovAndrei"

for task in "${tasks[@]}"; do
    # shellcheck disable=SC2046
    echo $(curl -s -X POST "${base_url}/${task}")
done
