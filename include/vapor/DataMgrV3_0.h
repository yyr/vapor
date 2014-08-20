#include <vector>
#include <vapor/VDC.h>

using namespace std;

//! \class DataMgrV3_0
//!
//! New methods to be added to the DataMgrV3_0
//!


class DataMgrV3_0 {
public:

 //! Return a list of names for all of the defined data variables.
 //! \test New in 3.0
 virtual std::vector <string> GetDataVarNames() const;

 //! Return a list of data variables with a given dimension rank
 //! \test New in 3.0
 virtual std::vector <string> GetDataVarNames(int ndim, bool spatial) const;

 //! Return a list of names for all of the defined coordinate variables.
 //! \test New in 3.0
 virtual std::vector <string> GetCoordVarNames() const;

 //! Return a list of coordinate variables with a given dimension rank
 //! \test New in 3.0
 virtual std::vector <string> GetCoordVarNames(int ndim, bool spatial) const;

 //! Get time coordinates
 //!
 //! Get a sorted list of all time coordinates defined for the 
 //! data set. Multiple time coordinates may be defined. This method
 //! collects the time coordinates from all time coordinate variables 
 //! and sorts them into a single, global time coordinate variable.
 //!
 //! \note Need to deal with different units (e.g. seconds and days).
 //! \note Should methods that take a time step argument \p ts expect
 //! "local" or "global" time steps
 //! \test New in 3.0
 void GetTimeCoordinates(vector <double> &timecoords) const;


 //! Return a data variable's definition
 //!
 //! Return a reference to a VDC::DataVar object describing
 //! the data variable named by \p varname
 //!
 //! \param[in] varname A string specifying the name of the variable.
 //! \param[out] datavar A DataVar object containing the definition
 //! of the named Data variable.
 //!
 //! \retval bool If the named data variable cannot be found false
 //! is returned and the values of \p datavar are undefined.
 //!
 //! \sa DefineCoordVar(), DefineCoordVarUniform(), SetCompressionBlock(),
 //! SetPeriodicBoundary()
 //!
 //! \test New in 3.0
 bool GetDataVarInfo( string varname, VAPoR::VDC::DataVar &datavar) const;

 //! Return metadata about a data or coordinate variable
 //!
 //! If the variable \p varname is defined as either a
 //! data or coordinate variable its metadata will
 //! be returned in \p var.
 //!
 //! \retval bool If the named variable cannot be found false
 //! is returned and the values of \p var are undefined.
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
 //! \retval bool False is returned if the named coordinate variable does
 //! not exist, and the contents of cvar will be undefined.
 //!
 //! \sa DefineCoordVar(), DefineCoordVarUniform(), SetCompressionBlock(),
 //! SetPeriodicBoundary()
 //!
 //! \test New in 3.0
 bool GetCoordVarInfo(string varname, VAPoR::VDC::CoordVar &cvar) const;

 //! Read and return variable data
 //!
 //! Reads all data for the variable named by \p varname for the time
 //! step indicated by \p ts ...
 //! \test New in 3.0
 RegularGrid *GetVariable (
	size_t ts, string varname, int reflevel, int lod, bool lock=false
 );

 //! Read and return a variable hyperslab
 //!
 //! This method is identical to the GetVariable() method, however,
 //! the subregion is specified in the user coordinate system. 
 //! \p min and \p max specify the minimum and maximum extents of an 
 //! axis-aligned bounding box containing the region of interest. The
 //! hyperslab returned is the smallest one that completely contains 
 //! all points within the specified axis-aligned box.
 //!
 //! \note Need to specify what happens if the box is not completely 
 //! contained by the data.
 //!
 //! \test New in 3.0
 RegularGrid *GetVariable (size_t ts, string varname, int reflevel, int lod, 
	vector <double> min, vector <double> max, bool lock=false
 );

 //! Compute the coordinate extents of a variable
 //!
 //! This method finds the spatial domain extents of a variable
 //!
 //! \test New in 3.0
 int GetVariableExtents(
	size_t ts, string varname, int reflevel,
	vector <double> &min , vector <double> &max
 );

 void UnlockGrid(const RegularGrid *rg);

 //! Clear the memory cache
 //!
 //! This method clears the internal memory cache of all entries
 //
 void	Clear();

