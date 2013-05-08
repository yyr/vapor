

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
	size_t ts, string varname, float &value
) const {

	if (WaveCodecIO::GetVMissingValue(ts, varname).size() == 1) {
		 value = GetVMissingValue(ts,varname)[0];
		return(true);
	}
	if (GetTSMissingValue(ts).size() == 1) {
		 value = GetTSMissingValue(ts)[0];
		return(true);
	}
	if (GetMissingValue().size() == 1) {
		 value = GetMissingValue()[0];
		return(true);
	}
	return(false);

}
