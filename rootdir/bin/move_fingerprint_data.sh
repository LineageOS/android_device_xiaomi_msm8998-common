#!/system/bin/sh
#
# Copyright (C) 2019 The LineageOS Project
#
# SPDX-License-Identifier: Apache-2.0
#

FPS_NAMES="fpc goodix"

for FPS in $FPS_NAMES; do
  DEST_PATH="/data/vendor/$FPS"
  FILES_MOVED="/data/vendor/$FPS/moved"
  SRC_PATH="/data/misc/$FPS"
  if [ ! -f "$FILES_MOVED" ]; then
    for i in "$SRC_PATH/"*; do
      dest_path=$DEST_PATH/"${i#$SRC_PATH/}"
      mv $i "$dest_path"
    done
    restorecon -R "$DEST_PATH"
    echo 1 > "$FILES_MOVED"
  fi
done
