#include <cassert>
#include <sstream>
#include <sstream>
#include <iterator>
#include "vapor/NetCDFCpp.h"
#include "vapor/MatWaveBase.h"

using namespace VAPoR;

#define MY_NC_ERR(rc, path, func) \
	if (rc != NC_NOERR) { \
		SetErrMsg( \
			"Error accessing netCDF file \"%s\", %s : %s -- file (%s), line(%d)",  \
			path.c_str(), func, nc_strerror(rc), __FILE__, __LINE__ \
		); \
		return(rc); \
	} 




NetCDFCpp::NetCDFCpp() {
	_ncid = -1;
	_path.clear();
}

NetCDFCpp::~NetCDFCpp() {
}


int NetCDFCpp::Create(
	string path, int cmode, size_t initialsz, 
	size_t &bufrsizehintp
) {
	NetCDFCpp::Close();

	_ncid = -1;
	_path.clear();

	int ncid;
    int rc = nc__create(path.c_str(), cmode, initialsz, &bufrsizehintp, &ncid);
	MY_NC_ERR(rc, path, "nc__create()");

	_path = path;
	_ncid = ncid;

	return(NC_NOERR);
}


int NetCDFCpp::Open(
	string path, int mode
) {
	NetCDFCpp::Close();

	_ncid = -1;
	_path.clear();

	int ncid;
    int rc = nc_open(path.c_str(), mode, &ncid);
	MY_NC_ERR(rc, path, "nc_open()");

	_path = path;
	_ncid = ncid;

	return(NC_NOERR);
}

int NetCDFCpp::SetFill(int fillmode, int &old_modep) {

	int rc = nc_set_fill(_ncid, fillmode, &old_modep);
	MY_NC_ERR(rc, _path, "nc_set_fill()");
	return(NC_NOERR);
}

int NetCDFCpp::EndDef() const {
	int rc = nc_enddef(_ncid);
	MY_NC_ERR(rc, _path, "nc_enddif()");
	return(NC_NOERR);
}

int NetCDFCpp::Close() {
	if (_ncid < 0) return(NC_NOERR);

	int rc = nc_close(_ncid);
	MY_NC_ERR(rc, _path, "nc_close()");

	_ncid = -1;
	_path.clear();

	return(NC_NOERR);
}

int NetCDFCpp::DefDim(string name, size_t len) const {

	int dimid;
	int rc = nc_def_dim(_ncid, name.c_str(), len, &dimid);
	MY_NC_ERR(rc, _path, "nc_def_dim()");

	return(NC_NOERR);
}

int NetCDFCpp::DefVar(
    string name, nc_type xtype, vector <string> dimnames
) {
	int dimids[NC_MAX_DIMS];

	for (int i=0; i<dimnames.size(); i++) {
		int rc = nc_inq_dimid(_ncid, dimnames[i].c_str(), &dimids[i]);
		MY_NC_ERR(rc, _path, "nc_inq_dim()");
	}
		

	int varid;
    int rc = nc_def_var(
		_ncid, name.c_str(), xtype, dimnames.size(), dimids, &varid
	);
	MY_NC_ERR(rc, _path, "nc_def_var()");
	return(NC_NOERR);
}


int NetCDFCpp::InqVarDims(
    string name, vector <string> &dimnames, vector <size_t> &dims
) const {
	dimnames.clear();
	dims.clear();

	int varid;
	int rc = NetCDFCpp::InqVarid(name, varid);
	if (rc<0) return(rc);

	int ndims;
	rc = nc_inq_varndims(_ncid, varid, &ndims);
	if (rc<0) return(rc);

	int dimids[NC_MAX_VAR_DIMS];
	rc = nc_inq_vardimid(_ncid, varid, dimids);
	if (rc<0) return(rc);

	for (int i=0; i<ndims; i++) {
		char dimnamebuf[NC_MAX_NAME+1];
		size_t dimlen;
		rc = nc_inq_dim(_ncid, dimids[i], dimnamebuf, &dimlen);
		if (rc<0) return(rc);
		dimnames.push_back(dimnamebuf);
		dims.push_back(dimlen);
	}
	return(NC_NOERR);
}

int NetCDFCpp::InqDimlen(string name, size_t &len) const {

    len = 0;

    int dimid;
    int rc = nc_inq_dimid(_ncid, name.c_str(), &dimid);
    MY_NC_ERR(rc, _path, "nc_inq_dimid()");


    rc = nc_inq_dimlen(_ncid, dimid, &len);
    MY_NC_ERR(rc, _path, "nc_inq_varid()");
    return(0);
}


	
//
// PutAtt - Integer
//
int NetCDFCpp::PutAtt(
	string varname, string attname, int value
) const {
	return(NetCDFCpp::PutAtt(varname, attname, &value, 1));
}

