//
// $Id$
//


#ifndef	_NetCDFCollection_h_
#define	_NetCDFCollection_h_

#include <vector>
#include <map>

#include <sstream>
#include <vapor/MyBase.h>
#include <vapor/NetCDFSimple.h>

namespace VAPoR {

//
//! \class NetCDFCollection
//! \brief Wrapper for a collection of netCDF files
//! \author John Clyne
//! \version $Revision$
//! \date    $Date$
//!
//! This class treats a collection of netCDF files as a single,
//! time-varying data set, providing a unified interface for accessing
//! named 0D, 1D, 2D, and 3D spatial variables at a specified timestep, 
//! independent of which netCDF file contains the data.
//!
//! The netCDF variables may be "explicitly" time-varying: the slowest
//! varying dimension may correspond to time, while the remaining dimensions
//! correspond to space. Or the variables may be implicitly time-varying,
//! possessing no explicit time dimension, in which case the time index
//! of a variable is determined by file ordering. For example, if the 
//! variable "v" is found in the files "a.nc" and "b.nc" and nowhere else, 
//! and "a.nc" preceeds "b.nc" in the list of files provided to the class
//! constructor, then a.nc is assumed to contain "v" at time step 0, 
//! and "b.nc" contains "v" at time step 1.
//!  
//! The class understands the Arakawa C-Grid ("staggered" variables), and 
//! can interpolate them onto a common (unstaggered) grid.
//!
//! \note The specification of dimensions and coordinates in this class
//! follows the netCDF API convention of ordering from slowest
//! varying dimension to fastest varying dimension. For example, if
//! 'dims' is a vector of dimensions, then dims[0] is the slowest varying
//! dimension, dim[1] is the next slowest, and so on. This ordering is the
//! opposite of the ordering used by most of the VAPoR API.
//!
//! Several file organizations are supported and discussed below. To
//! ease discussion we introduce the terminology:
//!
//! define TD : Time Dimension <br>
//! define TCV : Time Coordinate Variable <br>
//! define TVV : an explicitly Time Varying Variable (it has a time 
//! dimension) <br>
//! define ITVV : an Implicit Time Varying Variable (has no
//!	time dimension, but has multiple occurences => order determines
//!	time) <br>
//! define CV: Constant Variable (has no TD, occurs only once). CV variables 
//! are define for all time steps
//!
//! Four file organizations are supported as described below:
//!
//! <b>case 1:</b> No TD specified (therefore no TCV):
//!	+ All variables are ITVV 
//!	+ synthesize TCV by order of appearance of variables (file order only)
//!	+ ITVV time derived by order of appearance (file order only).
//!	+ only one time step per variable per file
//!
//! <b>case 2:</b> TD specified, but no TCV:
//!	+ All variables are TVV or CV 
//!	+ synthesize TCV by order of appearance (file order, then TD).
//!	+ multiple time steps per variable per file
//!	+ variables with TD are TVV 
//!	+ variables with no TD are CV (available for all time)
//!	+ A CV that occurs multiple times overwrites prior occurence
//!	(there can be no ITVV variables).
//!
//! <b>case 3a</b>: TD & TCV specified, TCV appears in only file:
//!	+ All variables are TVV or CV 
//!	+ TVVs indexed to time in TCV by order of appearance (file order, TD
//!	within file)
//!	+ A CV that occurs multiple times overwrites prior occurence
//!	+ Require TCV to appear in first file
//!
//! <b>case 3b</b>: TD & TCV specified, TCV appears in every file with TVV:
//!	+ Only have TVV and CV variables
//!	+ TVVs indexed to time by TD offset within a file
//!	+ A CV that occurs multiple times overwrites prior occurence
//!	+ Order of file specification is irrelevant (except for multiple
//!	occurences of CVs of same name)
//!


class VDF_API NetCDFCollection : public VetsUtil::MyBase {
public:
 
 NetCDFCollection();
 virtual ~NetCDFCollection();

