#include <cstdio>
#include <cstring>
#include <cassert>
#include <vector>
#include <vapor/DataMgrFactory.h>
#include <vapor/DataMgrWB.h>
#include <vapor/DataMgrWC.h>
#include <vapor/DataMgrAMR.h>
#include <vapor/DataMgrWRF.h>
#include <vapor/DataMgrROMS.h>
#include <vapor/DataMgrMOM.h>
#include <vapor/DataMgrGRIB.h>

using namespace VAPoR;

DataMgr *DataMgrFactory::New(
	const vector <string> &files, size_t mem_size, string ftype
) {
	if (files.size() == 0) return (NULL);

	if (ftype.compare("vdf") == 0) {
		MetadataVDC *md = new MetadataVDC(files[0]);
		if (MetadataVDC::GetErrCode() != 0) return (NULL);

		if (md->GetGridType().compare("block_amr") == 0) {
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
		return new DataMgrWRF(files, mem_size);
	}
	else if (ftype.compare("roms") == 0) {
		return new DataMgrROMS(files, mem_size);
	}
	else if (ftype.compare("mom4") == 0) {
		return new DataMgrMOM(files, mem_size);
	}
	else if (ftype.compare("grib") == 0) {
		return new DataMgrGRIB(files, mem_size);
	}
	else if (ftype.compare("cam") == 0) {
		return new DataMgrROMS(files, mem_size);	// CAM uses ROMS reader
	}
	else {
		SetErrMsg("Unknown data set type : %s", ftype.c_str());
		return(NULL);
	}
}
