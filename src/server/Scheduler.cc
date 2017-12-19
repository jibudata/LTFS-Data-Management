#include "ServerIncludes.h"

/** @page scheduler Scheduler

 */

std::mutex Scheduler::mtx;
std::condition_variable Scheduler::cond;
std::mutex Scheduler::updmtx;
std::condition_variable Scheduler::updcond;
std::map<int, std::atomic<bool>> Scheduler::updReq;

bool Scheduler::poolResAvail(unsigned long minFileSize)

{
    bool found;
    bool unmountedExists = false;
    std::shared_ptr<LTFSDMCartridge> cart;

    assert(pool.compare("") != 0);

    for (std::string cartname : Server::conf.getPool(pool)) {
        if ((cart = inventory->getCartridge(cartname)) == nullptr) {
            MSG(LTFSDMX0034E, cartname);
            Server::conf.poolRemove(pool, cartname);
        }
        if (cart->getState() == LTFSDMCartridge::TAPE_MOUNTED) {
            tapeId = cart->GetObjectID();
            found = false;
            for (std::shared_ptr<LTFSDMDrive> drive : inventory->getDrives()) {
                if (drive->get_slot() == cart->get_slot()
                        && 1024 * 1024 * cart->get_remaining_cap()
                                >= minFileSize) {
                    assert(drive->isBusy() == false);
                    cart->setState(LTFSDMCartridge::TAPE_INUSE);
                    TRACE(Trace::always, drive->GetObjectID());
                    drive->setBusy();
                    found = true;
                    break;
                }
            }
            assert(
                    found == true || 1024*1024*cart->get_remaining_cap() < minFileSize);
            if (found == true)
                return true;
        } else if (cart->getState() == LTFSDMCartridge::TAPE_UNMOUNTED)
            unmountedExists = true;
    }

    if (unmountedExists == false)
        return false;

    for (std::shared_ptr<LTFSDMDrive> drive : inventory->getDrives()) {
        if (drive->isBusy() == true)
            continue;
        found = false;
        for (std::shared_ptr<LTFSDMCartridge> card : inventory->getCartridges()) {
            if (drive->get_slot() == card->get_slot()
                    && card->getState() == LTFSDMCartridge::TAPE_MOUNTED) {
                found = true;
                break;
            }
        }
        if (found == false) {
            for (std::string cartname : Server::conf.getPool(pool)) {
                if ((cart = inventory->getCartridge(cartname)) == nullptr) {
                    MSG(LTFSDMX0034E, cartname);
                    Server::conf.poolRemove(pool, cartname);
                }
                if (cart->getState() == LTFSDMCartridge::TAPE_UNMOUNTED
                        && 1024 * 1024 * cart->get_remaining_cap()
                                >= minFileSize) {
                    TRACE(Trace::always, drive->GetObjectID());
                    drive->setBusy();
                    drive->setUnmountReqNum(reqNum);
                    cart->setState(LTFSDMCartridge::TAPE_MOVING);
                    subs.enqueue(std::string("m:") + cart->GetObjectID(), mount,
                            drive->GetObjectID(), cart->GetObjectID());
                    return false;
                }
            }

        }
    }

    for (std::shared_ptr<LTFSDMDrive> drive : inventory->getDrives())
        if (drive->getUnmountReqNum() == reqNum)
            return false;

    for (std::shared_ptr<LTFSDMDrive> drive : inventory->getDrives()) {
        if (drive->isBusy() == true)
            continue;
        for (std::shared_ptr<LTFSDMCartridge> card : inventory->getCartridges()) {
            if ((drive->get_slot() == card->get_slot())
                    && (card->getState() == LTFSDMCartridge::TAPE_MOUNTED)) {
                TRACE(Trace::normal, drive->GetObjectID());
                drive->setBusy();
                drive->setUnmountReqNum(reqNum);
                card->setState(LTFSDMCartridge::TAPE_MOVING);
                subs.enqueue(std::string("unm:") + card->GetObjectID(), unmount,
                        drive->GetObjectID(), card->GetObjectID());
                return false;
            }
        }
    }

    return false;
}

bool Scheduler::tapeResAvail()

