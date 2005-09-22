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
	float** m_pUDataArray;				// U data value
	float** m_pVDataArray;				// V data value
	float** m_pWDataArray;				// W data value
	int m_nNodeNum;						// how many nodes each time step
	float m_fMinMag;					// minimal magnitude
	float m_fMaxMag;					// maximum magnitude
	float m_fTimeScaleFactor;			

    int m_nTimeSteps;					// how many time steps in logic
	int m_nStartT;						// start timestep in physical space
	int m_nEndT;						// end timestep
	int m_nTimeIncrement;				// timesteps sampling rate
										// for example, if m_nTimeIncrement = 4, the T0 is data0, 
										// T1 is data4, and data between T0 an T1 is interpolated
	int m_nUserTimeStepInc;				// usertimestep between previous time step and previous
										// sampled time step
	int m_nUserTimeStep;				// usertimestep between current time step and next time step
	int* m_pUserTimeSteps;				// time step increment between two sampled time steps

public:
	// constructor
	Solution();
	Solution(float** pUData, float** pVData, float** pWData, int nodeNum, int timeSteps);
	~Solution();

	void Reset();

	// solution functions
	void SetValue(int t, float* pUData, float* pVData, float* pWData);
	float* GetUValue(int t) {return m_pUDataArray[t];};
	float* GetVValue(int t) {return m_pVDataArray[t];};
	float* GetWValue(int t) {return m_pWDataArray[t];};
	bool isTimeVarying(void);
	int GetValue(int id, const float t, VECTOR3& nodeData);
	void Normalize(bool bLocal);
	void SetTime(int startT, int endT) { m_nStartT = startT; m_nEndT = endT;}
	int GetStartTime(void) {return m_nStartT;}
	int GetEndTime(void) {return m_nEndT;}
	void SetTimeIncrement(int timeInc) { m_nTimeIncrement = timeInc; }
	int GetTimeIncrement(void) { return m_nTimeIncrement; }
	void SetUserTimeStepInc(int timeInc, int curTimeInc) { m_nUserTimeStepInc = timeInc; m_nUserTimeStep = curTimeInc; }
	void SetUserTimeSteps(int* pUserTimeSteps) { m_pUserTimeSteps = pUserTimeSteps; }
	void SetTimeScaleFactor(float timeScaleFactor) { m_fTimeScaleFactor = timeScaleFactor; }
};
};
#endif