 //! Initialze a new collection 
 //!
 //! Variables contained in the netCDF files may be explicitly time-varying
 //! (i.e. have a time dimension defined), or their time step may 
 //! be determined implicitly by which file they are contained in. 
 //!
 //! The time ordering of time-varying variables is determined by:
 //!
 //! 1. A 1D time coordinate variable, if it exists
 //! 2. A time dimension, if it exists
 //! 3. The ordering of the files in \p files in which the variable occurs
 //!
 //! \param[in] files A list of file containing time-varying 1D, 2D, and
 //! 3D variables. In the absense of an explicit  time dimension, or time
 //! coordinate variable, the ordering of the files determines the 
 //! the time steps of the variables contained therein.
 //!
 //! \param[in] time_dimnames A list of netCDF time dimensions. A variable
 //! whose slowest varying dimension name matches a name in \p time_dimnames
 //! is an explict time-varying variable with a time dimension.
 //!
 //! \param[in] time_coordvar The name of a coordinate variable containing
 //! time data. If a 1D variable with a name given by \p time_coordvar exists,
 //! this variable will be used to determine the time associated with each
 //! time step of a variable.
 //!
 virtual int Initialize(
	const std::vector <string> &files, 
	const std::vector <string> &time_dimnames, 
	const std::vector <string> &time_coordvar
 );

 //! Return a boolean indicating whether a variable exists in the 
 //! data collection.
 //!
 //! This method returns true iff the variable named by \p varname 
 //! is exists in the data set
 //!
 //! \param[in] varname A netCDF variable name
 //!
 //! \sa GetTimes(), GetVariables()
 //
 virtual bool VariableExists(string varname) const;

 //! Return a boolean indicating whether a variable is defined for a
 //! particular time step.
 //!
 //! This method returns true iff the variable named by \p varname 
 //! is available at the time step \p ts
 //!
 //! \param[in] ts A valid data set time step in the range from zero to
 //! GetNumTimeSteps() - 1.
 //! \param[in] varname A netCDF variable name
 //!
 //! \sa GetTimes(), GetNumTimeSteps(), GetVariables()
 //
 virtual bool VariableExists(size_t ts, string varname) const;

	
 //! Returns true if the specified variable has any staggered dimensions
 //!
 //! If the variabled named by \p varname has any staggered dimensions, true
 //! is returned
 //!
 //! \param[in] varname Name of variable
 //! \retval boolean indicating where variable \p varname has any staggered
 //! dimensions.
 //!
 //! \sa SetStaggeredDims()
 //!
 virtual bool IsStaggeredVar(string varname) const;

 //! Returns true if the specified dimension is staggered
 //!
 //! If the dimension named by \p dimname is a staggered dimension, true
 //! is returned
 //!
 //! \param[in] dimname Name of dimension
 //!
 //! \sa SetStaggeredDims()
 //!
 virtual bool IsStaggeredDim(string dimname) const;

 //! Return a list of variables with a given rank
 //!
 //! Returns a list of variables having a spatial dimension rank 
 //! of \p ndim. If the named variable is explicitly time varying, the
 //! time-varying dimension is not counted. For example, if a variable
 //! named 'v' is defined with 4 dimensions in the netCDF file, and the
 //! slowest varying dimension name matches a named dimension 
 //! specified in Initialize()
 //! by \p time_dimnames, then the variable 'v' would be returned by a 
 //! query with ndim==3.
 //!
 //! \param[in] ndim Rank of spatial dimensions
 //
 virtual std::vector <string> GetVariableNames(int ndim) const;

 //! Return a variables spatial dimensions
 //! 
 //! Returns the ordered list of spatial dimensions of the named variable. 
 //! \note The order follows the netCDF convention (slowest varying
 //! dimension is the first vector element returned), not the VDC 
 //! convention
 //!
 //! \param[in] varname Name of variable
 //! \retval dims An ordered vector containing the variables spatial 
 //! dimensions.
 //!
 virtual std::vector <size_t>  GetDims(string varname) const;

 virtual std::vector <string>  GetDimNames(string varname) const;

 //! Return true if variable is time varying
 //!
 //! Returns true if the variable named by \p varname has a 
 //! time varying coordinate
 //!
 //! \param[in] varname Name of variable
 //
 virtual bool IsTimeVarying(string varname) const;

 //! Return the names of all the dimensions.
 //! 
 //! Returns a list of all dimension names defined by all netCDF 
 //! files in the entire collection.
 //!
 //! \retval dimnames An ordered (by name) vector containing all 
 //! dimension names.
 //!
 virtual std::vector <string>  GetDimNames() const {return (_dimNames); };

 //! Return the number of time steps in the data collection all variables
 //!
 //! Return the number of time steps present. The number of time steps
 //! returned is the number of unique times present, and 
 //!
 //! \note Not all variables in a collection need have the same number 
 //! of time steps
 //!
 //! \param[in] varname Name of variable
 //!
 //! \retval value The number of time steps
 //!
 //
 virtual size_t GetNumTimeSteps() const {return (_times.size());}



