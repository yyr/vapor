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

#ifndef _SOLUTION_H_
#define _SOLUTION_H_

#include "header.h"
#include "VectorMatrix.h"
#include "Interpolator.h"

namespace VAPoR
{
class FLOW_API Solution : public VetsUtil::MyBase
{
private:
	VECTOR3** m_pDataArray;				// data value
	int m_nNodeNum;						// how many nodes each time step
	float m_fMinMag;					// minimal magnitude
	float m_fMaxMag;					// maximum magnitude

    int m_nTimeSteps;					// how many time steps in logic
	int m_nStartT;						// start timestep in physical space
	int m_nEndT;						// end timestep
	int m_nTimeIncrement;				// timesteps sampling rate
										// for example, if m_nTimeIncrement = 4, the T0 is data0, 
										// T1 is data4, and data between T0 an T1 is interpolated

public:
	// constructor
	Solution();
	Solution(VECTOR3** pData, int nodeNum, int timeSteps);
	~Solution();

	void Reset();

	// solution functions
	void SetValue(int t, VECTOR3* pData);
	bool isTimeVarying(void);
	int GetValue(int id, const float t, VECTOR3& nodeData);
	void Normalize(bool bLocal);
	void SetTime(int startT, int endT, int timeInc) { m_nStartT = startT; m_nEndT = endT; m_nTimeIncrement = timeInc; }
};
};
#endif