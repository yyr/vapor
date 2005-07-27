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
Solution::Solution(VECTOR3** pData, int nodeNum, int timeSteps)
{
	assert((pData != NULL) && (nodeNum > 0) && (timeSteps > 0));

	m_pDataArray = pData;
	m_nNodeNum = nodeNum;
	m_nTimeSteps = timeSteps;			
}

Solution::~Solution()
{
	int iFor;

	if(m_pDataArray != NULL)
	{
		for(iFor = 0; iFor < m_nTimeSteps; iFor++)
			delete m_pDataArray[iFor];
		delete[] m_pDataArray;
	}
}

void Solution::Reset()
{
	m_pDataArray = NULL;
	m_nNodeNum = 0;
	m_nTimeSteps = 1;		
	m_nStartT = 0;
	m_nEndT = 0;
	m_nTimeIncrement = 0;
}

//////////////////////////////////////////////////////////////////////////
// change vector data on-the-fly
//////////////////////////////////////////////////////////////////////////
void Solution::SetValue(int t, VECTOR3* pData)
{
	if((t > 0) && (t < m_nTimeSteps))
		m_pDataArray[t] = pData;
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
	if( (t < m_nStartT) || (t > m_nEndT))		// time is valid
		return -1;

	if(!isTimeVarying())
		nodeData = m_pDataArray[(int)t][id];
	else
	{
		int lowT, highT;
		float ratio;
		float offset;
		offset = (t - (float)m_nStartT)/(float)m_nTimeIncrement;
		lowT = (int)floor(offset);
		ratio = t - (float)floor(t);
		highT = lowT + 1;
		if(lowT >= (m_nTimeSteps-1))
		{
			highT = lowT;
			ratio = 0.0;
		}
		nodeData.Set(Lerp(m_pDataArray[lowT][id][0], m_pDataArray[highT][id][0], ratio), 
					 Lerp(m_pDataArray[lowT][id][1], m_pDataArray[highT][id][1], ratio),
					 Lerp(m_pDataArray[lowT][id][2], m_pDataArray[highT][id][2], ratio));
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
	float mag, u, v, w;

	m_fMinMag = FLT_MAX;
	m_fMaxMag = -FLT_MAX;

	if(bLocal)
	{
		for(iFor = 0; iFor < m_nTimeSteps; iFor++)
		{
			for(jFor = 0; jFor < m_nNodeNum; jFor++)
			{
				mag = m_pDataArray[iFor][jFor].GetMag();
				if(mag != 0.0)
				{
					u = m_pDataArray[iFor][jFor][0]/mag;
					v = m_pDataArray[iFor][jFor][1]/mag;
					w = m_pDataArray[iFor][jFor][2]/mag;
					m_pDataArray[iFor][jFor].Set(u, v, w);
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
				mag = m_pDataArray[iFor][jFor].GetMag();
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
				u = m_pDataArray[iFor][jFor][0]/m_fMaxMag;
				v = m_pDataArray[iFor][jFor][1]/m_fMaxMag;
				w = m_pDataArray[iFor][jFor][2]/m_fMaxMag;
				m_pDataArray[iFor][jFor].Set(u, v, w);
			}
		}
	}
}