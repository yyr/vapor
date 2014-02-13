#include <vector>
#include <map>
#include <iostream>
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
 ~NetCDFCpp();
 int Create(
	string path, int cmode, size_t initialsz,
    size_t &bufrsizehintp, int &ncid
 );

 int Enddef() const;

 int Close() const;

 int DefDim(string name, size_t len, int &dimidp) const;

	
 //
 // PutAtt - Integer
 //
 int PutAtt(
	string varname, string attname, int value
 ) const; 
 int PutAtt(
	string varname, string attname, vector <int> values
 ) const; 
 int PutAtt(
	string varname, string attname, const int values[], size_t n
 ) const; 

 //
 // GetAtt - Integer
 //
 int GetAtt(
	string varname, string attname, int &value
 ) const;
 int GetAtt(
	string varname, string attname, vector <int> &values
 ) const; 
 int GetAtt(
	string varname, string attname, int values[], size_t n
 ) const; 

 //
 // PutAtt - size_t
 //
 int PutAtt(
	string varname, string attname, size_t value
 ) const; 
 int PutAtt(
	string varname, string attname, vector <size_t> values
 ) const; 
 int PutAtt(
	string varname, string attname, 
	const size_t values[], size_t n
 ) const; 

 //
 // GetAtt - size_t
 //
 int GetAtt(
	string varname, string attname, size_t &value
 ) const; 
 int GetAtt(
	string varname, string attname, vector <size_t> &values
 ) const; 
 int GetAtt(
	string varname, string attname, size_t values[], size_t n
 ) const; 

 //
 // PutAtt - Double
 //
 int PutAtt(
	string varname, string attname, double value
 ) const; 
 int PutAtt(
	string varname, string attname, vector <double> values
 ) const;
 int PutAtt(
	string varname, string attname, const double values[], size_t n
 ) const; 

 //
 // GetAtt - Double
 //
 int GetAtt(
	string varname, string attname, double &value
 ) const; 
 int GetAtt(
	string varname, string attname, vector <double> &values
 ) const; 
 int GetAtt(
	string varname, string attname, double values[], size_t n
 ) const; 

 //
 // PutAtt - String
 //
 int PutAtt(
	string varname, string attname, string value
 ) const; 
 int PutAtt(
	string varname, string attname, vector <string> values
 ) const;
 int PutAtt(
	string varname, string attname, const char values[], size_t n
 ) const; 

 //
 // GetAtt - String
 //
 int GetAtt(
	string varname, string attname, string &value
 ) const; 
 int GetAtt(
	string varname, string attname, char values[], size_t n
 ) const; 

 int InqVarid(string varname, int &varid ) const; 

private:
 int _ncid;
 string _path;

};

}

#endif //	_NetCDFCpp_H_