 //! Return the current data range as a two-element array
 //!
 //! This method returns the minimum and maximum data values
 //! for the indicated time step and variable
 //!
 //! \param[in] ts A valid time step between 0 and GetNumTimesteps()-1
 //! \param[in] varname Name of variable 
 //! \param[in] reflevel Refinement level requested
 //! \param[in] lod Level of detail requested
 //! \param[out] range  A two-element vector containing the current 
 //! minimum and maximum.
 //! \retval status Returns a non-negative value on success 
 //! quantization mapping.
 //!
 //! \note replaced by RegularGrid?
 //
 int GetDataRange(
	size_t ts, const char *varname, float range[2],
	int reflevel = 0, int lod = 0
 );

 //! 
 virtual int VariableExists(
	size_t ts,
	const char *varname,
	int reflevel = 0,
	int lod = 0
 );

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


 //! \deprecated Deprecated
 virtual void   GetDim(size_t dim[3], int reflevel = 0) const;
 

 //! \deprecated See GetBaseVarInfo()
 int GetNumTransforms() const;

 //! \deprecated See GetBaseVarInfo()
 virtual vector <size_t> GetCRatios() const;

 //! \todo Need to define
 //
 virtual string GetCoordSystemType() const;

 //! \deprecated 
 virtual string GetGridType() const { return(_GetGridType()); };

 //! \deprecated - see GetVariableExtents() 
 virtual vector<double> GetExtents(size_t ts = 0) const ;

 //! \todo Need to define
 //
 virtual long GetNumTimeSteps() const;


 //! \todo Need to define
 virtual string GetMapProjection() const;

 //! \deprecated See GetDataVarNames(), GetCoordVarNames()
 virtual vector <string> GetVariableNames() const;

 //! \deprecated
 virtual vector <string> GetVariables3D() const;

 //! \deprecated
 virtual vector <string> GetVariables2DXY() const;

 //! \deprecated
 virtual vector <string> GetVariables2DXZ() const;

 //! \deprecated
 virtual vector <string> GetVariables2DYZ() const;

 //! \deprecated See GetDataVarNames(), GetCoordVarNames()
 virtual vector <string> GetCoordinateVariables() const ;

 //! \deprecated See GetBaseVarInfo()
 //
 virtual vector<long> GetPeriodicBoundary() const {
	return(_GetPeriodicBoundary());
 };

 //! \todo need to define
 virtual vector<long> GetGridPermutation() const;

 //! \todo need to define
 //! \deprecated need to define
 virtual double GetTSUserTime(size_t ts);

 //! \todo need to define
 //! \deprecated need to define
 virtual void GetTSUserTimeStamp(size_t ts, string &s) const;

 //! deprecated - See GetVariable()
 RegularGrid   *GetGrid( size_t ts, string varname, int reflevel, int lod,
    const size_t min[3], const size_t max[3], bool lock = false
 );

 //! \deprecated  - see GetVariableExtents()
 //!
 int GetValidRegion( size_t ts, const char *varname, int reflevel,
    size_t min[3], size_t max[3]
 );

 //! \deprecated
 bool BestMatch(
    size_t ts, const char *varname, int req_reflevel, int req_lod,
    int &reflevel, int &lod
 );

 //! \deprecated - See GetBaseVarInfo()
 //
 virtual VarType_T GetVarType(const string &varname) const; 

 //! \deprecated - See GetBaseVarInfo()
 //
 bool GetMissingValue(string varname, float &value) const;

 //! \deprecated
 //
 virtual void   MapUserToVox(
    size_t timestep,
    const double vcoord0[3], size_t vcoord1[3], int reflevel = 0, int lod = 0
 );

 //! \deprecated
 //
 virtual void   MapVoxToUser(
    size_t timestep,
    const size_t vcoord0[3], double vcoord1[3], int ref_level = 0, int lod=0
 );

 //! \deprecated
 virtual void    GetEnclosingRegion(
    size_t ts, const double minu[3], const double maxu[3],
    size_t min[3], size_t max[3],
    int reflevel = 0, int lod = 0
 );

 //! \deprecated
 virtual bool IsCoordinateVariable(string varname) const;


}