 //! Return the user time associated with a time step 
 //!
 //! This method returns the time, in user-defined coordinates,
 //! associated with the time step, \p ts.
 //! If no time coordinate variable was specified the user time is 
 //! equivalent to the time step, \p ts.
 //!
 //! \param[in] ts A valid data set time step in the range from zero to
 //! GetNumTimeSteps() - 1.
 //!
 //! \param[out] time The user time
 //!
 //! \retval retval A negative int is returned on failure.
 //!
 //
 virtual int GetTime(size_t ts, double &time) const; 

 //! Return all the user times associated with a variable
 //!
 //! This method returns the times, in user-defined coordinates,
 //! associated with the the variable, \p varname. 
 //!
 //! \param[in] varname Name of variable
 //! \param[times] time A vector of user times ordered by time step
 //!
 //! \retval retval A negative int is returned on failure.
 //!
 //
 virtual int GetTimes(string varname, std::vector <double> &times) const;

 //! Return all the user times for this collection
 //!
 //! This method returns the times, in user-defined coordinates,
 //! for this data collection.
 //!
 //!
 //! \retval retval times A vector of user times.
 //!
 //! \sa Initialize()
 //
 virtual std::vector <double> GetTimes() const {return (_times); };

 //! Return the path to the netCDF file containing a variable
 //!
 //! This method returns the path to the netCDF file containing the 
 //! variable \p varname, at the time step \p ts.
 //!
 //! \param[in] varname Name of variable
 //! \param[out] file The netCDF file path
 //!
 //! \retval retval A negative int is returned on failure.
 //!
 //
 virtual int GetFile(size_t ts, string varname, string &file) const;

 //! Return the NetCDFSimple::Variable for the named variable
 //!
 //! This method copies the contents of the NetCDFSimple::Variable
 //! class for the named variable into \p varinfo
 //!
 //! \param[in] ts A valid data set time step in the range from zero to
 //! GetNumTimeSteps() - 1.
 //! \param[in] varname Name of variable
 //! 
 //! \retval retval A negative int is returned on failure.
 //!
 virtual int GetVariableInfo(
	size_t ts, string varname, NetCDFSimple::Variable &varinfo
 ) const;

 //! Get info for first variable at first time step
 //!
 virtual int GetVariableInfo(
	string varname, NetCDFSimple::Variable &varinfo
 ) const;


 //! Return the missing value, if any, for a variable
 //!
 //! This method returns the value of the missing data value marker,
 //! if defined, for the variable named by \p varname.
 //!
 //! \param[in] varname The variable name
 //! \param[out] mv The missing value for the variabled name by \p varname
 //!
 //! \retval bool The boolean true is returned if a missing value is defined.
 //! If no missing variable is defined the return value is false and the 
 //! value of \p mv is not defined.
 //!
 virtual bool GetMissingValue(string varname, double &mv) const;

 //! Open the named variable for reading
 //!
 //! This method prepares a netCDF variable, indicated by a
 //! variable name and time step pair, for subsequent read operations by
 //! methods of this class.  
 //!
 //! \param[in] ts Time step of the variable to read
 //! \param[in] varname Name of the variable to read
 //!
 //! \retval status Returns a non-negative value on success
 //!
 //! \sa Read(), ReadNative(), ReadSliceNative() ReadSlice()
 //!
 virtual int OpenRead(size_t ts, string varname);

 //! Read data from the currently opened variable on an unstaggered grid
 //!
 //! Read the hyperslice defined by \p start and \p count from 
 //! the currently opened variable into the buffer pointed to by \p data.
 //! 
 //! This method is identical in functionality to the netCDF function,
 //! nc_get_var_float, with the following two exceptions: 
 //!
 //!
 //! 1. If the opened variable has staggered dimensions, the data are
 //! resampled onto the non-staggered grid. Moreover, the region coordinates
 //! (\p start and \p count) should be specified in the 
 //! non-staggered coordinates.
 //!
 //! 2. If the currently
 //! opened variable is explicitly time-varying (has a time-varying 
 //! dimension), only the spatial dimensions should be provied
 //! by \p start and \p count. I.e. if the variable contains n dimensions
 //! in the netCDF file, only the n-1 fastest-varying (spatial) dimensions 
 //! should be specified.
 //!
 //! \param[in] start Start vector with one element for each dimension 
 //! \param[in] count Count vector with one element for each dimension 
 //! \retval retval A negative int is returned on failure.
 //!
 //! \sa SetStaggeredDims(), ReadNative(), NetCDFSimple::Read(),
 //! SetMissingValueAttName(), SetMissingValueAttName()
 //!
 virtual int Read(size_t start[], size_t count[], float *data);
 virtual int Read(size_t start[], size_t count[], int *data);

