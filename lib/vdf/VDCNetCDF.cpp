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

VDCNetCDF::VDCNetCDF() : VDC() {
	_ncid = -1;
	_dimidMap.clear();
}

int VDCNetCDF::Initialize(string path, AccessMode mode) {
	_ncid = -1;
	return(VDC::Initialize(path, mode));
}


int VDCNetCDF::_WriteMasterMeta() {
	const size_t NC_CHUNKSIZEHINT = 4*1024*1024;

	_ncid = -1;
	_dimidMap.clear();

    size_t chsz = NC_CHUNKSIZEHINT;
    int rc = nc__create(_master_path.c_str(), NC_64BIT_OFFSET, 0, &chsz, &_ncid);
	MY_NC_ERR(rc, _master_path, "nc__create()");


	map <string, Dimension>::const_iterator itr;
	for (itr = _dimsMap.begin(); itr != _dimsMap.end(); ++itr) {
		const Dimension &dimension = itr->second;
		int dimid;

		rc = nc_def_dim(
			_ncid, dimension.GetName().c_str(), dimension.GetLength(), &dimid
		);
		MY_NC_ERR(rc, _master_path, "nc_def_dim()");
		_dimidMap[dimension.GetName()] = dimid;
	}
	

	rc = _PutAtt(_master_path, "", "VDC.Version", 1);
	if (rc<0) return(rc);

	rc = _PutAtt(_master_path, "", "VDC.BlockSize", _bs, 3);
	if (rc<0) return(rc);

	rc = _PutAtt(_master_path, "", "VDC.WaveName", _wname);
	if (rc<0) return(rc);

	rc = _PutAtt(_master_path, "", "VDC.WaveMode", _wmode);
	if (rc<0) return(rc);

	rc = _PutAtt(_master_path, "", "VDC.CompressionRatios", _cratios);
	if (rc<0) return(rc);

	rc = _PutAtt(_master_path, "", "VDC.MaxTSPerFile", _max_ts_per_file);
	if (rc<0) return(rc);

	int periodic[] = {_periodic[0], _periodic[1], _periodic[2]};
	rc = _PutAtt(_master_path, "", "VDC.Periodic", periodic, 3);
	if (rc<0) return(rc);

	rc = _PutAtt(_master_path, "", "VDC.MaxMFArray", _max_mf_array);
	if (rc<0) return(rc);


	rc = _WriteMasterDimensions();
	if (rc<0) return(rc);

	rc = _WriteMasterAttributes();
	if (rc<0) return(rc);

	rc = _WriteMasterCoordVars();
	if (rc<0) return(rc);

	rc = _WriteMasterDataVars();
	if (rc<0) return(rc);

	rc = nc_enddef(_ncid);
	MY_NC_ERR(rc, _master_path, "nc_enddef()");

	rc = nc_close(_ncid);
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
	int rc = _PutAtt(_master_path, "", tag, s);
	if (rc<0) return(rc);

	
	for (itr = _dimsMap.begin(); itr != _dimsMap.end(); ++itr) {
		const Dimension &dimension = itr->second;

		tag = "VDC.Dimension." + dimension.GetName() + ".Length";
		rc = _PutAtt(_master_path, "", tag, dimension.GetLength());
		if (rc<0) return(rc);

		tag = "VDC.Dimension." + dimension.GetName() + ".Axis";
		rc = _PutAtt(_master_path, "", tag, dimension.GetAxis());
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
	int rc = _PutAtt(_master_path, "", tag, s);
	if (rc<0) return(rc);

	
	for (itr = atts.begin(); itr != atts.end(); ++itr) {
		const Attribute &attr = itr->second;

		tag = prefix + ".Attribute." + attr.GetName() + ".XType";
		rc = _PutAtt(_master_path, "", tag, attr.GetXType());
		if (rc<0) return(rc);

		tag = prefix + ".Attribute." + attr.GetName() + ".Values";
		switch (attr.GetXType()) {
			case FLOAT:
			case DOUBLE: {
				vector <double> values;
				attr.GetValues(values);
				rc = _PutAtt(_master_path, "", tag, values);
				if (rc<0) return(rc);
			break;
			}
			case INT32:
			case INT64: {
				vector <int> values;
				attr.GetValues(values);
				rc = _PutAtt(_master_path, "", tag, values);
				if (rc<0) return(rc);
			break;
			}
			case TEXT: {
				string values;
				attr.GetValues(values);
				rc = _PutAtt(_master_path, "", tag, values);
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

	int rc = _PutAtt(_master_path, "", tag, s);
	if (rc<0) return(rc);

	tag = prefix + "." + var.GetName() + ".Units";
	rc = _PutAtt(_master_path, "", tag, var.GetUnits());
	if (rc<0) return(rc);

	tag = prefix + "." + var.GetName() + ".XType";
	rc = _PutAtt(_master_path, "", tag, (int) var.GetXType());
	if (rc<0) return(rc);

	tag = prefix + "." + var.GetName() + ".Compressed";
	rc = _PutAtt(_master_path, "", tag, (int) var.GetCompressed());
	if (rc<0) return(rc);

	tag = prefix + "." + var.GetName() + ".BlockSize";
	rc = _PutAtt(_master_path, "", tag, var.GetBS(), 3);
	if (rc<0) return(rc);

	tag = prefix + "." + var.GetName() + ".WaveName";
	rc = _PutAtt(_master_path, "", tag, var.GetWName());
	if (rc<0) return(rc);

	tag = prefix + "." + var.GetName() + ".WaveMode";
	rc = _PutAtt(_master_path, "", tag, var.GetWMode());
	if (rc<0) return(rc);

	tag = prefix + "." + var.GetName() + ".CompressionRatios";
	rc = _PutAtt(_master_path, "", tag, var.GetCRatios());
	if (rc<0) return(rc);

	tag = prefix + "." + var.GetName() + ".Periodic";
	const bool *ptr = var.GetPeriodic();
	int iperiodic[] = {ptr[0], ptr[1], ptr[2]};
	rc = _PutAtt(_master_path, "", tag, iperiodic, 3);
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
	int rc = _PutAtt(_master_path, "", tag, s);
	if (rc<0) return(rc);


	string prefix = "VDC.CoordVar";
	for (itr = _coordVars.begin(); itr != _coordVars.end(); ++itr) {
		const CoordVar &cvar = itr->second;

		tag = prefix + "." + cvar.GetName() + ".Axis";
		int rc = _PutAtt(_master_path, "", tag, cvar.GetAxis());
		if (rc<0) return(rc);

		tag = prefix + "." + cvar.GetName() + ".Uniform";
		rc = _PutAtt(_master_path, "", tag, (int) cvar.GetUniform());
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
	int rc = _PutAtt(_master_path, "", tag, s);
	if (rc<0) return(rc);


	string prefix = "VDC.DataVar";
	for (itr = _dataVars.begin(); itr != _dataVars.end(); ++itr) {
		const DataVar &var = itr->second;

		tag = prefix + "." + var.GetName() + ".CoordVars";
		int rc = _PutAtt(_master_path, "", tag, var.GetCoordvars());
		if (rc<0) return(rc);

		tag = prefix + "." + var.GetName() + ".HasMissing";
		rc = _PutAtt(_master_path, "", tag, (int) var.GetHasMissing());
		if (rc<0) return(rc);

		tag = prefix + "." + var.GetName() + ".MissingValue";
		rc = _PutAtt(_master_path, "", tag, (int) var.GetMissingValue());
		if (rc<0) return(rc);

		rc = _WriteMasterVarBase(prefix, var);
		if (rc<0) return(rc);
	}
	return(0);

}

	
//
// PutAtt - Integer
//
int VDCNetCDF::_PutAtt(
	string path, string varname, string attname, int value
) {
	return(_PutAtt(path, varname, attname, &value, 1));
}

int VDCNetCDF::_PutAtt(
	string path, string varname, string attname, vector <int> values
) {
	size_t n = values.size();
	int *buf = new int[n];
	for (size_t i=0; i<n; i++) buf[i] = values[i];

	int rc = _PutAtt(path, varname, attname, buf, n);
	delete [] buf;

	return(rc);
}

int VDCNetCDF::_PutAtt(
	string path, string varname, string attname, const int values[], size_t n
) {
    int varid;
	int ncid;
    int rc = _GetVarID(path, varname, ncid, varid);
    if (rc<0) return(-1);

    rc = nc_put_att_int(ncid,varid,attname.c_str(),NC_INT, n, values);
	MY_NC_ERR(rc, path, "nc_put_att_int");

	return(0);
}

//
// GetAtt - Integer
//
int VDCNetCDF::_GetAtt(
	string path, string varname, string attname, int &value
) const {
	return(_GetAtt(path, varname, attname, &value, 1));
}

int VDCNetCDF::_GetAtt(
	string path, string varname, string attname, vector <int> &values
) const {
	values.clear();

    int varid;
	int ncid;
    int rc = _GetVarID(path, varname, ncid, varid);
    if (rc<0) return(-1);

	size_t n;
	nc_inq_attlen(ncid, varid, attname.c_str(), &n);
	MY_NC_ERR(rc, path, "nc_inq_attlen");

	int *buf = new int[n];

	rc = _GetAtt(path, varname, attname, buf, n);

	for (int i=0; i<n; i++) {
		values.push_back(buf[i]);
	}
	delete [] buf;

	return(rc);
}

int VDCNetCDF::_GetAtt(
	string path, string varname, string attname, int values[], size_t n
) const {
    int varid;
	int ncid;
    int rc = _GetVarID(path, varname, ncid, varid);
    if (rc<0) return(-1);

	size_t len;
	nc_inq_attlen(ncid, varid, attname.c_str(), &len);
	MY_NC_ERR(rc, path, "nc_inq_attlen");

	int *buf = new int[len];

    rc = nc_get_att_int(ncid,varid,attname.c_str(),buf);
	if (rc != NC_NOERR) delete [] buf;
	MY_NC_ERR(rc, path, "nc_get_att_int");

	for (size_t i=0; i < len && i < n; i++) {
		values[i] = buf[i];
	}
	delete [] buf;

	return(0);
}

//
// PutAtt - size_t
//
int VDCNetCDF::_PutAtt(
	string path, string varname, string attname, size_t value
) {
	return(_PutAtt(path, varname, attname, &value, 1));
}

int VDCNetCDF::_PutAtt(
	string path, string varname, string attname, vector <size_t> values
) {
	vector <int> ivalues;
	for (size_t i=0; i<values.size(); i++) ivalues.push_back(values[i]);

	return(_PutAtt(path, varname, attname, ivalues));
}

int VDCNetCDF::_PutAtt(
	string path, string varname, string attname, 
	const size_t values[], size_t n
) {
	vector <int> ivalues;

	for (size_t i=0; i<n; i++) ivalues.push_back(values[i]);

	return(_PutAtt(path, varname, attname, ivalues));
}

//
// GetAtt - size_t
//
int VDCNetCDF::_GetAtt(
	string path, string varname, string attname, size_t &value
) const {
	return(_GetAtt(path, varname, attname, &value, 1));
}

int VDCNetCDF::_GetAtt(
	string path, string varname, string attname, vector <size_t> &values
) const {
	values.clear();

	vector <int> ivalues;
	int rc = _GetAtt(path, varname, attname, ivalues);
	for (int i=0; i<ivalues.size(); i++) values.push_back(ivalues[i]);
	return(rc);

}

int VDCNetCDF::_GetAtt(
	string path, string varname, string attname, size_t values[], size_t n
) const {

	vector <int> ivalues;
	int rc = _GetAtt(path, varname, attname, ivalues);
	for (int i=0; i<ivalues.size() && i<n; i++) values[i] = ivalues[i];

	return(rc);
}

//
// PutAtt - Double
//

int VDCNetCDF::_PutAtt(
	string path, string varname, string attname, double value
) {
	return(_PutAtt(path, varname, attname, &value, 1));
}

int VDCNetCDF::_PutAtt(
	string path, string varname, string attname, vector <double> values
) {
	size_t n = values.size();
	double *buf = new double[n];
	for (size_t i=0; i<n; i++) buf[i] = values[i];

	int rc = _PutAtt(path, varname, attname, buf, n);
	delete [] buf;

	return(rc);
}

int VDCNetCDF::_PutAtt(
	string path, string varname, string attname, const double values[], size_t n
) {
    int varid;
	int ncid;
    int rc = _GetVarID(path, varname, ncid, varid);
    if (rc<0) return(-1);

    rc = nc_put_att_double(ncid,varid,attname.c_str(),NC_DOUBLE, n, values);
	MY_NC_ERR(rc, path, "nc_put_att_double");

	return(0);
}

//
// GetAtt - Double
//
int VDCNetCDF::_GetAtt(
	string path, string varname, string attname, double &value
) const {
	return(_GetAtt(path, varname, attname, &value, 1));
}

int VDCNetCDF::_GetAtt(
	string path, string varname, string attname, vector <double> &values
) const {
	values.clear();

    int varid;
	int ncid;
    int rc = _GetVarID(path, varname, ncid, varid);
    if (rc<0) return(-1);

	size_t n;
	nc_inq_attlen(ncid, varid, attname.c_str(), &n);
	MY_NC_ERR(rc, path, "nc_inq_attlen");

	double *buf = new double[n];

	rc = _GetAtt(path, varname, attname, buf, n);

	for (int i=0; i<n; i++) {
		values.push_back(buf[i]);
	}
	delete [] buf;

	return(rc);
}

int VDCNetCDF::_GetAtt(
	string path, string varname, string attname, double values[], size_t n
) const {
    int varid;
	int ncid;
    int rc = _GetVarID(path, varname, ncid, varid);
    if (rc<0) return(-1);

	size_t len;
	nc_inq_attlen(ncid, varid, attname.c_str(), &len);
	MY_NC_ERR(rc, path, "nc_inq_attlen");

	double *buf = new double[len];

    rc = nc_get_att_double(ncid,varid,attname.c_str(),buf);
	if (rc != NC_NOERR) delete [] buf;
	MY_NC_ERR(rc, path, "nc_get_att_double");

	for (size_t i=0; i < len && i < n; i++) {
		values[i] = buf[i];
	}
	delete [] buf;

	return(0);
}

//
// PutAtt - String
//

int VDCNetCDF::_PutAtt(
	string path, string varname, string attname, string value
) {

	size_t n = value.length();
	char *buf = new char[n + 1];
	strcpy(buf, value.c_str());
	int rc = _PutAtt(path, varname, attname, buf, n);
	delete [] buf;
	return(rc);
}
int VDCNetCDF::_PutAtt(
	string path, string varname, string attname, vector <string> values
) {
	string s;
	for (int i=0; i<values.size(); i++) {
		s += values[i];
		s += " ";
	}
	return(_PutAtt(path, varname, attname, s));
}

int VDCNetCDF::_PutAtt(
	string path, string varname, string attname, const char values[], size_t n
) {
    int varid;
	int ncid;
    int rc = _GetVarID(path, varname, ncid, varid);
    if (rc<0) return(-1);

    rc = nc_put_att_text(ncid,varid,attname.c_str(), n, values);
	MY_NC_ERR(rc, path, "nc_put_att_text");

	return(0);
}

//
// GetAtt - String
//
int VDCNetCDF::_GetAtt(
	string path, string varname, string attname, string &value
) const {
	value.clear();

    int varid;
	int ncid;
    int rc = _GetVarID(path, varname, ncid, varid);
    if (rc<0) return(-1);

	size_t n;
	nc_inq_attlen(ncid, varid, attname.c_str(), &n);
	MY_NC_ERR(rc, path, "nc_inq_attlen");

	char *buf = new char[n+1];

	rc = _GetAtt(path, varname, attname, buf, n);
	value = buf;

	delete [] buf;

	return(rc);
}

int VDCNetCDF::_GetAtt(
	string path, string varname, string attname, char values[], size_t n
) const {
    int varid;
	int ncid;
    int rc = _GetVarID(path, varname, ncid, varid);
    if (rc<0) return(-1);

	size_t len;
	nc_inq_attlen(ncid, varid, attname.c_str(), &len);
	MY_NC_ERR(rc, path, "nc_inq_attlen");

	char *buf = new char[len+1];

    rc = nc_get_att_text(ncid,varid,attname.c_str(),buf);
	if (rc != NC_NOERR) delete [] buf;
	MY_NC_ERR(rc, path, "nc_get_att_text");

	size_t i;
	for (i=0; i < len && i < n; i++) {
		values[i] = buf[i];
	}
	values[i] = '\0';
	delete [] buf;

	return(0);
}

int VDCNetCDF::_GetVarID(
	string path, string varname, int &ncid, int &varid 
) const {
	if (path.compare(_master_path) != 0) {
		SetErrMsg("Not implemented!!!");
		return(-1);
	}
	if (! varname.empty()) {
		SetErrMsg("Not implemented!!!");
		return(-1);
	}
	ncid = _ncid;
	varid = NC_GLOBAL;

	return(0);
}
