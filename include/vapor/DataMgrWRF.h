//
//      $Id$
//

#ifndef	_DataMgrWRF_h_
#define	_DataMgrWRF_h_


#include <vector>
#include <string>
#include <vapor/WRFReader.h>
#include <vapor/DataMgr.h>
#include <vapor/common.h>

namespace VAPoR {

//
//! \class DataMgrWRF
//! \brief A cache based data reader
//! \author John Clyne
//! \version $Revision$
//! \date    $Date$
//!
//
class VDF_API DataMgrWRF : public DataMgr, WRFReader {

public:

 DataMgrWRF(
	const vector <string> &files,
	size_t mem_size
 );

 DataMgrWRF(
	const MetadataWRF &metadata,
	size_t mem_size
 );


 virtual ~DataMgrWRF() {  }; 

 virtual int _VariableExists(
	size_t ts,
	const char *varname,
	int reflevel = 0,
	int lod  = 0
 ) const {
	return (WRFReader::VariableExists(ts,varname));
 };

 //
 //	Metadata methods
 //

 virtual const size_t *GetBlockSize() const {
	return(WRFReader::GetBlockSize());
 }

 virtual void GetBlockSize(size_t bs[3], int reflevel) const {
	WRFReader::GetBlockSize(bs, reflevel);
 }

 virtual int GetNumTransforms() const {
	return(WRFReader::GetNumTransforms());
 };

 virtual vector<double> GetExtents(size_t ts = 0) const {
	return(WRFReader::GetExtents(ts));
 };

 virtual long GetNumTimeSteps() const {
	return(WRFReader::GetNumTimeSteps());
 };

 virtual vector <string> _GetVariables3D() const {
	return(WRFReader::GetVariables3D());
 };

 virtual vector <string> _GetVariables2DXY() const {
	return(WRFReader::GetVariables2DXY());
 };
 virtual vector <string> _GetVariables2DXZ() const {
	return(WRFReader::GetVariables2DXZ());
 };
 virtual vector <string> _GetVariables2DYZ() const {
	return(WRFReader::GetVariables2DYZ());
 };

 virtual vector<long> GetPeriodicBoundary() const {
	return(WRFReader::GetPeriodicBoundary());
 };

 virtual vector<long> GetGridPermutation() const {
	return(WRFReader::GetGridPermutation());
 };

 virtual double GetTSUserTime(size_t ts) const {
	return(WRFReader::GetTSUserTime(ts));
 };

 virtual void GetTSUserTimeStamp(size_t ts, string &s) const {
    WRFReader::GetTSUserTimeStamp(ts,s);
 }

 virtual void   GetGridDim(size_t dim[3]) const {
    return(WRFReader::GetGridDim(dim));
 };


 virtual string GetMapProjection() const {
	return(WRFReader::GetMapProjection());
 };

 virtual string GetGridType() const { return("layered"); };

	
protected:

 virtual const float *GetDataRange() const {
	return(NULL);	// Not implemented. Let DataMgr figure it out
 }

 virtual int    OpenVariableRead(
    size_t timestep,
    const char *varname,
    int,
    int
 ) {
	return(WRFReader::OpenVariableRead(timestep, varname));
 };

 virtual int    CloseVariable() {
	return (WRFReader::CloseVariable());
 };

 virtual int    BlockReadRegion(
    const size_t bmin[3], const size_t bmax[3],
    float *region, bool unblock = true
 ) {
	return(WRFReader::ReadVariable(region));
 };

 virtual RegularGrid    *MakeGrid(
	size_t ts, string varname, int reflevel, int lod,
	const size_t bmin[3], const size_t bmax[3], float *blocks
 );

 virtual RegularGrid    *ReadGrid(
	size_t ts, string varname, int reflevel, int lod,
	const size_t bmin[3], const size_t bmax[3], float *blocks
 );

 virtual void GetValidRegion(
    size_t min[3], size_t max[3], int reflevel
 ) const {
	size_t dim[3]; WRFReader::GetDim(dim, reflevel);
	min[0] = min[1] = min[2] = 0;
	max[0] = dim[0]-1; max[1] = dim[1]-1; max[2] = dim[2]-1;
 };

private:
 float **_blkptrs;


};

};

#endif	//	_DataMgrWRF_h_