int NetCDFCpp::PutAtt(
	string varname, string attname, vector <int> values
) const {
	size_t n = values.size();
	int *buf = new int[n];
	for (size_t i=0; i<n; i++) buf[i] = values[i];

	int rc = NetCDFCpp::PutAtt(varname, attname, buf, n);
	delete [] buf;

	return(rc);
}

int NetCDFCpp::PutAtt(
	string varname, string attname, const int values[], size_t n
) const {

	int varid;
	int rc = NetCDFCpp::InqVarid(varname, varid);
	if (rc<0) return(rc);

	rc = nc_put_att_int(_ncid,varid,attname.c_str(),NC_INT, n, values);
	MY_NC_ERR(rc, _path, "nc_put_att_int()");

	return(NC_NOERR);
}

//
// GetAtt - Integer
//
int NetCDFCpp::GetAtt(
	string varname, string attname, int &value
) const {
	return(NetCDFCpp::GetAtt(varname, attname, &value, 1));
}

int NetCDFCpp::GetAtt(
	string varname, string attname, vector <int> &values
) const {
	values.clear();

    int varid;
    int rc = InqVarid(varname, varid);
    if (rc<0) return(rc);

	size_t n;
	rc = nc_inq_attlen(_ncid, varid, attname.c_str(), &n);
	MY_NC_ERR(rc, _path, "nc_inq_attlen()");

	int *buf = new int[n];

	rc = NetCDFCpp::GetAtt(varname, attname, buf, n);

	for (int i=0; i<n; i++) {
		values.push_back(buf[i]);
	}
	delete [] buf;

	return(rc);
}

int NetCDFCpp::GetAtt(
	string varname, string attname, int values[], size_t n
) const {
    int varid;
    int rc = InqVarid(varname, varid);
    if (rc<0) return(rc);

	size_t len;
	rc = nc_inq_attlen(_ncid, varid, attname.c_str(), &len);
	MY_NC_ERR(rc, _path, "nc_inq_attlen()");

	int *buf = new int[len];

    rc = nc_get_att_int(_ncid,varid,attname.c_str(),buf);
	if (rc != NC_NOERR) delete [] buf;
	MY_NC_ERR(rc, _path, "nc_get_att_int()");

	for (size_t i=0; i < len && i < n; i++) {
		values[i] = buf[i];
	}
	delete [] buf;

	return(NC_NOERR);
}

//
// PutAtt - size_t
//
int NetCDFCpp::PutAtt(
	string varname, string attname, size_t value
) const {
	return(NetCDFCpp::PutAtt(varname, attname, &value, 1));
}

int NetCDFCpp::PutAtt(
	string varname, string attname, vector <size_t> values
) const {
	vector <int> ivalues;
	for (size_t i=0; i<values.size(); i++) ivalues.push_back(values[i]);

	return(NetCDFCpp::PutAtt(varname, attname, ivalues));
}

int NetCDFCpp::PutAtt(
	string varname, string attname, 
	const size_t values[], size_t n
) const {
	vector <int> ivalues;

	for (size_t i=0; i<n; i++) ivalues.push_back(values[i]);

	return(NetCDFCpp::PutAtt(varname, attname, ivalues));
}

//
// GetAtt - size_t
//
int NetCDFCpp::GetAtt(
	string varname, string attname, size_t &value
) const {
	return(NetCDFCpp::GetAtt(varname, attname, &value, 1));
}

int NetCDFCpp::GetAtt(
	string varname, string attname, vector <size_t> &values
) const {
	values.clear();

	vector <int> ivalues;
	int rc = NetCDFCpp::GetAtt(varname, attname, ivalues);
	for (int i=0; i<ivalues.size(); i++) values.push_back(ivalues[i]);
	return(rc);

}

int NetCDFCpp::GetAtt(
	string varname, string attname, size_t values[], size_t n
) const {

	vector <int> ivalues;
	int rc = NetCDFCpp::GetAtt(varname, attname, ivalues);
	for (int i=0; i<ivalues.size() && i<n; i++) values[i] = ivalues[i];

	return(rc);
}

//
// PutAtt - Double
//

int NetCDFCpp::PutAtt(
	string varname, string attname, double value
) const {
	return(NetCDFCpp::PutAtt(varname, attname, &value, 1));
}

