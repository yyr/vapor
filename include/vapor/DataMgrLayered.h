//
//      $Id$
//

#ifndef	_DataMgrLayered_h_
#define	_DataMgrLayered_h_


#include <vector>
#include <string>
#include "vapor/DataMgr.h"
#include "vapor/LayeredIO.h"
#include "vaporinternal/common.h"

namespace VAPoR {

//
//! \class DataMgrLayered
//! \brief A cache based data reader
//! \author John Clyne
//! \version $Revision$
//! \date    $Date$
//!
//! This class provides a wrapper to the Layered()
//! and WaveletBlock2DRegionReader()
//! classes that includes a memory cache. Data regions read from disk through 
//! this
//! interface are stored in a cache in main memory, where they may be
//! be available for future access without reading from disk.
//
class VDF_API DataMgrLayered : public DataMgr, LayeredIO {

public:

 DataMgrLayered(
	const string &metafile,
	size_t mem_size
 );
// ) : DataMgr(mem_size), LayeredIO(metafile.c_str()) {};

 DataMgrLayered(
	const MetadataVDC &metadata,
	size_t mem_size
 );


 virtual ~DataMgrLayered() {}; 

 virtual int VariableExists(
	size_t ts,
	const char *varname,
	int reflevel = 0,
	int lod = 0
 ) const {
	return (LayeredIO::VariableExists(ts,varname,reflevel));
 };

 //
 //	Metadata methods
 //

 virtual const size_t *GetBlockSize() const {
	return(LayeredIO::GetBlockSize());
 }

 virtual void GetBlockSize(size_t bs[3], int reflevel) const {
	LayeredIO::GetBlockSize(bs, reflevel);
 }

 virtual const size_t *GetDimension() const {
	return(LayeredIO::GetDimension());
 };

 virtual int GetNumTransforms() const {
	return(LayeredIO::GetNumTransforms());
 };

 virtual vector<double> GetExtents() const {
	return(LayeredIO::GetExtents());
 };

 virtual long GetNumTimeSteps() const {
	return(LayeredIO::GetNumTimeSteps());
 };

 virtual vector <string> GetVariables3D() const {
	return(LayeredIO::GetVariables3D());
 };

 virtual vector <string> GetVariables2DXY() const {
	return(LayeredIO::GetVariables2DXY());
 };
 virtual vector <string> GetVariables2DXZ() const {
	return(LayeredIO::GetVariables2DXZ());
 };
 virtual vector <string> GetVariables2DYZ() const {
	return(LayeredIO::GetVariables2DYZ());
 };

 virtual vector<long> GetPeriodicBoundary() const {
	return(LayeredIO::GetPeriodicBoundary());
 };

 virtual vector<long> GetGridPermutation() const {
	return(LayeredIO::GetGridPermutation());
 };

 virtual double GetTSUserTime(size_t ts) const {
	return(LayeredIO::GetTSUserTime(ts));
 };

 virtual void GetTSUserTimeStamp(size_t ts, string &s) const {
    LayeredIO::GetTSUserTimeStamp(ts,s);
 }

 virtual vector<double> GetTSExtents(size_t ts) const {
	return(LayeredIO::GetTSExtents(ts));
 };

 virtual void   GetDim(size_t dim[3], int reflevel) const {
	return(LayeredIO::GetDim(dim, reflevel));
 };

 virtual void   GetDimBlk(size_t bdim[3], int reflevel) {
	return(LayeredIO::GetDimBlk(bdim, reflevel));
 };

 virtual string GetMapProjection() const {
	return(LayeredIO::GetMapProjection());
 };

 void SetLowVals(const vector<string>& varNames, const vector<float>& values) { 
	LayeredIO::SetLowVals(varNames, values);
 }
 void SetHighVals(const vector<string>& varNames, const vector<float>& values) { 
	LayeredIO::SetHighVals(varNames, values);
 }

 void SetInterpolateOnOff(bool on) {
	LayeredIO::SetInterpolateOnOff(on);
 };


 int SetGridHeight(size_t height) {
	return(LayeredIO::SetGridHeight(height));
 }

 size_t GetGridHeight() const {
	return(LayeredIO::GetGridHeight());
 }
	
 virtual void   MapVoxToUser(
    size_t timestep,
    const size_t vcoord0[3], double vcoord1[3], int reflevel = 0
 ) const {
	LayeredIO::MapVoxToUser(timestep, vcoord0, vcoord1, reflevel);
 };

 virtual void   MapUserToVox(
    size_t timestep,
    const double vcoord0[3], size_t vcoord1[3], int reflevel = 0
 ) const {
	LayeredIO::MapUserToVox(timestep, vcoord0, vcoord1, reflevel);
 };






protected:

 virtual int	OpenVariableRead(
	size_t timestep,
	const char *varname,
	int reflevel = 0,
	int lod = 0
 ) {
	return(LayeredIO::OpenVariableRead(
		timestep, varname, reflevel)
	); 
 };

 virtual int	CloseVariable() {
	 return (LayeredIO::CloseVariable());
 };

 virtual int    BlockReadRegion(
    const size_t bmin[3], const size_t bmax[3],
    float *region
 )  {
 	return(LayeredIO::BlockReadRegion(
		bmin, bmax, region, 1)
	);
 }; 

 virtual void GetValidRegion(
    size_t min[3], size_t max[3], int reflevel
 ) const {
 	return(LayeredIO::GetValidRegion(
		min, max, reflevel)
	);
 };

 virtual const float *GetDataRange() const {
	return(LayeredIO::GetDataRange());
 }

};

};

#endif	//	_DataMgrLayered_h_
