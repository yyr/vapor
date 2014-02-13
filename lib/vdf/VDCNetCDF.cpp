#include <cassert>
#include <sstream>
#include <map>
#include <vector>
#include <netcdf.h>
#include "vapor/VDCNetCDF.h"

using namespace VAPoR;

#define MY_NC_ERR(rc, path, func) \
    if (rc != NC_NOERR) { \
        SetErrMsg( \
            "Error accessing netCDF file \"%s\", %s : %s",  \
            path.c_str(), func, nc_strerror(rc) \
        ); \
        return(-1); \
    }

VDCNetCDF::VDCNetCDF(
	size_t master_threshold, size_t variable_threshold
) : VDC() {

	_dimidMap.clear();
	_master_threshold = master_threshold;
	_variable_threshold = variable_threshold;
}

int VDCNetCDF::Initialize(string path, AccessMode mode) {
	return(VDC::Initialize(path, mode));
}

//
// Figure out where variable lives. This algorithm will most likely
// change.
//
int VDCNetCDF::GetPath(
	string varname, size_t ts, int lod, string &path, size_t &file_ts
) const {
	path.clear();
	file_ts = 0;
	if (! VDC::IsTimeVarying(varname)) {
		ts = 0;	// Could be anything if data aren't time varying;
	}

	// Really??
	//
	if (_defineMode) {
		SetErrMsg("Operation not permitted in define mode");
		return(-1);
	}
	const VarBase *varbase;
	VDC::DataVar dvar;
	VDC::CoordVar cvar;

	if (VDC::IsDataVar(varname)) {
		(void) VDC::GetDataVar(varname, dvar);
		varbase = &dvar;
	}
	else if (VDC::IsCoordVar(varname)) {
		(void) VDC::GetCoordVar(varname, cvar);
		varbase = &cvar;
	}
	else {
		SetErrMsg("Undefined variable name : %s", varname.c_str());
		return(-1);
	}

	size_t nelements = 1;
	size_t ngridpoints = 1;
	for (int i=0; i<varbase->GetDimensions().size(); i++) {
		nelements *= varbase->GetDimensions()[i].GetLength(); 
		if (varbase->GetDimensions()[i].GetAxis() != 3) {
			ngridpoints *= varbase->GetDimensions()[i].GetLength();
		}
	}

	if (nelements < _master_threshold &&  ! varbase->GetCompressed()) {
		path = _master_path;
		file_ts = ts;
		return(0);
	}

	path = _master_path;
	path.erase(path.rfind(".nc"));
	path += "_data";

	path += "/";

	if (dynamic_cast<const VDC::DataVar *>(varbase)) {
		path += "data";
		path += "/";
	}
	else {
		path += "coordinates";
		path += "/";
	}

	path += varname;
	path += ".";

	if (VDC::IsTimeVarying(varname)) { 
		int idx;
		ostringstream oss;
		size_t numts = VDC::GetNumTimeSteps(varname);
		assert(numts>0);
		size_t max_ts_per_file = _variable_threshold / ngridpoints;
		if (max_ts_per_file == 0) {
			idx = ts;
			file_ts = ts;
		}
		else {
		 	idx = ts / max_ts_per_file;;
			file_ts = ts % max_ts_per_file;
		}
		int width = (int) log10((double) numts-1) + 1;
		if (width < 4) width = 4;
		oss.width(width); oss.fill('0'); oss << idx;	

		path += oss.str();
	}

	path += ".";
	path += "nc";
	if (varbase->GetCompressed()) {
		if (lod<0) lod = varbase->GetCRatios().size()-1;
		ostringstream oss;
		oss << lod;
		path += oss.str();
	}
	
	return(0);
}


