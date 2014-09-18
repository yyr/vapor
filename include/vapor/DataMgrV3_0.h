#include <vector>
#include <list>
#include <vapor/BlkMemMgr.h>
#include <vapor/VDC.h>
#include <vapor/MyBase.h>
#include <vapor/RegularGrid.h>

using namespace std;

//! \class DataMgrV3_0
//! \brief A cache based data reader
//! \author John Clyne
//!
//! The DataMgr class is an abstract class that defines public methods for
//! accessing (reading) 1D, 2D and 3D field variables. The class implements a
//! memory cache to speed data access -- once a variable is read it is
//! stored in cache for subsequent access. The DataMgr class is abstract:
//! it declares a number of public and protected pure virtual methods 
//! that must be
//! implemented by specializations of this class to allow access to particular
//! file formats.
//!
//! This class inherits from VetsUtil::MyBase. Unless otherwise documented
//! any method that returns an integer value is returning status. A negative
//! value indicates failure. Error messages are logged via
//! VetsUtil::MyBase::SetErrMsg(). Methods that return a boolean do
//! not, unless otherwise documented, log an error message upon
//! failure (return of false).
//

namespace VAPoR {
class PipeLine;

class DataMgrV3_0 : public VetsUtil::MyBase {
public:

 //! Constructor for the DataMgrV3_0 class.
 //!
 //! The DataMgrV3_0 will attempt to cache previously read data and coordinate
 //! variables in memory. The \p mem_size specifies the requested cache
 //! size in MEGABYTES!!!
 //!
 //! \param[in] mem_size Size of memory cache to be created, specified
 //! in MEGABYTES!!
 //!
 //
 DataMgrV3_0(size_t mem_size);

 virtual ~DataMgrV3_0();

 //! Initialize the class
 //!
 //! This method must be called to initialize the DataMgrV3_0 class with
 //! a list of input data files.
 //!
 //! \param[in] files A list of file paths
 //! 
 //! \retval status A negative int is returned on failure and an error
 //! message will be logged with MyBase::SetErrMsg()
 //!
 //
 virtual int Initialize(const vector <string> &files);


 //! Return a list of names for all of the defined data variables.
 //! 
 //! This method returns a list of all data variables defined 
 //! in the data set.
 //!
 //! \retval list A vector containing a list of all the data variable names
 //!
 //! \sa GetCoordVarNames()
 //!
 //! \test New in 3.0
 virtual std::vector <string> GetDataVarNames() const;

 //! Return a list of data variables with a given dimension rank
 //!
 //! This method returns a list of all data variables defined 
 //! in the data set with the specified dimensionality
 //!
 //! \param[in] ndim Variable rank
 //! \param[in] spatial A boolean indicating whether only the variables
 //! spatial rank should be compared against \p ndim
 //!
 //! \retval list A vector containing a list of all the data variable names
 //! with the specified number of dimensions.
 //!
 //! \test New in 3.0
 virtual std::vector <string> GetDataVarNames(int ndim, bool spatial) const;

 //! Return a list of names for all of the defined coordinate variables.
 //! 
 //! This method returns a list of all coordinate variables defined 
 //! in the data set.
 //!
 //! \retval list A vector containing a list of all the coordinate 
 //! variable names
 //!
 //! \sa GetDataVarNames()
 //!
 //! \test New in 3.0
 virtual std::vector <string> GetCoordVarNames() const;

 //! Return a list of coordinate variables with a given dimension rank
 //!
 //! This method returns a list of all coordinate variables defined 
 //! in the coordinate set with the specified dimensionality
 //!
 //! \param[in] ndim Variable rank
 //! \param[in] spatial A boolean indicating whether only the variables
 //! spatial rank should be compared against \p ndim
 //!
 //! \retval list A vector containing a list of all the coordinate 
 //! variable names
 //! with the specified number of dimensions.
 //!
 //! \test New in 3.0
 virtual std::vector <string> GetCoordVarNames(int ndim, bool spatial) const;

 //! Get time coordinates
 //!
 //! Get a sorted list of all time coordinates defined for the 
 //! data set. Multiple time coordinate variables may be defined. This method
 //! collects the time coordinates from all time coordinate variables 
 //! and sorts them into a single, global time coordinate variable.
 //!
 //! \note Need to deal with different units (e.g. seconds and days).
 //! \note Should methods that take a time step argument \p ts expect
 //! "local" or "global" time steps
 //! \test New in 3.0
 //!
 void GetTimeCoordinates(vector <double> &timecoords) const {
	timecoords = _timeCoordinates;
 };


