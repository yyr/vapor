#include <vector>
#include <vapor/VDCNetCDF.h>
#include <vapor/MyBase.h>
#include <vapor/DataMgrV3_0.h>


//! \class DataMgrVDC
//! \brief A cache based data reader for VDC data
//! \author John Clyne
//!


namespace VAPoR {

class DataMgrVDC : public VAPoR::DataMgrV3_0 {
public:

 DataMgrVDC(size_t mem_size, int numthreads = 0);

 virtual ~DataMgrVDC();

protected: 

 virtual int _Initialize(const vector <string> &files);

 virtual std::vector <string> _GetDataVarNames() const {
	return(_vdc.GetDataVarNames());
 }
 
 virtual std::vector <string> _GetCoordVarNames() const {
	return(_vdc.GetCoordVarNames());
 }

 bool _GetBaseVarInfo(string varname, VAPoR::VDC::BaseVar &var) {
	return(_vdc.GetBaseVarInfo(varname, var));
 }

 bool _GetDataVarInfo(string varname, VAPoR::VDC::DataVar &var) {
	return(_vdc.GetDataVarInfo(varname, var));
 }

 bool _GetCoordVarInfo(string varname, VAPoR::VDC::CoordVar &var) {
	return(_vdc.GetCoordVarInfo(varname, var));
 }

 int _GetDimLensAtLevel(
    string varname, int level, vector <size_t> &dims_at_level,
    vector <size_t> &bs_at_level
 ) const {
	return(
		_vdc.GetDimLensAtLevel(varname, level, dims_at_level, bs_at_level)
	);
 }

 int _GetNumRefLevels(string varname) const {
	return(_vdc.GetNumRefLevels(varname));
 }

 virtual bool _VariableExists(
	size_t ts, string varname,
	int reflevel = 0,
	int lod = 0
 ) {
	return(_vdc.VariableExists(ts, varname, reflevel, lod));
 }

 int _ReadVariableBlock (
	size_t ts, string varname, int reflevel, int lod, 
	vector <size_t> bmin, vector <size_t> bmax, float *blocks
 );
 

private:

 VDCNetCDF _vdc;

};

};