 virtual int Read(float *data);

 //! Read data from the currently opened variable on the native grid
 //!
 //! This method is identical to the Read() method except that 
 //! staggered variables are not interpolated to the non-staggered grid. 
 //! Hence, \p start and \p count should be specified in staggered 
 //! coordinates if the variable is staggered, and non-staggerd coordinates
 //! if the variable is not staggered.
 //!
 //! \param[in] start Start vector with one element for each dimension 
 //! \param[in] count Count vector with one element for each dimension 
 //! \retval retval A negative int is returned on failure.
 //!
 //! \sa SetStaggeredDims(), NetCDFSimple::Read()
 //!
 virtual int ReadNative(size_t start[], size_t count[], float *data);
 virtual int ReadNative(size_t start[], size_t count[], int *data);

 virtual int ReadNative(float *data);
 virtual int ReadNative(int *data);

 //! Read a 2D slice from a 2D or 3D variable
 //!
 //! The method will read 2D slices from a 2D or 3D variable until all 
 //! slices have been processed. If the variable is staggered the slices
 //! will be interpolated onto the non-staggered grid for the currently
 //! opened variable. For 3D data each call to ReadSlice() will return
 //! a subsequent slice until all slices have been processed. The total
 //! number of slices is the value of the slowest varying dimension.
 //!
 //! \param[out] data The possibly resampled 2D data slice. It is the 
 //! caller's responsibility to ensure that \p data points to sufficient
 //! space.
 //!
 //! \retval status Returns 1 if successful, 0 if there are no more
 //! slices to read, and a negative integer on error.
 //!
 //! \sa OpenRead(), Read(), SetStaggeredDims(),
 //! SetMissingValueAttName(), SetMissingValueAttName()
 //
 virtual int ReadSlice(float *data);

 //! Read a 2D slice from a 2D or 3D variable on the native grid
 //!
 //! This method is identical to ReadSlice() with the exception that
 //! if the currently opened variable is staggered, the variable will
 //! not be resampled to a non-staggered grid.
 //!
 //! \param[out] data The possibly resampled 2D data slice. It is the 
 //! caller's responsibility to ensure that \p data points to sufficient
 //! space.
 //!
 //! \retval status Returns 1 if successful, 0 if there are no more
 //! slices to read, and a negative integer on error.
 //!
 //! \sa OpenRead(), Read(), SetStaggeredDims()
 //
 virtual int ReadSliceNative(float *data);

 //! Enable reading in reverse order
 //!
 //! This method enables ReadSlice() and ReadNativeSlice to read in
 //! reverse order (last slice first). 
 //! 
 //! \note Results are undefined if this method is called between
 //! OpenRead and Close();
 //!
 //! \param[in] on A boolean indicating wether reverse reading should
 //! be enabled. The default is false.
 //!
 //! \sa ReadSlice(), ReadSliceNative()
 //!
 void EnableLastFirstRead(bool on) {_reverseRead = on;};


 //! Close the currently opened variable
 //!
 //! \sa OpenRead()
 //
 virtual int Close();

 //! Identify staggered dimensions
 //!
 //! This method informs the class instance of any staggered dimensions.
 //! Class methods that read variable data will use the staggered
 //! dimension list to identify staggered variables and may interpolate
 //! the data to a non staggered grid
 //!
 //! \param dimnames A list of staggered netCDF dimension names 
 //
 virtual void SetStaggeredDims(const std::vector <string> dimnames) {
	_staggeredDims = dimnames;
 }

 //! Identify missing value variable attribute name
 //!
 //! This method informs the class instance of the name of a netCDF
 //! variable attribute, if one exists, that contains the missing value
 //! for the variable. During interpolation of staggered variables
 //! the missing value is needed for correct processing
 //!
 //! \param attname Name of netCDF variable attribute specifying the
 //! missing value
 //
 virtual void SetMissingValueAttName(string attname) {
	_missingValAttName = attname;
 }

 //! Return a list of variables that are not available for access
 //!
 //! This method returns a list of variables that were detected in the
 //! collection of netCDF files but are not accessible from the
 //! NetCDFCollection class. 
 //!
 virtual std::vector <string> GetFailedVars() const {return(_failedVars); };

 friend std::ostream &operator<<(
	std::ostream &o, const NetCDFCollection &ncdfc
 );

