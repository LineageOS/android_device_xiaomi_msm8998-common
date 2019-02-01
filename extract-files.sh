#!/bin/bash
#
# Copyright (C) 2016 The CyanogenMod Project
# Copyright (C) 2017 The LineageOS Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

set -e

# Load extract_utils and do some sanity checks
MY_DIR="${BASH_SOURCE%/*}"
if [[ ! -d "${MY_DIR}" ]]; then MY_DIR="${PWD}"; fi

LINEAGE_ROOT="${MY_DIR}"/../../..

HELPER="${LINEAGE_ROOT}/vendor/lineage/build/tools/extract_utils.sh"
if [ ! -f "${HELPER}" ]; then
    echo "Unable to find helper script at $HELPER"
    exit 1
fi
source "${HELPER}"

# Default to sanitizing the vendor folder before extraction
CLEAN_VENDOR=true

while [ "${#}" -gt 0 ]; do
    case "${1}" in
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
    vendor/etc/permissions/qti_libpermissions.xml)
        sed -i 's|name=\"android.hidl.manager-V1.0-java|name=\"android.hidl.manager@1.0-java|g' "${2}"
        ;;
    vendor/etc/permissions/qti-vzw-ims-internal.xml)
        sed -i 's|/system/vendor/framework/qti-vzw-ims-internal.jar|/vendor/framework/qti-vzw-ims-internal.jar|g' "${2}"
        ;;
    vendor/etc/permissions/qcrilhook.xml)
        sed -i 's|/system/framework/qcrilhook.jar|/vendor/framework/qcrilhook.jar|g' "${2}"
        ;;
    vendor/lib/libMiCameraHal.so)
        sed -i 's|system/etc/dualcamera.png|vendor/etc/dualcamera.png|g' "${2}"
        patchelf --replace-needed "libicuuc.so" "libicuuc-v27.so" "${2}"
        ;;
    vendor/lib/hw/camera.msm8998.so)
        patchelf --replace-needed "libminikin.so" "libminikin-v27.so" "${2}"
        ;;
    vendor/lib/libicuuc-v27.so)
        patchelf --set-soname "libicuuc-v27.so" "${2}"
        ;;
    vendor/lib/libminikin-v27.so)
        patchelf --set-soname "libminikin-v27.so" "${2}"
        ;;
    vendor/lib/libmmcamera2_sensor_modules.so)
        sed -i "s|/system/etc/camera/|/vendor/etc/camera/|g" "${2}"
        ;;
    vendor/lib/libFaceGrade.so)
        patchelf --remove-needed "libandroid.so" "${2}"
        ;;
    vendor/lib/libarcsoft_beauty_shot.so)
        patchelf --remove-needed "libandroid.so" "${2}"
        ;;
    vendor/lib/libmmcamera2_stats_modules.so)
        patchelf --remove-needed "libandroid.so" "${2}"
        ;;
    vendor/lib/libmpbase.so)
        patchelf --remove-needed "libandroid.so" "${2}"
        ;;
    vendor/lib64/lib-dplmedia.so)
        patchelf --remove-needed "libmedia.so" "${2}"
        ;;
    vendor/etc/init/com.qualcomm.qti.wifidisplayhal@1.0-service.rc)
        sed -i "/ disabled/Q" "${2}"
        ;;
    vendor/bin/mlipayd)
        patchelf --remove-needed "libandroid_runtime.so" "${2}"
        patchelf --remove-needed "vendor.xiaomi.hardware.mtdservice@1.0.so" "${2}"
        ;;
    vendor/lib64/libmlipay.so)
        patchelf --remove-needed "libandroid_runtime.so" "${2}"
        patchelf --remove-needed "vendor.xiaomi.hardware.mtdservice@1.0.so" "${2}"
        ;;
	esac
}

# Initialize the helper for common device
setup_vendor "${DEVICE_COMMON}" "${VENDOR}" "${LINEAGE_ROOT}" true "${CLEAN_VENDOR}"

extract "${MY_DIR}/proprietary-files.txt" "${SRC}" \
        "${KANG}" --section "${SECTION}"

if [ -s "${MY_DIR}/../${DEVICE}/proprietary-files.txt" ]; then
    # Reinitialize the helper for device
    source "${MY_DIR}/../${DEVICE}/extract-files.sh"
    setup_vendor "${DEVICE}" "${VENDOR}" "${LINEAGE_ROOT}" false "${CLEAN_VENDOR}"

    extract "${MY_DIR}/../${DEVICE}/proprietary-files.txt" "${SRC}" \
            "${KANG}" --section "${SECTION}"
fi

source "${MY_DIR}/setup-makefiles.sh"
