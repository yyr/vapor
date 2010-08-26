#include <cstdio>
#include <cstring>
#include <cassert>
#include <vector>
#include <vapor/DataMgrFactory.h>
#include <vapor/DataMgrLayered.h>
#include <vapor/DataMgrWB.h>
#include <vapor/DataMgrWC.h>
#include <vapor/DataMgrAMR.h>
#include <vapor/DataMgrWRF.h>

using namespace VAPoR;

DataMgr *DataMgrFactory::New(
	const vector <string> &files, size_t mem_size, string ftype
) {
	if (files.size() == 0) return (NULL);

	if (ftype.compare("vdf") == 0) {
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
	else if (ftype.compare("wrf") == 0) {
		MetadataWRF *md = new MetadataWRF(files);
		if (! md) return (NULL);

		return new DataMgrWRF(*md, mem_size);

	}
	else {
		SetErrMsg("Unknown data set type : %s", ftype.c_str());
		return(NULL);
	}
}
