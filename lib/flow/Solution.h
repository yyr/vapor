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
#include "vapor/RegularGrid.h"

namespace VAPoR
{
enum TIME_DIR{ BACKWARD = -1, FORWARD = 1};		// advection direction
class FLOW_API Solution : public VetsUtil::MyBase
{
private:
	
	RegularGrid** m_pUGrid;
	RegularGrid** m_pVGrid;
	RegularGrid** m_pWGrid;
	int m_nNodeNum;						// how many nodes each time step
	float m_fMinMag;					// minimal magnitude
	float m_fMaxMag;					// maximum magnitude
	float m_fTimeScaleFactor;			

    int m_nTimeSteps;					// how many time steps in logic
	int m_nStartT;						// start timestep in physical space
	int m_nEndT;						// end timestep
	int m_nTimeIncrement;				// time steps between sampled time steps
										
	float m_nUserTimeStepInc;				// usertimestep between previous time step and previous
										// sampled time step
	//float m_nUserTimeStep;				// usertimestep between current time step and next time step
										// divided by the number of time steps!!!
	float* m_pUserTimeSteps;				// time step increment between two sampled time steps
	TIME_DIR m_TimeDir;			// time direction forward or backwards
	float m_fUserTimePerVaporTS;    //User time per vapor time step.

public:
	// constructor
	Solution();
	
	Solution(RegularGrid** uGrid, RegularGrid** vGrid, RegularGrid** wGrid,int timeSteps, bool periodicity[3]);
	~Solution();

	void Reset();

	// solution functions
	
	void SetGrid(int t, RegularGrid* pUData, RegularGrid* pVData, RegularGrid* pWData, bool periodicDim[3]);
	int getFieldValue(VECTOR3& point,const float t,  VECTOR3& fieldVal);
	
	bool isTimeVarying(void);
	int GetValue(int id, const float t, VECTOR3& nodeData);
	
	void SetTime(int startT, int endT) { m_nStartT = startT; m_nEndT = endT;}
	int GetStartTime(void) {return m_nStartT;}
	int GetEndTime(void) {return m_nEndT;}
	void SetTimeIncrement(int timeInc, VAPoR::TIME_DIR isForward) { m_nTimeIncrement = timeInc; m_TimeDir = isForward; }
	
	void SetUserTimeStepInc(float timeInc) { m_nUserTimeStepInc = timeInc; }
	void SetUserTimeSteps(float* pUserTimeSteps) { m_pUserTimeSteps = pUserTimeSteps; }
	void SetTimeScaleFactor(float timeScaleFactor) { m_fTimeScaleFactor = timeScaleFactor; }
	void SetUserTimePerVaporTS(float val) {m_fUserTimePerVaporTS = val;}
	float GetTimeScaleFactor() {return m_fTimeScaleFactor;}
	
	float GetUserTimePerVaporTS() {return m_fUserTimePerVaporTS;}
	void getMinGridSpacing(int timestep, double mincell[3]);
};
};
#endif
