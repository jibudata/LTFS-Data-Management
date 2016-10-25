#include <iostream>
#include <fstream>
#include <mutex>

#include "src/common/const/Const.h"
#include "src/common/errors/errors.h"

#include "Message.h"

Message messageObject;

Message::~Message()

{
	if (messagefile.is_open())
		messagefile.close();
}

void Message::init()

{
	messagefile.exceptions(std::ofstream::failbit | std::ofstream::badbit);

	try {
		messagefile.open(Const::LOG_FILE, std::fstream::out | std::fstream::app);
	}
	catch(...) {
		std::cerr << messages[LTFSDMX0003E];
		exit((int) Error::LTFSDM_GENERAL_ERROR);
	}
}

void Message::writeOut(std::string msgstr)

{
	mtx.lock();
	std::cout << msgstr;
	mtx.unlock();
}

void Message::writeLog(std::string msgstr)

{
	try {
		mtx.lock();
		messagefile << msgstr;
		messagefile.flush();
		mtx.unlock();
	}
	catch(...) {
		mtx.unlock();
		std::cerr << messages[LTFSDMX0004E];
		exit((int) Error::LTFSDM_GENERAL_ERROR);
	}
}
