#include <vector>
#include <map>
#include <iostream>
#include <netcdf.h>
#include <vapor/NetCDFCpp.h>
#include <vapor/Compressor.h>

#ifndef	_WASP_H_
#define	_WASP_H_

namespace VAPoR {

//! \class WASP
//!
//! Implements WASP compression conventions for NetCDF
//!
//! The WASP conventions establish a policy for compressing, storing,
//! and accessing arrays of data in NetCDF. This API provides 
//! an interface for NetCDF data adhering to the WASP conventions.
//!
//! Fundamental concepts of the WASP compression conventions include the 
//! following:
//!
//! \li \b Blocking Compressed arrays are decomposed into fixed size blocks,
//! and each block is compressed individually and atomically. The rank of the 
//! block may be equal-to, or less-than that of the array. In the latter
//! case blocking (and subsequent compression) is only performed on the
//! the \em n fastest varying array dimensions, where \em n is the rank of
//! the block.
//!
//! \li \b Transformation Each block is transformed prior to compression
//! in an effort to decorrelate (remove redundancy) from the data. The
//! transformation process is typically lossless up to floating point 
//! round off.
//!
//! \li \b Multi-resolution Some transforms, notably wavelets, exhibit the
//! the property of multi-resolution: arrays can be reconstructed from 
//! transformed coefficients at progressively finer resolution. Resolution
//! levels in the WASP API are specified with a \b level parameter, in the
//! range 0..max, where 0 is the coarsest resolution possible for a given
//! variable, and \em max corresponds to the original resolution of the 
//! array. The value of \b -1 is an alias for \em max.
//!
//! \li <b> Progressive access </b> Aka embedded encoding, is the property by
//! which compressed data may be progressively refined during decoding by
//! transmitting (reading) more of the encoded data. The WASP API supports
//! progressive access, but supports a discrete form of refinement: only
//! a small, finite set of refinement levels, indicated by a \b lod 
//! parameter that indexes into an ordered vector of available
//! compression  ratios.
//!
//
class WASP : public VetsUtil::NetCDFCpp {
public:
 WASP();
 ~WASP();

 //! Create a new NetCDF data set with support for WASP conventions
 //!
 //! \param[in] path The file base name of the new NetCDF data set
 //! \param[in] cmode Same as in NetCDFCpp::Create()
 //! \param[in] initialsz Same as in NetCDFCpp::Create()
 //! \param[in] bufrsizehintp Same as in NetCDFCpp::Create()
 //! \param[in] wname Name of biorthogonal wavelet to use for data 
 //! transformation. See VAPoR::WaveFiltBior.
 //! \param[in] ncratios Number of discrete progressive access refinement
 //! levels.
 //! \param[in] multifile A boolean flag indicating whether compressed
 //! variables should be stored in separate files, one compression level
 //! per file.
 //!
 //! \sa NetCDFCpp::Create()
 //
 int Create(
	string path, int cmode, size_t initialsz,
    size_t &bufrsizehintp, string wname, 
	int ncratios, bool multifile
 );

 //! Open an existing NetCDF file
 //!
 //! \param[in] path The file base name of the new NetCDF data set
 //! \param[in] mode Same as in NetCDFCpp::Open()
 //!
 //! \sa NetCDFCpp::Open()
 //
 int Open(string path, int mode);


 //! Return the dimension lengths associated with a variable.
 //!
 //! Returns the dimensions of the named variable at the indicated
 //! multi-resolution level indicated by \p level.  If the variable
 //! does not support multi-resolution (is not compressed with a 
 //! multi-resolution transform) the \p level parameter is ignored.
 //!
 //! \param[in] name Name of NetCDF variable
 //! \param[out] dims Ordered list of variable's \p name dimension lengths
 //! at the grid hierarchy level indicated by \p level
 //! \param[in] level Grid refinement level
 //!
 //! \sa NetCDFCpp::InqVarDims()
 //
 int InqVarDimslens(
	string name, vector <size_t> &dims, int level
 ) const;

 //! Define a new compressed variable
 //!
 //! \param[in] name Same as NetCDFCpp::DefVar()
 //! \param[in] xtype Same as NetCDFCpp::DefVar()
 //! \param[in] dimnames Same as NetCDFCpp::DefVar()
 //! \param[in] bs An ordered list of block dimensions that specifies the 
 //! block decomposition of the variable. Each element of \p bs is in the 
 //! range 1 to \b dimlen, where \b dimlen is the dimension length of the 
 //! array's associated dimension. The rank of \p bs make be equal to 
 //! or less than 
 //! that of \p dimnames. In the latter case only the rank(bs) fastest
 //! varying dimensions of the variable will be blocked.
 //! \param[in] cratios A monotonically decreasing vector of length \p 
 //! of compression ratios. Each element of \p cratios is in the range 1 
 //! (indicating no compression) to 
 //! \b max, where \b max is the maximum compression supported by the 
 //! specified combination of block size, \p bs, and 
 //! wavelet (See GetMaxCRatio()).
 //!
 //! \sa NetCDFCpp::DefVar()
 //
 int DefVar(
    string name, int xtype, vector <string> dimnames, vector <size_t> cratios
 );

