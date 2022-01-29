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
#include <time.h>
#include <sys/resource.h>
#include <blkid/blkid.h>
#include <libmount/libmount.h>

#include <vector>
#include <string>
#include <mutex>
#include <map>
#include <set>
#include <atomic>

#include "src/common/Message.h"
#include "src/common/Trace.h"
#include "src/common/Const.h"
#include "src/common/FileSystems.h"
#include "src/common/Configuration.h"

#include "src/connector/file/FileConnector.h"
#include "src/connector/Connector.h"


std::atomic<bool> Connector::connectorTerminate(false);
std::atomic<bool> Connector::forcedTerminate(false);
std::atomic<bool> Connector::recallEventSystemStopped(false);

std::mutex FileConnector::mtx;
std::vector<std::string> FileConnector::managedFss;

Configuration *Connector::conf = nullptr;

Connector::Connector(bool _cleanup, Configuration *_conf) :
    cleanup(_cleanup)
{
    clock_gettime(CLOCK_REALTIME, &starttime);
    conf = _conf;
}

Connector::~Connector()
{
    if (cleanup) {
        try {
            std::string mountpt;

            MSG(LTFSDMF0025I);
            FileConnector::managedFss.clear();
            MSG(LTFSDMF0027I);
        } catch (const std::exception& e) {
            TRACE(Trace::error, e.what());
        }
    }

}

void Connector::initTransRecalls()
{
}

void Connector::endTransRecalls()
{
}

Connector::rec_info_t Connector::getEvents()
{
    Connector::rec_info_t recinfo;

    recinfo = (Connector::rec_info_t ) { 0, 0, (fuid_t) {0,0,0,0}, "" };

    return recinfo;
}

void Connector::respondRecallEvent(Connector::rec_info_t recinfo, bool success)
{
}

void Connector::terminate()
{
    Connector::connectorTerminate = true;
}

