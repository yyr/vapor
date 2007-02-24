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
enum CellType
{
	TRIANGLE,
	CUBE,
	POLYGONE,
	TETRAHEDRON
};

// define the cell type
enum CellTopoType
{
	T0_CELL,					// vertex
	T1_CELL,					// edge
	T2_CELL,					// triangle, quarilateral
	T3_CELL						// tetrahedra, cube
};

typedef struct PointInfo
{
	VECTOR3 phyCoord;			// physical coordinates
	VECTOR3 interpolant;		// interpolation coefficients
	int		fromCell;			// from which cell, used for advection of tetrahedra grids
	int		inCell;				// in which cell

	void Set(VECTOR3& pcoord, VECTOR3& coeff, int fCell, int iCell)
	{
		phyCoord = pcoord;
		interpolant = coeff;
		fromCell = fCell;
		inCell = iCell;
	};

	void Set(PointInfo& pInfo)
	{
		phyCoord = pInfo.phyCoord;
		interpolant = pInfo.interpolant;
		fromCell = pInfo.fromCell;
		inCell = pInfo.inCell;
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

	//////////////////////////////////////////////////////////////////////////
	// cell related
	//////////////////////////////////////////////////////////////////////////
	// physical coordinate of vertex verIdx
	virtual bool at_vertex(int verIdx, VECTOR3& pos) = 0;
	// whether the physical point is in the boundary
	virtual bool at_phys(VECTOR3& pos) = 0;			
	// get vertex list of a cell
	virtual int getCellVertices(int cellId, CellTopoType cellType, vector<int>& vVertices) = 0;
	// get the cell id and also interpolating coefficients for the given physical position
	virtual int phys_to_cell(PointInfo& pInfo) = 0;
	// interpolation
	virtual void interpolate(VECTOR3& nodeData, vector<VECTOR3>& vData, VECTOR3 coeff) = 0;
	// the volume of cell
	virtual float cellVolume(int cellId) = 0;
	// whether in a cell
	virtual bool isInCell(PointInfo& pInfo, const int cellId) = 0;
	// type of cell
	virtual CellType GetCellType(void) = 0;
	// whether a cell is on boundary
	virtual bool isCellOnBoundary(int cellId) = 0;

	//////////////////////////////////////////////////////////////////////////
	// boundary
	//////////////////////////////////////////////////////////////////////////
	// compute bounding box
	virtual void ComputeBBox(void) = 0;
	// set bounding box
	virtual void SetBoundary(VECTOR3& minB, VECTOR3& maxB) = 0;
	// set region bound
	virtual void SetRegionExtents(VECTOR3& minR, VECTOR3& maxR) = 0;
	// get min and maximal boundary
	virtual void Boundary(VECTOR3& minB, VECTOR3& maxB) = 0;
	// whether the point is in the bounding box
	virtual bool isInBBox(VECTOR3& pos) = 0;
	// whether the point is in the region
	virtual bool isInRegion(VECTOR3& pos) = 0;
	// get the minimal cell spacing in x,y,z dimensions for cell cellId
	virtual float GetGridSpacing(int cellId) = 0;
};

//////////////////////////////////////////////////////////////////////////
//
//							cartesian grid
//
//-------> Regular Cartesian Grid
//////////////////////////////////////////////////////////////////////////

// map coordinates in computational space to physical space
#define UCGridPhy2Comp(x, y, f) (((x) - (y))*(f))

class FLOW_API CartesianGrid : public Grid
{

private:
	int m_nDimension[3];				// dimensions of the grid that is mapped to the 
										// current block-region
	int m_nMaxRegionDim[3];				// dimensions of the possibly smaller grid
										// which represents the largest valid
										// x,y,z coordinates in the region.
										// Needed for periodic bound testing.
	VECTOR3 m_vMinBound, m_vMaxBound;	// min and max  user coord bounds of the block-region of data
										//obtained from the datamgr.
	VECTOR3 m_vMinRegBound, m_vMaxRegBound;	// min and max region user coord extents specified by UI
	float mappingFactorX;				// mapping from physical space to computational space
	float mappingFactorY;				// Used to map a discrete grid to the bounds of the
	float mappingFactorZ;				// block region
	float oneOvermappingFactorX;
	float oneOvermappingFactorY;
	float oneOvermappingFactorZ;
	float gridSpacing;					// the minimal grid spacing of all dimensions
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
	int xcelldim(void) {return (m_nDimension[0] - 1);}
	int ycelldim(void) {return (m_nDimension[1] - 1);}
	int zcelldim(void) {return (m_nDimension[2] - 1);}
	void GetDimension(int& xdim, int& ydim, int& zdim){xdim = m_nDimension[0]; ydim = m_nDimension[1]; zdim = m_nDimension[2];}

	void setPeriod(const float* prd){period[0] = prd[0]; period[1]=prd[1]; period[2]=prd[2];}
	//////////////////////////////////////////////////////////////////////////
	// cell related
	//////////////////////////////////////////////////////////////////////////
	// physical coordinate of vertex verIdx
	bool at_vertex(int verIdx, VECTOR3& pos);
	// just determine whether the physical point is inside the region extents.  If
	// the data is periodic in a dimension, it's always inside the extents in that dimension
	bool at_phys(VECTOR3& pos);			
	// get vertex list of a cell, i.e. the integers used to index the vertices of the cell
	// in the data returned from the dataMgr.  If the data is periodic, then it's necessary
	// to "roll over" the end vertices back to the start of the array.
	int getCellVertices(int cellId, CellTopoType cellType, vector<int>& vVertices);
	// get the cell id and also interpolating coefficients for the given physical position
	int phys_to_cell(PointInfo& pInfo);
	// interpolation
	void interpolate(VECTOR3& nodeData, vector<VECTOR3>& vData, VECTOR3 coeff);
	// the volume of cell
	float cellVolume(int cellId);
	// whether in a cell
	bool isInCell(PointInfo& pInfo, const int cellId);
	// cell type
	CellType GetCellType(void) {return CUBE;}
	// whether a cell is on boundary
	bool isCellOnBoundary(int cellId);

	//////////////////////////////////////////////////////////////////////////
	// boundary
	//////////////////////////////////////////////////////////////////////////
	
	// compute bounding box
	void ComputeBBox(void);
	// set bounding box
	void SetBoundary(VECTOR3& minB, VECTOR3& maxB);
	// set region extents
	void SetRegionExtents(VECTOR3&minR, VECTOR3& maxR);
	// get min and maximal boundary
	void Boundary(VECTOR3& minB, VECTOR3& maxB);
	// whether the point is in the bounding box
	bool isInBBox(VECTOR3& pos);
	// Or in region (replaces above!)
	bool isInRegion(VECTOR3& pos);
	float GetGridSpacing(int cellId){ return gridSpacing; }
};

//////////////////////////////////////////////////////////////////////////
//
// curvilinear grid
//
//////////////////////////////////////////////////////////////////////////
class FLOW_API CurvilinearGrid : public Grid
{
private:
	int m_nDimension[3];				// dimension

public:
	// constructor and deconstructor
	CurvilinearGrid();
	virtual ~CurvilinearGrid(){}
	CurvilinearGrid(int xdim, int ydim, int zdim);
	

	// cell
	// cell type
	CellType GetCellType(void) {return POLYGONE;}

	// dimension
	void SetDimension(int xdim, int ydim, int zdim) {m_nDimension[0] = xdim; m_nDimension[1] = ydim; m_nDimension[2] = zdim;}
	void GetDimension(int& xdim, int& ydim, int& zdim) {xdim = m_nDimension[0]; ydim = m_nDimension[1]; zdim = m_nDimension[2];}
};

};
#endif
