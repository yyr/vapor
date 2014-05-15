#include <vector>
#include <map>
#include <iostream>
#include <netcdf.h>
#include <vapor/MyBase.h>

#ifndef	_NetCDFCpp_H_
#define	_NetCDFCpp_H_

namespace VAPoR {

//! \class NetCDFCpp
//!
//! Defines simple C++ wrapper for NetCDF
//!
class NetCDFCpp : public VetsUtil::MyBase {
public:
 NetCDFCpp();
 virtual ~NetCDFCpp();
 virtual int Create(
	string path, int cmode, size_t initialsz,
    size_t &bufrsizehintp
 );

 virtual int Open(string path, int mode);

 virtual int DefDim(string name, size_t len) const;

 virtual int DefVar(
	string name, int xtype, vector <string> dimnames
 );

 virtual int InqVarDims(
	string name, vector <string> &dimnames, vector <size_t> &dims
 ) const;

 virtual int InqDimlen(string name, size_t &len) const;




 //
 // PutAtt - Integer
 //
 virtual int PutAtt(
	string varname, string attname, int value
 ) const; 
 virtual int PutAtt(
	string varname, string attname, vector <int> values
 ) const; 
 virtual int PutAtt(
	string varname, string attname, const int values[], size_t n
 ) const; 

 //
 // GetAtt - Integer
 //
 virtual int GetAtt(
	string varname, string attname, int &value
 ) const;
 virtual int GetAtt(
	string varname, string attname, vector <int> &values
 ) const; 
 virtual int GetAtt(
	string varname, string attname, int values[], size_t n
 ) const; 

 //
 // PutAtt - size_t
 //
 virtual int PutAtt(
	string varname, string attname, size_t value
 ) const; 
 virtual int PutAtt(
	string varname, string attname, vector <size_t> values
 ) const; 
 virtual int PutAtt(
	string varname, string attname, 
	const size_t values[], size_t n
 ) const; 

 //
 // GetAtt - size_t
 //
 virtual int GetAtt(
	string varname, string attname, size_t &value
 ) const; 
 virtual int GetAtt(
	string varname, string attname, vector <size_t> &values
 ) const; 
 virtual int GetAtt(
	string varname, string attname, size_t values[], size_t n
 ) const; 

 //
 // PutAtt - Double
 //
 virtual int PutAtt(
	string varname, string attname, double value
 ) const; 
 virtual int PutAtt(
	string varname, string attname, vector <double> values
 ) const;
 virtual int PutAtt(
	string varname, string attname, const double values[], size_t n
 ) const; 

 //
 // GetAtt - Double
 //
 virtual int GetAtt(
	string varname, string attname, double &value
 ) const; 
 virtual int GetAtt(
	string varname, string attname, vector <double> &values
 ) const; 
 virtual int GetAtt(
	string varname, string attname, double values[], size_t n
 ) const; 

 //
 // PutAtt - String
 //
 virtual int PutAtt(
	string varname, string attname, string value
 ) const; 
 virtual int PutAtt(
	string varname, string attname, vector <string> values
 ) const;
 virtual int PutAtt(
	string varname, string attname, const char values[], size_t n
 ) const; 

 //
 // GetAtt - String
 //
 virtual int GetAtt(
	string varname, string attname, string &value
 ) const; 
 virtual int GetAtt(
	string varname, string attname, char values[], size_t n
 ) const; 

 // Tokenize string using space as delimiter
 //
 virtual int GetAtt(
	string varname, string attname, vector <string> &values
 ) const; 

 virtual int InqVarid(string varname, int &varid ) const; 

 virtual int InqAtt(
	string varname, string attname, nc_type &xtype, size_t &len
 ) const;

 virtual int SetFill(int fillmode, int &old_modep);

 virtual int EndDef() const;

 virtual int Close();

 virtual int PutVara(
	string varname,
	vector <size_t> start, vector <size_t> count, const void *data
 );

 virtual int GetVara(
	string varname,
	vector <size_t> start, vector <size_t> count, void *data
 );

 //! Return the size in bytes of a NetCDF external data type
 //!
 size_t SizeOf(int nctype) const;

private:

 int _ncid;
 string _path;

};

}

#endif //	_NetCDFCpp_H_