{
    bool found;

    assert(tapeId.compare("") != 0);

    if (inventory->getCartridge(tapeId)->getState()
            == LTFSDMCartridge::TAPE_MOVING)
        return false;

    if (inventory->getCartridge(tapeId)->getState()
            == LTFSDMCartridge::TAPE_INUSE) {
        found = false;
        for (std::shared_ptr<LTFSDMDrive> drive : inventory->getDrives()) {
            if (drive->get_slot()
                    == inventory->getCartridge(tapeId)->get_slot()) {
                TRACE(Trace::always, op, drive->getToUnblock(),
                        drive->GetObjectID());
                if (op < drive->getToUnblock())
                    drive->setToUnblock(op);
                found = true;
                break;
            }
        }
        assert(found == true);
        return false;
    } else if (inventory->getCartridge(tapeId)->getState()
            == LTFSDMCartridge::TAPE_MOUNTED) {
        found = false;
        for (std::shared_ptr<LTFSDMDrive> drive : inventory->getDrives()) {
            if (drive->get_slot()
                    == inventory->getCartridge(tapeId)->get_slot()) {
                inventory->getCartridge(tapeId)->setState(
                        LTFSDMCartridge::TAPE_INUSE);
                assert(drive->isBusy() == false);
                TRACE(Trace::always, drive->GetObjectID());
                drive->setBusy();
                found = true;
                break;
            }
        }
        assert(found == true);
        return true;
    }

    // looking for a free drive
    for (std::shared_ptr<LTFSDMDrive> drive : inventory->getDrives()) {
        if (drive->isBusy() == true)
            continue;
        found = false;
        for (std::shared_ptr<LTFSDMCartridge> card : inventory->getCartridges()) {
            if ((drive->get_slot() == card->get_slot())
                    && (card->getState() == LTFSDMCartridge::TAPE_MOUNTED)) {
                found = true;
                break;
            }
        }
        if (found == false) {
            if (inventory->getCartridge(tapeId)->getState()
                    == LTFSDMCartridge::TAPE_UNMOUNTED) {
                TRACE(Trace::always, drive->GetObjectID());
                drive->setBusy();
                drive->setUnmountReqNum(reqNum);
                inventory->getCartridge(tapeId)->setState(
                        LTFSDMCartridge::TAPE_MOVING);
                subs.enqueue(std::string("m:") + tapeId, mount,
                        drive->GetObjectID(), tapeId);
                return false;
            }
        }
    }

    // looking for a tape to unmount
    for (std::shared_ptr<LTFSDMDrive> drive : inventory->getDrives()) {
        if (drive->isBusy() == true)
            continue;
        for (std::shared_ptr<LTFSDMCartridge> card : inventory->getCartridges()) {
            if ((drive->get_slot() == card->get_slot())
                    && (card->getState() == LTFSDMCartridge::TAPE_MOUNTED)) {
                TRACE(Trace::always, drive->GetObjectID());
                drive->setBusy();
                drive->setUnmountReqNum(reqNum);
                card->setState(LTFSDMCartridge::TAPE_MOVING);
                inventory->getCartridge(tapeId)->unsetRequested();
                subs.enqueue(std::string("unm:") + card->GetObjectID(), unmount,
                        drive->GetObjectID(), card->GetObjectID());
                return false;
            }
        }
    }

    if (inventory->getCartridge(tapeId)->isRequested())
        return false;

    // suspend an operation
    for (std::shared_ptr<LTFSDMDrive> drive : inventory->getDrives()) {
        if (op < drive->getToUnblock()) {
            TRACE(Trace::always, op, drive->getToUnblock(),
                    drive->GetObjectID());
            drive->setToUnblock(op);
            inventory->getCartridge(tapeId)->setRequested();
            break;
        }
    }

    return false;
}

bool Scheduler::resAvail(unsigned long minFileSize)

{
    if (op == DataBase::MIGRATION && tapeId.compare("") == 0)
        return poolResAvail(minFileSize);
    else
        return tapeResAvail();
}

unsigned long Scheduler::smallestMigJob(int reqNum, int replNum)

{
    unsigned long min;

    SQLStatement stmt = SQLStatement(Scheduler::SMALLEST_MIG_JOB) << reqNum
            << FsObj::RESIDENT << replNum;
    stmt.prepare();
    stmt.step(&min);
    stmt.finalize();

    return min;
}

void Scheduler::run(long key)

{
    TRACE(Trace::normal, __PRETTY_FUNCTION__);

    SQLStatement selstmt;
    SQLStatement updstmt;
    std::stringstream ssql;
    std::unique_lock<std::mutex> lock(mtx);
    unsigned long minFileSize;

    while (true) {
        cond.wait(lock);
        if (Server::terminate == true) {
            TRACE(Trace::always, (bool) Server::terminate);
            lock.unlock();
            break;
        }

        selstmt(Scheduler::SELECT_REQUEST) << DataBase::REQ_NEW;

        selstmt.prepare();
        while (selstmt.step(&op, &reqNum, &tgtState, &numRepl, &replNum, &pool,
                &tapeId)) {
            std::lock_guard<std::recursive_mutex> lock(LTFSDMInventory::mtx);

            if (op == DataBase::MIGRATION)
                minFileSize = smallestMigJob(reqNum, replNum);
            else
                minFileSize = 0;

            if (resAvail(minFileSize) == false)
                continue;

            TRACE(Trace::always, reqNum, tgtState, numRepl, replNum, pool);

            std::stringstream thrdinfo;

            switch (op) {
                case DataBase::MIGRATION:
                    updstmt(Scheduler::UPDATE_MIG_REQUEST)
                            << DataBase::REQ_INPROGRESS << tapeId << reqNum
                            << replNum << pool;
                    updstmt.doall();

                    thrdinfo << "M(" << reqNum << "," << replNum << "," << pool
                            << ")";

                    subs.enqueue(thrdinfo.str(), &Migration::execRequest,
                            Migration(getpid(), reqNum, { }, numRepl, tgtState),
                            replNum, pool, tapeId, true /* needsTape */);
                    break;
                case DataBase::SELRECALL:
                    updstmt(Scheduler::UPDATE_REC_REQUEST)
                            << DataBase::REQ_INPROGRESS << reqNum << tapeId;
                    updstmt.doall();

                    thrdinfo << "SR(" << reqNum << ")";
                    subs.enqueue(thrdinfo.str(), &SelRecall::execRequest,
                            SelRecall(getpid(), reqNum, tgtState), tapeId,
                            true /* needsTape */);
                    break;
                case DataBase::TRARECALL:
                    updstmt(Scheduler::UPDATE_REC_REQUEST)
                            << DataBase::REQ_INPROGRESS << reqNum << tapeId;
                    updstmt.doall();

                    thrdinfo << "TR(" << reqNum << ")";
                    subs.enqueue(thrdinfo.str(), &TransRecall::execRequest,
                            TransRecall(), reqNum, tapeId);
                    break;
                default:
                    TRACE(Trace::error, op);
            }
        }
        selstmt.finalize();
    }
    MSG(LTFSDMS0081I);
    subs.waitAllRemaining();
    MSG(LTFSDMS0082I);
}
