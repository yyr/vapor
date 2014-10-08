#include <vector>
#include <vapor/VDCNetCDF.h>
#include <vapor/MyBase.h>
#include <vapor/DataMgrV3_0.h>

#ifndef DataMgrVDC_h
#define DataMgrVDC_h

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

 virtual bool _GetBaseVarInfo(string varname, VAPoR::VDC::BaseVar &var) const {
	return(_vdc.GetBaseVarInfo(varname, var));
 }

 virtual bool _GetDataVarInfo(string varname, VAPoR::VDC::DataVar &var) const {
	return(_vdc.GetDataVarInfo(varname, var));
 }

 virtual bool _GetCoordVarInfo(string varname, VAPoR::VDC::CoordVar &var) const {
	return(_vdc.GetCoordVarInfo(varname, var));
 }

 virtual int _GetDimLensAtLevel(
    string varname, int level, vector <size_t> &dims_at_level,
    vector <size_t> &bs_at_level
 ) const {
	return(
		_vdc.GetDimLensAtLevel(varname, level, dims_at_level, bs_at_level)
	);
 }

 virtual int _GetNumRefLevels(string varname) const {
	return(_vdc.GetNumRefLevels(varname));
 }

 virtual bool _VariableExists(
	size_t ts, string varname,
	int reflevel = 0,
	int lod = 0
 ) const {
	return(_vdc.VariableExists(ts, varname, reflevel, lod));
 }

 virtual int _ReadVariableBlock (
	size_t ts, string varname, int reflevel, int lod, 
	vector <size_t> bmin, vector <size_t> bmax, float *blocks
 );

 virtual int _ReadVariable(
    string varname, int level, int lod, float *data
 );
 

private:

 VDCNetCDF _vdc;

};

};
#endif
