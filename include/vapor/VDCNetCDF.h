#include <vector>
#include <map>
#include <iostream>
#include "vapor/VDC.h"
#include "vapor/WASP.h"

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

 //! Class constructor
 //!
 //! \param[in] master_theshold Variables that either compressed or whose
 //! total number of elements are larger than \p master_theshold will
 //! not be stored in the master file. Ignored if the file is open 
 //! for appending or reading.
 //!
 //! \param[in] variable_threshold Variables not stored in the master
 //! file and whose
 //! total number of elements are larger than \p variable_treshold 
 //! will be stored with one time step per file. Ignored if the file is open 
 //! for appending or reading.
 //
 VDCNetCDF(
	int numthreads = 0,
	size_t master_theshold=10*1024*1024, 
	size_t variable_threshold=100*1024*1024
 );
 virtual ~VDCNetCDF();

 //! \copydoc VDC::GetPath()
 //!
 //! \par Algorithm
 //! If the size of a variable (total number of elements) is less than
 //! GetMasterThreshold() and the variable is not compressed it will
 //! be stored in the file master file. Otherwise, the variable will
 //! be stored in either the coordinate variable or data variable
 //! directory, as appropriate. Variables stored in coordinate or 
 //! data directories are stored one variable per file. If the size
 //! of the variable is less than GetVariableThreshold() and the 
 //! variable is time varying multiple time steps may be saved in a single
 //! file. If the variable is compressed each compression level is stored
 //! in a separate file.
 //!
 //! \sa VDCNetCDF::VDCNetCDF(), GetMasterThreshold(), GetVariableThreshold()
 //!
 virtual int GetPath(
    string varname, size_t ts, string &path, size_t &file_ts,
	size_t &max_ts
 ) const;

 //! Initialize the VDCNetCDF class
 //!
 //! \param[in] chunksizehint : NetCDF chunk size hint.  A value of
 //! zero results in NC_SIZEHINT_DEFAULT being used.
 //
 virtual int Initialize(string path, AccessMode mode, size_t chunksizehint = 0);

 //! Return the master file size threshold
 //!
 //! \sa VDCNetCDF::VDCNetCDF(), GetMasterThreshold(), GetVariableThreshold()
 //
 size_t GetMasterThreshold() const {return _master_threshold; };

 //! Return the variable size  threshold
 //!
 //! \sa VDCNetCDF::VDCNetCDF(), GetMasterThreshold(), GetVariableThreshold()
 //
 size_t GetVariableThreshold() const {return _variable_threshold; };

 int OpenVariableRead(
    size_t ts, string varname, int reflevel=0, int lod=-1
 );
 int CloseVariable();


 int OpenVariableWrite(size_t ts, string varname, int lod=-1);

 int Write(const float *region);

 int WriteSlice(const float *slice);

 int Read(float *region);

 int ReadSlice(float *slice);

 int ReadRegion(
    const size_t min[3], const size_t max[3], float *region
 );




protected:
 virtual int _WriteMasterMeta();
 virtual int _ReadMasterMeta();

private:
 int _version;
 NetCDFCpp *_master;	// Master NetCDF file
 NetCDFCpp *_open_file;	// Currently opened data file
 bool _open_write;	// opened for writing?
 VarBase *_open_var;
 size_t _open_slice_num; // index of current slice for WriteSlice, ReadSlice
 size_t _open_ts;	// global time step of current open variable
 size_t _open_file_ts;	// local (within file) time step of current open var
 string _open_varname;	// name of current open variable
 
 VetsUtil::SmartBuf _sb_slice_buffer;
 float *_slice_buffer;
 
 size_t _chunksizehint;	// NetCDF chunk size hint for file creates
 size_t _master_threshold;
 size_t _variable_threshold;
 int _nthreads;

 int _WriteMasterDimensions();
 int _WriteMasterAttributes (
	string prefix, const map <string, Attribute> &atts
 ); 
 int _WriteMasterAttributes ();
 int _WriteMasterVarBaseDefs(string prefix, const VarBase &var); 
 int _WriteMasterCoordVarsDefs(); 
 int _WriteMasterDataVarsDefs(); 
 int _WriteSlice(WASP *file, const float *slice);
 int _WriteSlice(NetCDFCpp *file, const float *slice);

 int _ReadMasterDimensions();
 int _ReadMasterAttributes (
	string prefix, map <string, Attribute> &atts
 ); 
 int _ReadMasterAttributes ();
 int _ReadMasterVarBaseDefs(string prefix, VarBase &var); 
 int _ReadMasterCoordVarsDefs(); 
 int _ReadMasterDataVarsDefs(); 
 int _ReadSlice(WASP *file, float *slice);
 int _ReadSlice(NetCDFCpp *file, float *slice);

	
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