int VDCNetCDF::_WriteMasterMeta() {
	const size_t NC_CHUNKSIZEHINT = 4*1024*1024;

	_dimidMap.clear();

	int dummy;
    size_t chsz = NC_CHUNKSIZEHINT;
    int rc = _netcdf.Create(_master_path, NC_64BIT_OFFSET, 0, chsz, dummy);
	if (rc<0) return(-1);


	map <string, Dimension>::const_iterator itr;
	for (itr = _dimsMap.begin(); itr != _dimsMap.end(); ++itr) {
		const Dimension &dimension = itr->second;
		int dimid;

		rc = _netcdf.DefDim(
			dimension.GetName(), dimension.GetLength(), dimid
		);
		if (rc<0) return(-1);
		_dimidMap[dimension.GetName()] = dimid;
	}
	

	rc = _netcdf.PutAtt("", "VDC.Version", 1);
	if (rc<0) return(rc);

	rc = _netcdf.PutAtt("", "VDC.BlockSize", _bs);
	if (rc<0) return(rc);

	rc = _netcdf.PutAtt("", "VDC.WaveName", _wname);
	if (rc<0) return(rc);

	rc = _netcdf.PutAtt("", "VDC.WaveMode", _wmode);
	if (rc<0) return(rc);

	rc = _netcdf.PutAtt("", "VDC.CompressionRatios", _cratios);
	if (rc<0) return(rc);

	rc = _netcdf.PutAtt("", "VDC.MasterThreshold", _master_threshold);
	if (rc<0) return(rc);

	rc = _netcdf.PutAtt("", "VDC.VariableThreshold",_variable_threshold);
	if (rc<0) return(rc);

	vector <int> periodic;
	for (int i=0; i<_periodic.size(); i++) {
		periodic.push_back((int) _periodic[i]);
	}
	rc = _netcdf.PutAtt("", "VDC.Periodic", periodic);
	if (rc<0) return(rc);

	if (rc<0) return(rc);


	rc = _WriteMasterDimensions();
	if (rc<0) return(rc);

	rc = _WriteMasterAttributes();
	if (rc<0) return(rc);

	rc = _WriteMasterCoordVars();
	if (rc<0) return(rc);

	rc = _WriteMasterDataVars();
	if (rc<0) return(rc);

	rc = _netcdf.Enddef();
	MY_NC_ERR(rc, _master_path, "nc_enddef()");

	rc = _netcdf.Close();
	MY_NC_ERR(rc, _master_path, "nc_close()");

	return(0);
}

int VDCNetCDF::_ReadMasterMeta() {
	return(-1);
}


int VDCNetCDF::_WriteMasterDimensions() {
	map <string, Dimension>::const_iterator itr;
	string s;
	for (itr = _dimsMap.begin(); itr != _dimsMap.end(); ++itr) {
		s += itr->first;
		s+= " ";
	}
	string tag = "VDC.DimensionNames";
	int rc = _netcdf.PutAtt("", tag, s);
	if (rc<0) return(rc);

	
	for (itr = _dimsMap.begin(); itr != _dimsMap.end(); ++itr) {
		const Dimension &dimension = itr->second;

		tag = "VDC.Dimension." + dimension.GetName() + ".Length";
		rc = _netcdf.PutAtt("", tag, dimension.GetLength());
		if (rc<0) return(rc);

		tag = "VDC.Dimension." + dimension.GetName() + ".Axis";
		rc = _netcdf.PutAtt("", tag, dimension.GetAxis());
		if (rc<0) return(rc);
		
	}
	return(0);
}

int VDCNetCDF::_WriteMasterAttributes (
	string prefix, const map <string, Attribute> atts
) {

	map <string, Attribute>::const_iterator itr;
	string s;
	for (itr = atts.begin(); itr != atts.end(); ++itr) {
		s += itr->first;
		s+= " ";
	}
	string tag = prefix + ".AttributeNames";
	int rc = _netcdf.PutAtt("", tag, s);
	if (rc<0) return(rc);

	
	for (itr = atts.begin(); itr != atts.end(); ++itr) {
		const Attribute &attr = itr->second;

		tag = prefix + ".Attribute." + attr.GetName() + ".XType";
		rc = _netcdf.PutAtt("", tag, attr.GetXType());
		if (rc<0) return(rc);

		tag = prefix + ".Attribute." + attr.GetName() + ".Values";
		switch (attr.GetXType()) {
			case FLOAT:
			case DOUBLE: {
				vector <double> values;
				attr.GetValues(values);
				rc = _netcdf.PutAtt("", tag, values);
				if (rc<0) return(rc);
			break;
			}
			case INT32:
			case INT64: {
				vector <int> values;
				attr.GetValues(values);
				rc = _netcdf.PutAtt("", tag, values);
				if (rc<0) return(rc);
			break;
			}
			case TEXT: {
				string values;
				attr.GetValues(values);
				rc = _netcdf.PutAtt("", tag, values);
				if (rc<0) return(rc);
			break;
			}
			default:
				SetErrMsg(
					"Invalid value, Attribute::GetXType() : %d", 
					attr.GetXType()
				);
				return(-1);
			break;
		}
	}
	return(0);
}

int VDCNetCDF::_WriteMasterAttributes () {

	string prefix = "VDC";
	return (_WriteMasterAttributes(prefix, _atts));
}

