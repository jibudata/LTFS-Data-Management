#include <unistd.h>
#include <sys/resource.h>
#include <blkid/blkid.h>

#include <string>
#include <list>
#include <set>
#include <sstream>
#include <exception>

#include "src/common/errors.h"
#include "src/common/LTFSDMException.h"
#include "src/common/Message.h"
#include "src/common/Trace.h"

#include "src/communication/ltfsdm.pb.h"
#include "src/communication/LTFSDmComm.h"
#include "src/common/FileSystems.h"
#include "src/common/Configuration.h"

#include "src/connector/Connector.h"
#include "src/connector/file/FileHandle.h"

#include "LTFSDMCommand.h"
#include "RecoverCommand.h"

/** @page ltfsdm_recover ltfsdm recover
    The ltfsdm recover command is used to recover stub files information.
    Note: Currently, this command is only for debug use.

    <tt>@LTFSDMC0004I</tt>

    parameters | description
    ---|---
    -f \<configure file path\> | a configuration file containing all the required info of stub file

    Example:

    @verbatim
    [root@tapedev bin]# ltfsdm recover -f stub_file_info.json
    Stub files has been successfully recovered.
    @endverbatim

    The corresponding class is @ref RecoverCommand.

    @page recover_processing stub file recover processing

    Stub files configuration example:
    @verbatim
    [
        {
            "filename": "/mnt/dm1/file.1",
            "state": 2,
            "size": 2097024,
            "blocks": 671088640,
            "fuid": {
                "fsidh": 0,
                "fsidl": 0,
                "igen": 2936569639,
                "inum": 69
            },
            "tapeinfo": {
                "tapeid": "004639L7",
                "startblock": 12346
            }
        },        {
            "filename": "/mnt/dm1/file.2",
            "state": 2,
            "size": 1310720,
            "blocks": 671088640,
            "fuid": {
                "fsidh": 0,
                "fsidl": 0,
                "igen": 3180831201,
                "inum": 70
            },
            "tapeinfo": {
                "tapeid": "004639L7",
                "startblock": 12346
            }
        }
    ]
    @endverbatim

 */

void RecoverCommand::printUsage()
{
    INFO(LTFSDMC0004I);
}

void RecoverCommand::doCommand(int argc, char **argv)
{
    processOptions(argc, argv);
    LTFSDMCommand::traceParms();

    printUsage();

    // TBD: client for recover command

    // sample for test only
    try {
        connect();
    } catch (const std::exception& e) {
        MSG(LTFSDMC0026E);
        return;
    }

    LTFSDmProtocol::LTFSDmRecoverRequest *recoverreq =
            commCommand.mutable_recoverrequest();
    std::string filename = "/mnt/dm1/testrecover1";

    recoverreq->set_key(key);
    recoverreq->set_reqnumber(requestNumber);
    LTFSDmProtocol::LTFSDmInfoFilesResp* fileinfos = recoverreq->add_fileinfo();
    fileinfos->set_migstate(mig_state_attr_t::MIGRATED);
    fileinfos->set_filename(filename);
    fileinfos->set_size(22);
    LTFSDmProtocol::LTFSDmFuidInfo* fuidinfo = fileinfos->mutable_fuidinfo();
    fuidinfo->set_fsidh(0);
    fuidinfo->set_fsidl(0);
    fuidinfo->set_inum(75);
    fuidinfo->set_igen(195596983);
    LTFSDmProtocol::LTFSDmTapeInfo* tapeinfo = fileinfos->add_tapeinfo();
    tapeinfo->set_tapeid("004639L7");
    tapeinfo->set_startblock(184939);

    try {
        commCommand.send();
    } catch (const std::exception& e) {
        MSG(LTFSDMC0027E);
        THROW(Error::GENERAL_ERROR);
    }

    try {
        commCommand.recv();
    } catch (const std::exception& e) {
        MSG(LTFSDMC0028E);
        THROW(Error::GENERAL_ERROR);
    }

    const LTFSDmProtocol::LTFSDmRecoverResp recoverresp =
            commCommand.recoverresp();

    INFO(LTFSDMC0110I, recoverresp.filenames_size(), recoverresp.success());
}