 //! Define a compressed variable with missing data values
 //!
 //! The defined variable may contain missing data values. These 
 //! values will not be transformed and compressed
 //!
 //! \copydoc DefVar(
 //!	string name, int xtype, vector <string> dimnames, 
 //!	vector <size_t> cratios
 //! )
 //!
 //! \param[in] missing_value Value of missing value indicator. 
 //!
 //! \sa NetCDFCpp::DefVar()
 //!
 int DefVar(
	string name, int xtype, vector <string> dimnames, vector <size_t> cratios,
	double missing_value
 );

 //! Inquire whether a named variable is compressed
 //!
 //! \param[in] name The name of the variable
 //! \param[out] compressed A boolean return value indicating whether
 //! variable \p name is compressed
 //
 int InqVarCompressed(
    string varname, bool &compressed
 ) const;

 //! Get maximum compression ratio
 //!
 //! This method returns the maximum compression ratio, \p cratio, possible 
 //! for the 
 //! the specified combination of block size, \p bs, and wavelet, \p wname.
 //! The maximum compression ratio is \p cratio:1.
 //!
 //! \param[in] bs Compression block dimensions
 //! \param[in] wname Name of biorthogonal wavelet to use for data 
 //! transformation. See VAPoR::WaveFiltBior.
 //! \param[out] Maximum possible compression ratio
 //!
 int GetMaxCRatio(
    vector <size_t> bs, string wname, size_t &cratio
 ) const;

 //! Prepare a variable for writing 
 //!
 //! This method initializes the variable named by \p name for writing
 //! using the PutVara() method. If the variable is defined as compressed
 //! the \p lod parameter indicates which compression levels will be stored.
 //! Valid values for \p pod are in the range 0..max, where \p max the size
 //! of cratios - 1.
 //!
 //! Any currently opened variable is first closed with Close()
 //!
 //! \param[in] name Name of variable
 //! \param[in] lod Level-of-detail to save. 
 //!
 //! \note Is \p lod needed? Since cratios can be specified on a per
 //! variable basis perhaps this is not needed?
 //!
 //! \sa PutVara()
 //
 int OpenVarWrite(string name, int lod);

 //! Prepare a variable for reading 
 //!
 //! This method initializes the variable named by \p name for reading
 //! using the GetVara() method. If the variable is defined as compressed
 //! the \p lod parameter indicates which compression levels will used
 //! during reconstruction of the variable.
 //! Valid values for \p pod are in the range 0..max, where \p max the size
 //! of cratios - 1.
 //! If the transform used to compress this variable supports
 //! multiresolution then the \p level parameter indicates the 
 //! grid hierarchy refinement level for which to reconstruct the data.
 //!
 //! Any currently opened variable is first closed with Close()
 //!
 //! \param[in] name Name of variable
 //! \param[in] lod Level-of-detail to read. 
 //! \param[in] level Grid refinement level
 //!
 //! \sa GetVara()
 //
 int OpenVarRead(string name, int lod, int level);

 //! Close the currently opened variable
 //!
 //! If a variable is opened for writing this method will flush 
 //! all buffers to disk and perform cleanup. If opened for reading
 //! only cleanup is performed. If not variables are open this method
 //! is a no-op.
 //
 //!
 int CloseVar();

 //! Write an array of values to the currently opened variable
 //!
 //! The currently opened variable may or may not be compressed
 //!
 //! The combination of \p start and \p count specify the 
 //! coordinates of a hyperslab to write as described by
 //! NetCDFCpp::PutVara(). However, for compressed data dimensions
 //! the values of \p start and \p count must be block aligned  
 //! unless the hyperslab includes the array boundary, in which case
 //! the hyperslab must be aligned to the boundary. 
 //!
 //! \param[in] start A vector of block-aligned integers specifying 
 //! the index in the variable where the first of the data values will 
 //! be read. See NetCDFCpp::PutVara() 
 //! \param[in] count A vector of size_t, block-aligned integers 
 //! specifying the edge 
 //! lengths along each dimension of the block of data values to be read. 
 //! See NetCDFCpp::PutVara()
 //! \param[in] data Same as NetCDFCpp::PutVara()
 //!
 //! \sa OpenVarWrite();
 //
 int PutVara(
	vector <size_t> start, vector <size_t> count, const float *data
 );

 //! Read an array of values from the currently opened variable
 //!
 //! The currently opened variable may or may not be compressed
 //!
 //! If a compressed variable is being read and the transform
 //! supports multi-resolution the 
 //!
 //! \param[in] start A vector of size_t integers specifying the index in 
 //! the variable where the first of the data values will be written.
 //! The coordinates are specified relative to the dimensions of the
 //! array at the currently opened refinement level.
 //! See NetCDFCpp::PutVara()
 //! \param[in] count  A vector of size_t integers specifying the 
 //! edge lengths along each dimension of the block of data values to 
 //! be written.
 //! The coordinates are specified relative to the dimensions of the
 //! array at the currently opened refinement level.
 //! See as NetCDFCpp::PutVara()
 //! \param[in] data Same as NetCDFCpp::PutVara()
 //!
 //! \sa OpenVarWrite();
 //
 int GetVara(
	vector <size_t> start, vector <size_t> count, float *data
 );

