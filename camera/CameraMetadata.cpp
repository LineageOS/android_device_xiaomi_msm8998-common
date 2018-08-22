/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// #define LOG_NDEBUG 0

#define LOG_TAG "CamComm1.0-MD"
#include <log/log.h>
#include <utils/Errors.h>

#include "CameraMetadata.h"
#include "VendorTagDescriptor.h"

namespace android {
namespace hardware {
namespace camera {
namespace common {
namespace V1_0 {
namespace helper {

#define ALIGN_TO(val, alignment) \
    (((uintptr_t)(val) + ((alignment) - 1)) & ~((alignment) - 1))

CameraMetadata::CameraMetadata() :
        mBuffer(NULL), mLocked(false) {
}

CameraMetadata::CameraMetadata(size_t entryCapacity, size_t dataCapacity) :
        mLocked(false)
{
    mBuffer = allocate_camera_metadata(entryCapacity, dataCapacity);
}

CameraMetadata::CameraMetadata(const CameraMetadata &other) :
        mLocked(false) {
    mBuffer = clone_camera_metadata(other.mBuffer);
}

CameraMetadata::CameraMetadata(camera_metadata_t *buffer) :
        mBuffer(NULL), mLocked(false) {
    acquire(buffer);
}

CameraMetadata &CameraMetadata::operator=(const CameraMetadata &other) {
    return operator=(other.mBuffer);
}

CameraMetadata &CameraMetadata::operator=(const camera_metadata_t *buffer) {
    if (mLocked) {
        ALOGE("%s: Assignment to a locked CameraMetadata!", __FUNCTION__);
        return *this;
    }

    if (CC_LIKELY(buffer != mBuffer)) {
        camera_metadata_t *newBuffer = clone_camera_metadata(buffer);
        clear();
        mBuffer = newBuffer;
    }
    return *this;
}

CameraMetadata::~CameraMetadata() {
    mLocked = false;
    clear();
}

const camera_metadata_t* CameraMetadata::getAndLock() const {
    mLocked = true;
    return mBuffer;
}

status_t CameraMetadata::unlock(const camera_metadata_t *buffer) const {
    if (!mLocked) {
        ALOGE("%s: Can't unlock a non-locked CameraMetadata!", __FUNCTION__);
        return INVALID_OPERATION;
    }
    if (buffer != mBuffer) {
        ALOGE("%s: Can't unlock CameraMetadata with wrong pointer!",
                __FUNCTION__);
        return BAD_VALUE;
    }
    mLocked = false;
    return OK;
}

camera_metadata_t* CameraMetadata::release() {
    if (mLocked) {
        ALOGE("%s: CameraMetadata is locked", __FUNCTION__);
        return NULL;
    }
    camera_metadata_t *released = mBuffer;
    mBuffer = NULL;
    return released;
}

void CameraMetadata::clear() {
    if (mLocked) {
        ALOGE("%s: CameraMetadata is locked", __FUNCTION__);
        return;
    }
    if (mBuffer) {
        free_camera_metadata(mBuffer);
        mBuffer = NULL;
    }
}

void CameraMetadata::acquire(camera_metadata_t *buffer) {
    if (mLocked) {
        ALOGE("%s: CameraMetadata is locked", __FUNCTION__);
        return;
    }
    clear();
    mBuffer = buffer;

    ALOGE_IF(validate_camera_metadata_structure(mBuffer, /*size*/NULL) != OK,
             "%s: Failed to validate metadata structure %p",
             __FUNCTION__, buffer);
}

void CameraMetadata::acquire(CameraMetadata &other) {
    if (mLocked) {
        ALOGE("%s: CameraMetadata is locked", __FUNCTION__);
        return;
    }
    acquire(other.release());
}

status_t CameraMetadata::append(const CameraMetadata &other) {
    return append(other.mBuffer);
}

status_t CameraMetadata::append(const camera_metadata_t* other) {
    if (mLocked) {
        ALOGE("%s: CameraMetadata is locked", __FUNCTION__);
        return INVALID_OPERATION;
    }
    size_t extraEntries = get_camera_metadata_entry_count(other);
    size_t extraData = get_camera_metadata_data_count(other);
    resizeIfNeeded(extraEntries, extraData);

    return append_camera_metadata(mBuffer, other);
}

size_t CameraMetadata::entryCount() const {
    return (mBuffer == NULL) ? 0 :
            get_camera_metadata_entry_count(mBuffer);
}

bool CameraMetadata::isEmpty() const {
    return entryCount() == 0;
}

status_t CameraMetadata::sort() {
    if (mLocked) {
        ALOGE("%s: CameraMetadata is locked", __FUNCTION__);
        return INVALID_OPERATION;
    }
    return sort_camera_metadata(mBuffer);
}

status_t CameraMetadata::checkType(uint32_t tag, uint8_t expectedType) {
    int tagType = get_camera_metadata_tag_type(tag);
    if ( CC_UNLIKELY(tagType == -1)) {
        ALOGE("Update metadata entry: Unknown tag %d", tag);
        return INVALID_OPERATION;
    }
    if ( CC_UNLIKELY(tagType != expectedType) ) {
        ALOGE("Mismatched tag type when updating entry %s (%d) of type %s; "
                "got type %s data instead ",
                get_camera_metadata_tag_name(tag), tag,
                camera_metadata_type_names[tagType],
                camera_metadata_type_names[expectedType]);
        return INVALID_OPERATION;
    }
    return OK;
}

status_t CameraMetadata::update(uint32_t tag,
        const int32_t *data, size_t data_count) {
    status_t res;
    if (mLocked) {
        ALOGE("%s: CameraMetadata is locked", __FUNCTION__);
        return INVALID_OPERATION;
    }
    if ( (res = checkType(tag, TYPE_INT32)) != OK) {
        return res;
    }
    return updateImpl(tag, (const void*)data, data_count);
}

status_t CameraMetadata::update(uint32_t tag,
        const uint8_t *data, size_t data_count) {
    status_t res;
    if (mLocked) {
        ALOGE("%s: CameraMetadata is locked", __FUNCTION__);
        return INVALID_OPERATION;
    }
    if ( (res = checkType(tag, TYPE_BYTE)) != OK) {
        return res;
    }
    return updateImpl(tag, (const void*)data, data_count);
}

status_t CameraMetadata::update(uint32_t tag,
        const float *data, size_t data_count) {
    status_t res;
    if (mLocked) {
        ALOGE("%s: CameraMetadata is locked", __FUNCTION__);
        return INVALID_OPERATION;
    }
    if ( (res = checkType(tag, TYPE_FLOAT)) != OK) {
        return res;
    }
    return updateImpl(tag, (const void*)data, data_count);
}

status_t CameraMetadata::update(uint32_t tag,
        const int64_t *data, size_t data_count) {
    status_t res;
    if (mLocked) {
        ALOGE("%s: CameraMetadata is locked", __FUNCTION__);
        return INVALID_OPERATION;
    }
    if ( (res = checkType(tag, TYPE_INT64)) != OK) {
        return res;
    }
    return updateImpl(tag, (const void*)data, data_count);
}

status_t CameraMetadata::update(uint32_t tag,
        const double *data, size_t data_count) {
    status_t res;
    if (mLocked) {
        ALOGE("%s: CameraMetadata is locked", __FUNCTION__);
        return INVALID_OPERATION;
    }
    if ( (res = checkType(tag, TYPE_DOUBLE)) != OK) {
        return res;
    }
    return updateImpl(tag, (const void*)data, data_count);
}

status_t CameraMetadata::update(uint32_t tag,
        const camera_metadata_rational_t *data, size_t data_count) {
    status_t res;
    if (mLocked) {
        ALOGE("%s: CameraMetadata is locked", __FUNCTION__);
        return INVALID_OPERATION;
    }
    if ( (res = checkType(tag, TYPE_RATIONAL)) != OK) {
        return res;
    }
    return updateImpl(tag, (const void*)data, data_count);
}

status_t CameraMetadata::update(uint32_t tag,
        const String8 &string) {
    status_t res;
    if (mLocked) {
        ALOGE("%s: CameraMetadata is locked", __FUNCTION__);
        return INVALID_OPERATION;
    }
    if ( (res = checkType(tag, TYPE_BYTE)) != OK) {
        return res;
    }
    // string.size() doesn't count the null termination character.
    return updateImpl(tag, (const void*)string.string(), string.size() + 1);
}

status_t CameraMetadata::update(const camera_metadata_ro_entry &entry) {
    status_t res;
    if (mLocked) {
        ALOGE("%s: CameraMetadata is locked", __FUNCTION__);
        return INVALID_OPERATION;
    }
    if ( (res = checkType(entry.tag, entry.type)) != OK) {
        return res;
    }
    return updateImpl(entry.tag, (const void*)entry.data.u8, entry.count);
}

status_t CameraMetadata::updateImpl(uint32_t tag, const void *data,
        size_t data_count) {
    status_t res;
    if (mLocked) {
        ALOGE("%s: CameraMetadata is locked", __FUNCTION__);
        return INVALID_OPERATION;
    }
    int type = get_camera_metadata_tag_type(tag);
    if (type == -1) {
        ALOGE("%s: Tag %d not found", __FUNCTION__, tag);
        return BAD_VALUE;
    }
    // Safety check - ensure that data isn't pointing to this metadata, since
    // that would get invalidated if a resize is needed
    size_t bufferSize = get_camera_metadata_size(mBuffer);
    uintptr_t bufAddr = reinterpret_cast<uintptr_t>(mBuffer);
    uintptr_t dataAddr = reinterpret_cast<uintptr_t>(data);
    if (dataAddr > bufAddr && dataAddr < (bufAddr + bufferSize)) {
        ALOGE("%s: Update attempted with data from the same metadata buffer!",
                __FUNCTION__);
        return INVALID_OPERATION;
    }

    size_t data_size = calculate_camera_metadata_entry_data_size(type,
            data_count);

    res = resizeIfNeeded(1, data_size);

    if (res == OK) {
        camera_metadata_entry_t entry;
        res = find_camera_metadata_entry(mBuffer, tag, &entry);
        if (res == NAME_NOT_FOUND) {
            res = add_camera_metadata_entry(mBuffer,
                    tag, data, data_count);
        } else if (res == OK) {
            res = update_camera_metadata_entry(mBuffer,
                    entry.index, data, data_count, NULL);
        }
    }

    if (res != OK) {
        ALOGE("%s: Unable to update metadata entry %s.%s (%x): %s (%d)",
                __FUNCTION__, get_camera_metadata_section_name(tag),
                get_camera_metadata_tag_name(tag), tag, strerror(-res), res);
    }

    IF_ALOGV() {
        ALOGE_IF(validate_camera_metadata_structure(mBuffer, /*size*/NULL) !=
                 OK,

                 "%s: Failed to validate metadata structure after update %p",
                 __FUNCTION__, mBuffer);
    }

    return res;
}

bool CameraMetadata::exists(uint32_t tag) const {
    camera_metadata_ro_entry entry;
    return find_camera_metadata_ro_entry(mBuffer, tag, &entry) == 0;
}

camera_metadata_entry_t CameraMetadata::find(uint32_t tag) {
    status_t res;
    camera_metadata_entry entry;
    if (mLocked) {
        ALOGE("%s: CameraMetadata is locked", __FUNCTION__);
        entry.count = 0;
        return entry;
    }
    res = find_camera_metadata_entry(mBuffer, tag, &entry);
    if (CC_UNLIKELY( res != OK )) {
        entry.count = 0;
        entry.data.u8 = NULL;
    }
    return entry;
}

camera_metadata_ro_entry_t CameraMetadata::find(uint32_t tag) const {
    status_t res;
    camera_metadata_ro_entry entry;
    res = find_camera_metadata_ro_entry(mBuffer, tag, &entry);
    if (CC_UNLIKELY( res != OK )) {
        entry.count = 0;
        entry.data.u8 = NULL;
    }
    return entry;
}

status_t CameraMetadata::erase(uint32_t tag) {
    camera_metadata_entry_t entry;
    status_t res;
    if (mLocked) {
        ALOGE("%s: CameraMetadata is locked", __FUNCTION__);
        return INVALID_OPERATION;
    }
    res = find_camera_metadata_entry(mBuffer, tag, &entry);
    if (res == NAME_NOT_FOUND) {
        return OK;
    } else if (res != OK) {
        ALOGE("%s: Error looking for entry %s.%s (%x): %s %d",
                __FUNCTION__,
                get_camera_metadata_section_name(tag),
                get_camera_metadata_tag_name(tag), tag, strerror(-res), res);
        return res;
    }
    res = delete_camera_metadata_entry(mBuffer, entry.index);
    if (res != OK) {
        ALOGE("%s: Error deleting entry %s.%s (%x): %s %d",
                __FUNCTION__,
                get_camera_metadata_section_name(tag),
                get_camera_metadata_tag_name(tag), tag, strerror(-res), res);
    }
    return res;
}

void CameraMetadata::dump(int fd, int verbosity, int indentation) const {
    dump_indented_camera_metadata(mBuffer, fd, verbosity, indentation);
}

status_t CameraMetadata::resizeIfNeeded(size_t extraEntries, size_t extraData) {
    if (mBuffer == NULL) {
        mBuffer = allocate_camera_metadata(extraEntries * 2, extraData * 2);
        if (mBuffer == NULL) {
            ALOGE("%s: Can't allocate larger metadata buffer", __FUNCTION__);
            return NO_MEMORY;
        }
    } else {
        size_t currentEntryCount = get_camera_metadata_entry_count(mBuffer);
        size_t currentEntryCap = get_camera_metadata_entry_capacity(mBuffer);
        size_t newEntryCount = currentEntryCount +
                extraEntries;
        newEntryCount = (newEntryCount > currentEntryCap) ?
                newEntryCount * 2 : currentEntryCap;

        size_t currentDataCount = get_camera_metadata_data_count(mBuffer);
        size_t currentDataCap = get_camera_metadata_data_capacity(mBuffer);
        size_t newDataCount = currentDataCount +
                extraData;
        newDataCount = (newDataCount > currentDataCap) ?
                newDataCount * 2 : currentDataCap;

        if (newEntryCount > currentEntryCap ||
                newDataCount > currentDataCap) {
            camera_metadata_t *oldBuffer = mBuffer;
            mBuffer = allocate_camera_metadata(newEntryCount,
                    newDataCount);
            if (mBuffer == NULL) {
                ALOGE("%s: Can't allocate larger metadata buffer", __FUNCTION__);
                return NO_MEMORY;
            }
            append_camera_metadata(mBuffer, oldBuffer);
            free_camera_metadata(oldBuffer);
        }
    }
    return OK;
}

void CameraMetadata::swap(CameraMetadata& other) {
    if (mLocked) {
        ALOGE("%s: CameraMetadata is locked", __FUNCTION__);
        return;
    } else if (other.mLocked) {
        ALOGE("%s: Other CameraMetadata is locked", __FUNCTION__);
        return;
    }

    camera_metadata* thisBuf = mBuffer;
    camera_metadata* otherBuf = other.mBuffer;

    other.mBuffer = thisBuf;
    mBuffer = otherBuf;
}

status_t CameraMetadata::getTagFromName(const char *name,
        const VendorTagDescriptor* vTags, uint32_t *tag) {

    if (name == nullptr || tag == nullptr) return BAD_VALUE;

    size_t nameLength = strlen(name);

    const SortedVector<String8> *vendorSections;
    size_t vendorSectionCount = 0;

    if (vTags != NULL) {
        vendorSections = vTags->getAllSectionNames();
        vendorSectionCount = vendorSections->size();
    }

    // First, find the section by the longest string match
    const char *section = NULL;
    size_t sectionIndex = 0;
    size_t sectionLength = 0;
    size_t totalSectionCount = ANDROID_SECTION_COUNT + vendorSectionCount;
    for (size_t i = 0; i < totalSectionCount; ++i) {

        const char *str = (i < ANDROID_SECTION_COUNT) ? camera_metadata_section_names[i] :
                (*vendorSections)[i - ANDROID_SECTION_COUNT].string();

        ALOGV("%s: Trying to match against section '%s'", __FUNCTION__, str);

        if (strstr(name, str) == name) { // name begins with the section name
            size_t strLength = strlen(str);

            ALOGV("%s: Name begins with section name", __FUNCTION__);

            // section name is the longest we've found so far
            if (section == NULL || sectionLength < strLength) {
                section = str;
                sectionIndex = i;
                sectionLength = strLength;

                ALOGV("%s: Found new best section (%s)", __FUNCTION__, section);
            }
        }
    }

    if (section == NULL) {
        return NAME_NOT_FOUND;
    } else {
        ALOGV("%s: Found matched section '%s' (%zu)",
              __FUNCTION__, section, sectionIndex);
    }

    // Get the tag name component of the name
    const char *nameTagName = name + sectionLength + 1; // x.y.z -> z
    if (sectionLength + 1 >= nameLength) {
        return BAD_VALUE;
    }

    // Match rest of name against the tag names in that section only
    uint32_t candidateTag = 0;
    if (sectionIndex < ANDROID_SECTION_COUNT) {
        // Match built-in tags (typically android.*)
        uint32_t tagBegin, tagEnd; // [tagBegin, tagEnd)
        tagBegin = camera_metadata_section_bounds[sectionIndex][0];
        tagEnd = camera_metadata_section_bounds[sectionIndex][1];

        for (candidateTag = tagBegin; candidateTag < tagEnd; ++candidateTag) {
            const char *tagName = get_camera_metadata_tag_name(candidateTag);

            if (strcmp(nameTagName, tagName) == 0) {
                ALOGV("%s: Found matched tag '%s' (%d)",
                      __FUNCTION__, tagName, candidateTag);
                break;
            }
        }

        if (candidateTag == tagEnd) {
            return NAME_NOT_FOUND;
        }
    } else if (vTags != NULL) {
        // Match vendor tags (typically com.*)
        const String8 sectionName(section);
        const String8 tagName(nameTagName);

        status_t res = OK;
        if ((res = vTags->lookupTag(tagName, sectionName, &candidateTag)) != OK) {
            return NAME_NOT_FOUND;
        }
    }

    *tag = candidateTag;
    return OK;
}


} // namespace helper
} // namespace V1_0
} // namespace common
} // namespace camera
} // namespace hardware
} // namespace android
