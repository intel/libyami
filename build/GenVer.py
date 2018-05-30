#! /usr/bin/env python3
"""
 * Copyright (C) 2013 Intel Corporation. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
"""

import re
import os.path as path

def query(config, name):
     m = re.search('m4_define\\(\\[' + name + '\\], *([0-9]*)\\)', config)
     return m.group(1)

def replace(tpl, config, dest, src):
    v = query(config, src)
    return tpl.replace(dest, v)

cur = path.dirname(__file__)
root = path.abspath(path.join(cur, ".."))
with open(path.join(root, "interface/YamiVersion.h.in")) as f:
    tpl = f.read()
with open(path.join(root, "configure.ac")) as f:
    config = f.read()
tpl = replace(tpl, config, '@YAMI_API_MAJOR_VERSION@', 'yami_api_major_version')
tpl = replace(tpl, config, '@YAMI_API_MINOR_VERSION@', 'yami_api_minor_version')
tpl = replace(tpl, config, '@YAMI_API_MICRO_VERSION@', 'yami_api_micro_version')

with open(path.join(root, "interface/YamiVersion.h"), "w") as f:
     f.write(tpl)
