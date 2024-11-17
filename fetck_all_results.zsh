#!/bin/bash

# List of tasks
tasks=(
    "00-ps"
    "01-lsof"
    "02-fuse-helloworld"
    "03-io_uring"
    "04-ext2-read-file"
    "05-ext2-read-dir"
    "06-ext2-walk-path"
    "07-ntfs-read-file"
    "08-ext2-read-sparse-file"
    "09-btree"
    "10-ext2-fuse"
    "11-grpc"
    "12-grpc-prometheus"
    "13-realpath"
    "14-ext2-blkiter"
)

# Base URL for the API
base_url="http://185.106.102.104:9091/results/OgurtsovAndrei"

# ANSI escape codes for colors
GREEN='\033[0;32m'
RED='\033[0;31m'
RESET='\033[0m'

# Loop through each task and fetch results
for task in "${tasks[@]}"; do
    response=$(curl -s -X GET "${base_url}/${task}/last")
    result=$(echo "$response" | jq -r '.result // "unknown"')
    if [[ "$result" == "null" ]]; then
        echo -e "${task} ${RED}error: invalid response${RESET}"
    elif [[ "$result" == "ok" ]]; then
        echo -e "${task} ${GREEN}ok${RESET}"
    else
        echo -e "${task} ${RED}$result${RESET}"
    fi
done
