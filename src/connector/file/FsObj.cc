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
#include <sys/stat.h>
#include <linux/fs.h>
#include <sys/xattr.h>
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <libmount/libmount.h>
#include <blkid/blkid.h>
#include <uuid/uuid.h>

#include <string>
#include <mutex>
#include <vector>
#include <map>
#include <set>
#include <atomic>

#include "src/common/Message.h"
#include "src/common/Trace.h"
#include "src/common/Const.h"
#include "src/common/FileSystems.h"

#include "src/common/Configuration.h"
#include "src/connector/file/FileConnector.h"
#include "src/connector/file/FileHandle.h"
#include "src/connector/Connector.h"

// static functions define
static mig_state_attr_t getMigInfoAt(int fd);
static void setMigInfoAt(int fd, mig_state_attr_t::state_num state);
static void getUUID(std::string fsName, uuid_t *uuid);
static mig_state_attr_t genMigInfoAt(int fd, mig_state_attr_t::state_num state);

// Util functions
void getUUID(std::string fsName, uuid_t *uuid)
{
    blkid_cache cache;
    char *uuidstr;
    int rc;

    if ((rc = blkid_get_cache(&cache, NULL)) != 0) {
        TRACE(Trace::error, rc, errno);
        MSG(LTFSDMF0055E, fsName);
        THROW(Error::GENERAL_ERROR, errno);
    }

    if ((uuidstr = blkid_get_tag_value(cache, "UUID", fsName.c_str())) == NULL) {
        blkid_put_cache(cache);
        TRACE(Trace::error, errno);
        MSG(LTFSDMF0055E, fsName);
        THROW(Error::GENERAL_ERROR, errno);
    }

    if ((rc = uuid_parse(uuidstr, *uuid)) != 0) {
        blkid_put_cache(cache);
        free(uuidstr);
        TRACE(Trace::error, rc, errno);
        MSG(LTFSDMF0055E, fsName);
        THROW(Error::GENERAL_ERROR, errno);
    }

    blkid_put_cache(cache);
    free(uuidstr);
}

mig_state_attr_t
genMigInfoAt(int fd, mig_state_attr_t::state_num state)

{
    mig_state_attr_t miginfo;
    struct stat statbuf;

    memset(&miginfo, 0, sizeof(miginfo));

    if (fstat(fd, &statbuf)) {
        TRACE(Trace::error, errno);
        THROW(Error::GENERAL_ERROR, errno, fd);
    }

    miginfo.typeId = typeid(mig_state_attr_t).hash_code();
    miginfo.size = statbuf.st_size;
    miginfo.atime = statbuf.st_atim;
    miginfo.mtime = statbuf.st_mtim;
    miginfo.state = state;

    clock_gettime(CLOCK_REALTIME, &miginfo.changed);

    return miginfo;
}

void setMigInfoAt(int fd, mig_state_attr_t::state_num state)
{
    ssize_t size;
    mig_state_attr_t miginfo_new;
    mig_state_attr_t miginfo;

    TRACE(Trace::full, fd, state);

    memset(&miginfo, 0, sizeof(miginfo));

    miginfo_new = genMigInfoAt(fd, state);

    if ((size = fgetxattr(fd, Const::LTFSDM_EA_MIGSTATE.c_str(),
            (void *) &miginfo, sizeof(miginfo))) == -1) {
        if ( errno != ENODATA) {
            THROW(Error::GENERAL_ERROR, errno, fd);
        }
    } else if (size != sizeof(miginfo)
            || miginfo.typeId != typeid(mig_state_attr_t).hash_code()) {
        errno = EIO;
        THROW(Error::GENERAL_ERROR, size, sizeof(miginfo), miginfo.typeId,
                typeid(mig_state_attr_t).hash_code(), fd);
    }

    if (miginfo.state != mig_state_attr_t::state_num::RESIDENT) {
        // keep the previous settings
        miginfo_new.size = miginfo.size;
        miginfo_new.atime = miginfo.atime;
        miginfo_new.mtime = miginfo.mtime;
    }

    if (fsetxattr(fd, Const::LTFSDM_EA_MIGSTATE.c_str(), (void *) &miginfo_new,
            sizeof(miginfo_new), 0) == -1) {
        THROW(Error::GENERAL_ERROR, errno, fd);
    }
}

