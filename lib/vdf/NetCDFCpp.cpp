#include <cassert>
#include <sstream>
#include <netcdf.h>
#include "vapor/NetCDFCpp.h"

using namespace VAPoR;

#define MY_NC_ERR(rc, path, func) \
	if (rc != NC_NOERR) { \
		SetErrMsg( \
			"Error accessing netCDF file \"%s\", %s : %s",  \
			path.c_str(), func, nc_strerror(rc) \
		); \
		return(-1); \
	}

NetCDFCpp::NetCDFCpp() {
	_ncid = -1;
	_path.clear();
}
NetCDFCpp::~NetCDFCpp() {}

int NetCDFCpp::Create(
	string path, int cmode, size_t initialsz, 
	size_t &bufrsizehintp, int &ncid
) {

    int rc = nc__create(path.c_str(), cmode, initialsz, &bufrsizehintp, &ncid);

	MY_NC_ERR(rc, path, "nc__create()");
	_ncid = ncid;
	_path = path;
	
	return(0);
}

int NetCDFCpp::Enddef() const {
    int rc = nc_enddef(_ncid);
	MY_NC_ERR(rc, _path, "nc_enddif()");
}

int NetCDFCpp::Close() const {
    int rc = nc_close(_ncid);
	MY_NC_ERR(rc, _path, "nc_close()");
}

int NetCDFCpp::DefDim(string name, size_t len, int &dimidp) const {
    int rc = nc_def_dim(_ncid, name.c_str(), len, &dimidp);
	MY_NC_ERR(rc, _path, "nc_def_dim()");
}

	
//
// PutAtt - Integer
//
int NetCDFCpp::PutAtt(
	string varname, string attname, int value
) const {
	return(PutAtt(varname, attname, &value, 1));
}

int NetCDFCpp::PutAtt(
	string varname, string attname, vector <int> values
) const {
	size_t n = values.size();
	int *buf = new int[n];
	for (size_t i=0; i<n; i++) buf[i] = values[i];

	int rc = PutAtt(varname, attname, buf, n);
	delete [] buf;

	return(rc);
}

int NetCDFCpp::PutAtt(
	string varname, string attname, const int values[], size_t n
) const {
    int varid;
    int rc = InqVarid(varname, varid);
    if (rc<0) return(-1);

    rc = nc_put_att_int(_ncid,varid,attname.c_str(),NC_INT, n, values);
	MY_NC_ERR(rc, _path, "nc_put_att_int()");

	return(0);
}

//
// GetAtt - Integer
//
int NetCDFCpp::GetAtt(
	string varname, string attname, int &value
) const {
	return(GetAtt(varname, attname, &value, 1));
}

int NetCDFCpp::GetAtt(
	string varname, string attname, vector <int> &values
) const {
	values.clear();

    int varid;
    int rc = InqVarid(varname, varid);
    if (rc<0) return(-1);

	size_t n;
	nc_inq_attlen(_ncid, varid, attname.c_str(), &n);
	MY_NC_ERR(rc, _path, "nc_inq_attlen()");

	int *buf = new int[n];

	rc = GetAtt(varname, attname, buf, n);

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
    if (rc<0) return(-1);

	size_t len;
	nc_inq_attlen(_ncid, varid, attname.c_str(), &len);
	MY_NC_ERR(rc, _path, "nc_inq_attlen()");

	int *buf = new int[len];

    rc = nc_get_att_int(_ncid,varid,attname.c_str(),buf);
	if (rc != NC_NOERR) delete [] buf;
	MY_NC_ERR(rc, _path, "nc_get_att_int()");

	for (size_t i=0; i < len && i < n; i++) {
		values[i] = buf[i];
	}
	delete [] buf;

	return(0);
}

//
// PutAtt - size_t
//
int NetCDFCpp::PutAtt(
	string varname, string attname, size_t value
) const {
	return(PutAtt(varname, attname, &value, 1));
}

int NetCDFCpp::PutAtt(
	string varname, string attname, vector <size_t> values
) const {
	vector <int> ivalues;
	for (size_t i=0; i<values.size(); i++) ivalues.push_back(values[i]);

	return(PutAtt(varname, attname, ivalues));
}

int NetCDFCpp::PutAtt(
	string varname, string attname, 
	const size_t values[], size_t n
) const {
	vector <int> ivalues;

	for (size_t i=0; i<n; i++) ivalues.push_back(values[i]);

	return(PutAtt(varname, attname, ivalues));
}

