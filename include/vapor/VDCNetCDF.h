#include <vector>
#include <map>
#include <iostream>
#include "vapor/VDC.h"

#ifndef	_VDCNetCDF_H_
#define	_VDCNetCDF_H_

namespace VAPoR {

//! \class VDCNetCDF
//!
//! Implements the VDC abstract class, providing storage of VDC data 
//! in NetCDF files.
//!
class VDCNetCDF : public VAPoR::VDC {
public:

 //! Class constuctor
 //!
 //!
 VDCNetCDF();
 virtual ~VDCNetCDF() {}

 //! Initialize the VDCNetCDF class
 //!
 //
 virtual int Initialize(string path, AccessMode mode);

protected:
 virtual int _WriteMasterMeta();
 virtual int _ReadMasterMeta();

private:
 int _ncid;
 map <string, int> _dimidMap;

 int _WriteMasterDimensions();
 int _WriteMasterAttributes (
	string prefix, const map <string, Attribute> atts
 ); 
 int _WriteMasterAttributes ();
 int _WriteMasterVarBase(string prefix, const VarBase &var); 
 int _WriteMasterCoordVars(); 
 int _WriteMasterDataVars(); 

	
 //
 // PutAtt - Integer
 //
 int _PutAtt(
	string path, string varname, string attname, int value
 ); 
 int _PutAtt(
	string path, string varname, string attname, vector <int> values
 ); 
 int _PutAtt(
	string path, string varname, string attname, const int values[], size_t n
 ); 

 //
 // GetAtt - Integer
 //
 int _GetAtt(
	string path, string varname, string attname, int &value
 ) const;
 int _GetAtt(
	string path, string varname, string attname, vector <int> &values
 ) const; 
 int _GetAtt(
	string path, string varname, string attname, int values[], size_t n
 ) const; 

 //
 // PutAtt - size_t
 //
 int _PutAtt(
	string path, string varname, string attname, size_t value
 ); 
 int _PutAtt(
	string path, string varname, string attname, vector <size_t> values
 ); 
 int _PutAtt(
	string path, string varname, string attname, 
	const size_t values[], size_t n
 ); 

 //
 // GetAtt - size_t
 //
 int _GetAtt(
	string path, string varname, string attname, size_t &value
 ) const; 
 int _GetAtt(
	string path, string varname, string attname, vector <size_t> &values
 ) const; 
 int _GetAtt(
	string path, string varname, string attname, size_t values[], size_t n
 ) const; 

 //
 // PutAtt - Double
 //
 int _PutAtt(
	string path, string varname, string attname, double value
 ); 
 int _PutAtt(
	string path, string varname, string attname, vector <double> values
 );
 int _PutAtt(
	string path, string varname, string attname, const double values[], size_t n
 ); 

 //
 // GetAtt - Double
 //
 int _GetAtt(
	string path, string varname, string attname, double &value
 ) const; 
 int _GetAtt(
	string path, string varname, string attname, vector <double> &values
 ) const; 
 int _GetAtt(
	string path, string varname, string attname, double values[], size_t n
 ) const; 

 //
 // PutAtt - String
 //
 int _PutAtt(
	string path, string varname, string attname, string value
 ); 
 int _PutAtt(
	string path, string varname, string attname, vector <string> values
 );
 int _PutAtt(
	string path, string varname, string attname, const char values[], size_t n
 ); 

 //
 // GetAtt - String
 //
 int _GetAtt(
	string path, string varname, string attname, string &value
 ) const; 
 int _GetAtt(
	string path, string varname, string attname, char values[], size_t n
 ) const; 

 int _GetVarID(
	string path, string varname, int &ncid, int &varid 
 ) const; 

};
};

#endif