 class TimeVaryingVar {
 public:
  TimeVaryingVar();
  int Insert(
	const NetCDFSimple::Variable &variable, string file, 
	const std::vector <string> &time_dimnames, 
	const std::map <string, std::vector <double> > &timesmap,
	int file_org
  );
  std::vector <size_t> GetSpatialDims() const {return(_dims); };
  std::vector <string> GetSpatialDimNames() const {return(_dim_names); };
  string GetName() const {return(_name); };

  size_t GetNumTimeSteps() const {return(_tvmaps.size()); };
  int GetTime(size_t ts, double &time) const; 
  std::vector <double> GetTimes() const; 
  int GetTimeStep(double time, size_t &ts) const;
  size_t GetLocalTimeStep(size_t ts) const; 
  int GetFile(size_t ts, string &file) const;
  int GetVariableInfo(size_t ts, NetCDFSimple::Variable &variable) const;
  bool GetTimeVarying() const {return (_time_varying); };
  bool GetMissingValue(string attname, double &mv) const;

  friend std::ostream &operator<<(std::ostream &o, const TimeVaryingVar &var);


  typedef struct {
	int _fileidx;		// Index into _files
	int _varidx;		// Index into _variables
    double _time;		// user time for this time step
	size_t _local_ts;	// time step offset within file
  } tvmap_t;
 private:
  std::vector <NetCDFSimple::Variable> _variables;
  std::vector <string> _files;
  std::vector <tvmap_t> _tvmaps;
  std::vector <size_t> _dims;	// **spatial** dimensions
  std::vector <string> _dim_names;
  string _name;			// variable name
  bool _time_varying;	// true if variable's slowest varying dimension
						// is a time dimension.
  friend bool tvmap_cmp(
    NetCDFCollection::TimeVaryingVar::tvmap_t a,
    NetCDFCollection::TimeVaryingVar::tvmap_t b);

 };
private:

 std::map <string, TimeVaryingVar > _variableList;
 NetCDFSimple *_ncdfptr;
 std::map <string, NetCDFSimple *> _ncdfmap;
 std::vector <string> _staggeredDims;
 std::vector <string> _dimNames; // Names of all dimensions
 string _missingValAttName;
 std::map <string, vector <double> > _timesMap; // map variable to time
 std::vector <double> _times;	// all valid time coordinates
 std::vector <string> _failedVars;	// Varibles that could not be added
 bool _reverseRead;

 //
 // Open file data
 //
 size_t _ovr_local_ts;
 int _ovr_slice;
 unsigned char *_ovr_slicebuf;
 size_t _ovr_slicebufsz;
 unsigned char *_ovr_linebuf;
 size_t _ovr_linebufsz;
 TimeVaryingVar _ovr_tvvars;
 bool _ovr_has_missing;
 double _ovr_missing_value;
 bool _ovr_open;	// open for reading?

 int _InitializeTimesMap(
    const std::vector <string> &files, 
	const std::vector <string> &time_dimnames,
    const std::vector <string> &time_coordvars,
    std::map <string, std::vector <double> > &timesMap,
	std::vector <double> &times,
	int &file_org
 ) const;

 int _InitializeTimesMapCase1(
	const std::vector <string> &files,
    std::map <string, std::vector <double> > &timesMap
 ) const;

 int _InitializeTimesMapCase2(
	const std::vector <string> &files,
	const std::vector <string> &time_dimnames,
    std::map <string, std::vector <double> > &timesMap
 ) const;

 int _InitializeTimesMapCase3(
	const std::vector <string> &files,
	const std::vector <string> &time_dimnames,
	const std::vector <string> &time_coordvars,
    std::map <string, std::vector <double> > &timesMap
 ) const;

 void _InterpolateLine(
	const float *src, size_t n, size_t stride, 
    bool has_missing, float mv, float *dst
 ) const;

 void _InterpolateSlice(
    size_t nx, size_t ny, bool xstag, bool ystag, 
	bool has_missing, float mv, float *slice
 ) const;

int _GetTimesMap(
	NetCDFSimple *netcdf,
	const std::vector <string> &time_coordvars,
	const std::vector <string> &time_dimnames,
	std::map <string, std::vector <double> > &timesmap
 ) const;

 float *_Get1DVar(
    NetCDFSimple *netcdf,
    const NetCDFSimple::Variable &variable
 ) const ;

 int _get_var_index(
    const vector <NetCDFSimple::Variable> variables, string varname
 ) const;


};
};
 
 
#endif
