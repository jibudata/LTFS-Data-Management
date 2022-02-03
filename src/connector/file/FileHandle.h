/*******************************************************************************
 * Copyright 2022 Jibu Tech. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *******************************************************************************/


struct FileHandle
{
    char mountpoint[PATH_MAX];
    char filepath[PATH_MAX];
    unsigned long fsid_h;
    unsigned long fsid_l;
    int fd;
};

//! [migration state attribute]
struct mig_state_attr_t
{
    unsigned long typeId;
    enum state_num
    {
        RESIDENT = 0,
        IN_MIGRATION = 1,
        PREMIGRATED = 2,
        STUBBING = 3,
        MIGRATED = 4,
        IN_RECALL = 5
    } state;
    unsigned long size;
    struct timespec atime;
    struct timespec mtime;
    struct timespec changed;
};
