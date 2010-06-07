#ifndef	_WaveCodeIO_h_
#define	_WaveCodeIO_h_

#include <vapor/VDFIOBase.h>
#include <vapor/SignificanceMap.h>
#include <vapor/Compressor.h>

namespace VAPoR {

class VDF_API WaveCodecIO : public VDFIOBase {
public:

 WaveCodecIO(const MetadataVDC &metadata);

 WaveCodecIO(const string &metafile);

#ifdef	DEAD
 WaveCodecIO(
	const size_t dim[3], const size_t bs[3], int numTransforms, 
	const vector <size_t> cratios,
	const string &wname, const string &filebase
 );

 WaveCodecIO(const vector <string> &files);
#endif

 virtual ~WaveCodecIO();

 virtual int OpenVariableRead(
	size_t timestep, const char *varname, int reflevel=0, int lod=0
 );

 virtual int OpenVariableWrite(
	size_t timestep, const char *varname, 
	int reflevel=-1 /*ignored*/, int lod=-1
 );

 virtual int CloseVariable();

 virtual int BlockReadRegion(
	const size_t bmin[3], const size_t bmax[3], float *region, int unblock=1
 );

 //! Write a volume subregion to the currently opened progressive
 //! access data volume.
 //!
 //! This method is identical to the WriteRegion() method with the exception
 //! that the region boundaries are defined in block, not voxel, coordinates.
 //! Secondly, unless the 'block' parameter  is set, the internal
 //! blocking of the data will be preserved. I.e. the data are assumed
 //! to already be blocked. 
 //!
 //! The number of voxels contained in \p region must be the product
 //! over i :
 //!
 //! \p (bmax[i] - \p bmin[i] + 1) * bs[i]
 //!
 //! where bs[i] is the ith dimension of the block size.
 //!
 //! \param[in] bmin Minimum region extents in block coordinates
 //! \param[in] bmax Maximum region extents in block coordinates
 //! \param[in] region The volume subregion to write
 //! \param[in] block If true, block the data before writing/transforming
 //! \retval status Returns a non-negative value on success
 //! \sa OpenVariableWrite(), GetBlockSize(), MapVoxToBlk()
 //! \sa SetBoundarPadOnOff()
 //
 virtual int BlockWriteRegion(
	const float *region, const size_t bmin[3], const size_t bmax[3], int block=1
 );

 //! Read a single volume slice
 //!
 //! \note ReadSlice() must be called exactly xxx times
 //!
 virtual int ReadSlice(float *slice);

 //! Write a single volume slice
 //!
 //! \note WriteSlice() must be called exactly xxx times
 //!
 virtual int WriteSlice(const float *slice);

 virtual void SetBoundaryPadOnOff(bool pad) {
	_pad = pad;
 };

 const float *GetDataRange() const {return (_dataRange);}

 // Returns extents of minimum and maximum valid block (voxel?). There 
 // may be holes in the data if not all blocks were written
 //
 void    GetValidRegion(
	size_t min[3], size_t max[3], int reflevel
 ) const;


 //! Returns true if indicated data volume exists on disk
 //!
 //! Returns true if the variable identified by the timestep, variable
 //! name, and refinement level is present on disk. Returns 0 if
 //! the variable is not present.
 //! \param[in] ts A valid time step from the Metadata object used
 //! to initialize the class
 //! \param[in] varname A valid variable name
 //! \param[in] reflevel Ignored
 //! \param[in] lod Compression level of detail requested. The coarsest 
 //! approximation level is 0 (zero). A value of -1 indicates the finest
 //! refinement level contained in the VDC.
 //
 virtual int    VariableExists(
    size_t ts,
    const char *varname,
    int reflevel = 0,
	int lod = 0
 ) const ;


 virtual int GetNumTransforms() const;
 virtual void GetBlockSize(size_t bs[3], int reflevel) const;


 static size_t GetMaxCRatio(const size_t bs[3], string wavename, string wmode);

private:
 SignificanceMap **_sigmaps;
 vector <size_t> _sigmapsizes;	// size of each encoded sig map
 Compressor *_compressor3D;	// 3D compressor
 Compressor *_compressor2DXY;
 Compressor *_compressor2DXZ;
 Compressor *_compressor2DYZ;
 Compressor *_compressor;	// compressor for currently opened variable

 VarType_T _vtype;  // Type (2d, or 3d) of currently opened variable
 VarType_T _compressorType;  // Type (2d, or 3d) of current _compressor
 int _lod;	// compression level of currently opened file
 int _reflevel;	// current refinement level
 size_t _validRegMin[3];	// min region bounds of current file
 size_t _validRegMax[3];	// max region bounds of current file
 bool _writeMode;	// true if opened for writes
 bool _isOpen;	// true if a file is opened
 size_t _timeStep;	// currently opened time step
 string _varName;	// Currently opened variable
 vector <string> _ncpaths; 
 vector <int> _ncids; 
 vector <int> _nc_sig_vars;	// ncdf ids for wave and sig vars 
 vector <int> _nc_wave_vars; 
 float *_cvector;	// storage for wavelet coefficients
 size_t _cvectorsize;	// amount of space allocated to _cvector 
 unsigned char *_svector;	// storage for encoded signficance map 
 size_t _svectorsize;	// amount of space allocated to _svector 
 float *_block;	// storage for an untransformed block
 bool _firstWrite; // false after first block written;
 float _dataRange[2];
 vector <size_t> _ncoeffs; // num wave coeff. at each compression level
 vector <size_t> _cratios3D;	// 3D compression ratios
 vector <size_t> _cratios2D;	// 2D compression ratios
 vector <size_t> _cratios;	// compression ratios for currently opened file

 float *_sliceBuffer;
 size_t _sliceBufferSize;	// size of slice buffer in elements
 int _sliceCount;	// num slices written 

 bool _pad;	// Padding enabled?

 int _OpenVarWrite(const string &basename);
 int _OpenVarRead(const string &basename);
 int _WaveCodecIO();
 int _SetupCompressor();
 int _WriteBlock(size_t bx, size_t by, size_t bz);
 int _FetchBlock(size_t bx, size_t by, size_t bz);
 void _pad_line( float *line_start, size_t l1, size_t l2, size_t stride) const;

 void _UnpackCoord(
    VarType_T vtype, const size_t src[3], size_t dst[3], size_t fill
 ) const;

 void _PackCoord(
	VarType_T vtype, const size_t src[3], size_t dst[3], size_t fill
 ) const;

 void _FillPackedCoord(
	VarType_T vtype, const size_t src[3], size_t dst[3], size_t fill
 ) const;


};
};

#endif	// _WaveCodeIO_h_