 //! Return a data variable's definition
 //!
 //! Return a reference to a VDC::DataVar object describing
 //! the data variable named by \p varname
 //!
 //! \param[in] varname A string specifying the name of the variable.
 //! \param[out] datavar A DataVar object containing the definition
 //! of the named Data variable.
 //!
 //! \retval bool If true the method was successful. If false the
 //! named variable is invalid (unknown)
 //!
 //! \sa GetCoordVarInfo()
 //!
 //! \test New in 3.0
 bool GetDataVarInfo( string varname, VAPoR::VDC::DataVar &datavar) const;

 //! Return metadata about a data or coordinate variable
 //!
 //! If the variable \p varname is defined as either a
 //! data or coordinate variable its metadata will
 //! be returned in \p var.
 //!
 //! \retval bool If true the method was successful. If false the
 //! named variable is invalid (unknown)
 //!
 //! \sa GetDataVarInfo(), GetCoordVarInfo()
 //! \test New in 3.0
 bool GetBaseVarInfo(string varname, VAPoR::VDC::BaseVar &var) const;

 //! Return a coordinate variable's definition
 //!
 //! Return a reference to a VDC::CoordVar object describing
 //! the coordinate variable named by \p varname
 //!
 //! \param[in] varname A string specifying the name of the coordinate
 //! variable.
 //! \param[out] coordvar A CoordVar object containing the definition
 //! of the named variable.
 //!
 //! \retval bool If true the method was successful. If false the
 //! named variable is invalid (unknown)
 //!
 //! \sa GetDataVarInfo()
 //!
 //! \test New in 3.0
 bool GetCoordVarInfo(string varname, VAPoR::VDC::CoordVar &cvar) const;

 //! Return a boolean indicating whether a variable is time varying
 //!
 //! This method returns \b true if the variable named by \p varname is defined
 //! and it has a time axis dimension. If either of these conditions
 //! is not true the method returns false.
 //!
 //! \param[in] varname A string specifying the name of the variable. 
 //! \retval bool Returns true if variable \p varname exists and is 
 //! time varying.
 //!
 bool IsTimeVarying(string varname) const;

 //! Return a boolean indicating whether a variable is compressed
 //!
 //! This method returns \b true if the variable named by \p varname is defined
 //! and it has a compressed representation. If either of these conditions
 //! is not true the method returns false.
 //!
 //! \param[in] varname A string specifying the name of the variable. 
 //! \retval bool Returns true if variable \p varname exists and is 
 //! compressed
 //!
 //! \sa DefineCoordVar(), DefineDataVar(), VDC::BaseVar::GetCompressed()
 //
 bool IsCompressed(string varname) const;

 //! Return the time dimension length for a variable
 //!
 //! Returns the number of time steps (length of the time dimension)
 //! for which a variable is defined. If \p varname does not have a 
 //! time coordinate 0 is returned. If \p varname is not defined 
 //! as a variable a negative int is returned.
 //!
 //! \param[in] varname A string specifying the name of the variable. 
 //! \retval count The length of the time dimension, or a negative
 //! int if \p varname is undefined.
 //!
 int GetNumTimeSteps(string varname) const;

 //! Return the number of refinement levels for the indicated variable
 //!
 //! Compressed variables have a multi-resolution grid representation.
 //! This method returns the number of levels in the hiearchy. A value
 //! of one indicates that only the native resolution is available. 
 //! A value of two indicates that two levels, the native plus the
 //! next coarsest are available, and so on.
 //!
 //! \param[in] varname Data or coordinate variable name.
 //!
 //! \retval num If \p varname is unknown zero is returned. if \p varname
 //! is not compressed (has no multi-resolution representation) one is
 //! returned. Otherwise the total number of levels in the multi-resolution
 //! hierarchy are returned.
 //
 int GetNumRefLevels(string varname) const;

 //! Return the compression ratio vector for the indicated variable
 //!
 //! Return the compression ratio vector for the indicated variable.
 //! The vector returned contains an ordered list of available
 //! compression ratios for the variable named by \p variable.
 //! If the variable is not compressed, the \p cratios parameter will
 //! contain a single element, one.
 //!
 //! \param[in] varname Data or coordinate variable name.
 //! \param[out] cratios Ordered vector of compression ratios
 //!
 //! \retval status A negative int is returned on failure
 //!
 //! \sa VDC::BaseVar::GetCRatios();
 //
int GetCRatios(string varname, vector <size_t> &cratios) const;

