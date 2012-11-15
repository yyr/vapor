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
//	assert((pGrid != NULL) && (pSolution != NULL));
	m_pGrid = pGrid;
	m_pSolution = pSolution;
	m_nTimeSteps = timesteps;
	
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
	
}

//////////////////////////////////////////////////////////////////////////
// whether field is time varying
//////////////////////////////////////////////////////////////////////////
bool CVectorField::isTimeVarying(void)
{
	return (m_nTimeSteps > 1);
}




int CVectorField::getFieldValue(
						  VECTOR3& pos, 
						  const float t, 
						  VECTOR3& fieldValue)
{
	//Check if pos is in the region. if not return -1.
	if (!m_pGrid->isInRegion(pos)) return -1;
	//Obtain the values of the field variables from the RegularGrids in the Solution,
	//Put the result in fieldValue.
	//Return 2 if a component is missing_value
	return m_pSolution->getFieldValue(pos, t, fieldValue);

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






void CVectorField::getDimension(int& xdim, int& ydim, int& zdim)
{
	m_pGrid->GetDimension(xdim, ydim, zdim);
}



float FieldData::getFieldMag(float point[3])
{
	VECTOR3 pos(point[0],point[1],point[2]);
	VECTOR3 vel;

	int rc = pField->getFieldValue( pos, (float)timeStep, vel);
	if (rc < 0) 
		return -1.f;

	return vel.GetMag();
}

float FieldData::getValidFieldMag(float point[3])
{
	VECTOR3 pos(point[0],point[1],point[2]);
	VECTOR3 vel;

	int rc = pField->getFieldValue( pos, (float)timeStep, vel);
	if (rc < 0) 
		return (float)rc;
	if (rc == 2) return -2.;

	return vel.GetMag();
}

//This is initialized by the VaporFlow class.  Just holds some
//pointers until the field data access is complete
void FieldData::setup(CVectorField* fld, CartesianGrid* grd, 
					  RegularGrid** xGrid, RegularGrid** yGrid, RegularGrid** zGrid, int tstep, 
					  bool periodicDims[3]){
	pField = fld; pCartesianGrid = grd; 
	pUGrid = xGrid; pVGrid = yGrid; pWGrid = zGrid;
	pField->SetSolutionGrid(0, pUGrid, pVGrid, pWGrid, periodicDims);
	timeStep = tstep;
}
//The destructor unlocks the da
FieldData::~FieldData(){
	delete pField;

	if (pUGrid){
		delete [] pUGrid;
	}
	if (pVGrid){
		delete [] pVGrid;
	}
	if (pWGrid){
		delete [] pWGrid;
	}
}
void FieldData::releaseData(DataMgr* dataMgr){
	if (pUGrid){
		dataMgr->UnlockGrid(*pUGrid);
		delete pUGrid;
		pUGrid = 0;
	}
	if (pVGrid){
		dataMgr->UnlockGrid(*pVGrid);
		delete pVGrid;
		pVGrid = 0;
	}
	if (pWGrid){
		dataMgr->UnlockGrid(*pWGrid);
		delete pWGrid;
		pWGrid = 0;
	}
}
