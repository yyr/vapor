/*************************************************************************
*						OSU Flow Vector Field							 *
*																		 *
*																		 *
*	Created:	Han-Wei Shen, Liya Li									 *
*				The Ohio State University								 *
*	Date:		06/2005													 *
*																		 *
*	Vector Field: 3D Static or Time-Varying								 *
*************************************************************************/
#ifdef WIN32
#pragma warning(disable : 4251 4100)
#endif

#include "Field.h"

using namespace VetsUtil;
using namespace VAPoR;

//////////////////////////////////////////////////////////////////////////
//
// CVectorField class definition
//
//////////////////////////////////////////////////////////////////////////
CVectorField::CVectorField()
{
	Reset();
}

CVectorField::CVectorField(Grid* pGrid, Solution* pSolution, int timesteps)
{
	assert((pGrid != NULL) && (pSolution != NULL));
	m_pGrid = pGrid;
	m_pSolution = pSolution;
	m_nTimeSteps = timesteps;
	m_bIsNormalized = false;
}

CVectorField::~CVectorField()
{
	//printf("called CVectorField\n");
	if(m_pGrid != NULL)	delete m_pGrid;
	if(m_pSolution != NULL) delete m_pSolution;
}

void CVectorField::Reset(void)
{
	m_nTimeSteps = 0;
	m_pGrid = NULL;
	m_pSolution = NULL;
	m_bIsNormalized = false;
}

//////////////////////////////////////////////////////////////////////////
// whether field is time varying
//////////////////////////////////////////////////////////////////////////
bool CVectorField::isTimeVarying(void)
{
	return (m_nTimeSteps > 1);
}

//////////////////////////////////////////////////////////////////////////
// to obtain one or more field values at cell vertices
//
// input
//		cellId:			from which cell to obtain value
//		eCellTopoType:	the cell type
//		t:				which time step
// output
//		vNodeData:	include one or more node values
// return
//		1:			operation successful
//		-1:			operation fail
//////////////////////////////////////////////////////////////////////////
int CVectorField::at_cell(int cellId,
						  CellTopoType eCellTopoType,
						  const float t,
						  vector<VECTOR3>& vNodeData)
{
	VECTOR3 nodeData;
	vector<int> vVerIds;
	int iFor;

	vNodeData.clear();
	switch(eCellTopoType)
	{
	case T0_CELL:								// cellId is the node Id
		if(m_pSolution->GetValue(cellId, t, nodeData) == -1)
			return -1;
		vNodeData.push_back(nodeData);
		break;

	case T1_CELL:								// cellId is the edge Id
	case T2_CELL:								// cellId is the face id
	case T3_CELL:								// cellId is tetra or cube id
		if(m_pGrid->getCellVertices(cellId, eCellTopoType, vVerIds) == -1)
			return -1;

		for(iFor = 0; iFor < (int)vVerIds.size(); iFor++)
		{
			if(m_pSolution->GetValue(vVerIds[iFor], t, nodeData) == -1)
				return -1;
			vNodeData.push_back(nodeData);
		}
		break;

	default:
		return -1;
		break;
	}

	return 1;
}

//////////////////////////////////////////////////////////////////////////
// to obtain node data at the physical position pos in timestep t
//
// input
//		fromCell:	if not -1, which cell this position is generated from
//		pos:		physical position of node
//		t:			time step
// output
//		nodeData:	data value of this node
// return
//		1:			operation successful
//		-1:			operation fail
//////////////////////////////////////////////////////////////////////////
int CVectorField::at_phys(const int fromCell, 
						  VECTOR3& pos, 
						  PointInfo& pInfo,
						  const float t, VECTOR3& nodeData)
{
	vector<VECTOR3> vNodeData;
	
	// find the cell this position belongs to
	pInfo.Set(pos, pInfo.interpolant, fromCell, -1);
	if(m_pGrid->phys_to_cell(pInfo) == -1)
		return -1;

	// get vector field value at cell vertices
	if(at_cell(pInfo.inCell, T3_CELL, t, vNodeData) == -1)
		return -1;

	// interpolate in the cell
	m_pGrid->interpolate(nodeData, vNodeData, pInfo.interpolant);

	return 1;
}

