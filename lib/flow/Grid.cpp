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

CartesianGrid::CartesianGrid(int xdim, int ydim, int zdim, bool xperiodic, bool yperiodic, bool zperiodic)
{
	m_nDimension[0] = xdim;
	m_nDimension[1] = ydim;
	m_nDimension[2] = zdim;
	periodicDim[0] = xperiodic;
	periodicDim[1] = yperiodic;
	periodicDim[2] = zperiodic;
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
// set bounding box
//////////////////////////////////////////////////////////////////////////
void CartesianGrid::SetBoundary(VECTOR3& minB, VECTOR3& maxB)
{
	m_vMinBound = minB;
	m_vMaxBound = maxB;
	mappingFactorX = (float)(xdim()-1)/(m_vMaxBound[0] - m_vMinBound[0]);
	mappingFactorY = (float)(ydim()-1)/(m_vMaxBound[1] - m_vMinBound[1]);
	mappingFactorZ = (float)(zdim()-1)/(m_vMaxBound[2] - m_vMinBound[2]);
	oneOvermappingFactorX = (m_vMaxBound[0] - m_vMinBound[0])/(float)(xdim()-1);
	oneOvermappingFactorY = (m_vMaxBound[1] - m_vMinBound[1])/(float)(ydim()-1);
	oneOvermappingFactorZ = (m_vMaxBound[2] - m_vMinBound[2])/(float)(zdim()-1);
	// grid spacing
	gridSpacing = min(min(oneOvermappingFactorX, oneOvermappingFactorY), oneOvermappingFactorZ);
}
//////////////////////////////////////////////////////////////////////////
// set region bounds
//////////////////////////////////////////////////////////////////////////
void CartesianGrid::SetRegionExtents(VECTOR3& minR, VECTOR3& maxR)
{
	m_vMinRegBound = minR;
	m_vMaxRegBound = maxR;
	
}

void CartesianGrid::Boundary(VECTOR3& minB, VECTOR3& maxB)
{
	minB = m_vMinBound;
	maxB = m_vMaxBound;
}

//////////////////////////////////////////////////////////////////////////
// whether the physical point pos is in bounding box
//////////////////////////////////////////////////////////////////////////
bool CartesianGrid::isInBBox(VECTOR3& pos)
{
	if( (pos[0] >= m_vMinBound[0]) && (pos[0] <= m_vMaxBound[0]) &&
		(pos[1] >= m_vMinBound[1]) && (pos[1] <= m_vMaxBound[1]) &&
		(pos[2] >= m_vMinBound[2]) && (pos[2] <= m_vMaxBound[2]))
		return true;
	else
		return false;
}
//////////////////////////////////////////////////////////////////////////
// test whether the physical point pos is in region.
// It's ok to be out, if that dimension is periodic
//////////////////////////////////////////////////////////////////////////
bool CartesianGrid::isInRegion(VECTOR3& pos)
{
	
	if( (periodicDim[0]||(pos[0] >= m_vMinRegBound[0]) && (pos[0] <= m_vMaxRegBound[0]) )&&
		(periodicDim[1]||(pos[1] >= m_vMinRegBound[1]) && (pos[1] <= m_vMaxRegBound[1]) )&&
		(periodicDim[2]||(pos[2] >= m_vMinRegBound[2]) && (pos[2] <= m_vMaxRegBound[2]))  )
		return true;
	else
		return false;
}

void CartesianGrid::ComputeBBox(void)
{
	VECTOR3 minB, maxB;

	minB.Set(0, 0, 0);
	maxB.Set(xdim()-1, ydim()-1, zdim()-1);

	SetBoundary(minB, maxB);
}

//////////////////////////////////////////////////////////////////////////
// whether a cell is on boundary
//////////////////////////////////////////////////////////////////////////
bool CartesianGrid::isCellOnBoundary(int cellId)
{
	return true;
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

//////////////////////////////////////////////////////////////////////////
// get vertex list of a cell
// input
//		cellId:		cell Id
//		cellType:	cell type
// output
//		vVertices: the vertex lis of the cell
//////////////////////////////////////////////////////////////////////////
int CartesianGrid::getCellVertices(int cellId, 
								   CellTopoType cellType, 
								   vector<int>& vVertices)
{
	int totalCell = xcelldim() * ycelldim() * zcelldim();
	int xidx, yidx, zidx, index;

	if((cellId < 0) || (cellId >= totalCell))
		return -1;

	vVertices.clear();
	zidx = cellId / (xcelldim() * ycelldim());
	yidx = cellId % (xcelldim() * ycelldim());
	yidx = yidx / xcelldim();
	xidx = cellId - zidx * xcelldim() * ycelldim() - yidx * xcelldim();

	for(int kFor = 0; kFor < 2; kFor++)
		for(int jFor = 0; jFor < 2; jFor++)
            for(int iFor = 0; iFor < 2; iFor++)
			{
				index = (zidx+kFor) * ydim() * xdim() + (yidx + jFor) * xdim() + (xidx + iFor);
				vVertices.push_back(index);
			}

	return 1;
}

//////////////////////////////////////////////////////////////////////////
// get the physical coordinate of the vertex
//
// input:
// verIdx: index of vertex
// output:
// pos: physical coordinate of vertex
//////////////////////////////////////////////////////////////////////////
bool CartesianGrid::at_vertex(int verIdx, VECTOR3& pos)
{
	int xidx, yidx, zidx;
    int totalVer = xdim() * ydim() * zdim();
	if((verIdx < 0) || (verIdx >= totalVer))
		return false;

	zidx = verIdx / (xdim() * ydim());
	yidx = verIdx % (xdim() * ydim());
	yidx = verIdx / xdim();
	xidx = verIdx - zidx * xdim() * ydim() - yidx * xdim();

	pos.Set((float)xidx, (float)yidx, (float)zidx);
	return true;
}

//////////////////////////////////////////////////////////////////////////
// whether the point in the physical position is in the cell
//
// input:
// phyCoord:	physical position
// interpolant:	interpolation coefficients
// output:
// pInfo.interpolant: interpolation coefficient
// return:		returns 1 if in cell
//////////////////////////////////////////////////////////////////////////
bool CartesianGrid::isInCell(PointInfo& pInfo, const int cellId)
{
	
	if(!isInRegion(pInfo.phyCoord))
		return false;

	int xidx, yidx, zidx;
	VECTOR3 compVec;		// position in computational space
	compVec.Set(UCGridPhy2Comp(pInfo.phyCoord[0], m_vMinBound[0], mappingFactorX), 
				UCGridPhy2Comp(pInfo.phyCoord[1], m_vMinBound[1], mappingFactorY),
				UCGridPhy2Comp(pInfo.phyCoord[2], m_vMinBound[2], mappingFactorZ));

	xidx = (int)floor(compVec[0]);
	yidx = (int)floor(compVec[1]);
	zidx = (int)floor(compVec[2]);

	int inCell = zidx * ycelldim() * xcelldim() + yidx * xcelldim() + xidx;
	if(cellId == inCell)
	{
		pInfo.interpolant.Set(compVec[0] - (float)xidx, compVec[1] - (float)yidx, compVec[2] - (float)zidx);
		return true;
	}
	else
		return false;
}

//////////////////////////////////////////////////////////////////////////
// get the cell id and also interpolating coefficients for the given
// physical position
//
// input:
// phyCoord:	physical position
// interpolant:	interpolation coefficients ?? not an input ??
// fromCell:	if -1, initial point; otherwise this point is advected from others
// output:
// pInfo.inCell: in which cell
// pInfo.interpolant: interpolation coefficients
// return:	returns 1 if successful; otherwise returns -1
//////////////////////////////////////////////////////////////////////////
int CartesianGrid::phys_to_cell(PointInfo& pInfo)
{
	int xidx, yidx, zidx;

	
	if(!isInRegion(pInfo.phyCoord))
		return -1;

	//Convert physical coords if boundaries are periodic:
	float realPhyCoord[3];
	for (int i = 0; i<3; i++){
		realPhyCoord[i] = pInfo.phyCoord[i];
		if (periodicDim[i]){
			while (realPhyCoord[i] < m_vMinBound[i]) realPhyCoord[i] += (m_vMaxBound[i]-m_vMinBound[i]);
			while (realPhyCoord[i] > m_vMaxBound[i]) realPhyCoord[i] -= (m_vMaxBound[i]-m_vMinBound[i]);
		}
	}

	VECTOR3 compVec;		// position in computational space
	compVec.Set(UCGridPhy2Comp(realPhyCoord[0], m_vMinBound[0], mappingFactorX), 
				UCGridPhy2Comp(realPhyCoord[1], m_vMinBound[1], mappingFactorY),
				UCGridPhy2Comp(realPhyCoord[2], m_vMinBound[2], mappingFactorZ));

	xidx = (int)floor(compVec[0]);
	yidx = (int)floor(compVec[1]);
	zidx = (int)floor(compVec[2]);

	int inCell = zidx * ycelldim() * xcelldim() + yidx * xcelldim() + xidx;

	pInfo.inCell = inCell;
	pInfo.interpolant.Set(compVec[0] - (float)xidx, compVec[1] - (float)yidx, compVec[2] - (float)zidx);
	return 1;
}

//////////////////////////////////////////////////////////////////////////
// barycentric interpolation
// input
// nodeData:	8 corners of cube cell
// coeff:		bilinear interpolation coefficents
// output
// vData:		output
//////////////////////////////////////////////////////////////////////////
void CartesianGrid::interpolate(VECTOR3& nodeData, 
								vector<VECTOR3>& vData,
								VECTOR3 coeff)
{
	float fCoeff[3];
	fCoeff[0] = coeff[0];
	fCoeff[1] = coeff[1];
	fCoeff[2] = coeff[2];

	for(int iFor = 0; iFor < 3; iFor++)
	{
		nodeData[iFor] = TriLerp(vData[0][iFor], vData[1][iFor], vData[2][iFor], vData[3][iFor],
								 vData[4][iFor], vData[5][iFor], vData[6][iFor], vData[7][iFor],
								 fCoeff);
	}
}

//////////////////////////////////////////////////////////////////////////
// get tetra volume
// input
// cellId:	which cell
// return the volume of this cell in physical space
//////////////////////////////////////////////////////////////////////////
float CartesianGrid::cellVolume(int cellId)
{
	float volume;
	
	volume = oneOvermappingFactorX * oneOvermappingFactorY * oneOvermappingFactorZ;
	
	return volume;
}
