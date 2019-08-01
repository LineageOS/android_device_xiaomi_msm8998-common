# Copyright (C) 2009 The Android Open Source Project
# Copyright (c) 2011, The Linux Foundation. All rights reserved.
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

import hashlib
import common
import re

def FullOTA_Assertions(info):
  AddTrustZoneAssertion(info, info.input_zip)
  return

def IncrementalOTA_Assertions(info):
  AddTrustZoneAssertion(info, info.target_zip)
  return

def AddTrustZoneAssertion(info, input_zip):
  android_info = info.input_zip.read("OTA/android-info.txt")
  t = re.search(r'require\s+version-trustzone\s*=\s*(\S+)', android_info)
  f = re.search(r'require\s+version-firmware\s*=\s*(.+)', android_info)
  if t and f:
    versions_trustzone = t.group(1).split('|')
    version_firmware = f.group(1).rstrip()
    if ((len(versions_trustzone) and '*' not in versions_trustzone) and \
    (len(version_firmware) and '*' not in version_firmware)):
      cmd = 'assert(xiaomi.verify_trustzone(' + ','.join(['"%s"' % tz for tz in versions_trustzone]) + ') == "1" || \
abort("Error: This package requires MIUI firmware version ' + version_firmware + \
' or newer. Please upgrade firmware and retry!"););'
      info.script.AppendExtra(cmd)
  return
