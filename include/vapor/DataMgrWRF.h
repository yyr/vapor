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

protected:


 //
 //	Metadata methods
 //

 virtual void   _GetDim(size_t dim[3], int reflevel) const {
	return(WRFReader::GetDim(dim, reflevel));
 };

 virtual void _GetBlockSize(size_t bs[3], int reflevel) const {
	return(DataMgrWRF::_GetDim(bs, -1));
 }

 virtual int _GetNumTransforms() const {
	return(0);
 };

 virtual string _GetGridType() const { return("layered"); };

 virtual vector<double> _GetExtents(size_t ts) const {
	return(WRFReader::GetExtents(ts));
 };

 virtual long _GetNumTimeSteps() const {
	return(WRFReader::GetNumTimeSteps());
 };

 virtual string _GetMapProjection() const {
	return(WRFReader::GetMapProjection());
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

 virtual vector<long> _GetPeriodicBoundary() const {
	return(WRFReader::GetPeriodicBoundary());
 };

 virtual vector<long> _GetGridPermutation() const {
	return(WRFReader::GetGridPermutation());
 };

 virtual double _GetTSUserTime(size_t ts) const {
	return(WRFReader::GetTSUserTime(ts));
 };

 virtual void _GetTSUserTimeStamp(size_t ts, string &s) const {
    WRFReader::GetTSUserTimeStamp(ts,s);
 }

 virtual int _VariableExists(
	size_t ts,
	const char *varname,
	int reflevel = 0,
	int lod  = 0
 ) const {
	return (WRFReader::VariableExists(ts,varname));
 };


 virtual int    _OpenVariableRead(
    size_t timestep,
    const char *varname,
    int,
    int
 ) {
	return(WRFReader::OpenVariableRead(timestep, varname));
 };

 virtual const float *_GetDataRange() const {
	return(NULL);	// Not implemented. Let DataMgr figure it out
 }

 virtual void _GetValidRegion(
    size_t min[3], size_t max[3], int reflevel
 ) const {
	size_t dim[3]; WRFReader::GetDim(dim, reflevel);
	min[0] = min[1] = min[2] = 0;
	max[0] = dim[0]-1; max[1] = dim[1]-1; max[2] = dim[2]-1;
 };

 virtual int    _BlockReadRegion(
    const size_t *, const size_t *, float *region
 ) {
	return(WRFReader::ReadVariable(region));
 };

 virtual int    _CloseVariable() {
	return (WRFReader::CloseVariable());
 };

};

};

#endif	//	_DataMgrWRF_h_
