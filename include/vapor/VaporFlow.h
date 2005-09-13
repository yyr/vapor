//	File:		VaporFlow.h
//
//	Author:		Liya Li
//
//	Date:		July 2005
//
//	Description:	Definition of VaporFlow class. It contains the interface
//					between gui and underlying flow functions.
//

#ifndef	_VaporFlow_h_
#define	_VaporFlow_h_

#include "vapor/DataMgr.h"
#include "vapor/MyBase.h"
#include "vaporinternal/common.h"

#define END_FLOW_FLAG 1.e30f
#define STATIONARY_STREAM_FLAG -1.e30f

namespace VAPoR
{
	class VECTOR3;

	class FLOW_API VaporFlow : public VetsUtil::MyBase
	{
	public:
		// constructor and destructor
		VaporFlow(DataMgr* dm = NULL);
		~VaporFlow();
		void Reset(void);

		void SetFieldComponents(const char* xvar, const char* yvar, const char* zvar);
		void SetRegion(size_t num_xforms, const size_t min[3], const size_t max[3]);
		void SetTimeStepInterval(size_t startT, size_t endT, size_t tInc);
		void ScaleTimeStepSizes(double userTimeStepMultiplier, double animationTimeStepMultiplier);
		void SetRandomSeedPoints(const float min[3], const float max[3], int numSeeds);
		void SetRegularSeedPoints(const float min[3], const float max[3], const size_t numSeeds[3]);
		void SetIntegrationParams(float initStepSize, float maxStepSize);
		bool GenStreamLines(float* positions, int maxPoints, unsigned int randomSeed, float* speeds=0);
		bool GenStreakLines(float* positions, int maxPoints, unsigned int randomSeed, int startInjection, int endInjection, int injectionTimeIncrement, float* speeds=0);
		//bool GenIncrementalStreakLines(float* positions, int maxTimeSteps, unsigned int* randomSeed, int injectionTime, (void progressCB)(int completedTimeStep));
		float* GetData(size_t ts, const char* varName);

	private:
		size_t userTimeUnit;						// time unit in the original data
		size_t userTimeStep;						// enumerate time steps in source data
		double userTimeStepSize;					// number of userTimeUnits between consecutive steps, which
													// may not be constant
		double animationTimeStepSize;				// successive positions in userTimeUnits
		double userTimeStepMultiplier;
		double animationTimeStepMultiplier;
		double animationTimeStep;					// which frame in animation
		double integrationTimeStepSize;				// used for integration

		size_t startTimeStep;						// refer to userTimeUnit
		size_t endTimeStep;
		size_t timeStepIncrement;

		float minRakeExt[3];						// minimal rake range 
		float maxRakeExt[3];						// maximal rake range
		unsigned int numSeeds[3];							// number of seeds
		bool bUseRandomSeeds;						// whether use randomly or regularly generated seeds

		float initialStepSize;						// for integration
		float maxStepSize;

		DataMgr* dataMgr;							// data manager
		char *xVarName, *yVarName, *zVarName;		// name of three variables for vector field
		size_t numXForms, minRegion[3], maxRegion[3];// in block coordinate
	};
};

#endif
