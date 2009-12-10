//
//      $Id$
//

#ifndef	_DataMgrWB_h_
#define	_DataMgrWB_h_


#include <vector>
#include <string>
#include "vapor/DataMgr.h"
#include "vapor/WaveletBlock3DRegionReader.h"
#include "vaporinternal/common.h"

namespace VAPoR {

//
//! \class DataMgrWB
//! \brief A cache based data reader
//! \author John Clyne
//! \version $Revision$
//! \date    $Date$
//!
//! This class provides a wrapper to the WaveletBlock3DRegionReader()
//! and WaveletBlock2DRegionReader()
//! classes that includes a memory cache. Data regions read from disk through 
//! this
//! interface are stored in a cache in main memory, where they may be
//! be available for future access without reading from disk.
//
class VDF_API DataMgrWB : public DataMgr, public WaveletBlock3DRegionReader {

public:

 DataMgrWB(
	const string &metafile,
	size_t mem_size
 );
// ) : DataMgr(mem_size), WaveletBlock3DRegionReader(metafile()) {};

 DataMgrWB(
	const MetadataVDC &metadata,
	size_t mem_size
 );


 virtual ~DataMgrWB() {}; 

 virtual int VariableExists(
	size_t ts,
	const char *varname,
	int reflevel = 0
 ) const {
	return (WaveletBlock3DRegionReader::VariableExists(ts,varname,reflevel));
 };

 //
 //	Metadata methods
 //

 virtual const size_t *GetBlockSize() const {
	return(WaveletBlock3DRegionReader::GetBlockSize());
 }

 virtual const size_t *GetDimension() const {
	return(WaveletBlock3DRegionReader::GetDimension());
 };

 virtual int GetNumTransforms() const {
	return(WaveletBlock3DRegionReader::GetNumTransforms());
 };

 virtual const vector<double> &GetExtents() const {
	return(WaveletBlock3DRegionReader::GetExtents());
 };

 virtual long GetNumTimeSteps() const {
	return(WaveletBlock3DRegionReader::GetNumTimeSteps());
 };


 virtual const vector <string> &GetVariableNames() const {
	return(WaveletBlock3DRegionReader::GetVariableNames());
 };

 virtual const vector <string> &GetVariables3D() const {
	return(WaveletBlock3DRegionReader::GetVariables3D());
 };

 virtual const vector <string> &GetVariables2DXY() const {
	return(WaveletBlock3DRegionReader::GetVariables2DXY());
 };
 virtual const vector <string> &GetVariables2DXZ() const {
	return(WaveletBlock3DRegionReader::GetVariables2DXZ());
 };
 virtual const vector <string> &GetVariables2DYZ() const {
	return(WaveletBlock3DRegionReader::GetVariables2DYZ());
 };

 virtual const vector<long> &GetPeriodicBoundary() const {
	return(WaveletBlock3DRegionReader::GetPeriodicBoundary());
 };

 virtual const vector<long> &GetGridPermutation() const {
	return(WaveletBlock3DRegionReader::GetGridPermutation());
 };

 virtual const vector<double> &GetTSUserTime(size_t ts) const {
	return(WaveletBlock3DRegionReader::GetTSUserTime(ts));
 };

 virtual void GetTSUserTimeStamp(size_t ts, string &s) const {
	WaveletBlock3DRegionReader::GetTSUserTimeStamp(ts,s);
 };

 virtual const vector<double> &GetTSExtents(size_t ts) const {
	return(WaveletBlock3DRegionReader::GetTSExtents(ts));
 };

 virtual void   GetDim(size_t dim[3], int reflevel) {
	return(WaveletBlock3DRegionReader::GetDim(dim, reflevel));
 };

 virtual void   GetDimBlk(size_t bdim[3], int reflevel) {
	return(WaveletBlock3DRegionReader::GetDimBlk(bdim, reflevel));
 };

 virtual const string &GetMapProjection() const {
	return(WaveletBlock3DRegionReader::GetMapProjection());
 };

 const string &GetCoordSystemType() const {
	return(WaveletBlock3DRegionReader::GetCoordSystemType());
 };
	



protected:

 virtual int	OpenVariableRead(
	size_t timestep,
	const char *varname,
	int reflevel = 0
 ) {
	return(WaveletBlock3DRegionReader::OpenVariableRead(
		timestep, varname, reflevel)
	); 
 };

 virtual int	CloseVariable() {
	 return (WaveletBlock3DRegionReader::CloseVariable());
 };

 virtual int    BlockReadRegion(
    const size_t bmin[3], const size_t bmax[3],
    float *region, int unblock = 1
 )  {
 	return(WaveletBlock3DRegionReader::BlockReadRegion(
		bmin, bmax, region, unblock)
	);
 }; 

 virtual void GetValidRegion(
    size_t min[3], size_t max[3], int reflevel
 ) const {
 	return(WaveletBlock3DRegionReader::GetValidRegion(
		min, max, reflevel)
	);
 };

 virtual const float *GetDataRange() const {
	return(WaveletBlock3DRegionReader::GetDataRange());
 }

};

};

#endif	//	_DataMgrWB_h_