int NetCDFCpp::PutAtt(
	string varname, string attname, vector <double> values
) const {
	size_t n = values.size();
	double *buf = new double[n];
	for (size_t i=0; i<n; i++) buf[i] = values[i];

	int rc = NetCDFCpp::PutAtt(varname, attname, buf, n);
	delete [] buf;

	return(rc);
}

int NetCDFCpp::PutAtt(
	string varname, string attname, const double values[], size_t n
) const {

	int varid;
	int rc = InqVarid(varname, varid);
	if (rc<0) return(rc);

	rc = nc_put_att_double(
		_ncid ,varid,attname.c_str(),NC_DOUBLE, n, values
	);
	MY_NC_ERR(rc, _path, "nc_put_att_double()");

	return(NC_NOERR);
}

//
// GetAtt - Double
//
int NetCDFCpp::GetAtt(
	string varname, string attname, double &value
) const {
	return(NetCDFCpp::GetAtt(varname, attname, &value, 1));
}

int NetCDFCpp::GetAtt(
	string varname, string attname, vector <double> &values
) const {
	values.clear();

    int varid;
    int rc = InqVarid(varname, varid);
    if (rc<0) return(rc);

	size_t n;
	rc = nc_inq_attlen(_ncid, varid, attname.c_str(), &n);
	MY_NC_ERR(rc, _path, "nc_inq_attlen()");

	double *buf = new double[n];

	rc = NetCDFCpp::GetAtt(varname, attname, buf, n);

	for (int i=0; i<n; i++) {
		values.push_back(buf[i]);
	}
	delete [] buf;

	return(rc);
}

int NetCDFCpp::GetAtt(
	string varname, string attname, double values[], size_t n
) const {
    int varid;
    int rc = InqVarid(varname, varid);
    if (rc<0) return(rc);

	size_t len;
	rc = nc_inq_attlen(_ncid, varid, attname.c_str(), &len);
	MY_NC_ERR(rc, _path, "nc_inq_attlen()");

	double *buf = new double[len];

    rc = nc_get_att_double(_ncid,varid,attname.c_str(),buf);
	if (rc != NC_NOERR) delete [] buf;
	MY_NC_ERR(rc, _path, "nc_get_att_double()");

	for (size_t i=0; i < len && i < n; i++) {
		values[i] = buf[i];
	}
	delete [] buf;

	return(NC_NOERR);
}

//
// PutAtt - String
//

int NetCDFCpp::PutAtt(
	string varname, string attname, string value
) const {

	size_t n = value.length();
	char *buf = new char[n + 1];
	strcpy(buf, value.c_str());
	int rc = NetCDFCpp::PutAtt(varname, attname, buf, n);
	delete [] buf;
	return(rc);
}
int NetCDFCpp::PutAtt(
	string varname, string attname, vector <string> values
) const {
	string s;
	for (int i=0; i<values.size(); i++) {
		s += values[i];
		s += " ";
	}
	return(NetCDFCpp::PutAtt(varname, attname, s));
}

int NetCDFCpp::PutAtt(
	string varname, string attname, const char values[], size_t n
) const {

	int varid;
	int rc = NetCDFCpp::InqVarid(varname, varid);
	if (rc<0) return(rc);

	rc = nc_put_att_text(_ncid,varid,attname.c_str(), n, values);
	MY_NC_ERR(rc, _path, "nc_put_att_text()");

	return(NC_NOERR);
}

//
// GetAtt - String
//
int NetCDFCpp::GetAtt(
	string varname, string attname, string &value
) const {
	value.clear();

    int varid;
    int rc = InqVarid(varname, varid);
    if (rc<0) return(rc);

	size_t n;
	rc = nc_inq_attlen(_ncid, varid, attname.c_str(), &n);
	MY_NC_ERR(rc, _path, "nc_inq_attlen()");

	char *buf = new char[n+1];

	rc = NetCDFCpp::GetAtt(varname, attname, buf, n);
	value = buf;

	delete [] buf;

	return(rc);
}

int NetCDFCpp::GetAtt(
	string varname, string attname, char values[], size_t n
) const {
    int varid;
    int rc = InqVarid(varname, varid);
    if (rc<0) return(rc);

	size_t len;
	rc = nc_inq_attlen(_ncid, varid, attname.c_str(), &len);
	MY_NC_ERR(rc, _path, "nc_inq_attlen()");

	char *buf = new char[len+1];

    rc = nc_get_att_text(_ncid,varid,attname.c_str(),buf);
	if (rc != NC_NOERR) delete [] buf;
	MY_NC_ERR(rc, _path, "nc_get_att_text()");

	size_t i;
	for (i=0; i < len && i < n; i++) {
		values[i] = buf[i];
	}
	values[i] = '\0';
	delete [] buf;

	return(NC_NOERR);
}

