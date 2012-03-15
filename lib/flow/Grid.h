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

#ifndef _GRID_H_
#define _GRID_H_

#include "header.h"
#include "VectorMatrix.h"
#include "Interpolator.h"

namespace VAPoR
{


typedef struct PointInfo
{
	VECTOR3 phyCoord;			// physical coordinates
	
	void Set(VECTOR3& pcoord)
	{
		phyCoord = pcoord;
	};

	void Set(PointInfo& pInfo)
	{
		phyCoord = pInfo.phyCoord;
	};
}PointInfo;

//////////////////////////////////////////////////////////////////////////
//
// base class for grid
//
//////////////////////////////////////////////////////////////////////////
class FLOW_API Grid : public VetsUtil::MyBase
{
public:
	// reset parameters
	virtual void Reset(void) = 0;
	virtual void GetDimension(int& xdim, int& ydim, int& zdim) = 0;
	virtual ~Grid() {}

	// whether the physical point is in the boundary
	virtual bool at_phys(VECTOR3& pos) = 0;			
	

	//////////////////////////////////////////////////////////////////////////
	// boundary
	//////////////////////////////////////////////////////////////////////////
	
	
	// set region bound
	virtual void SetRegionExtents(VECTOR3& minR, VECTOR3& maxR) = 0;

	
	// whether the point is in the region
	virtual bool isInRegion(VECTOR3& pos) = 0;
	// get the minimal cell spacing in x,y,z dimensions for cell cellId
	
	virtual float GetMaxMinGridSpacing() = 0;
	virtual void GetMinGridSpacing(float minspace[3]) = 0;
};

//////////////////////////////////////////////////////////////////////////
//
//							cartesian grid
//
//-------> Regular Cartesian Grid
//////////////////////////////////////////////////////////////////////////


class FLOW_API CartesianGrid : public Grid
{

private:
	int m_nDimension[3];				// dimensions of the grid that is mapped to the 
										// current block-region
	int m_nMaxRegionDim[3];				// dimensions of the possibly smaller grid
										// which represents the largest valid
										// x,y,z coordinates in the region.
										// Needed for periodic bound testing.
	
	VECTOR3 m_vMinRegBound, m_vMaxRegBound;	// min and max region user coord extents specified by UI
	
	
	
	bool periodicDim[3];				//Identify which of the dimensions are periodic
										//False if the region coord extents are not full
	float period[3];					//Period of the data when periodic.  Not the same
										//as the region extents.

public:
	// constructor and deconstructor
	CartesianGrid(int xdim, int ydim, int zdim, 
		bool xper, bool yper, bool zper, size_t maxIntCoords[3]);
	CartesianGrid();
	virtual ~CartesianGrid(){}
	

	// reset parameters
	void Reset(void);

	// dimensio related
	int xdim(void) { return m_nDimension[0];}
	int ydim(void) { return m_nDimension[1];}
	int zdim(void) { return m_nDimension[2];}
	
	void GetDimension(int& xdim, int& ydim, int& zdim){xdim = m_nDimension[0]; ydim = m_nDimension[1]; zdim = m_nDimension[2];}

	void setPeriod(const float* prd){period[0] = prd[0]; period[1]=prd[1]; period[2]=prd[2];}
	
	// just determine whether the physical point is inside the region extents.  If
	// the data is periodic in a dimension, it's always inside the extents in that dimension
	bool at_phys(VECTOR3& pos);			
	
	// set region extents
	void SetRegionExtents(VECTOR3&minR, VECTOR3& maxR);

	// check in region 
	bool isInRegion(VECTOR3& pos);
	void GetMinGridSpacing(float spacing[3]);
	float GetMaxMinGridSpacing();
};



};
#endif