 //! Read and return variable data
 //!
 //! Reads all data for the data or coordinate variable named by \p varname 
 //! for the time step, refinement level, and leve-of-detail indicated 
 //! by \p ts, \p level, and \p lod, respectively.
 //!
 //! \param[in] ts
 //! An integer offset into the time coordinate variable
 //! returned by GetTimeCoordinates() indicting the time step of the 
 //! variable to access.
 //! 
 //! \param[in] varname The name of the data or coordinate variable to access
 //!
 //! \param[in] level
 //! \parblock
 //! To provide maximum flexibility as well as compatibility with previous
 //! versions of the VDC the interpretation of \p level is somewhat
 //! complex. Both positive and negative values may be used to specify
 //! the refinement level and have different interpretations.
 //!
 //! For positive
 //! values of \p level, a value of \b 0 indicates the coarsest
 //! member of the
 //! grid hierarchy. A value of \b 1 indicates the next grid refinement
 //! after the coarsest, and so on. Using postive values the finest level
 //! in the hierarchy is given by GetNumRefLevels() - 1. Values of \p level
 //! that are greater than GetNumRefLevels() - 1 are treated as if they
 //! were equal to GetNumRefLevels() - 1.
 //!
 //! For negative values of \p level a value of -1 indicates the
 //! variable's native grid resolution (the finest resolution available).
 //! A value of -2 indicates the next coarsest member in the hierarchy after
 //! the finest, and so
 //! on. Using negative values the coarsest available level in the hierarchy is
 //! given by negating the value returned by GetNumRefLevels(). Values of
 //! \p level that are less than the negation of GetNumRefLevels() are
 //! treated as if they were equal to the negation of the GetNumRefLevels()
 //! return value.
 //!
 //! \param[in] lod The level-of-detail parameter, \p lod, selects
 //! the approximation level. Valid values for \p lod are integers in
 //! the range 0..n-1, where \e n is returned by
 //! VDC::BaseVar::GetCRatios().size(), or the value -1 may be used
 //! to select the best approximation available
 //!
 //! \param[in] lock If true, the memory region will be locked into the 
 //! cache (i.e. valid after subsequent GetVariable() calls). Otherwise,
 //! the memory owned by the returned RegularGrid may be overwritten 
 //! after subsequent calls to GetVariable()
 //! \endparblock
 //!
 //! \note The RegularGrid structure returned is allocated from the heap. 
 //! it is the caller's responsiblity to delete the returned object
 //! when it is no longer in use.
 //!
 //! \test New in 3.0
 VAPoR::RegularGrid *GetVariable (
	size_t ts, string varname, int level, int lod, bool lock=false
 );

 //! Read and return a variable hyperslab
 //!
 //! This method is identical to the GetVariable() method, however,
 //! a subregion is specified in the user coordinate system. 
 //! \p min and \p max specify the minimum and maximum extents of an 
 //! axis-aligned bounding box containing the region of interest. The
 //! VAPoR::RegularGrid object returned contains the intersection between the 
 //! specified 
 //! hyperslab and the variable's spatial domain (which is not necessarily 
 //! rectangular or axis-aligned).
 //! If the requested hyperslab lies entirely outside of the domain of the 
 //! requested variable an empty RegularGrid structure is returned.
 //!
 //! \param[in] min A one, two, or three element array specifying the 
 //! minimum extents, in user coordinates, of an axis-aligned box defining
 //! the region-of-interest. The spatial dimensionality of the variable
 //! determines the number of elements in \p min.
 //!
 //! \param[in] max A one, two, or three element array specifying the 
 //! maximum extents, in user coordinates, of an axis-aligned box defining
 //! the region-of-interest. The spatial dimensionality of the variable
 //! determines the number of elements in \p max.
 //!
 //! \note The RegularGrid structure returned is allocated from the heap. 
 //! it is the caller's responsiblity to delete the returned object
 //! when it is no longer in use.
 //!
 //! \test New in 3.0
 VAPoR::RegularGrid *GetVariable (
	size_t ts, string varname, int level, int lod, 
	vector <double> min, vector <double> max, bool lock=false
 );

