#!/bin/bash

# Assign arguments to variables for readability
IMAGE_FILE="./../../NUP/FileSystems/ext2_large_image.img"
START_INODE=10
END_INODE=20000
NUM_THREADS=10

# Function to extract and print individual blocks for an inode
extract_blocks() {
  echo "stat <$2>" | debugfs $1 2>/dev/null | grep -A1 "BLOCKS:" | tail -n1 | sed 's/([A-Z]*IND):[^,]*,//g; s/([0-9-]*):\?//g' | tr ',' '\n' | while read interval; do
      if [[ $interval == *"-"* ]]; then
        # Split the range and print each value in the range
        start=$(echo $interval | cut -d'-' -f1)
        end=$(echo $interval | cut -d'-' -f2)
        seq $start $end
      else
        # Print single block numbers directly
        echo $interval
      fi
    done
}

# Function to check and compare blocks
check_blocks() {
  local inode=$1
  local file=$2
  local inode_blocks_nc=$(extract_blocks "$file" "$inode")
  local a_out_blocks_nc=$(./14-ext2-blkiter/a.out "$file" "$inode" | tr -d '\n')

  # Remove newlines for direct comparison
  local inode_blocks=$(echo "$inode_blocks_nc" | tr -d '\n')
  local a_out_blocks=$(echo "$a_out_blocks_nc" | tr -d '\n')

  # Check if inode_blocks is empty
  if [[ -z "$inode_blocks" ]]; then
    # Check if a_out_blocks contains only zeros
    if [[ "$a_out_blocks" =~ ^0+$ ]]; then
      echo "Inode $inode: OK"
    else
      echo "Difference detected: $inode (Expected only zeros, got other values)"
    fi
  elif [[ "$inode_blocks" != "$a_out_blocks" ]]; then
    echo "Difference detected for inode $inode:"

    # Write blocks to temporary files
    echo "$inode_blocks_nc" | sort > /tmp/inode_blocks_$inode.txt
    echo "$a_out_blocks_nc" | sort > /tmp/a_out_blocks_$inode.txt

    # Use diff to display differences
    diff /tmp/inode_blocks_$inode.txt /tmp/a_out_blocks_$inode.txt

    # Clean up temporary files
    rm /tmp/inode_blocks_$inode.txt /tmp/a_out_blocks_$inode.txt
  else
    echo "Inode $inode: OK"
  fi
}

export -f extract_blocks
export -f check_blocks

seq $START_INODE $END_INODE | parallel -j $NUM_THREADS bash -c "check_blocks {} \"$IMAGE_FILE\"" _ {}

#14023