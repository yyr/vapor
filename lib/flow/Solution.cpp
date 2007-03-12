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

//////////////////////////////////////////////////////////////////////////
// input
//		pData:		this is a 2D array for storing static or 
//					time-varying data
//		nodeNum:	number of total nodes
//		timeSteps:	how many time steps, for static data, this is 1
//////////////////////////////////////////////////////////////////////////
Solution::Solution(float** pUData, float** pVData, float** pWData,
				   int nodeNum, int timeSteps)
{
	m_pUDataArray = pUData;
	m_pVDataArray = pVData;
	m_pWDataArray = pWData;
	m_nNodeNum = nodeNum;
	m_nTimeSteps = timeSteps;		
	m_fTimeScaleFactor = 1.0;
	m_nTimeIncrement = 1;
	m_nUserTimeStepInc = 0;
	m_nUserTimeStep = 1;
	m_pUserTimeSteps = NULL;
}

Solution::~Solution()
{
	m_pUserTimeSteps = NULL;
}

void Solution::Reset()
{
	m_pUDataArray = NULL;
	m_pVDataArray = NULL;
	m_pWDataArray = NULL;
	m_nNodeNum = 0;
	m_nTimeSteps = 1;		
	m_fTimeScaleFactor = 1.0;
	m_nStartT = 0;
	m_nEndT = 0;
	m_nTimeIncrement = 1;
	m_nUserTimeStepInc = 0;
	m_nUserTimeStep = 1;
	m_pUserTimeSteps = NULL;
}

//////////////////////////////////////////////////////////////////////////
// change vector data on-the-fly
//////////////////////////////////////////////////////////////////////////
void Solution::SetValue(int t, float* pUData, float* pVData, float* pWData)
{
	if((t >= 0) && (t < m_nTimeSteps))
	{
		m_pUDataArray[t] = pUData;
		m_pVDataArray[t] = pVData;
		m_pWDataArray[t] = pWData;
	}
}

//////////////////////////////////////////////////////////////////////////
// whether field is time varying
//////////////////////////////////////////////////////////////////////////
bool Solution::isTimeVarying(void)
{
	return (m_nTimeSteps > 1);
}

//////////////////////////////////////////////////////////////////////////
// get value of node id at time step t
// input
//		id:			node Id
//		t:			time step in check
// output
//		nodeData:	vector value at this node
// return
//		1:			operation successful
//		-1:			invalid id
//////////////////////////////////////////////////////////////////////////
int Solution::GetValue(int id, float t, VECTOR3& nodeData)
{
	if((id < 0) || (id >= m_nNodeNum))			// id is valid?
		return -1;
	if(m_TimeDir == FORWARD && ( (t < m_nStartT) || (t > m_nEndT)))		// time is valid
		return -1;
	if(m_TimeDir == BACKWARD && ( (t > m_nStartT) || (t < m_nEndT)))		// time is valid
		return -1;

	if(!isTimeVarying())
		nodeData.Set(m_pUDataArray[(int)(t - m_nStartT)][id]*m_fTimeScaleFactor, 
					 m_pVDataArray[(int)(t - m_nStartT)][id]*m_fTimeScaleFactor, 
					 m_pWDataArray[(int)(t - m_nStartT)][id]*m_fTimeScaleFactor);
	else
	{
		int lowT, highT;
		float ratio = 0.0;
		float offset;
		offset = ((t - (float)m_nStartT)*m_TimeDir)/(float)m_nTimeIncrement;
		assert(offset >= 0.0f);
		lowT = (int)floor(offset);
		highT = lowT + 1;
		if(offset != (float)lowT)
		{
			ratio = (m_nUserTimeStepInc + (t - (int)t)*m_nUserTimeStep)/(float)m_pUserTimeSteps[lowT];
			if (m_TimeDir == BACKWARD) ratio = 1.f - ratio;
		}
		else
			ratio = 0.0;

		if(lowT*m_TimeDir >= m_nEndT*m_TimeDir)
			ratio = 0.0;
		
		if(ratio == 0.0)
			nodeData.Set(m_pUDataArray[lowT][id]*m_fTimeScaleFactor, 
						 m_pVDataArray[lowT][id]*m_fTimeScaleFactor, 
						 m_pWDataArray[lowT][id]*m_fTimeScaleFactor);
		else
            nodeData.Set(m_fTimeScaleFactor*Lerp(m_pUDataArray[lowT][id], m_pUDataArray[highT][id], ratio), 
						 m_fTimeScaleFactor*Lerp(m_pVDataArray[lowT][id], m_pVDataArray[highT][id], ratio),
						 m_fTimeScaleFactor*Lerp(m_pWDataArray[lowT][id], m_pWDataArray[highT][id], ratio));
	}
	
	return 1;
}

//////////////////////////////////////////////////////////////////////////
// to normalize the vector field
// input
// bLocal: whether to normalize in each timestep or through all timesteps.
//		   if locally, then divide its magnitude; if globally, then divide
//		   by the maximal magnitude through the whole field
//////////////////////////////////////////////////////////////////////////
void Solution::Normalize(bool bLocal)
{
	int iFor, jFor;
	float mag = 0.f; 
	float u, v, w;

	m_fMinMag = FLT_MAX;
	m_fMaxMag = -FLT_MAX;

	if(bLocal)
	{
		for(iFor = 0; iFor < m_nTimeSteps; iFor++)
		{
			for(jFor = 0; jFor < m_nNodeNum; jFor++)
			{
				VECTOR3 tmpVec;
				tmpVec.Set(m_pUDataArray[iFor][jFor], m_pVDataArray[iFor][jFor], m_pWDataArray[iFor][jFor]);
				mag = tmpVec.GetMag();
				if(mag != 0.0)
				{
					u = m_pUDataArray[iFor][jFor]/mag;
					v = m_pVDataArray[iFor][jFor]/mag;
					w = m_pWDataArray[iFor][jFor]/mag;
					m_pUDataArray[iFor][jFor] = u;
					m_pVDataArray[iFor][jFor] = v;
					m_pWDataArray[iFor][jFor] = w;
				}
				
				if(mag < m_fMinMag)
					m_fMinMag = mag;
				if(mag > m_fMaxMag)
					m_fMaxMag = mag;
			}
		}
	}
	else
	{
		for(iFor = 0; iFor < m_nTimeSteps; iFor++)
		{
			for(jFor = 0; jFor < m_nNodeNum; jFor++)
			{
				VECTOR3 tmpVec;
				tmpVec.Set(m_pUDataArray[iFor][jFor], m_pVDataArray[iFor][jFor], m_pWDataArray[iFor][jFor]);
				mag = tmpVec.GetMag();
				if(mag < m_fMinMag)
					m_fMinMag = mag;
				if(mag > m_fMaxMag)
					m_fMaxMag = mag;
			}
		}

		for(iFor = 0; iFor < m_nTimeSteps; iFor++)
		{
			for(jFor = 0; jFor < m_nNodeNum; jFor++)
			{
				u = m_pUDataArray[iFor][jFor]/mag;
				v = m_pVDataArray[iFor][jFor]/mag;
				w = m_pWDataArray[iFor][jFor]/mag;
				m_pUDataArray[iFor][jFor] = u;
				m_pVDataArray[iFor][jFor] = v;
				m_pWDataArray[iFor][jFor] = w;
			}
		}
	}
}