 VAPoR::RegularGrid *GetVariable(
	size_t ts, string varname, int level, int lod,
	vector <size_t> min, vector <size_t> max, bool lock=false
 );

 //! Compute the coordinate extents of a variable
 //!
 //! This method finds the spatial domain extents of a variable
 //!
 //! This method returns the min and max extents 
 //! specified in user 
 //! coordinates, of the smallest axis-aligned bounding box that is 
 //! guaranteed to contain
 //! the variable indicated by \p varname, and the given refinement level,
 //! \p level
 //! 
 //! \test New in 3.0
 int GetVariableExtents(
	size_t ts, string varname, int level,
	vector <double> &min , vector <double> &max
 );

 //! Unlock a floating-point region of memory
 //!
 //! Decrement the lock counter associatd with a
 //! region of memory, and if zero,
 //! unlock region of memory previously locked with GetVariable().
 //! When the lock counter reaches zero the region is simply
 //! marked available for
 //! internal garbage collection during subsequent GetVariable() calls
 //!
 //! \param[in] rg A pointer to a RegularGrid previosly
 //! returned by GetVariable()
 //! \retval status Returns a non-negative value on success
 //!
 //! \sa GetVariable()
 //
 void UnlockGrid(const VAPoR::RegularGrid *rg);

 //! Clear the memory cache
 //!
 //! This method clears the internal memory cache of all entries
 //
 void	Clear();

 //! Returns true if indicated data volume is available
 //!
 //! Returns true if the variable identified by the timestep, variable
 //! name, refinement level, and level-of-detail is present in
 //! the data set. Returns false if
 //! the variable is not available.
 //!
 //! \param[in] ts A valid time step between 0 and GetNumTimesteps()-1
 //! \param[in] varname A valid variable name
 //! \param[in] level Refinement level requested. 
 //! \param[in] lod Compression level of detail requested. 
 //! refinement level contained in the VDC.
 //
 virtual bool VariableExists(
	size_t ts,
	string varname,
	int level = 0,
	int lod = 0
 );

#ifdef	DEAD

 //!
 //! Add a pipeline stage to produce derived variables
 //!
 //! Add a new pipline stage for derived variable calculation. If a 
 //! pipeline already exists with the same
 //! name it is replaced. The output variable names are added to
 //! the list of variables available for this data 
 //! set (see GetVariables3D, etc.).
 //!
 //! An error occurs if:
 //!
 //! \li The output variable names match any of the native variable 
 //! names - variable names returned via _GetVariables3D(), etc. 
 //! \li The output variable names match the output variable names
 //! of pipeline stage previously added with NewPipeline()
 //! \li A circular dependency is introduced by adding \p pipeline
 //!
 //! \retval status A negative int is returned on failure.
 //!
 int NewPipeline(PipeLine *pipeline);

 //!
 //! Remove the named pipline if it exists. Otherwise this method is a
 //! no-op
 //!
 //! \param[in] name The name of the pipeline as returned by 
 //! PipeLine::GetName()
 //!
 void RemovePipeline(string name);
#endif

 //! Return true if the named variable is the output of a pipeline
 //!
 //! This method returns true if \p varname matches a variable name
 //! in the output list (PipeLine::GetOutputs()) of any pipeline added
 //! with NewPipeline()
 //!
 //! \sa NewPipeline()
 //
 bool IsVariableDerived(string varname) const;

 //! Return true if the named variable is availble from the derived 
 //! classes data access methods. 
 //!
 //! A return value of true does not imply that the variable can
 //! be read (\see VariableExists()), only that it is part of the 
 //! data set known to the derived class
 //!
 //! \sa NewPipeline()
 //
 bool IsVariableNative(string varname) const;

	
 //! Purge the cache of a variable
 //!
 //! \param[in] varname is the variable name
 //!
 void PurgeVariable(string varname);

protected: 

 //! \copydoc Initialize()
 //
 virtual int _Initialize(const vector <string> &files) = 0;

 //! \copydoc GetDataVarNames()
 //
 virtual std::vector <string> _GetDataVarNames() const = 0;
 
 //! \copydoc GetCoordVarNames()
 //
 virtual std::vector <string> _GetCoordVarNames() const = 0;

 //! \copydoc GetBaseVarInfo()
 //
 virtual bool _GetBaseVarInfo(
	string varname, VAPoR::VDC::BaseVar &var
 ) const = 0;