int NetCDFCpp::GetAtt(
	string varname, string attname, vector <string> &values
) const {
	values.clear();

	string s;

	int rc = NetCDFCpp::GetAtt(varname, attname, s);
	if (rc < 0) return(rc);

	string buf;
	stringstream ss(s);
	while (ss >> buf) values.push_back(buf);

	return(0);
}

int NetCDFCpp::InqVarid(
	string varname, int &varid 
) const {

	if (varname.empty()) {
		varid = NC_GLOBAL;
		return(NC_NOERR);
	}
	int my_varid = -1;
	int rc = nc_inq_varid (_ncid, varname.c_str(), &my_varid);
	MY_NC_ERR(rc, _path, "nc_inq_varid()");
	
	varid = my_varid;

	return(NC_NOERR);
}

int NetCDFCpp::InqAtt(
    string varname, string attname, nc_type &xtype, size_t &len
 ) const {

	int varid;
	int rc = NetCDFCpp::InqVarid(varname, varid);
	if (rc<0) return(rc);

	rc = nc_inq_att(_ncid, varid, attname.c_str(), &xtype, &len);
	MY_NC_ERR(rc, _path, "nc_inq_att()");

	return(NC_NOERR);
}


size_t NetCDFCpp::SizeOf(nc_type xtype) const {
	switch (xtype) {
	case NC_BYTE:
	case NC_UBYTE:
	case NC_CHAR:
		return(1);
	case NC_SHORT: 
	case NC_USHORT:
		return(2);
	case NC_INT:	// NC_LONG and NC_INT
	case NC_UINT:
	case NC_FLOAT:
		return(4);
	case NC_INT64:
	case NC_UINT64:
	case NC_DOUBLE:
		return(8);
	default:
		return(0);
	}
}

bool NetCDFCpp::ValidFile(string path) {

	bool valid = false;
	
	int ncid;
    int rc = nc_open(path.c_str(), 0, &ncid);
	if (rc == NC_NOERR) {
		valid = true;
		nc_close(ncid);
	}
	return(valid);
}




int NetCDFCpp::PutVara(
	string varname,
	vector <size_t> start, vector <size_t> count, const void *data
) {
	assert(start.size() == count.size());

    int varid;
    int rc = NetCDFCpp::InqVarid(varname, varid);
    if (rc<0) return(rc);

	size_t mystart[NC_MAX_VAR_DIMS];
	size_t mycount[NC_MAX_VAR_DIMS];

	for (int i=0; i<start.size(); i++) {
		mystart[i] = start[i];
		mycount[i] = count[i];
	}
	rc = nc_put_vara(_ncid, varid, mystart, mycount, data);
	MY_NC_ERR(rc, _path, "nc_put_vara()");

	return(rc);
}

int NetCDFCpp::PutVar(string varname, const void *data) {

    int varid;
    int rc = NetCDFCpp::InqVarid(varname, varid);
    if (rc<0) return(rc);

	rc = nc_put_var(_ncid, varid, data);
	MY_NC_ERR(rc, _path, "nc_put_var()");

	return(rc);
}

int NetCDFCpp::GetVara(
	string varname,
	vector <size_t> start, vector <size_t> count, void *data
) {
	assert(start.size() == count.size());

    int varid;
    int rc = NetCDFCpp::InqVarid(varname, varid);
    if (rc<0) return(rc);

	size_t mystart[NC_MAX_VAR_DIMS];
	size_t mycount[NC_MAX_VAR_DIMS];

	for (int i=0; i<start.size(); i++) {
		mystart[i] = start[i];
		mycount[i] = count[i];
	}
	rc = nc_get_vara(_ncid, varid, mystart, mycount, data);
	MY_NC_ERR(rc, _path, "nc_get_vara()");

	return(rc);
}

int NetCDFCpp::GetVar(string varname, void *data) {

    int varid;
    int rc = NetCDFCpp::InqVarid(varname, varid);
    if (rc<0) return(rc);

	rc = nc_get_var(_ncid, varid, data);
	MY_NC_ERR(rc, _path, "nc_get_var()");

	return(rc);
}