mig_state_attr_t getMigInfoAt(int fd)
{
    ssize_t size;
    mig_state_attr_t miginfo;

    memset(&miginfo, 0, sizeof(miginfo));

    if ((size = fgetxattr(fd, Const::LTFSDM_EA_MIGSTATE.c_str(),
            (void *) &miginfo, sizeof(miginfo))) == -1) {
        /* TODO - check for errno */
        return miginfo;
    }

    if (size != sizeof(miginfo)
            || miginfo.typeId != typeid(mig_state_attr_t).hash_code()) {
        errno = EIO;
        THROW(Error::ATTR_FORMAT, size, sizeof(miginfo), miginfo.typeId,
                typeid(mig_state_attr_t).hash_code(), fd);
    }

    return miginfo;
}

fuid_t getxattrfuid(int fd) {
    ssize_t size;
    fuid_t fuidinfo;
    memset(&fuidinfo, 0, sizeof(fuidinfo));

    if ((size = fgetxattr(fd, Const::LTFSDM_EA_FILEINFO.c_str(),
            (void *) &fuidinfo, sizeof(fuidinfo))) == -1) {
        /* TODO - check for errno */
        return fuidinfo;
    }

    if (size != sizeof(fuidinfo)) {
        errno = EIO;
        THROW(Error::ATTR_FORMAT, size, sizeof(fuidinfo), fd);
    }

    return fuidinfo; 
}

/*
void FsObj::setfuid(int fd, unsigned long fuid_h, unsigned long fuid_l, unsigned int igen, unsigned long inum)
{
    fuid_t fuidinfo = {fuid_h, fuid_l, igen, inum};

    if (fsetxattr(fd, Const::LTFSDM_EA_FILEINFO.c_str(), (void *) &fuidinfo,
            sizeof(fuidinfo), 0) == -1) {
        THROW(Error::GENERAL_ERROR, errno, fd);
    }
}
*/

// FsObj member functions
FsObj::FsObj(std::string fileName)
{
    FileHandle *fh = new FileHandle();
    struct stat stbuf;
    bool found = false;

    fh->fd = Const::UNSET;

    if (::stat(fileName.c_str(), &stbuf) == -1) {
        TRACE(Trace::error, errno);
        THROW(Error::GENERAL_ERROR, fileName, errno);
    }
    if (S_ISDIR(stbuf.st_mode)) {
        strncpy(fh->mountpoint, fileName.c_str(), PATH_MAX - 1);
    } else {
        // Not directory, See if file path is under one mount point
        fh->fd = Const::UNSET;
        TRACE(Trace::always, FileConnector::managedFss.size(), fileName.c_str());
        strncpy(fh->filepath, fileName.c_str(), PATH_MAX - 1);
        for (auto mpath: FileConnector::managedFss) {
            TRACE(Trace::always, mpath, fileName.c_str());
            if (fileName.compare(0, mpath.size(), mpath) == 0) {
                fh->fd = open(fileName.c_str(), O_RDWR);
                TRACE(Trace::always, fh->fd, fileName.c_str());
                if (fh->fd == -1) {
                    delete (fh);
                    TRACE(Trace::error, errno);
                    THROW(Error::GENERAL_ERROR, fileName, errno);
                }
                found = true;
            }
        }
    }

    if (!found) {
        // Not managed
        delete (fh);
        THROW(Error::FILE_FROM_UNMANAGED_MP, fileName, 0);
    }

    handle = (void *) fh;
    handleLength = fileName.size();
}