 //! \copydoc GetDataVarInfo()
 //
 virtual bool _GetDataVarInfo(
	string varname, VAPoR::VDC::DataVar &datavar
 ) const = 0;

 //! \copydoc GetBaseVarInfo()
 //
 virtual bool _GetCoordVarInfo(
	string varname, VAPoR::VDC::CoordVar &cvar
 ) const = 0;

 //! Return a variable's dimension lengths at a specified refinement level
 //!
 //! Compressed variables have a multi-resolution grid representation.
 //! This method returns the variable's ordered dimension lengths,
 //! and block dimensions
 //! at the multiresolution refinement level specified by \p level.
 //!
 //! If the variable named by \p varname is not compressed the variable's
 //! native dimensions are returned.
 //!
 //! \param[in] varname Data or coordinate variable name.
 //! \param[in] level Specifies a member of a multi-resolution variable's
 //! grid hierarchy as described above.
 //! \param[out] dims_at_level An ordered vector containing the variable's
 //! dimensions at the specified refinement level
 //! \param[out] bs_at_level An ordered vector containing the variable's
 //! block dimensions at the specified refinement level
 //!
 //! \retval status Zero is returned upon success, otherwise -1.
 //!
 //! \sa VAPoR::VDC, VDC::BaseVar::GetBS(), VDC::BaseVar::GetDimensions()
 //
 virtual int _GetDimLensAtLevel(
    string varname, int level, vector <size_t> &dims_at_level,
    vector <size_t> &bs_at_level
 ) const = 0;

 virtual int _GetNumRefLevels(string varname) const = 0;


 //! \copydoc VariableExists()
 //
 virtual bool _VariableExists(
	size_t ts, string,
	int level = 0,
	int lod = 0
 ) const = 0;

 //! Read and return variable data
 //!
 //! Reads all data for the variable named by \p varname for the time
 //! step indicated by \p ts ...
 //! \test New in 3.0
 //
 virtual int _ReadVariableBlock (
	size_t ts, string varname, int level, int lod, 
	vector <size_t> min, vector <size_t> max, float *blocks
 ) = 0;

 //! Read and return variable data
 //!
 //! Reads the variable in its entirety. All time steps, etc.
 //!
 virtual int _ReadVariable(
	string varname, int level, int lod, float *data
 ) = 0;
 
 //! \todo Need to define
 virtual string GetMapProjection() const {return(""); };

private:
 class blkexts {
 public:
  blkexts(
	std::vector <size_t> bs, std::vector <size_t> bdims,
	const float *blks
  );
  void getexts(std::vector <size_t> blkcrds, float &min, float &max) const;
 private:
  std::vector <float> _mins;
  std::vector <float> _maxs;
  std::vector <size_t> _bdims;
 };

 //
 // Cache for various metadata attributes 
 //
 class VarInfoCache  {
 public:

	//
	//
	void Set(
		size_t ts, std::vector <string> varnames, int level, int lod, string key,
		const std::vector <size_t> &values
	);
	void Set(
		size_t ts, string varname, int level, int lod, string key,
		const std::vector <size_t> &values
	) { 
		std::vector <string> varnames;
		varnames.push_back(varname);
		Set(ts, varnames, level, lod, key, values);
	}

	bool Get(
		size_t ts, std::vector <string> varnames, int level, int lod, string key,
		std::vector <size_t> &values
	) const;

	bool Get(
		size_t ts, string varname, int level, int lod, string key,
		std::vector <size_t> &values
	) const { 
		std::vector <string> varnames;
		varnames.push_back(varname);
		return Get(ts, varnames, level, lod, key, values);
	}

	void PurgeSize_t(
		size_t ts, std::vector <string> varnames, int level, int lod, string key
	); 
	void PurgeSize_t(
		size_t ts, string varname, int level, int lod, string key
	) {
		std::vector <string> varnames;
		varnames.push_back(varname);
		PurgeSize_t(ts, varnames, level, lod, key);
	} 

	void Set(
		size_t ts, std::vector <string> varnames, int level, int lod, string key,
		const std::vector <double> &values
	);
	void Set(
		size_t ts, string varname, int level, int lod, string key,
		const std::vector <double> &values
	) { 
		std::vector <string> varnames;
		varnames.push_back(varname);
		Set(ts, varnames, level, lod, key, values);
	}

	bool Get(
		size_t ts, std::vector <string> varnames, int level, int lod, string key,
		std::vector <double> &values
	) const;

