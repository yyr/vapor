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
#include <vapor/DataMgr.h>

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
	
public:
	// constructor and deconstructor
	CVectorField();
	CVectorField(Grid* pGrid, Solution* pSolution, int timesteps);
	~CVectorField();

	// reset
	void Reset(void);

	// field functions
	bool isTimeVarying(void);
	
	int getFieldValue(VECTOR3& pos, const float t, VECTOR3& fieldData);
	bool is_in_grid(VECTOR3& pos) {return (m_pGrid->isInRegion(pos));}
	int at_comp(const int i, const int j, const int k, const float t, VECTOR3& dataValue);
	
	
	void getDimension(int& xdim, int& ydim, int& zdim);
	int GetTimeSteps(void) {return m_nTimeSteps;}
	
	void SetSolutionGrid(int t, RegularGrid** pUGrid, RegularGrid** pVGrid, RegularGrid** pWGrid, bool periodicDims[3]) {m_pSolution->SetGrid(t, *pUGrid, *pVGrid, *pWGrid, periodicDims);}
	void ClearSolutionGrid(int t) {m_pSolution->SetGrid(t, 0, 0, 0, 0);}
	
	int GetStartTime(void) { return m_pSolution->GetStartTime(); }
	int GetEndTime(void) { return m_pSolution->GetEndTime(); }
	void SetUserTimeStepInc(float timeInc) { m_pSolution->SetUserTimeStepInc(timeInc); }
	void SetUserTimeSteps(float* pUserTimeSteps) { m_pSolution->SetUserTimeSteps(pUserTimeSteps); }
	void SetUserTimePerVaporTS(float val){m_pSolution->SetUserTimePerVaporTS(val);}
	float getTimeScaleFactor(){return m_pSolution->GetTimeScaleFactor();}
	
	float GetUserTimePerVaporTS() { return m_pSolution->GetUserTimePerVaporTS();}
	float GetMinCellVolume() { return m_pGrid->GetMinCellVolume();}

};
//Helper class to get data out of a vector field
class FLOW_API FieldData {
public:
	//Create a FieldData, with specified field variables, at a specified time step
	//This is created by VaporFlow, because it has region info
	~FieldData();
	
	//This is initialized by the VaporFlow class
	void setup(CVectorField* fld, CartesianGrid* grd, 
		RegularGrid **xgrid, RegularGrid** ygrid, RegularGrid** zgrid, int tstep, bool periodicDims[3]);
	float getFieldMag(float point[3]);
	float getValidFieldMag(float point[3]); //Returns -2 if there is missing value, -1 if out of region
	void releaseData(DataMgr*);
private:
	//information kept in the FieldData: 
	int timeStep;
	CVectorField* pField;
	CartesianGrid* pCartesianGrid;
	
	RegularGrid** pUGrid, **pVGrid, **pWGrid;
};
};//end VAPoR namespace
//////////////////////////////////////////////////////////////////////////
// scalar field class
//////////////////////////////////////////////////////////////////////////

#endif