//
// GetAtt - size_t
//
int NetCDFCpp::GetAtt(
	string varname, string attname, size_t &value
) const {
	return(GetAtt(varname, attname, &value, 1));
}

int NetCDFCpp::GetAtt(
	string varname, string attname, vector <size_t> &values
) const {
	values.clear();

	vector <int> ivalues;
	int rc = GetAtt(varname, attname, ivalues);
	for (int i=0; i<ivalues.size(); i++) values.push_back(ivalues[i]);
	return(rc);

}

int NetCDFCpp::GetAtt(
	string varname, string attname, size_t values[], size_t n
) const {

	vector <int> ivalues;
	int rc = GetAtt(varname, attname, ivalues);
	for (int i=0; i<ivalues.size() && i<n; i++) values[i] = ivalues[i];

	return(rc);
}

//
// PutAtt - Double
//

int NetCDFCpp::PutAtt(
	string varname, string attname, double value
) const {
	return(PutAtt(varname, attname, &value, 1));
}

int NetCDFCpp::PutAtt(
	string varname, string attname, vector <double> values
) const {
	size_t n = values.size();
	double *buf = new double[n];
	for (size_t i=0; i<n; i++) buf[i] = values[i];

	int rc = PutAtt(varname, attname, buf, n);
	delete [] buf;

	return(rc);
}

int NetCDFCpp::PutAtt(
	string varname, string attname, const double values[], size_t n
) const {
    int varid;
    int rc = InqVarid(varname, varid);
    if (rc<0) return(-1);

    rc = nc_put_att_double(_ncid,varid,attname.c_str(),NC_DOUBLE, n, values);
	MY_NC_ERR(rc, _path, "nc_put_att_double()");

	return(0);
}

//
// GetAtt - Double
//
int NetCDFCpp::GetAtt(
	string varname, string attname, double &value
) const {
	return(GetAtt(varname, attname, &value, 1));
}

int NetCDFCpp::GetAtt(
	string varname, string attname, vector <double> &values
) const {
	values.clear();

    int varid;
    int rc = InqVarid(varname, varid);
    if (rc<0) return(-1);

	size_t n;
	nc_inq_attlen(_ncid, varid, attname.c_str(), &n);
	MY_NC_ERR(rc, _path, "nc_inq_attlen()");

	double *buf = new double[n];

	rc = GetAtt(varname, attname, buf, n);

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
    if (rc<0) return(-1);

	size_t len;
	nc_inq_attlen(_ncid, varid, attname.c_str(), &len);
	MY_NC_ERR(rc, _path, "nc_inq_attlen()");

	double *buf = new double[len];

    rc = nc_get_att_double(_ncid,varid,attname.c_str(),buf);
	if (rc != NC_NOERR) delete [] buf;
	MY_NC_ERR(rc, _path, "nc_get_att_double()");

	for (size_t i=0; i < len && i < n; i++) {
		values[i] = buf[i];
	}
	delete [] buf;

	return(0);
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
	int rc = PutAtt(varname, attname, buf, n);
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
	return(PutAtt(varname, attname, s));
}

int NetCDFCpp::PutAtt(
	string varname, string attname, const char values[], size_t n
) const {
    int varid;
    int rc = InqVarid(varname, varid);
    if (rc<0) return(-1);

    rc = nc_put_att_text(_ncid,varid,attname.c_str(), n, values);
	MY_NC_ERR(rc, _path, "nc_put_att_text()");

	return(0);
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
    if (rc<0) return(-1);

	size_t n;
	nc_inq_attlen(_ncid, varid, attname.c_str(), &n);
	MY_NC_ERR(rc, _path, "nc_inq_attlen()");

	char *buf = new char[n+1];

	rc = GetAtt(varname, attname, buf, n);
	value = buf;

	delete [] buf;

	return(rc);
}

int NetCDFCpp::GetAtt(
	string varname, string attname, char values[], size_t n
) const {
    int varid;
    int rc = InqVarid(varname, varid);
    if (rc<0) return(-1);

	size_t len;
	nc_inq_attlen(_ncid, varid, attname.c_str(), &len);
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

	return(0);
}

int NetCDFCpp::InqVarid(
	string varname, int &varid 
) const {

	if (varname.empty()) {
		varid = NC_GLOBAL;
		return(0);
	}
	int my_varid = -1;
	int rc = nc_inq_varid (_ncid, varname.c_str(), &my_varid);
	MY_NC_ERR(rc, _path, "nc_inq_varid()");
	
	varid = my_varid;

	return(0);
}
