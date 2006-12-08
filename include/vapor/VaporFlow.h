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
#include "vapor/flowlinedata.h"




namespace VAPoR
{
	class VECTOR3;
	class CVectorField;
	class Grid;
	class FlowLineData;
	class PathLineData;
	
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
		
		void SetUnsteadyTimeSteps(int timeStepList[], size_t numSteps, bool forward);
		void SetSteadyTimeSteps(size_t timeStep, int direction){
			steadyStartTimeStep = timeStep;
			steadyFlowDirection = direction;
		}
		
		void ScaleSteadyTimeStepSizes(double userTimeStepMultiplier, double animationTimeStepMultiplier);
		void ScaleUnsteadyTimeStepSizes(double userTimeStepMultiplier, double animationTimeStepMultiplier);
		
		void SetRandomSeedPoints(const float min[3], const float max[3], int numSeeds);
		void SetRegularSeedPoints(const float min[3], const float max[3], const size_t numSeeds[3]);
		void SetIntegrationParams(float initStepSize, float maxStepSize);
		bool GenStreamLines(float* positions, int maxPoints, unsigned int randomSeed, float* speeds=0);
		bool GenStreamLines(float* positions, float* seeds, int numSeeds, int maxPoints, float* speeds=0);
		//New version for API.  Uses rake, then puts integration results in container
		bool GenStreamLines(FlowLineData* container, unsigned int randomSeed);
		//Version for field line advection, takes seeds from unsteady container
		bool GenStreamLines (FlowLineData* steadyContainer, PathLineData* unsteadyContainer, int timeStep, bool prioritize);

		//Version that actually does the work
		bool GenStreamLinesNoRake(FlowLineData* container, float* seeds);
		//Start field line advection:
		//bool StartFieldLineAdvection(FlowLineData* container, unsigned int randomSeed, int startTimeStep);
		//bool StartFieldLineAdvectionNoRake(FlowLineData* container, float* seeds, int startTimeStep);
		//bool AdvectFieldLines(FlowLineData* container, int nextTimeStep);
		
		//Incrementally do path lines:
		bool ExtendPathLines(PathLineData* container, int startTimeStep, int endTimeStep);

		bool GenPathLines(float* positions, int maxPoints, unsigned int randomSeed, int startInjection, int endInjection, int injectionTimeIncrement, float* speeds=0);
		bool GenPathLines(float* positions, float* seeds, int numSeeds, int maxPoints, int startInjection, int endInjection, int injectionTimeIncrement, float* speeds=0);
		bool GenPathLines(FlowLineData* container, unsigned int randomSeed, int startInjection, int endInjection, int injectionTimeIncrement);
		bool GenPathLinesNoRake(FlowLineData* container, float* seedList, int startInjection, int endInjection, int injectionTimeIncrement);
		
		void SetPeriodicDimensions(bool xPeriodic, bool yPeriodic, bool zPeriodic);
		bool GenStreamLinesNoRake(float* positions, int maxPoints, int totalSeeds, float* speeds=0);
		bool GenPathLinesNoRake(float* positions, int maxPoints, int totalSeeds, int startInjection, int endInjection, int injectionTimeIncrement, float* speeds=0);
		
		//bool GenIncrementalPathLines(float* positions, int maxTimeSteps, unsigned int* randomSeed, int injectionTime, (void progressCB)(int completedTimeStep));
		
		float* GetData(size_t ts, const char* varName);
		bool regionPeriodicDim(int i) {return (periodicDim[i] && fullInDim[i]);}
		void VaporFlow::SetPrioritizationField(const char* varx, const char* vary, const char* varz,
			float minField = 0.f, float maxField = 1.e30f);
		

	protected:
		bool prioritizeSeeds(FlowLineData* container, PathLineData* pathContainer, int timestep);
		//Evaluate the priority at a point, using current priority field.
		float priorityVal(float point[3], CVectorField*, Grid*);
		//bool UnsteadyAdvectPoints(int prevTime, int nextTime, float* pointList);
	private:
		size_t userTimeUnit;						// time unit in the original data
		size_t userTimeStep;						// enumerate time steps in source data
		double userTimeStepSize;					// number of userTimeUnits between consecutive steps, which
													// may not be constant
		double animationTimeStepSize;				// successive positions in userTimeUnits
		double steadyUserTimeStepMultiplier;
		double steadyAnimationTimeStepMultiplier;
		double unsteadyUserTimeStepMultiplier;
		double unsteadyAnimationTimeStepMultiplier;
		double animationTimeStep;					// which frame in animation
		double integrationTimeStepSize;				// used for integration

		size_t steadyStartTimeStep;					// refer to userTimeUnit.  Used only for
		size_t endTimeStep;							// steady flow
		size_t timeStepIncrement;
		int steadyFlowDirection;					// -1, 0 or 1

		int* unsteadyTimestepList;
		size_t numUnsteadyTimesteps;
		bool unsteadyIsForward;

		float minRakeExt[3];						// minimal rake range 
		float maxRakeExt[3];						// maximal rake range
		size_t numSeeds[3];							// number of seeds
		bool periodicDim[3];						// specify the periodic dimensions
		bool fullInDim[3];							// determine if the current region is full in each dimension
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
		float flowPeriod[3];						//Used if data is periodic
		float* flowLineAdvectionSeeds;
		float minPriorityVal, maxPriorityVal;
		int currentFlowAdvectionTime;
	};
};

#endif

