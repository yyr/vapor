#include <cstdio>
#include <cstring>
#include <cassert>
#include <vector>
#include <vapor/DataMgrFactory.h>
#include <vapor/DataMgrLayered.h>
#include <vapor/DataMgrWB.h>
#include <vapor/DataMgrWC.h>
#include <vapor/DataMgrAMR.h>

using namespace VAPoR;

DataMgr *DataMgrFactory::New(const vector <string> &files, size_t mem_size) {
	if (files.size() == 0) return (NULL);

	if (files[0].compare(files[0].length()-4, 4, ".vdf") == 0) {
		MetadataVDC *md = new MetadataVDC(files[0]);
		if (! md) return (NULL);

		string type = md->GetGridType();
		if (type.compare("layered") == 0) {
			return new DataMgrLayered(*md, mem_size);
		}
		else if (type.compare("block_amr") == 0) {
			return new DataMgrAMR(*md, mem_size);
		}
		else if (md->GetVDCType() == 1) {
			return new DataMgrWB(*md, mem_size);
		}
		else if (md->GetVDCType() == 2) {
			return new DataMgrWC(*md, mem_size);
		}
		else {
			return(NULL);
		}
	}
	else {
		return(NULL);
	}
}