 static string AttNameWavelet() {return("WASP.Wavelet");}
 static string AttNameBlockSize() {return("WASP.BlockSize");}
 static string AttNameNumCRatios() {return("WASP.NumCRatios");}
 static string AttNameCRatios() {return("WASP.CRatios");}
 static string AttNameMultifile() {return("WASP.Multifile");}
 static string AttNameCompressed() {return("WASP.Compressed");}
 static string AttNameDimNames() {return("WASP.DimNames");}


private:

 //
 // Helper class for hyperslab indexing arithmetic 
 //
 class vectorinc {
 public:
  // start and count define hyperslab coordinates
  // dims define overall dimensions of array
  // inc is the increment step to be added to start 
  //
  vectorinc(
	vector <size_t> start, vector <size_t> count,
	vector <size_t> dims, vector <size_t> inc
  );

  // Compute next starting coordinates by adding inc to previous
  // starting coordinates. As a convenience return the linear offset
  // of the new starting coordinates coordinates.
  //
  bool next(vector <size_t> &newstart, size_t &offset);

 private:
  vector <size_t> _start;
  vector <size_t> _count;
  vector <size_t> _end;	// ending coordinates (start + count)
  vector <size_t> _dims;
  vector <size_t> _next;
  vector <size_t> _inc;
 };



 std::vector <string> mkmultipaths(string path, int n) const;

 int _Create(
	string path, int cmode, size_t initialsz,
    size_t &bufrsizehintp, int &ncid
 );

 int _GetCompressedDims(
    vector <string> dimnames,
    vector <size_t> cratios,
	int xtype,
    vector <string> &cdimnames,
    vector <size_t> &cdims,
    vector <string> &encoded_dim_names,
    vector <size_t> &encoded_dims
 ) const;

 int _DefVar(
    int ncid, string name, nc_type xtype, vector <string> dimnames
 );
 int _InqVarid(int ncid, string varname, int &varid ) const; 

 int _InqDimlen(string name, size_t &len) const;

 int _InqVarDimsCompressed(
	string name, vector <string> &dimnames, vector <size_t> &dims
 ) const;

 int _PutVara(
	int ncid, string varname,
	vector <size_t> start, vector <size_t> count, const void *data
 );

 int _GetVara(
	int ncid, string varname,
	vector <size_t> start, vector <size_t> count, void *data
 );

 void _get_encoding_vectors(
    vector <size_t> bs, vector <size_t> cratios, int xtype,
    vector <size_t> &ncoeffs, vector <size_t> &encoded_dims
 ) const;

 vector <size_t> _get_block_sizes(int ndims) const;


 bool _validate_cratios(vector <size_t> cratios) const;

 bool _validate_put_vara_compressed(
    vector <size_t> start, vector <size_t> count, 
    vector <size_t> bs, vector <size_t> udims, vector <size_t> cratios
 ) const;

 bool _validate_get_vara_compressed(
	vector <size_t> start, vector <size_t> count,
	vector <size_t> bs, vector <size_t> udims, vector <size_t> cratios
 ) const;

 int _get_compression_params(
    string name, vector <size_t> &bs, vector <size_t> &cratios,
    vector <size_t> &udims, vector <size_t> &dims, string &wname
 ) const;

 int _dims_at_level(
    vector <size_t> dims,
    vector <size_t> bs,
    int level,
    vector <size_t> &dims_level,
    vector <size_t> &bs_level
 );

 void _Block(
    vector <size_t> dims,
    vector <size_t> start,
    vector <size_t> bs, 
	string mode, 
	const float *data, float *block,
    float &min, float &max
 ) const; 

 void _UnBlock(
	vector <size_t> dims,
	vector <size_t> start,
	vector <size_t> bs,
	float *data, const float *block
 ) const;

 int _DecomposeBlock(
	Compressor *cmp,
	const float *block,
	vector <size_t> ncoeffs,
    vector <size_t> encoded_dims,
	float *coeffs,
	unsigned char *maps
 ) const; 

 int _ReconstructBlock(
	Compressor *cmp,
	float *block,
	vector <size_t> ncoeffs,
	vector <size_t> encoded_dims,
	const float *coeffs,
	const unsigned char *maps,
	int level
 ) const;

 int _StoreBlock(
    string varname,
	vector <size_t> bcoords,
	vector <size_t> ncoeffs,
	vector <size_t> encoded_dims,
	const float *coeffs,
	unsigned char *maps
 );

 int _FetchBlock(
	string varname, vector <size_t> bcoords,
	vector <size_t> ncoeffs, vector <size_t> encoded_dims,
	float *coeffs, unsigned char *maps
 );


};

}

#endif //	_WASP_H_