FsObj::FsObj(std::string fileName, fuid_t fuid, mig_target_attr_t attr) {
    // create file
    int fd = open(fileName.c_str(),
    O_RDWR | O_CREAT | O_APPEND | O_CLOEXEC | O_SYNC, 0644);

    if (fd == Const::UNSET) {
        MSG(LTFSDMX0001E, errno);
        THROW(Error::GENERAL_ERROR, errno);
    }
    // set uid information
    // set extended attributes

}

FsObj::FsObj(Connector::rec_info_t recinfo) :
    FsObj::FsObj(recinfo.filename)
{
}

FsObj::~FsObj()
{
    FileHandle *fh = (FileHandle *) handle;
    if (fh->fd != Const::UNSET) {
        close(fh->fd);
    }
}

bool FsObj::isFsManaged()
{
    FileHandle *fh = (FileHandle *) handle;
    std::unique_lock<std::mutex> lock(FileConnector::mtx);
    std::set<std::string> fss;
    bool inConf;

    if (Connector::conf == nullptr)
        return false;

    fss = Connector::conf->getFss();

    if (fss.find(fh->mountpoint) == fss.end())
        return false;
    else
        inConf = true;

    if (inConf) {
        for (auto fsName: FileConnector::managedFss) {
            if (fsName.compare(fh->mountpoint) == 0) {
                return true;
            }
        }
    }

    // Only in configure file, not in managedFss vector
    return false;
}

void FsObj::manageFs(bool setDispo, struct timespec starttime)
{
    FileHandle *fh = (FileHandle *) handle;
    FileSystems fss;
    uuid_t uuid;
    FileSystems::fsinfo fs;
    std::set<std::string> fsset;

    TRACE(Trace::always, fh->mountpoint);

    for (auto fsName: FileConnector::managedFss) {
        if (fsName.compare(fh->mountpoint) == 0) {
            return;
        }
    }

    std::unique_lock<std::mutex> lock(FileConnector::mtx);
    FileConnector::managedFss.push_back(fh->mountpoint);

    fsset = Connector::conf->getFss();
    // Add the fs to config file
    if (fsset.find(fh->mountpoint) == fsset.end()) {
        try {
            // Get the filesystem info
            fs = fss.getByTarget(fh->mountpoint);
        } catch (const std::exception& e) {
            TRACE(Trace::error, e.what());
            MSG(LTFSDMS0080E, fh->mountpoint);
            THROW(Error::GENERAL_ERROR);
        }
        Connector::conf->addFs(fs);
    }

    TRACE(Trace::always, fs.source);

    try {
        getUUID(fs.source, &uuid);
    } catch (const std::exception& e) {
        THROW(Error::GENERAL_ERROR, fs.source, 0);
    }

    fh->fsid_h = be64toh(*(unsigned long *) &uuid[0]);
    fh->fsid_l = be64toh(*(unsigned long *) &uuid[8]);
}

struct stat FsObj::stat()
{
    struct stat statbuf;
    mig_state_attr_t miginfo;

    FileHandle *fh = (FileHandle *) handle;

    miginfo = getMigInfoAt(fh->fd);

    if (fstat(fh->fd, &statbuf) == -1) {
        TRACE(Trace::error, errno);
        THROW(Error::GENERAL_ERROR, errno, fh->filepath);
    }

    if (miginfo.state != mig_state_attr_t::state_num::RESIDENT
            && miginfo.state
                    != mig_state_attr_t::state_num::IN_MIGRATION) {
        statbuf.st_size = miginfo.size;
        statbuf.st_atim = miginfo.atime;
        statbuf.st_mtim = miginfo.mtime;
    }
    return statbuf;
}

