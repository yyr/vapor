/*************************************************************************
*						OSU Flow Vector Field							 *
*																		 *
*																		 *
*	Created:	Han-Wei Shen, Liya Li									 *
*				The Ohio State University								 *
*	Date:		06/2005													 *
*																		 *
*	Grid:		Irregular, Curvilinear, Cartesian grid					 *
*************************************************************************/
#ifdef WIN32
#pragma warning(disable : 4251 4100)
#endif

#include "Grid.h"
#include <vapor/flowlinedata.h>
using namespace VetsUtil;
using namespace VAPoR;

//////////////////////////////////////////////////////////////////////////
//																		//
//	definition of Cartesian Grid Class									//
//																		//
//////////////////////////////////////////////////////////////////////////
// constructor and deconstructor
CartesianGrid::CartesianGrid()
{
	Reset();
}

CartesianGrid::CartesianGrid(int xdim, int ydim, int zdim, 
			bool xperiodic, bool yperiodic, bool zperiodic, size_t maxIntCoords[3])
{
	m_nDimension[0] = xdim;
	m_nDimension[1] = ydim;
	m_nDimension[2] = zdim;
	periodicDim[0] = xperiodic;
	periodicDim[1] = yperiodic;
	periodicDim[2] = zperiodic;
	for (int i = 0; i< 3; i++) m_nMaxRegionDim[i] = maxIntCoords[i];
}



//////////////////////////////////////////////////////////////////////////
// Reset
//////////////////////////////////////////////////////////////////////////
void CartesianGrid::Reset(void)
{
	m_nDimension[0] = m_nDimension[1] = m_nDimension[2] = 0;
	periodicDim[0]=periodicDim[1]=periodicDim[2]=false;
}


//////////////////////////////////////////////////////////////////////////
// set region bounds in USER coordinates
//////////////////////////////////////////////////////////////////////////
void CartesianGrid::SetRegionExtents(VECTOR3& minR, VECTOR3& maxR)
{
	m_vMinRegBound = minR;
	m_vMaxRegBound = maxR;
	
}



//////////////////////////////////////////////////////////////////////////
// test whether the physical point pos is in region.
// It's ok to be out, if that dimension is periodic
//////////////////////////////////////////////////////////////////////////
bool CartesianGrid::isInRegion(VECTOR3& pos)
{
	if (pos[0] == END_FLOW_FLAG) return false;
	assert(pos[0] != STATIONARY_STREAM_FLAG);
	if( (periodicDim[0]||(pos[0] >= m_vMinRegBound[0]) && (pos[0] <= m_vMaxRegBound[0]) )&&
		(periodicDim[1]||(pos[1] >= m_vMinRegBound[1]) && (pos[1] <= m_vMaxRegBound[1]) )&&
		(periodicDim[2]||(pos[2] >= m_vMinRegBound[2]) && (pos[2] <= m_vMaxRegBound[2]))  )
		return true;
	else
		return false;
}


//////////////////////////////////////////////////////////////////////////
// for Cartesian grid, this funcion means whether the physical point is 
// in the boundary
//////////////////////////////////////////////////////////////////////////
bool CartesianGrid::at_phys(VECTOR3& pos)
{
	PointInfo pInfo;

	// whether in the bounding box
	
	if(!isInRegion(pos))
		return false;

	return true;
}




void CartesianGrid::GetMinGridSpacing(float minspace[3]){
	for (int i = 0; i<3; i++) minspace[i] = (m_vMaxRegBound[i]-m_vMinRegBound[i])/m_nDimension[i];
}
float CartesianGrid::GetMinCellVolume(){
	float minspace[3];
	GetMinGridSpacing(minspace);
	return (pow(minspace[0]*minspace[1]*minspace[2],0.333333f));
}
