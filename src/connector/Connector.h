#ifndef _CONNECTOR_H
#define _CONNECTOR_H

class Connector {
public:
	struct rec_info_t {
		void *evinfo;
		int evinfolen;
		unsigned long long fsid;
        unsigned int igen;
        unsigned long long ino;
	};
	Connector(bool cleanup);
	~Connector();
	rec_info_t getEvents();
};

class FsObj {
private:
	void *handle;
	unsigned long handleLength;
	bool isLocked;
public:
	struct attr_t {
		unsigned long typeId;
		int copies;
		char tapeId[Const::maxReplica][Const::tapeIdLength+1];
	};
	enum file_state {
		RESIDENT,
		PREMIGRATED,
		MIGRATED
	};
	FsObj(std::string fileName);
	FsObj(unsigned long long fsId, unsigned int iGen, unsigned long long iNode);
	~FsObj();
	struct stat stat();
	unsigned long long getFsId();
	unsigned int getIGen();
	unsigned long long getINode();
	std::string getTapeId();
	void lock();
	void unlock();
	long read(long offset, unsigned long size, char *buffer);
	long write(long offset, unsigned long size, char *buffer);
	void addAttribute(attr_t value);
	void remAttribute();
	attr_t getAttribute();
	void finishPremigration();
	void finishRecall(file_state fstate);
	void prepareStubbing();
	void stub();
	file_state getMigState();
};

#endif /* _CONNECTOR_H */