fuid_t FsObj::getfuid()
{
    FileHandle *fh = (FileHandle *) handle;
    fuid_t fuid = getxattrfuid(fh->fd);
    if (fuid.igen != 0) {
        return fuid;
    }
    struct stat statbuf;

    fuid.fsid_h = fh->fsid_h;
    fuid.fsid_l = fh->fsid_l;

    TRACE(Trace::always, fh->fd);

    if (ioctl(fh->fd, FS_IOC_GETVERSION, &fuid.igen) == -1) {
        TRACE(Trace::error, errno);
        THROW(Error::GENERAL_ERROR, errno, fh->filepath);
    }
    if (fstat(fh->fd, &statbuf) == -1) {
        TRACE(Trace::error, errno);
        THROW(Error::GENERAL_ERROR, errno, fh->filepath);
    }

    fuid.inum = statbuf.st_ino;

    return fuid;
}

std::string FsObj::getTapeId()
{
    return "";
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
    FileHandle *fh = (FileHandle *) handle;
    long rsize = 0;
    rsize = ::read(fh->fd, buffer, size);

    if (rsize == -1) {
        TRACE(Trace::error, errno);
        THROW(Error::GENERAL_ERROR, errno, fh->filepath);
    }
    return rsize;
}

long FsObj::write(long offset, unsigned long size, char *buffer)
{
    FileHandle *fh = (FileHandle *) handle;
    long wsize = 0;

    wsize = ::write(fh->fd, buffer, size);

    if (wsize == -1) {
        TRACE(Trace::error, errno);
        THROW(Error::GENERAL_ERROR, errno, fh->filepath);
    }
    return wsize;
}

void FsObj::addTapeAttr(std::string tapeId, long startBlock)
{
    FsObj::mig_target_attr_t attr;
    FileHandle *fh = (FileHandle *) handle;
    std::unique_lock<FsObj> fsolock(*this);

    attr = getAttribute();
    memset(attr.tapeInfo[attr.copies].tapeId, 0, Const::tapeIdLength + 1);
    strncpy(attr.tapeInfo[attr.copies].tapeId, tapeId.c_str(),
            Const::tapeIdLength);
    attr.tapeInfo[attr.copies].startBlock = startBlock;
    attr.copies++;

    if (fsetxattr(fh->fd, Const::LTFSDM_EA_MIGINFO.c_str(), (void *) &attr,
            sizeof(attr), 0) == -1) {
        TRACE(Trace::error, errno);
        THROW(Error::GENERAL_ERROR, errno, fh->filepath);
    }
}

void FsObj::remAttribute()
{
    FileHandle *fh = (FileHandle *) handle;

    if (fremovexattr(fh->fd, Const::LTFSDM_EA_MIGINFO.c_str()) == -1) {
        TRACE(Trace::error, errno);
        MSG(LTFSDMF0018W, Const::LTFSDM_EA_MIGINFO);
        if ( errno != ENODATA)
            THROW(Error::GENERAL_ERROR, errno, fh->filepath);
    }

    if (fremovexattr(fh->fd, Const::LTFSDM_EA_MIGSTATE.c_str()) == -1) {
        TRACE(Trace::error, errno);
        MSG(LTFSDMF0018W, Const::LTFSDM_EA_MIGSTATE);
        if ( errno != ENODATA)
            THROW(Error::GENERAL_ERROR, errno, fh->filepath);
    }
}

FsObj::mig_target_attr_t FsObj::getAttribute()
{
    FileHandle *fh = (FileHandle *) handle;
    FsObj::mig_target_attr_t value;
    memset(&value, 0, sizeof(mig_target_attr_t));

    if (fgetxattr(fh->fd, Const::LTFSDM_EA_MIGINFO.c_str(), (void *) &value,
            sizeof(value)) == -1) {
        if ( errno != ENODATA) {
            TRACE(Trace::error, errno);
            THROW(Error::GENERAL_ERROR, errno, fh->filepath);
        }

        value.typeId = typeid(value).hash_code();

        return value;
    }

    if (value.typeId != typeid(value).hash_code()) {
        TRACE(Trace::error, value.typeId);
        THROW(Error::ATTR_FORMAT, (unsigned long ) handle);
    }

    return value;
}

