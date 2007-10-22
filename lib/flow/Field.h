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
#include "vapor/DataMgr.h"

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
	bool is_in_grid(VECTOR3& pos) {return (m_pGrid->isInRegion(pos));}
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
	
	int GetStartTime(void) { return m_pSolution->GetStartTime(); }
	int GetEndTime(void) { return m_pSolution->GetEndTime(); }
	void SetUserTimeStepInc(float timeInc) { m_pSolution->SetUserTimeStepInc(timeInc); }
	void SetUserTimeSteps(float* pUserTimeSteps) { m_pSolution->SetUserTimeSteps(pUserTimeSteps); }
	void SetUserTimePerVaporTS(float val){m_pSolution->SetUserTimePerVaporTS(val);}
	float getTimeScaleFactor(){return m_pSolution->GetTimeScaleFactor();}
	//float GetCurUserTimeStep() {return m_pSolution->GetCurUserTimeStep();}
	float GetUserTimePerVaporTS() { return m_pSolution->GetUserTimePerVaporTS();}
	//int GetTimeIncrement(void) { return m_pSolution->GetTimeIncrement(); }
};
//Helper class to get data out of a vector field
class FLOW_API FieldData {
public:
	//Create a FieldData, with specified field variables, at a specified time step
	//This is created by VaporFlow, because it has region info
	~FieldData();
	//This is initialized by the VaporFlow class
	void setup(CVectorField* fld, CartesianGrid* grd, 
		float** xdata, float** ydata, float** zdata, int tstep);
		
	float getFieldMag(float point[3]);
	void releaseData(DataMgr*);
private:
	//information kept in the FieldData: 
	int timeStep;
	CVectorField* pField;
	CartesianGrid* pCartesianGrid;
	float** pUData;
	float** pVData;
	float** pWData;
};
};//end VAPoR namespace
//////////////////////////////////////////////////////////////////////////
// scalar field class
//////////////////////////////////////////////////////////////////////////

#endif
