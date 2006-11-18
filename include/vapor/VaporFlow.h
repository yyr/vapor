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

		void SetSteadyFieldComponents(const char* xvar, const char* yvar, const char* zvar);
		void SetUnsteadyFieldComponents(const char* xvar, const char* yvar, const char* zvar);

		void SetRegion(size_t num_xforms, const size_t min[3], const size_t max[3], const size_t min_bdim[3], const size_t max_bdim[3]);
		void SetTimeStepInterval(size_t startT, size_t endT, size_t tInc);
		
		void SetUnsteadyTimeSteps(int timeStepList[], size_t numSteps, bool forward);
		
		void ScaleTimeStepSizes(double userTimeStepMultiplier, double animationTimeStepMultiplier);
		void SetRandomSeedPoints(const float min[3], const float max[3], int numSeeds);
		void SetRegularSeedPoints(const float min[3], const float max[3], const size_t numSeeds[3]);
		void SetIntegrationParams(float initStepSize, float maxStepSize);
		bool GenStreamLines(float* positions, int maxPoints, unsigned int randomSeed, float* speeds=0);
		bool GenStreamLines(float* positions, float* seeds, int numSeeds, int maxPoints, float* speeds=0);
		bool GenPathLines(float* positions, int maxPoints, unsigned int randomSeed, int startInjection, int endInjection, int injectionTimeIncrement, float* speeds=0);
		bool GenPathLines(float* positions, float* seeds, int numSeeds, int maxPoints, int startInjection, int endInjection, int injectionTimeIncrement, float* speeds=0);
		void SetPeriodicDimensions(bool xPeriodic, bool yPeriodic, bool zPeriodic);
		bool GenStreamLinesNoRake(float* positions, int maxPoints, int totalSeeds, float* speeds=0);
	
		bool GenPathLinesNoRake(float* positions, int maxPoints, int totalSeeds, int startInjection, int endInjection, int injectionTimeIncrement, float* speeds=0);
		
		//bool GenIncrementalPathLines(float* positions, int maxTimeSteps, unsigned int* randomSeed, int injectionTime, (void progressCB)(int completedTimeStep));
		
		float* GetData(size_t ts, const char* varName);
		bool regionPeriodicDim(int i) {return (periodicDim[i] && fullInDim[i]);}
		

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

		size_t startTimeStep;						// refer to userTimeUnit.  Used only for
		size_t endTimeStep;							// steady flow
		size_t timeStepIncrement;

		int* unsteadyTimestepList;
		size_t numUnsteadyTimesteps;
		bool unsteadyIsForward;

		float minRakeExt[3];						// minimal rake range 
		float maxRakeExt[3];						// maximal rake range
		size_t numSeeds[3];							// number of seeds
		bool periodicDim[3];						// specify the periodic dimensions
		bool fullInDim[3];						// determine if the current region is full in each dimension
		bool bUseRandomSeeds;						// whether use randomly or regularly generated seeds

		float initialStepSize;						// for integration
		float maxStepSize;

		DataMgr* dataMgr;							// data manager
		char *xSteadyVarName, *ySteadyVarName, *zSteadyVarName;		
													// name of three variables for steady field
		char *xUnsteadyVarName, *yUnsteadyVarName, *zUnsteadyVarName;		
													// name of three variables for unsteady field
		char *xPriorityVarName, *yPriorityVarName, *zPriorityVarName;
													// field variables used for prioritizing seeds on flowlines
		char *xSeedDistVarName, *ySeedDistVarName, *zSeedDistVarName;
													// field variables used to determine random seed distribution
		size_t numXForms, minBlkRegion[3], maxBlkRegion[3];// in block coordinate
		size_t minRegion[3], maxRegion[3];			//Actual region bounds
		float flowPeriod[3];							//Used if data is periodic
	};
};

#endif