void FsObj::preparePremigration()
{
    FileHandle *fh = (FileHandle *) handle;

    setMigInfoAt(fh->fd, mig_state_attr_t::state_num::IN_MIGRATION);
}

void FsObj::finishPremigration()
{
    FileHandle *fh = (FileHandle *) handle;

    setMigInfoAt(fh->fd, mig_state_attr_t::state_num::PREMIGRATED);
}

void FsObj::prepareRecall()
{
    FileHandle *fh = (FileHandle *) handle;

    setMigInfoAt(fh->fd, mig_state_attr_t::state_num::IN_RECALL);
}

void FsObj::finishRecall(FsObj::file_state fstate)
{
    FileHandle *fh = (FileHandle *) handle;
    mig_state_attr_t miginfo;

    if (fstate == FsObj::PREMIGRATED) {
        setMigInfoAt(fh->fd, mig_state_attr_t::state_num::PREMIGRATED);
    } else {
        miginfo = getMigInfoAt(fh->fd);
        const timespec timestamp[2] = { miginfo.atime, miginfo.mtime };

        if (futimens(fh->fd, timestamp) == -1)
            MSG(LTFSDMF0017E, fh->filepath);

        setMigInfoAt(fh->fd, mig_state_attr_t::state_num::RESIDENT);
    }
}

void FsObj::prepareStubbing()
{
    FileHandle *fh = (FileHandle *) handle;

    setMigInfoAt(fh->fd, mig_state_attr_t::state_num::STUBBING);
}

void FsObj::stub()
{
    FileHandle *fh = (FileHandle *) handle;
    struct stat statbuf;
    std::stringstream spath;
    int fd;

    spath << fh->mountpoint << "/" << fh->filepath;

    if (fstat(fh->fd, &statbuf) == -1) {
        TRACE(Trace::error, errno);
        THROW(Error::GENERAL_ERROR, errno, fh->filepath);
    }

    if (ftruncate(fh->fd, 0) == -1) {
        TRACE(Trace::error, errno);
        MSG(LTFSDMF0016E, fh->filepath);
        setMigInfoAt(fh->fd, mig_state_attr_t::state_num::PREMIGRATED);
        THROW(Error::GENERAL_ERROR, errno, fh->filepath);
    }

    if ((fd = open(spath.str().c_str(), O_RDONLY | O_CLOEXEC)) != -1) {
        posix_fadvise(fd, 0, 0, POSIX_FADV_DONTNEED);
        close(fd);
    }

    setMigInfoAt(fh->fd, mig_state_attr_t::state_num::MIGRATED);
}

FsObj::file_state FsObj::getMigState()
{
    FileHandle *fh = (FileHandle *) handle;
    FsObj::file_state state = FsObj::RESIDENT;
    mig_state_attr_t miginfo;

    try {
        miginfo = getMigInfoAt(fh->fd);
    } catch (const LTFSDMException& e) {
        if (e.getError() == Error::ATTR_FORMAT) {
            MSG(LTFSDMF0034E, fh->mountpoint, fh->filepath);
        }
        THROW(Error::GENERAL_ERROR, fh->filepath);
    }

    switch (miginfo.state) {
        case mig_state_attr_t::state_num::RESIDENT:
        case mig_state_attr_t::state_num::IN_MIGRATION:
            state = FsObj::RESIDENT;
            break;
        case mig_state_attr_t::state_num::MIGRATED:
        case mig_state_attr_t::state_num::IN_RECALL:
            state = FsObj::MIGRATED;
            break;
        case mig_state_attr_t::state_num::PREMIGRATED:
        case mig_state_attr_t::state_num::STUBBING:
            state = FsObj::PREMIGRATED;
    }

    return state;
}

