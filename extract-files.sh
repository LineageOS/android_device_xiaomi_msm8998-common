#!/bin/bash
#
# Copyright (C) 2016 The CyanogenMod Project
# Copyright (C) 2017-2020 The LineageOS Project
#
# SPDX-License-Identifier: Apache-2.0
#

set -e

# Load extract_utils and do some sanity checks
MY_DIR="${BASH_SOURCE%/*}"
if [[ ! -d "${MY_DIR}" ]]; then MY_DIR="${PWD}"; fi

ANDROID_ROOT="${MY_DIR}/../../.."

HELPER="${ANDROID_ROOT}/tools/extract-utils/extract_utils.sh"
if [ ! -f "${HELPER}" ]; then
    echo "Unable to find helper script at ${HELPER}"
    exit 1
fi
source "${HELPER}"

# Default to sanitizing the vendor folder before extraction
CLEAN_VENDOR=true

ONLY_COMMON=
ONLY_TARGET=
KANG=
SECTION=

while [ "${#}" -gt 0 ]; do
    case "${1}" in
        --only-common )
                ONLY_COMMON=true
                ;;
        --only-target )
                ONLY_TARGET=true
                ;;
        -n | --no-cleanup )
                CLEAN_VENDOR=false
                ;;
        -k | --kang )
                KANG="--kang"
                ;;
        -s | --section )
                SECTION="${2}"; shift
                CLEAN_VENDOR=false
                ;;
        * )
                SRC="${1}"
                ;;
    esac
    shift
done

if [ -z "${SRC}" ]; then
    SRC="adb"
fi

