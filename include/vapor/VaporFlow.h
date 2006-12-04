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

	class FLOW_API FlowLineData {
	public:
		FlowLineData(int numLines, int maxPoints, bool useSpeeds, bool isSteady, int direction, bool doRGBAs, int samplesPerTimestep = 1);	
		~FlowLineData();
			
		float* getFlowPoint(int lineNum, int pointIndex){
			return flowLineLists[lineNum]+3*pointIndex;
		}
		float* getFlowRGBAs(int lineNum, int pointIndex){
			return ((flowRGBAs == 0) ? 0 : flowRGBAs[lineNum]+4*pointIndex);
		}
		void enableRGBAs() {
			if (!flowRGBAs) {
				flowRGBAs = new float*[nmLines];
				for (int i = 0; i< nmLines; i++){
					flowRGBAs[i] = new float[4*mxPoints];
				}
			}
		}
		float getSpeed(int lineNum, int index){
			return *(speedLists[lineNum] + index);
		}
		float* getFlowStart(int lineNum){
			return getFlowPoint(lineNum, 0);
		}
		int getFlowLength(int lineNum){
			return lineLengths[lineNum];
		}
		//For setting in values, base on integration order, not pointIndex.
		//IntegPos starts at 0, and is pos or negative depending on integration direction
		
		void setFlowPoint(int lineNum, int integPos, float x, float y, float z){
			if (flowDirection > 0){
				*(flowLineLists[lineNum]+ 3*integPos) = x;
				*(flowLineLists[lineNum]+ 3*integPos+1) = y;
				*(flowLineLists[lineNum]+ 3*integPos+2) = z;
			} else if (flowDirection == 0){
				*(flowLineLists[lineNum]+ 3*(mxPoints/2 + integPos)) = x;
				*(flowLineLists[lineNum]+ 3*(mxPoints/2 + integPos)+1) = y;
				*(flowLineLists[lineNum]+ 3*(mxPoints/2 + integPos)+2) = z;
			} else if (flowDirection < 0){  //go from the end backwards...
				*(flowLineLists[lineNum]+ 3*(mxPoints + integPos - 1)) = x;
				*(flowLineLists[lineNum]+ 3*(mxPoints + integPos - 1)+1) = y;
				*(flowLineLists[lineNum]+ 3*(mxPoints + integPos - 1)+2) = z;
			}
		}
		void setSpeed(int lineNum, int integPos, float s){
			assert(speedLists);
			if (flowDirection > 0){
				*(speedLists[lineNum]+ integPos) = s;
			} else if (flowDirection == 0){
				*(speedLists[lineNum]+ integPos + mxPoints/2) = s;
			} else if (flowDirection < 0){
				*(speedLists[lineNum]+ integPos + mxPoints - 1) = s;
			}
		}
		//RGBAs are set according to rendering order, not integration order
		void setRGBA(int lineNum, int pointIndex, float r, float g, float b, float a){
			assert(pointIndex >= 0 && pointIndex < mxPoints);
			assert(lineNum >= 0 && lineNum < nmLines);
			*(flowRGBAs[lineNum]+4*pointIndex) = r;
			*(flowRGBAs[lineNum]+4*pointIndex+1) = g;
			*(flowRGBAs[lineNum]+4*pointIndex+2) = b;
			*(flowRGBAs[lineNum]+4*pointIndex+3) = a;
		}
		void setRGB(int lineNum, int pointIndex, float r, float g, float b){
			assert(pointIndex >= 0 && pointIndex < mxPoints);
			assert(lineNum >= 0 && lineNum < nmLines);
			*(flowRGBAs[lineNum]+4*pointIndex) = r;
			*(flowRGBAs[lineNum]+4*pointIndex+1) = g;
			*(flowRGBAs[lineNum]+4*pointIndex+2) = b;
		}
		void setAlpha(int lineNum, int pointIndex, float alpha){
			assert(pointIndex >= 0 && pointIndex < mxPoints);
			assert(lineNum >= 0 && lineNum < nmLines);
			*(flowRGBAs[lineNum]+4*pointIndex+3) = alpha;
		}

		void releaseSpeeds() {
			if (speedLists) {
				for (int i = 0; i<nmLines; i++){
					delete speedLists[i];
				}
				delete speedLists;
				speedLists = 0;
			}
		}


		// Establish the first point (for rendering) of the flow line.
		// integPos is + or - depending on whether we are integrating forward or backward
		void setFlowStart(int lineNum, int integPos){
			assert(integPos <= 0);
			if (flowDirection > 0) {assert (integPos == 0); startIndices[lineNum] = 0;}
			if (flowDirection == 0) {startIndices[lineNum] = mxPoints/2 + integPos;}
			if (flowDirection < 0) {startIndices[lineNum] = mxPoints -1 + integPos;}
		}
		void setFlowEnd(int lineNum, int integPos);

		bool doSpeeds() {return speedLists != 0;}
		int getNumLines() {return nmLines;}
		int getMaxPoints() {return mxPoints;}
		//maximum length available in either forward or direction:  
		int getMaxLength(int dir) {
			if (flowDirection != 0) return mxPoints; // forward uses full data size
			//Bidirectional uses half the points
			if (dir < 0) return (1+mxPoints/2);
			else return (mxPoints - mxPoints/2); //dir > 0 
		}
		int getStartIndex(int lineNum) {return startIndices[lineNum];}
		int getEndIndex(int lineNum) {return startIndices[lineNum]+lineLengths[lineNum] -1;}
		float getTimestep(int lineNum, int pointIndex) {
			//See how many timeSteps are before seed issued...
			int emptyTimes = seedTimes[lineNum]*samplesPerTstep;
			//Exclude positions before/after seed injection
			if (flowDirection > 0 && pointIndex < emptyTimes) return -1.f;
			if (flowDirection < 0 && (mxPoints - pointIndex) <= emptyTimes) return -1.f;
			if (flowDirection > 0) return( (float)seedTimes[lineNum] + (float)pointIndex/(float)samplesPerTstep);
			else return(seedTimes[lineNum] - (float)pointIndex/(float)samplesPerTstep);
		}
	protected:
		int mxPoints;
		int nmLines;
		int flowDirection;
		int samplesPerTstep; //only for unsteady
		int* startIndices;
			//startIndices Indicates start position of the 'first' point
			//in the flow line (not the first point for integration).
			//Integration starts at index 0 for forward integration, starts at index
			// mxPoints/2 for bidirectional integration, and starts at last index,
			// startIndices[nline]+mxPoints - 1 for backwards integration
		int* lineLengths;
		float** flowLineLists;
		float** speedLists;
		float** flowRGBAs;
		int *seedTimes;
	};
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
		//New version for API
		bool GenStreamLines(FlowLineData* container, unsigned int randomSeed);
		//Version that actually does the work:
		bool GenStreamLinesNoRake(FlowLineData* container, float* seeds);
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

		size_t steadyStartTimeStep;						// refer to userTimeUnit.  Used only for
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
		float flowPeriod[3];						//Used if data is periodic
	};
};

#endif