int VDCNetCDF::_WriteMasterVarBase(string prefix, const VarBase &var) {
	
	string tag;

	tag = prefix + "." + var.GetName() + ".DimensionNames";
	const vector <Dimension> &dimensions = var.GetDimensions();
	string s;
	for (int i=0; i<dimensions.size(); i++) {
		s += dimensions[i].GetName();
		s+= " ";
	}

	int rc = _netcdf.PutAtt("", tag, s);
	if (rc<0) return(rc);

	tag = prefix + "." + var.GetName() + ".Units";
	rc = _netcdf.PutAtt("", tag, var.GetUnits());
	if (rc<0) return(rc);

	tag = prefix + "." + var.GetName() + ".XType";
	rc = _netcdf.PutAtt("", tag, (int) var.GetXType());
	if (rc<0) return(rc);

	tag = prefix + "." + var.GetName() + ".Compressed";
	rc = _netcdf.PutAtt("", tag, (int) var.GetCompressed());
	if (rc<0) return(rc);

	tag = prefix + "." + var.GetName() + ".BlockSize";
	rc = _netcdf.PutAtt("", tag, var.GetBS());
	if (rc<0) return(rc);

	tag = prefix + "." + var.GetName() + ".WaveName";
	rc = _netcdf.PutAtt("", tag, var.GetWName());
	if (rc<0) return(rc);

	tag = prefix + "." + var.GetName() + ".WaveMode";
	rc = _netcdf.PutAtt("", tag, var.GetWMode());
	if (rc<0) return(rc);

	tag = prefix + "." + var.GetName() + ".CompressionRatios";
	rc = _netcdf.PutAtt("", tag, var.GetCRatios());
	if (rc<0) return(rc);

	tag = prefix + "." + var.GetName() + ".Periodic";
	vector <bool> periodic = var.GetPeriodic();
	vector <int> iperiodic;
	for (int i=0; i<periodic.size(); i++) iperiodic.push_back(periodic[i]);
	rc = _netcdf.PutAtt("", tag, iperiodic);
	if (rc<0) return(rc);
	
	prefix += "." + var.GetName();
	return (_WriteMasterAttributes(prefix, var.GetAttributes()));
		
}

int VDCNetCDF::_WriteMasterCoordVars() {
	map <string, CoordVar>::const_iterator itr;
	string s;
	for (itr = _coordVars.begin(); itr != _coordVars.end(); ++itr) {
		s += itr->first;
		s+= " ";
	}

	string tag = "VDC.CoordVarNames";
	int rc = _netcdf.PutAtt("", tag, s);
	if (rc<0) return(rc);


	string prefix = "VDC.CoordVar";
	for (itr = _coordVars.begin(); itr != _coordVars.end(); ++itr) {
		const CoordVar &cvar = itr->second;

		tag = prefix + "." + cvar.GetName() + ".Axis";
		int rc = _netcdf.PutAtt("", tag, cvar.GetAxis());
		if (rc<0) return(rc);

		tag = prefix + "." + cvar.GetName() + ".Uniform";
		rc = _netcdf.PutAtt("", tag, (int) cvar.GetUniform());
		if (rc<0) return(rc);

		rc = _WriteMasterVarBase(prefix, cvar);
		if (rc<0) return(rc);
	}
	return(0);

}

int VDCNetCDF::_WriteMasterDataVars() {
	map <string, DataVar>::const_iterator itr;
	string s;
	for (itr = _dataVars.begin(); itr != _dataVars.end(); ++itr) {
		s += itr->first;
		s+= " ";
	}

	string tag = "VDC.DataVarNames";
	int rc = _netcdf.PutAtt("", tag, s);
	if (rc<0) return(rc);


	string prefix = "VDC.DataVar";
	for (itr = _dataVars.begin(); itr != _dataVars.end(); ++itr) {
		const DataVar &var = itr->second;

		tag = prefix + "." + var.GetName() + ".CoordVars";
		int rc = _netcdf.PutAtt("", tag, var.GetCoordvars());
		if (rc<0) return(rc);

		tag = prefix + "." + var.GetName() + ".HasMissing";
		rc = _netcdf.PutAtt("", tag, (int) var.GetHasMissing());
		if (rc<0) return(rc);

		tag = prefix + "." + var.GetName() + ".MissingValue";
		rc = _netcdf.PutAtt("", tag, (int) var.GetMissingValue());
		if (rc<0) return(rc);

		rc = _WriteMasterVarBase(prefix, var);
		if (rc<0) return(rc);
	}
	return(0);

}
