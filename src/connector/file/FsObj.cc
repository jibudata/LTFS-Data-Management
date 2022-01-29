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
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <linux/fs.h>
#include <sys/resource.h>
#include <libmount/libmount.h>
#include <blkid/blkid.h>

#include <string>
#include <mutex>
#include <vector>
#include <map>
#include <set>
#include <atomic>

#include "src/common/Message.h"
#include "src/common/Const.h"
#include "src/common/FileSystems.h"

#include "src/common/Configuration.h"
#include "src/connector/file/FileConnector.h"
#include "src/connector/file/FileHandle.h"
#include "src/connector/Connector.h"

// Get rid of build failure
static FileSystems fss;

FsObj::FsObj(std::string fileName)
{
}

FsObj::FsObj(Connector::rec_info_t recinfo) :
    FsObj::FsObj(recinfo.filename)
{
}

FsObj::~FsObj()
{
}

bool FsObj::isFsManaged()
{
    return false;
}

void FsObj::manageFs(bool setDispo, struct timespec starttime)
{
}

struct stat FsObj::stat()
{
    struct stat statbuf;
    return statbuf;
}

fuid_t FsObj::getfuid()
{
    fuid_t fuid;

    return fuid;
}

std::string FsObj::getTapeId()
{
    std::string tapeId;

    return tapeId;
}

void FsObj::lock()
{
}

bool FsObj::try_lock()
{
    return false;
}

void FsObj::unlock()
{
}

long FsObj::read(long offset, unsigned long size, char *buffer)
{
    long rsize = 0;
    return rsize;
}

long FsObj::write(long offset, unsigned long size, char *buffer)
{
    long wsize = 0;
    return wsize;
}

void FsObj::addTapeAttr(std::string tapeId, long startBlock)
{
}

void FsObj::remAttribute()
{
}

FsObj::mig_target_attr_t FsObj::getAttribute()
{
    FsObj::mig_target_attr_t attr;

    return attr;
}

void FsObj::preparePremigration()
{
}

void FsObj::finishPremigration()
{
}

void FsObj::prepareRecall()
{
}

void FsObj::finishRecall(FsObj::file_state fstate)
{
}

void FsObj::prepareStubbing()
{
}

void FsObj::stub()
{
}

FsObj::file_state FsObj::getMigState()
{
    FsObj::file_state state = FsObj::RESIDENT;
    return state;
}

