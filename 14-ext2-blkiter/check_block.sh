#!/bin/bash
inode=$1
file=$2

if (( inode % 100 == 0 )); then
  echo "Processing inode $inode"
fi

extract_blocks() {
  echo "stat <$2>" | debugfs $1 2>/dev/null | grep -A1 "BLOCKS:" | tail -n1 | sed 's/([A-Z]*IND):[^,]*,//g; s/([0-9-]*):\?//g' | tr ',' '\n' | while read interval; do
    if [[ $interval == *"-"* ]]; then
      start=$(echo $interval | cut -d'-' -f1)
      end=$(echo $interval | cut -d'-' -f2)
      seq $start $end
    else
      echo $interval
    fi
  done
}

inode_blocks_nc=$(extract_blocks "$file" "$inode")
a_out_blocks_nc=$(./14-ext2-blkiter/a.out "$file" "$inode" | tr -d '\n')
inode_blocks=$(echo "$inode_blocks_nc" | tr -d '\n')
a_out_blocks=$(echo "$a_out_blocks_nc" | tr -d '\n')

if [[ -z "$inode_blocks" ]]; then
  if [[ "$a_out_blocks" =~ ^0+$ ]]; then
#    echo "Inode $inode: OK"
    exit 0
  else
    echo "Difference detected: $inode (Expected only zeros, got other values)"
  fi
elif [[ "$inode_blocks" != "$a_out_blocks" ]]; then
  echo "Difference detected for inode $inode:"
  echo "$inode_blocks_nc" | sort > /tmp/inode_blocks_$inode.txt
  echo "$a_out_blocks_nc" | sort > /tmp/a_out_blocks_$inode.txt
  diff /tmp/inode_blocks_$inode.txt /tmp/a_out_blocks_$inode.txt
  rm /tmp/inode_blocks_$inode.txt /tmp/a_out_blocks_$inode.txt
else
#  echo "Inode $inode: OK"
  exit 0
fi
