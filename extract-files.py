#!/usr/bin/env -S PYTHONPATH=../../../tools/extract-utils python3
#
# SPDX-FileCopyrightText: 2024 The LineageOS Project
# SPDX-License-Identifier: Apache-2.0
#

import extract_utils.tools

extract_utils.tools.DEFAULT_PATCHELF_VERSION = '0_17_2'

from extract_utils.fixups_blob import (
    blob_fixup,
    blob_fixups_user_type,
)
from extract_utils.main import (
    ExtractUtils,
    ExtractUtilsModule,
)

blob_fixups: blob_fixups_user_type = {
    'system_ext/lib64/lib-imscamera.so': blob_fixup()
        .add_needed('libgui_shim.so'),
    'system_ext/lib64/lib-imsvideocodec.so': blob_fixup()
        .add_needed('libgui_shim.so')
        .replace_needed('libqdMetaData.so', 'libqdMetaData.system.so'),
    'vendor/bin/hw/android.hardware.drm@1.1-service.widevine': blob_fixup()
        .replace_needed('libhidltransport.so', 'libhidlbase.so')
        .remove_needed('libhwbinder.so'),
    'vendor/lib/hw/camera.msm8998.so': blob_fixup()
        .binary_regex_replace(b'service.bootanim.exit', b'service.bootanim.zzzz')
        .remove_needed('android.hidl.base@1.0.so')
        .remove_needed('libminikin.so'),
    (
     'vendor/lib/lib_lowlight.so',
     'vendor/lib/lib_lowlight_dxo.so',
     'vendor/lib/libSonyIMX386PdafLibrary.so',
     'vendor/lib/libXMFD_AgeGender.so',
     'vendor/lib/libarcsoft_beautyshot.so',
     'vendor/lib/libarcsoft_beautyshot_image_algorithm.so',
     'vendor/lib/libarcsoft_beautyshot_video_algorithm.so',
     'vendor/lib/libarcsoft_dualcam_optical_zoom.so',
     'vendor/lib/libarcsoft_dualcam_optical_zoom_control.so',
     'vendor/lib/libarcsoft_dualcam_refocus.so',
     'vendor/lib/libmmcamera_hdr_gb_lib.so',
     'vendor/lib/libmorpho_easy_hdr.so',
     'vendor/lib/libmorpho_hdr_checker.so',
    ): blob_fixup()
        .replace_needed('libstdc++.so', 'libstdc++_vendor.so'),
    'vendor/lib/libFaceGrade.so': blob_fixup()
        .remove_needed('libandroid.so')
        .replace_needed('libstdc++.so', 'libstdc++_vendor.so'),
    'vendor/lib/libMiCameraHal.so': blob_fixup()
        .add_needed('libcamera_shim.so')
        .add_needed('libpiex_shim.so')
        .remove_needed('libft2.so')
        .remove_needed('libharfbuzz_ng.so')
        .remove_needed('libheif.so')
        .remove_needed('libicuuc.so')
        .remove_needed('libminikin.so'),
    'vendor/lib/libarcsoft_beauty_shot.so': blob_fixup()
        .remove_needed('libandroid.so')
        .replace_needed('libstdc++.so', 'libstdc++_vendor.so'),
    'vendor/lib/libmmcamera2_sensor_modules.so': blob_fixup()
        .binary_regex_replace(b'/data/misc/camera/camera_lsc_caldata.txt', b'/data/vendor/camera/camera_lsc_calib.txt'),
    'vendor/lib/libmmcamera2_stats_modules.so': blob_fixup()
        .remove_needed('libandroid.so')
        .remove_needed('libgui.so'),
    'vendor/lib/libmpbase.so': blob_fixup()
        .remove_needed('libandroid.so')
        .replace_needed('libstdc++.so', 'libstdc++_vendor.so'),
    'vendor/lib64/libril-qc-hal-qmi.so': blob_fixup()
        .replace_needed('android.hardware.radio.config@1.0.so', 'android.hardware.radio.c_shim@1.0.so')
        .replace_needed('android.hardware.radio.config@1.1.so', 'android.hardware.radio.c_shim@1.1.so')
        .replace_needed('android.hardware.radio.config@1.2.so', 'android.hardware.radio.c_shim@1.2.so'),
    'vendor/lib64/libwvhidl.so': blob_fixup()
        .add_needed('libcrypto_shim.so'),
}  # fmt: skip

module = ExtractUtilsModule(
    'msm8998-common',
    'xiaomi',
    blob_fixups=blob_fixups,
    check_elf=False,
)

if __name__ == '__main__':
    utils = ExtractUtils.device(module)
    utils.run()