	bool Get(
		size_t ts, string varname, int level, int lod, string key,
		std::vector <double> &values
	) const { 
		std::vector <string> varnames;
		varnames.push_back(varname);
		return Get(ts, varnames, level, lod, key, values);
	}

	void PurgeDouble(
		size_t ts, std::vector <string> varnames, int level, int lod, string key
	); 
	void PurgeDouble(
		size_t ts, string varname, int level, int lod, string key
	) {
		std::vector <string> varnames;
		varnames.push_back(varname);
		PurgeDouble(ts, varnames, level, lod, key);
	} 

	void Clear() {
		_cacheSize_t.clear(); 
		_cacheDouble.clear(); 
	}


 private:

  map <string, vector <size_t> > _cacheSize_t;
  map <string, vector <double> > _cacheDouble;

  string _make_hash(
	string key, size_t ts, vector <string> cvars, int level, int lod
  ) const;



 };

 size_t _mem_size;
 std::vector <double> _timeCoordinates;

 typedef struct {
	size_t ts;
	string varname;
	int level;
	int lod;
	vector <size_t> bmin;
	vector <size_t> bmax;
	int lock_counter;
	float *blks;
 } region_t;

 // a list of all allocated regions
 std::list <region_t> _regionsList;

 VAPoR::BlkMemMgr  *_blk_mem_mgr;

 vector <PipeLine *> _PipeLines;


 VarInfoCache _varInfoCache;

 int _get_coord_vars(string varname, vector <string> cvars) const;

 int _DataMgrV3_0(size_t mem_size);
 int _GetTimeCoordinates(vector <double> &timecoords);

 VAPoR::RegularGrid *_make_grid_regular(
    const VAPoR::VDC::DataVar &var,
    const vector <size_t> &min,
    const vector <size_t> &max,
    const vector <size_t> &dims,
    const vector <float *> &blkvec,
    const vector <size_t> &bs,
    const vector <size_t> &bmin,
    const vector <size_t> &bmax
 ) const;

 VAPoR::RegularGrid *_make_grid(
	const VAPoR::VDC::DataVar &var,
	const vector <size_t> &min,
	const vector <size_t> &max,
	const vector <size_t> &dims,
	const vector <float *> &blkvec,
	const vector < vector <size_t > > &bsvec,
	const vector < vector <size_t > > &bminvec,
	const vector < vector <size_t > > &bmaxvec
 ) const;

 VAPoR::RegularGrid *_getVariable(
    size_t ts,
    string varname,
    int level,
    int lod,
    bool    lock,
    bool    dataless
 ); 

 VAPoR::RegularGrid *_getVariable(
    size_t ts,
    string varname,
    int level,
    int lod,
    vector <size_t> min,
    vector <size_t> max,
    bool    lock,
    bool    dataless
 ); 

 float   *_get_region_from_cache(
	size_t ts,
	string varname,
	int level,
	int lod,
	const vector <size_t> &bmin,
	const vector <size_t> &bmax,
	bool    lock
 );

 float *_get_region_from_fs(
	size_t ts, string varname, int level, int lod,
	const vector <size_t> &bs, const vector <size_t> &bmin,
	const vector <size_t> &bmax, bool lock
 );

 float *_get_region(
	size_t ts,
	string varname,
	int level,
	int lod,
	const vector <size_t> &bs,
	const vector <size_t> &bmin,
	const vector <size_t> &bmax,
	bool lock
 );

 int _get_regions(
	size_t ts,
	const vector <string> &varnames,
	int level, int lod,
	bool lock,
	const vector < vector <size_t > > &bsvec,
	const vector < vector <size_t> > &bminvec,
	const vector < vector <size_t> > &bmaxvec,
	vector <float *> &blkvec
 ); 

 void _unlock_blocks(const float *blks);

 vector <string> _get_native_variables() const;
 vector <string> _get_derived_variables() const;

 float   *_alloc_region(
	size_t ts,
	string varname,
	int level,
	int lod,
	vector <size_t> bmin,
	vector <size_t> bmax,
	vector <size_t> bs,
	int element_sz,
	bool    lock,
	bool fill
 ); 

 void    _free_region(
	size_t ts,
	string varname,
	int level,
	int lod,
	vector <size_t> bmin,
	vector <size_t> bmax
 );

 bool _free_lru();
 void _free_var(string varname);


};

};
