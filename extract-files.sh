#!/bin/bash
#
# SPDX-FileCopyrightText: 2016 The CyanogenMod Project
# SPDX-FileCopyrightText: 2017-2024 The LineageOS Project
# SPDX-License-Identifier: Apache-2.0
#

set -e

# Load extract_utils and do some sanity checks
MY_DIR="${BASH_SOURCE%/*}"
if [[ ! -d "${MY_DIR}" ]]; then MY_DIR="${PWD}"; fi

ANDROID_ROOT="${MY_DIR}/../../.."

export TARGET_ENABLE_CHECKELF=false

HELPER="${ANDROID_ROOT}/tools/extract-utils/extract_utils.sh"
if [ ! -f "${HELPER}" ]; then
    echo "Unable to find helper script at ${HELPER}"
    exit 1
fi
source "${HELPER}"

# Default to sanitizing the vendor folder before extraction
CLEAN_VENDOR=true

ONLY_COMMON=
ONLY_FIRMWARE=
ONLY_TARGET=
KANG=
SECTION=

while [ "${#}" -gt 0 ]; do
    case "${1}" in
        --only-common)
            ONLY_COMMON=true
            ;;
        --only-firmware)
            ONLY_FIRMWARE=true
            ;;
        --only-target)
            ONLY_TARGET=true
            ;;
        -n | --no-cleanup)
            CLEAN_VENDOR=false
            ;;
        -k | --kang)
            KANG="--kang"
            ;;
        -s | --section)
            SECTION="${2}"
            shift
            CLEAN_VENDOR=false
            ;;
        *)
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
        system_ext/lib64/lib-imsvideocodec.so)
            [ "$2" = "" ] && return 0
            grep -q "libgui_shim.so" "${2}" || "${PATCHELF}" --add-needed "libgui_shim.so" "${2}"
            "${PATCHELF}" --replace-needed "libqdMetaData.so" "libqdMetaData.system.so" "${2}"
            ;;
        vendor/lib/hw/camera.msm8998.so)
            [ "$2" = "" ] && return 0
            "${PATCHELF}" --remove-needed "android.hidl.base@1.0.so" "${2}"
            "${PATCHELF}" --remove-needed "libminikin.so" "${2}"
            sed -i "s/service.bootanim.exit/service.bootanim.zzzz/g" "${2}"
            ;;
        vendor/lib/lib_lowlight.so | \
            vendor/lib/lib_lowlight_dxo.so | \
            vendor/lib/libSonyIMX386PdafLibrary.so | \
            vendor/lib/libXMFD_AgeGender.so | \
            vendor/lib/libarcsoft_beautyshot.so | \
            vendor/lib/libarcsoft_beautyshot_image_algorithm.so | \
            vendor/lib/libarcsoft_beautyshot_video_algorithm.so | \
            vendor/lib/libarcsoft_dualcam_optical_zoom.so | \
            vendor/lib/libarcsoft_dualcam_optical_zoom_control.so | \
            vendor/lib/libarcsoft_dualcam_refocus.so | \
            vendor/lib/libmmcamera_hdr_gb_lib.so | \
            vendor/lib/libmorpho_easy_hdr.so | \
            vendor/lib/libmorpho_hdr_checker.so)
            [ "$2" = "" ] && return 0
            "${PATCHELF_0_17_2}" --replace-needed "libstdc++.so" "libstdc++_vendor.so" "${2}"
            ;;
        vendor/lib/libFaceGrade.so)
            [ "$2" = "" ] && return 0
            "${PATCHELF}" --remove-needed "libandroid.so" "${2}"
            "${PATCHELF_0_17_2}" --replace-needed "libstdc++.so" "libstdc++_vendor.so" "${2}"
            ;;
        vendor/lib/libMiCameraHal.so)
            [ "$2" = "" ] && return 0
            "${PATCHELF}" --remove-needed "libft2.so" "${2}"
            "${PATCHELF}" --remove-needed "libharfbuzz_ng.so" "${2}"
            "${PATCHELF}" --remove-needed "libheif.so" "${2}"
            "${PATCHELF}" --remove-needed "libicuuc.so" "${2}"
            "${PATCHELF}" --remove-needed "libminikin.so" "${2}"
            grep -q "libcamera_shim.so" "${2}" || "${PATCHELF}" --add-needed "libcamera_shim.so" "${2}"
            grep -q "libpiex_shim.so" "${2}" || "${PATCHELF}" --add-needed "libpiex_shim.so" "${2}"
            ;;
        vendor/lib/libarcsoft_beauty_shot.so)
            [ "$2" = "" ] && return 0
            "${PATCHELF}" --remove-needed "libandroid.so" "${2}"
            "${PATCHELF_0_17_2}" --replace-needed "libstdc++.so" "libstdc++_vendor.so" "${2}"
            ;;
        vendor/lib/libmmcamera2_sensor_modules.so)
            [ "$2" = "" ] && return 0
            sed -i 's|/data/misc/camera/camera_lsc_caldata.txt|/data/vendor/camera/camera_lsc_calib.txt|g' "${2}"
            ;;
        vendor/lib/libmmcamera2_stats_modules.so)
            [ "$2" = "" ] && return 0
            "${PATCHELF}" --remove-needed "libandroid.so" "${2}"
            "${PATCHELF}" --remove-needed "libgui.so" "${2}"
            ;;
        vendor/lib/libmpbase.so)
            [ "$2" = "" ] && return 0
            "${PATCHELF}" --remove-needed "libandroid.so" "${2}"
            "${PATCHELF_0_17_2}" --replace-needed "libstdc++.so" "libstdc++_vendor.so" "${2}"
            ;;
        vendor/lib64/libril-qc-hal-qmi.so)
            [ "$2" = "" ] && return 0
            for v in 1.{0..2}; do
                sed -i "s|android.hardware.radio.config@${v}.so|android.hardware.radio.c_shim@${v}.so|g" "${2}"
            done
            ;;
        *)
            return 1
            ;;
    esac

    return 0
}

function blob_fixup_dry() {
    blob_fixup "$1" ""
}

if [ -z "${ONLY_FIRMWARE}" ] && [ -z "${ONLY_TARGET}" ]; then
    # Initialize the helper for common device
    setup_vendor "${DEVICE_COMMON}" "${VENDOR_COMMON:-$VENDOR}" "${ANDROID_ROOT}" true "${CLEAN_VENDOR}"

    extract "${MY_DIR}/proprietary-files.txt" "${SRC}" "${KANG}" --section "${SECTION}"
fi

if [ -z "${ONLY_COMMON}" ] && [ -s "${MY_DIR}/../../${VENDOR}/${DEVICE}/proprietary-files.txt" ]; then
    # Reinitialize the helper for device
    source "${MY_DIR}/../../${VENDOR}/${DEVICE}/extract-files.sh"
    setup_vendor "${DEVICE}" "${VENDOR}" "${ANDROID_ROOT}" false "${CLEAN_VENDOR}"

    if [ -z "${ONLY_FIRMWARE}" ]; then
        extract "${MY_DIR}/../../${VENDOR}/${DEVICE}/proprietary-files.txt" "${SRC}" "${KANG}" --section "${SECTION}"
    fi

    if [ -z "${SECTION}" ] && [ -f "${MY_DIR}/../../${VENDOR}/${DEVICE}/proprietary-firmware.txt" ]; then
        extract_firmware "${MY_DIR}/../../${VENDOR}/${DEVICE}/proprietary-firmware.txt" "${SRC}"
    fi
fi

"${MY_DIR}/setup-makefiles.sh"
