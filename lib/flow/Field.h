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

#ifndef _VECTOR_FIELD_H_
#define _VECTOR_FIELD_H_

#include "header.h"
#include "Grid.h"
#include "Solution.h"

//////////////////////////////////////////////////////////////////////////
// vector field class
//////////////////////////////////////////////////////////////////////////
namespace VAPoR
{
class FLOW_API CVectorField : public VetsUtil::MyBase
{
private:
	Grid* m_pGrid;						// grid
	Solution* m_pSolution;				// vector data
	int m_nTimeSteps;
	bool m_bIsNormalized;				// whether the solution is normalized or not
	
public:
	// constructor and deconstructor
	CVectorField();
	CVectorField(Grid* pGrid, Solution* pSolution, int timesteps);
	~CVectorField();

	// reset
	void Reset(void);

	// field functions
	bool isTimeVarying(void);
	int lerp_phys_coord(int cellId, CellTopoType eCellTopoType, float* coeff, VECTOR3& pos);
	int at_cell(int cellId, CellTopoType eCellTopoType, const float t, vector<VECTOR3>& vNodeData);
	int getCellVertices(int cellId, CellTopoType cellType, vector<int>& vVertices) { return m_pGrid->getCellVertices(cellId, cellType, vVertices); }
	int at_phys(const int fromCell, VECTOR3& pos, PointInfo& pInfo,const float t, VECTOR3& nodeData);
	int phys_at_ver(int verId, VECTOR3& pos);
	int at_comp(const int i, const int j, const int k, const float t, VECTOR3& dataValue);
	float volume_of_cell(int cellId);
	void NormalizeField(bool bLocal);
	bool IsNormalized(void);
	void getDimension(int& xdim, int& ydim, int& zdim);
	int GetTimeSteps(void) {return m_nTimeSteps;}
	CellType GetCellType(void) {return m_pGrid->GetCellType();}
	void Boundary(VECTOR3& minB, VECTOR3& maxB) { m_pGrid->Boundary(minB, maxB); }
	bool isCellOnBoundary(int cellId);
	float GetGridSpacing(int cellId){return m_pGrid->GetGridSpacing(cellId);}
	void SetSolutionData(int t, float* pUData, float* pVData, float* pWData) {m_pSolution->SetValue(t, pUData, pVData, pWData);}
	float* GetSolutionData(int t) {return m_pSolution->GetUValue(t);}
};
};
//////////////////////////////////////////////////////////////////////////
// scalar field class
//////////////////////////////////////////////////////////////////////////

#endif