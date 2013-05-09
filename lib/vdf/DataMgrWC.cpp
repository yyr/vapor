

#include <vapor/DataMgrWC.h>

using namespace VetsUtil;
using namespace VAPoR;

 VAPoR::DataMgrWC::DataMgrWC(
    const string &metafile,
    size_t mem_size
 ) : DataMgr(mem_size), WaveCodecIO(metafile) 
{ }

 VAPoR::DataMgrWC::DataMgrWC(
    const MetadataVDC &metadata,
    size_t mem_size
 ) : DataMgr(mem_size), WaveCodecIO(metadata) 
{ }

bool VAPoR::DataMgrWC::_GetMissingValue(
	string varname, float &value
) const {

	//
	// DataMgr doesn't support missing values that vary over time
	//
	if (WaveCodecIO::GetVMissingValue(0, varname).size() == 1) {
		 value = GetVMissingValue(0,varname)[0];
		return(true);
	}
	if (WaveCodecIO::GetTSMissingValue(0).size() == 1) {
		 value = GetTSMissingValue(0)[0];
		return(true);
	}
	if (WaveCodecIO::GetMissingValue().size() == 1) {
		 value = WaveCodecIO::GetMissingValue()[0];
		return(true);
	}
	return(false);

}