//////////////////////////////////////////////////////////////////////////
// to obtain node data at the computational position (i, j, k) in time t
//
// input
//		(i, j, k):	position in computational space
//		t:			time step
// output
//		dataValue:	data value of this position
// return
//		1:			operation successful
//		-1:			operation fail
//////////////////////////////////////////////////////////////////////////
int CVectorField::at_comp(const int i, 
						  const int j, 
						  const int k, 
						  const float t, 
						  VECTOR3& dataValue)
{
	return 1;
}

//////////////////////////////////////////////////////////////////////////
// volume of a cell
//////////////////////////////////////////////////////////////////////////
float CVectorField::volume_of_cell(int cellId)
{
	return m_pGrid->cellVolume(cellId);
}

//////////////////////////////////////////////////////////////////////////
// to normalize the vector field
// input
// bLocal: whether to normalize in each timestep or through all timesteps.
//		   if locally, then divide the magnitude; if globally, then divide
//		   by the maximal magnitude through the whole field
//////////////////////////////////////////////////////////////////////////
bool CVectorField::IsNormalized()
{
	return m_bIsNormalized;
}

void CVectorField::NormalizeField(bool bLocal)
{
	if(IsNormalized() != true)
	{
		m_bIsNormalized = true;
		m_pSolution->Normalize(bLocal);
	}
}

//////////////////////////////////////////////////////////////////////////
// to get physical coordinate by interpolation
// input
// cellId: the known cell Id
// eCellTopoType: what kind of cell will be interpolated
// coeff: the interpolation coefficients
// output
// pos: the output physical coordinates
//////////////////////////////////////////////////////////////////////////
int CVectorField::lerp_phys_coord(int cellId, 
								  CellTopoType eCellTopoType, 
								  float* coeff, 
								  VECTOR3& pos)
{
	vector<int> vVertices;
	int iFor;
	VECTOR3 interpCoeff;
	vector<VECTOR3> vData;

	if(m_pGrid->getCellVertices(cellId, eCellTopoType, vVertices) == 0)
		return 0;

	switch(eCellTopoType)
	{
		case T0_CELL:								
			break;
		case T1_CELL:								
			break;
		case T2_CELL:						
			break;
		case T3_CELL:	
			vData.clear();
			for(iFor = 0; iFor < 4; iFor++)
			{
				VECTOR3 v;
				m_pGrid->at_vertex(vVertices[iFor], v);
				vData.push_back(v);
			}

			interpCoeff.Set(coeff[0], coeff[1], coeff[2]);
			m_pGrid->interpolate(pos, vData, interpCoeff);
			break;
		default:
			break;
	}
	return 1;
}

void CVectorField::getDimension(int& xdim, int& ydim, int& zdim)
{
	m_pGrid->GetDimension(xdim, ydim, zdim);
}

//////////////////////////////////////////////////////////////////////////
// get the physical position of vertex
//////////////////////////////////////////////////////////////////////////
int CVectorField::phys_at_ver(int verId, VECTOR3& pos)
{
	return m_pGrid->at_vertex(verId, pos);
}

//////////////////////////////////////////////////////////////////////////
// whether a cell is on the boundary of grid
//////////////////////////////////////////////////////////////////////////
bool CVectorField::isCellOnBoundary(int cellId)
{
	return m_pGrid->isCellOnBoundary(cellId);
}

float FieldData::getFieldMag(float point[3])
{
	VECTOR3 pos(point[0],point[1],point[2]);
	VECTOR3 vel;
	PointInfo pInfo;
	pInfo.Set(pos, pInfo.interpolant, -1, -1);
	if(pCartesianGrid->phys_to_cell(pInfo) == -1)
		return -1.f;

	int rc = pField->at_phys(-1, pInfo.phyCoord, pInfo, (float)timeStep, vel);
	if (rc < 0) 
		return -1.f;
	return vel.GetMag();
}
//This is initialized by the VaporFlow class
void FieldData::setup(CVectorField* fld, CartesianGrid* grd, 
					  float** xdata, float** ydata, float** zdata, int tstep){
	pField = fld; pCartesianGrid = grd; 
	pUData = xdata; pVData = ydata; pWData = zdata;
	timeStep = tstep;
}
FieldData::~FieldData(){
	delete pField;
	delete pUData;
	delete pVData;
	delete pWData;
}