function blob_fixup() {
    case "${1}" in
        system_ext/etc/init/dpmd.rc)
            sed -i "s|/system/product/bin/|/system/system_ext/bin/|g" "${2}"
            ;;
        system_ext/etc/permissions/com.qti.dpmframework.xml | system_ext/etc/permissions/dpmapi.xml)
            sed -i "s|/system/product/framework/|/system/system_ext/framework/|g" "${2}"
            ;;
        system_ext/etc/permissions/qcrilhook.xml)
            sed -i 's|/product/framework/qcrilhook.jar|/system/system_ext/framework/qcrilhook.jar|g' "${2}"
            ;;
        system_ext/etc/permissions/qti_libpermissions.xml)
            sed -i 's|name=\"android.hidl.manager-V1.0-java|name=\"android.hidl.manager@1.0-java|g' "${2}"
            ;;
        system_ext/lib64/lib-imscamera.so)
            grep -q "libgui_shim.so" "${2}" || "${PATCHELF}" --add-needed "libgui_shim.so" "${2}"
            ;;
        system_ext/lib64/lib-imsvideocodec.so)
            grep -q "libgui_shim.so" "${2}" || "${PATCHELF}" --add-needed "libgui_shim.so" "${2}"
            grep -q "libui_shim.so" "${2}" || "${PATCHELF}" --add-needed "libui_shim.so" "${2}"
            ;;
        system_ext/lib64/libdpmframework.so)
            grep -q "libcutils_shim.so" "${2}" || "${PATCHELF}" --add-needed "libcutils_shim.so" "${2}"
            ;;
        system_ext/lib64/vendor.qti.imsrtpservice@1.0.so)
           "${PATCHELF}" --replace-needed "libhidlbase.so" "libhidlbase-v32.so" "${2}" 
           ;;
        vendor/bin/pm-service)
            grep -q libutils-v33.so "${2}" || "${PATCHELF}" --add-needed "libutils-v33.so" "${2}"
            ;;
        vendor/lib/hw/camera.msm8998.so)
            "${PATCHELF}" --remove-needed "android.hidl.base@1.0.so" "${2}"
            "${PATCHELF}" --remove-needed "libminikin.so" "${2}"
            sed -i "s/service.bootanim.exit/service.bootanim.zzzz/g" "${2}"
            ;;
        vendor/lib/lib_lowlight.so|vendor/lib/lib_lowlight_dxo.so|vendor/lib/libSonyIMX386PdafLibrary.so|vendor/lib/libXMFD_AgeGender.so|vendor/lib/libarcsoft_beautyshot.so|vendor/lib/libarcsoft_beautyshot_image_algorithm.so|vendor/lib/libarcsoft_beautyshot_video_algorithm.so|vendor/lib/libarcsoft_dualcam_optical_zoom.so|vendor/lib/libarcsoft_dualcam_optical_zoom_control.so|vendor/lib/libarcsoft_dualcam_refocus.so|vendor/lib/libmmcamera_hdr_gb_lib.so|vendor/lib/libmorpho_easy_hdr.so|vendor/lib/libmorpho_hdr_checker.so)
            "${PATCHELF_0_17_2}" --replace-needed "libstdc++.so" "libstdc++_vendor.so" "${2}"
            ;;
        vendor/lib/libFaceGrade.so)
            "${PATCHELF}" --remove-needed "libandroid.so" "${2}"
            "${PATCHELF_0_17_2}" --replace-needed "libstdc++.so" "libstdc++_vendor.so" "${2}"
            ;;
        vendor/lib/libMiCameraHal.so)
            "${PATCHELF}" --remove-needed "libft2.so" "${2}"
            "${PATCHELF}" --remove-needed "libharfbuzz_ng.so" "${2}"
            "${PATCHELF}" --remove-needed "libheif.so" "${2}"
            "${PATCHELF}" --remove-needed "libicuuc.so" "${2}"
            "${PATCHELF}" --remove-needed "libminikin.so" "${2}"
            "${PATCHELF}" --add-needed "libcamera_shim.so" "${2}"
            grep -q "libpiex_shim.so" "${2}" || "${PATCHELF}" --add-needed "libpiex_shim.so" "${2}"
            ;;
        vendor/lib/libarcsoft_beauty_shot.so)
            "${PATCHELF}" --remove-needed "libandroid.so" "${2}"
            "${PATCHELF_0_17_2}" --replace-needed "libstdc++.so" "libstdc++_vendor.so" "${2}"
            ;;
        vendor/lib/libmmcamera2_sensor_modules.so)
            sed -i 's|/data/misc/camera/camera_lsc_caldata.txt|/data/vendor/camera/camera_lsc_calib.txt|g' "${2}"
            ;;
        vendor/lib/libmmcamera2_stats_modules.so)
            "${PATCHELF}" --remove-needed "libandroid.so" "${2}"
            "${PATCHELF}" --remove-needed "libgui.so" "${2}"
            ;;
        vendor/lib/libmpbase.so)
            "${PATCHELF}" --remove-needed "libandroid.so" "${2}"
            "${PATCHELF_0_17_2}" --replace-needed "libstdc++.so" "libstdc++_vendor.so" "${2}"
            ;;
        vendor/lib64/com.qualcomm.qti.imscmservice@2.0.so|vendor/lib64/com.qualcomm.qti.imscmservice@2.1.so|vendor/lib64/com.qualcomm.qti.uceservice@2.0.so|vendor/lib64/vendor.qti.hardware.radio.am@1.0.so|vendor/lib64/vendor.qti.hardware.radio.atcmdfwd@1.0.so|vendor/lib64/vendor.qti.hardware.radio.ims@1.0.so|vendor/lib64/vendor.qti.hardware.radio.lpa@1.0.so|vendor/lib64/vendor.qti.hardware.radio.qcrilhook@1.0.so|vendor/lib64/vendor.qti.hardware.radio.qtiradio@1.0.so|vendor/lib64/vendor.qti.hardware.radio.uim@1.0.so|vendor/lib64/vendor.qti.hardware.radio.uim@1.1.so|vendor/lib64/vendor.qti.hardware.radio.uim_remote_client@1.0.so|vendor/lib64/vendor.qti.hardware.radio.uim_remote_server@1.0.so|vendor/lib64/vendor.qti.hardware.soter@1.0.so|vendor/lib64/vendor.qti.ims.callinfo@1.0.so|vendor/lib64/vendor.qti.ims.rcsconfig@1.0.so|vendor/lib64/vendor.qti.imsrtpservice@1.0.so)
            "${PATCHELF}" --replace-needed "libhidlbase.so" "libhidlbase-v32.so" "${2}"
            ;;
    esac
}

if [ -z "${ONLY_TARGET}" ]; then
    # Initialize the helper for common device
    setup_vendor "${DEVICE_COMMON}" "${VENDOR}" "${ANDROID_ROOT}" true "${CLEAN_VENDOR}"

    extract "${MY_DIR}/proprietary-files.txt" "${SRC}" "${KANG}" --section "${SECTION}"
fi

if [ -z "${ONLY_COMMON}" ] && [ -s "${MY_DIR}/../${DEVICE}/proprietary-files.txt" ]; then
    # Reinitialize the helper for device
    source "${MY_DIR}/../${DEVICE}/extract-files.sh"
    setup_vendor "${DEVICE}" "${VENDOR}" "${ANDROID_ROOT}" false "${CLEAN_VENDOR}"

    extract "${MY_DIR}/../${DEVICE}/proprietary-files.txt" "${SRC}" "${KANG}" --section "${SECTION}"
fi

"${MY_DIR}/setup-makefiles.sh"
