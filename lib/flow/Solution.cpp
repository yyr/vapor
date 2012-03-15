/*************************************************************************
*						OSU Flow Vector Field							 *
*																		 *
*																		 *
*	Created:	Han-Wei Shen, Liya Li									 *
*				The Ohio State University								 *
*	Date:		06/2005													 *
*																		 *
*	Vector Field Data													 *
*************************************************************************/
#ifdef WIN32
#pragma warning(disable : 4251 4100)
#endif

#include "Solution.h"

using namespace VetsUtil;
using namespace VAPoR;
//////////////////////////////////////////////////////////////////////////
//
// definition of CSolution class
//
//////////////////////////////////////////////////////////////////////////

Solution::Solution()
{
	Reset();
}


Solution::Solution(RegularGrid** xGrid, RegularGrid** yGrid, RegularGrid** zGrid,
				  int timeSteps)
{
	m_pUGrid = xGrid;
	m_pVGrid = yGrid;
	m_pWGrid = zGrid;
	m_nTimeSteps = timeSteps;		
	m_fTimeScaleFactor = 1.0;
	m_nTimeIncrement = 1;
	m_nUserTimeStepInc = 0;
	m_fUserTimePerVaporTS = 1.f;
	m_pUserTimeSteps = NULL;
	m_TimeDir = FORWARD;
}
Solution::~Solution()
{
	m_pUserTimeSteps = NULL;
}

void Solution::Reset()
{
	
	m_nNodeNum = 0;
	m_nTimeSteps = 1;		
	m_fTimeScaleFactor = 1.0;
	m_nStartT = 0;
	m_nEndT = 0;
	m_nTimeIncrement = 1;
	m_nUserTimeStepInc = 0;
	m_fUserTimePerVaporTS = 1.f;
	m_pUserTimeSteps = NULL;
}


//////////////////////////////////////////////////////////////////////////
// change vector data on-the-fly
//////////////////////////////////////////////////////////////////////////
void Solution::SetGrid(int t, RegularGrid* pUGrid, RegularGrid* pVGrid, RegularGrid* pWGrid)
{
	if((t >= 0) && (t < m_nTimeSteps))
	{
		//Test before assigning; null pointer indicates zero data:
		if(m_pUGrid) m_pUGrid[t] = pUGrid;
		if(m_pVGrid) m_pVGrid[t] = pVGrid;
		if(m_pWGrid) m_pWGrid[t] = pWGrid;
	}
}
int Solution::getFieldValue(VECTOR3& point,const float t,  VECTOR3& fieldVal){
	float uVal = 0.f, vVal = 0.f, wVal = 0.f;
	double xval = point.x();
	double yval = point.y();
	double zval = point.z();
	if(m_TimeDir == FORWARD && ( (t < m_nStartT) || (t > m_nEndT)))		// time is valid
		return -1;
	if(m_TimeDir == BACKWARD && ( (t > m_nStartT) || (t < m_nEndT)))		// time is valid
		return -1;

	if(!isTimeVarying()){
		float uVal = 0.f;
		int tindex =   (int)(t - m_nStartT);
		if (m_pUGrid[tindex]) {
			uVal = m_pUGrid[tindex]->GetValue(xval, yval,zval);
			if (uVal == m_pUGrid[tindex]->GetMissingValue()) uVal = 0.f;
		}
		float vVal = 0.f;
		if (m_pVGrid[tindex]) {
			vVal = m_pVGrid[tindex]->GetValue(xval, yval,zval);
			if (vVal == m_pVGrid[tindex]->GetMissingValue()) vVal = 0.f;
		}
		float wVal = 0.f;
		if (m_pWGrid[tindex]) {
			wVal = m_pWGrid[tindex]->GetValue(xval, yval,zval);
			if (wVal == m_pWGrid[tindex]->GetMissingValue()) wVal = 0.f;
		}
		fieldVal.Set(uVal*m_fTimeScaleFactor,vVal*m_fTimeScaleFactor,wVal*m_fTimeScaleFactor);
	}
	else
	{
		int lowT, hiT;
		float ratio = 0.0;
		float offset;
		offset = ((t - (float)m_nStartT)*m_TimeDir)/(float)m_nTimeIncrement;
		assert(offset >= 0.0f && offset <= 1.0f);
		lowT = (int)floor(offset);
		if (lowT == 1) lowT = 0;
		
		hiT = lowT + 1;
		
		ratio = offset;
		
		float lowU = m_pUGrid[lowT] ? m_pUGrid[lowT]->GetValue(xval, yval,zval) : 0.f;
		if (lowU == m_pUGrid[lowT]->GetMissingValue()) lowU = 0.f;
		float lowV = m_pVGrid[lowT] ? m_pVGrid[lowT]->GetValue(xval, yval,zval) : 0.f;
		if (lowV == m_pVGrid[lowT]->GetMissingValue()) lowV = 0.f;
		float lowW = m_pWGrid[lowT] ? m_pWGrid[lowT]->GetValue(xval, yval,zval) : 0.f;
		if (lowW == m_pWGrid[lowT]->GetMissingValue()) lowW = 0.f;
		if(ratio == 0.0)
			fieldVal.Set(lowU*m_fTimeScaleFactor*m_fUserTimePerVaporTS, 
						 lowV*m_fTimeScaleFactor*m_fUserTimePerVaporTS, 
						 lowW*m_fTimeScaleFactor*m_fUserTimePerVaporTS);
		else{
			float hiU = m_pUGrid[hiT] ? m_pUGrid[hiT]->GetValue(xval, yval,zval) : 0.f;
			if (hiU == m_pUGrid[hiT]->GetMissingValue()) hiU = 0.f;
			float hiV = m_pVGrid[hiT] ? m_pVGrid[hiT]->GetValue(xval, yval,zval) : 0.f;
			if (hiV == m_pVGrid[hiT]->GetMissingValue()) hiV = 0.f;
			float hiW = m_pWGrid[hiT] ? m_pWGrid[hiT]->GetValue(xval, yval,zval) : 0.f;
			if (hiW == m_pWGrid[hiT]->GetMissingValue()) hiW = 0.f;
            fieldVal.Set(m_fTimeScaleFactor*Lerp(lowU,hiU, ratio)*m_fUserTimePerVaporTS, 
						 m_fTimeScaleFactor*Lerp(lowV,hiV, ratio)*m_fUserTimePerVaporTS,
						 m_fTimeScaleFactor*Lerp(lowW,hiW, ratio)*m_fUserTimePerVaporTS);
	
		}
	}
	return 1;
	
}
//////////////////////////////////////////////////////////////////////////
// whether field is time varying
//////////////////////////////////////////////////////////////////////////
bool Solution::isTimeVarying(void)
{
	return (m_nTimeSteps > 1);
}




void Solution::getMinGridSpacing(int timestep, double mincell[3]){
	mincell[0] = mincell[1] = mincell[2] = 1.;
	if (m_pUGrid[timestep]) m_pUGrid[timestep]->GetMinCellExtents(mincell, mincell+1, mincell+2);
	else if (m_pVGrid[timestep]) m_pVGrid[timestep]->GetMinCellExtents(mincell, mincell+1, mincell+2);
	else if (m_pWGrid[timestep]) m_pWGrid[timestep]->GetMinCellExtents(mincell, mincell+1, mincell+2);
	else assert(0);
	return;
}
