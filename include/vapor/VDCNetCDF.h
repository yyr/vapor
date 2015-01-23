#include <vector>
#include <map>
#include <algorithm>
#include <iostream>
#include "vapor/VDC.h"
#include "vapor/WASP.h"

#ifndef	_VDCNetCDF_H_
#define	_VDCNetCDF_H_

namespace VAPoR {

//! \class VDCNetCDF
//!	\ingroup Public_VDC
//!
//! \brief Implements the VDC 
//! abstract class, providing storage of VDC data 
//! in NetCDF files.
//!
//! \author John Clyne
//! \date    July, 2014
//!
//! Implements the VDC abstract class, providing storage of VDC data 
//! in NetCDF files. Data (variables) are stored in multiple NetCDF files.
//! The distribution of variables to files is described by GetPath().
//!
class VDCNetCDF : public VAPoR::VDC {
public:

 //! Class constructor
 //!
 //! \param[in] numthreads Number of parallel execution threads
 //! to be run during encoding and decoding of compressed data. A value
 //! of 0, the default, indicates that the thread count should be
 //! determined by the environment in a platform-specific manner, for
 //! example using sysconf(_SC_NPROCESSORS_ONLN) under *nix OSes.
 //!
 //! \param[in] master_theshold Variables that are either compressed, or whose
 //! total number of elements are larger than \p master_theshold, will
 //! not be stored in the master file. Ignored if the file is open 
 //! for appending or reading.
 //!
 //! \param[in] variable_threshold Variables not stored in the master
 //! file and whose
 //! total number of elements are larger than \p variable_threshold 
 //! will be stored with one time step per file. Ignored if the file is open 
 //! for appending or reading.
 //
 VDCNetCDF(
	int numthreads = 0,
	size_t master_theshold=10*1024*1024, 
	size_t variable_threshold=100*1024*1024
 );
 virtual ~VDCNetCDF();

 //! Return path to the data directory
 //!
 //! Return the file path to the data directory associated with the 
 //! master file named by \p path. Data files, those NetCDF files
 //! containing coordinate and data variables, are stored in separate 
 //! files from
 //! the VDC master file (See VDC::Initialize()). The data files reside
 //! under the directory returned by this command. 
 //!
 //! \param[in] path Path to VDC master file
 //!
 //! \retval dir : Path to the data directory
 //! \sa Initialize(), GetPath(), DataDirExists()
 //
 static string GetDataDir(string path);

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

 //! \copydoc VDC::GetDimLensAtLevel()
 //
 virtual int GetDimLensAtLevel(
    string varname, int level, std::vector <size_t> &dims_at_level,
	vector <size_t> &bs_at_level
 ) const;

 //! Return true if a data directory exists for the master file
 //! named by \p path
 //!
 //! \param[in] path Path to VDC master file
 //!
 //! \sa Initialize(), GetPath(), GetDataDir();
 //
 static bool DataDirExists(string path) ;


 //! Initialize the VDCNetCDF class
 //! \copydoc VDC::Initialize()
 //!
 //! \param[in] chunksizehint : NetCDF chunk size hint.  A value of
 //! zero results in NC_SIZEHINT_DEFAULT being used.
 //
 virtual int Initialize(
	const vector <string> &paths, AccessMode mode, size_t chunksizehint = 0
 );
 virtual int Initialize(
	string path, AccessMode mode, size_t chunksizehint = 0
 ) {
	std::vector <string> paths;
	paths.push_back(path);
	return(Initialize(paths, mode, chunksizehint));
 }

 //! Return the master file size threshold
 //!
 //! \sa VDCNetCDF::VDCNetCDF(), GetVariableThreshold()
 //
 size_t GetMasterThreshold() const {return _master_threshold; };

 //! Return the variable size  threshold
 //!
 //! \sa VDCNetCDF::VDCNetCDF(), GetMasterThreshold()
 //
 size_t GetVariableThreshold() const {return _variable_threshold; };

 //! \copydoc VDC::OpenVariableRead()
 //
 int OpenVariableRead(
    size_t ts, string varname, int level=0, int lod=-1
 );

 //! \copydoc VDC::CloseVariable()
 //
 int CloseVariable();


 //! \copydoc VDC::OpenVariableWrite()
 //
 int OpenVariableWrite(size_t ts, string varname, int lod=-1);

 //! \copydoc VDC::Write()
 //
 int Write(const float *region);

 int WriteSlice(const float *slice);

 //! \copydoc VDC::Read()
 //
 int Read(float *region);

 //! \copydoc VDC::ReadSlice()
 //
 int ReadSlice(float *slice);

 //! \copydoc VDC::ReadRegion()
 //
 int ReadRegion(
    const std::vector<size_t> &min, 
	const std::vector<size_t> &max, float *region
 );

 //! \copydoc VDC::ReadRegionBlock()
 //
 int ReadRegionBlock(
    const vector <size_t> &min, const vector <size_t> &max, float *region
 );

 //! \copydoc VDC::PutVar()
 //
 int PutVar(string varname, int lod, const float *data);

 //! \copydoc VDC::PutVar()
 //
 int PutVar(size_t ts, string varname, int lod, const float *data);

 //! \copydoc VDC::GetVar()
 //
 int GetVar(string varname, int level, int lod, float *data);

 //! \copydoc VDC::GetVar()
 //
 int GetVar(size_t ts, string varname, int level, int lod, float *data);


 //! \copydoc VDC::CompressionInfo()
 //
 bool CompressionInfo(
	std::vector <size_t> bs, string wname, size_t &nlevels, size_t &maxcratio
 ) const;

 virtual bool VariableExists(
    size_t ts,
    string varname,
    int reflevel = 0,
    int lod = 0
 ) const;



protected:

#ifndef	DOXYGEN_SKIP_THIS
 virtual int _WriteMasterMeta();
 virtual int _ReadMasterMeta();
#endif

private:
 int _version;
 WASP *_master;	// Master NetCDF file
 WASP *_open_file;	// Currently opened data file
 bool _open_write;	// opened for writing?
 BaseVar _open_var;
 size_t _open_slice_num; // index of current slice for WriteSlice, ReadSlice
 size_t _open_ts;	// global time step of current open variable
 size_t _open_file_ts;	// local (within file) time step of current open var
 string _open_varname;	// name of current open variable
 int _open_level;
 
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
 int _WriteMasterBaseVarDefs(string prefix, const BaseVar &var); 
 int _WriteMasterCoordVarsDefs(); 
 int _WriteMasterDataVarsDefs(); 
 int _WriteSlice(WASP *file, const float *slice);
 int _WriteSlice(NetCDFCpp *file, const float *slice);

 int _ReadMasterDimensions();
 int _ReadMasterAttributes (
	string prefix, map <string, Attribute> &atts
 ); 
 int _ReadMasterAttributes ();
 int _ReadMasterBaseVarDefs(string prefix, BaseVar &var); 
 int _ReadMasterCoordVarsDefs(); 
 int _ReadMasterDataVarsDefs(); 
 int _ReadSlice(WASP *file, float *slice);
 int _ReadSlice(NetCDFCpp *file, float *slice);

 int _PutAtt(
    WASP *ncdf,
    string varname,
    string tag,
    const Attribute &attr
 );

 int _DefVar(WASP *ncdf, const VDC::BaseVar &var, size_t max_ts);

 bool _var_in_master(const VDC::BaseVar &var) const;



};
};

#endif
