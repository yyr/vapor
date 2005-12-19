#ifdef WIN32
#pragma warning(disable : 4251 4100)
#endif

#include "vapor/VaporFlow.h"
#include "Rake.h"
#include "VTFieldLine.h"

using namespace VetsUtil;
using namespace VAPoR;

#ifdef DEBUG
	FILE* fDebug;
#endif

VaporFlow::VaporFlow(DataMgr* dm)
{
	dataMgr = dm;

	xVarName = NULL;
	yVarName = NULL;
	zVarName = NULL;

	numXForms = 0;
	minRegion[0] = minRegion[1] = minRegion[2] = 0;
	maxRegion[0] = maxRegion[1] = maxRegion[2] = 0;

	userTimeStepSize = 1.0;
	animationTimeStepSize = 1.0;
	userTimeStepMultiplier = 1.0;
	animationTimeStepMultiplier = 1.0;
	
	bUseRandomSeeds = false;

#ifdef DEBUG
	fDebug = fopen("C:\\Liya\\debug.txt", "w");
#endif
}

VaporFlow::~VaporFlow()
{
	if(xVarName)
	{
		delete[] xVarName;
		xVarName = NULL;
	}
	if(yVarName)
	{
		delete[] yVarName;
		yVarName = NULL;
	}
	if(zVarName)
	{
		delete[] zVarName;
		zVarName = NULL;
	}

#ifdef DEBUG
	fclose(fDebug);
#endif
}

void VaporFlow::Reset(void)
{
	numXForms = 0;
	minRegion[0] = minRegion[1] = minRegion[2] = 0;
	maxRegion[0] = maxRegion[1] = maxRegion[2] = 0;

	userTimeStepSize = 1.0;
	animationTimeStepSize = 1.0;
	userTimeStepMultiplier = 1.0;
	animationTimeStepMultiplier = 1.0;

	bUseRandomSeeds = false;
}

//////////////////////////////////////////////////////////////////////////
// specify the names of the three variables that define the components
// of the vector field
//////////////////////////////////////////////////////////////////////////
void VaporFlow::SetFieldComponents(const char* xvar,
								   const char* yvar,
								   const char* zvar)
{
	if(!xVarName)
		xVarName = new char[260];
	if(!yVarName)
		yVarName = new char[260];
	if(!zVarName)
		zVarName = new char[260];

	strcpy(xVarName, xvar);
	strcpy(yVarName, yvar);
	strcpy(zVarName, zvar);
}

//////////////////////////////////////////////////////////////////////////
// specify the spatial extent and resolution of the region that will be
// used for the flow calculations
//////////////////////////////////////////////////////////////////////////
void VaporFlow::SetRegion(size_t num_xforms, 
						  const size_t min[3], 
						  const size_t max[3])
{
	numXForms = num_xforms;
	minRegion[0] = min[0];
	minRegion[1] = min[1];
	minRegion[2] = min[2];
	maxRegion[0] = max[0];
	maxRegion[1] = max[1];
	maxRegion[2] = max[2];
}

//////////////////////////////////////////////////////////////////////////
// time interval for subsequent flow calculation
//////////////////////////////////////////////////////////////////////////
void VaporFlow::SetTimeStepInterval(size_t startT, 
									size_t endT, 
									size_t tInc)
{
	startTimeStep = startT;
	endTimeStep = endT;
	timeStepIncrement = tInc;
}

//////////////////////////////////////////////////////////////////////////
// specify the userTimeStepSize and animationTimeStepSize as multiples
// of the userTimeStepSize
//////////////////////////////////////////////////////////////////////////
void VaporFlow::ScaleTimeStepSizes(double userTimeStepMultiplier, 
								   double animationTimeStepMultiplier)
{
	this->userTimeStepMultiplier = userTimeStepMultiplier;
	this->animationTimeStepMultiplier = animationTimeStepMultiplier;
}

//////////////////////////////////////////////////////////////////////////
// specify a set of seed points randomly generated over the specified
// spatial interval. Points can be in axis aligned dimension 0, 1, 2, 3
//////////////////////////////////////////////////////////////////////////
void VaporFlow::SetRandomSeedPoints(const float min[3], 
									const float max[3], 
									int numSeeds)
{
	for(int iFor = 0; iFor < 3; iFor++)
	{
		minRakeExt[iFor] = min[iFor];
		maxRakeExt[iFor] = max[iFor];
	}
	this->numSeeds[0] = numSeeds;
	this->numSeeds[1] = 1;
	this->numSeeds[2] = 1;

	bUseRandomSeeds = true;
}

//////////////////////////////////////////////////////////////////////////
// specify a set of seed points regularly generated over the specified
// spatial interval. Points can be in axis aligned dimension 0, 1, 2, 3
//////////////////////////////////////////////////////////////////////////
void VaporFlow::SetRegularSeedPoints(const float min[3], 
									const float max[3], 
									const size_t numSeeds[3])
{
	for(int iFor = 0; iFor < 3; iFor++)
	{
		minRakeExt[iFor] = min[iFor];
		maxRakeExt[iFor] = max[iFor];
	}
	this->numSeeds[0] = (unsigned int)numSeeds[0];
	this->numSeeds[1] = (unsigned int)numSeeds[1];
	this->numSeeds[2] = (unsigned int)numSeeds[2];

	bUseRandomSeeds = false;
}

void VaporFlow::SetIntegrationParams(float initStepSize, float maxStepSize)
{
	initialStepSize = initStepSize;
	this->maxStepSize = maxStepSize;
}

//////////////////////////////////////////////////////////////////////////
// prepare the necessary data
//////////////////////////////////////////////////////////////////////////
float* VaporFlow::GetData(size_t ts, const char* varName)
{
	//AN 10/19/05
	//Trap read errors:
	//
	ErrMsgCB_T errorCallback = GetErrMsgCB();
	SetErrMsgCB(0);
	float *regionData = dataMgr->GetRegion(ts, varName, numXForms, minRegion, maxRegion);
	SetErrMsgCB(errorCallback);
	if (!regionData) {
		SetErrMsg("Error loading field data for timestep %d, variable %s",ts, varName);
	}
	return regionData;
}

//////////////////////////////////////////////////////////////////////////
// generate streamlines with rake
//////////////////////////////////////////////////////////////////////////
bool VaporFlow::GenStreamLines(float* positions, 
							   int maxPoints, 
							   unsigned int randomSeed, 
							   float* speeds)
{
	// first generate seeds
	float* seedPtr;
	int seedNum;
	seedNum = numSeeds[0]*numSeeds[1]*numSeeds[2];
	seedPtr = new float[seedNum*3];
	SeedGenerator* pSeedGenerator = new SeedGenerator(minRakeExt, maxRakeExt, numSeeds);
	pSeedGenerator->GetSeeds(seedPtr, bUseRandomSeeds, randomSeed);
	delete pSeedGenerator;

	//Then do streamlines with prepared seeds:
	bool rc = GenStreamLines(positions, seedPtr, seedNum, maxPoints, speeds);
	delete [] seedPtr;
	return rc;
}
bool VaporFlow::GenStreamLines(float* positions, 
							   float* seedPtr,
							   int seedNum,
							   int maxPoints, 
							   float* speeds)
{
	
	// scale animationTimeStep and userTimeStep
	if((dataMgr->GetMetadata()->HasTSUserTime(startTimeStep))&&(dataMgr->GetMetadata()->HasTSUserTime(startTimeStep+1)))
	{
		double diff;
		diff =  dataMgr->GetMetadata()->GetTSUserTime(startTimeStep+1)[0] - dataMgr->GetMetadata()->GetTSUserTime(startTimeStep)[0];
		userTimeStepSize = diff*userTimeStepMultiplier;
		animationTimeStepSize = diff*animationTimeStepMultiplier;
	}
	else
	{
		userTimeStepSize = userTimeStepMultiplier;
		animationTimeStepSize = animationTimeStepMultiplier;
	}

	// create field object
	CVectorField* pField;
	Solution* pSolution;
	CartesianGrid* pCartesianGrid;
	float **pUData, **pVData, **pWData;
	int totalXNum = (maxRegion[0]-minRegion[0]+1)* dataMgr->GetMetadata()->GetBlockSize()[0];
	int totalYNum = (maxRegion[1]-minRegion[1]+1)* dataMgr->GetMetadata()->GetBlockSize()[1];
	int totalZNum = (maxRegion[2]-minRegion[2]+1)* dataMgr->GetMetadata()->GetBlockSize()[2];
	int totalNum = totalXNum*totalYNum*totalZNum;
	pUData = new float*[1];
	pVData = new float*[1];
	pWData = new float*[1];
	pUData[0] = GetData(startTimeStep, xVarName);
	pVData[0] = GetData(startTimeStep, yVarName);
	pWData[0] = GetData(startTimeStep, zVarName);
	//AN 10/19/05
	//Check for invalid data
	//Note that read errors were already reported by GetData
	//
	if (pUData[0] == 0 || pVData[0] == 0 || pWData[0] == 0){
		return false;
	}

	pSolution = new Solution(pUData, pVData, pWData, totalNum, 1);
	pSolution->SetTimeScaleFactor(userTimeStepMultiplier);
	pSolution->SetTime(startTimeStep, startTimeStep);
	pCartesianGrid = new CartesianGrid(totalXNum, totalYNum, totalZNum);
	
	// set the boundary of physical grid
	VECTOR3 minB, maxB;
	vector<double> vExtent;
	vExtent = dataMgr->GetMetadata()->GetExtents();
	
	//Use current region to determine coords of grid boundary:
	const size_t* fullDim = dataMgr->GetMetadata()->GetDimension();
	int nlevels = dataMgr->GetMetadata()->GetNumTransforms();
	//Now adjust minB, maxB to actual region extents:
	for (int i = 0; i< 3; i++){
		minB[i] = vExtent[i]+ (vExtent[i+3] - vExtent[i])*
			(float)(minRegion[i]*(dataMgr->GetMetadata()->GetBlockSize()[i])/(float)((fullDim[i] >> (nlevels -numXForms))-1));
		maxB[i] = vExtent[i]+ (vExtent[i+3] - vExtent[i])*
			(float)(((maxRegion[i]+1)*(dataMgr->GetMetadata()->GetBlockSize()[i])-1)/(float)((fullDim[i] >> (nlevels -numXForms))-1));
	}
	

	pCartesianGrid->SetBoundary(minB, maxB);
	pField = new CVectorField(pCartesianGrid, pSolution, 1);
	pField->SetUserTimeStepInc(0, 1);
	
	// execute streamline
	vtCStreamLine* pStreamLine;


	

	float currentT = (float)startTimeStep;
	pStreamLine = new vtCStreamLine(pField);

	//AN: for memory debugging:
	pStreamLine->fullArraySize = maxPoints*seedNum*3;


	pStreamLine->setBackwardTracing(false);
	pStreamLine->setMaxPoints(maxPoints);
	pStreamLine->setSeedPoints(seedPtr, seedNum, currentT);
	pStreamLine->SetSamplingRate(animationTimeStepSize);
	pStreamLine->SetInitStepSize(initialStepSize);
	pStreamLine->SetMaxStepSize(maxStepSize);
	pStreamLine->setIntegrationOrder(FOURTH);
	pStreamLine->SetStationaryCutoff((0.01f*pCartesianGrid->GetGridSpacing(0))/(userTimeStepSize*maxPoints*animationTimeStepMultiplier));
	pStreamLine->execute((void *)&currentT, positions, speeds);
	
	//AN: Removed a call to Reset() here. This requires all vapor flow state to be
	// reestablished each time flow lines are generated.
	// release resource
	delete pStreamLine;
	delete pField;
	return true;
}

//////////////////////////////////////////////////////////////////////////
// generate streamlines without rake
//////////////////////////////////////////////////////////////////////////
bool VaporFlow::GenStreamLinesNoRake(float* positions, 
							   int maxPoints, 
							   int totalSeeds, 
							   float* speeds)
{
	// first copy seeds where they will be accessed:
	float* seedPtr;
	
	seedPtr = new float[totalSeeds*3];
	for (int k = 0; k<3*totalSeeds; k++){
		
		seedPtr[k] = positions[k];
		
	}
	//Then do streamlines with prepared seeds:
	bool rc = GenStreamLines(positions, seedPtr, totalSeeds, maxPoints, speeds);
	delete [] seedPtr;
	return rc;
}
	
	
//////////////////////////////////////////////////////////////////////////
// generate pathlines (with and without rake:
//////////////////////////////////////////////////////////////////////////
bool VaporFlow::GenPathLines(float* positions,
							   int maxPoints, 
							   unsigned int randomSeed, 
							   int startInjection, 
							   int endInjection,
							   int injectionTimeIncrement, 
							   float* speeds)
{
	//first generate seeds
	float* seedPtr;
	int seedNum;
	seedNum = numSeeds[0]*numSeeds[1]*numSeeds[2];
	seedPtr = new float[seedNum*3];
	SeedGenerator* pSeedGenerator = new SeedGenerator(minRakeExt, maxRakeExt, numSeeds);
	pSeedGenerator->GetSeeds(seedPtr, bUseRandomSeeds, randomSeed);
	delete pSeedGenerator;
	
	bool rc = GenPathLines(positions, seedPtr, seedNum, maxPoints, startInjection, endInjection, injectionTimeIncrement, speeds);
	delete [] seedPtr;
	return rc;
}

	//////////////////////////////////////////////////////////////////////////
// generate pathlines (with and without rake:
//////////////////////////////////////////////////////////////////////////
bool VaporFlow::GenPathLinesNoRake(float* positions,
							   int maxPoints, 
							   int totalSeeds, 
							   int startInjection, 
							   int endInjection,
							   int injectionTimeIncrement, 
							   float* speeds)
{
	// first copy seeds to where they will be accessed:
	float* seedPtr;
	
	seedPtr = new float[totalSeeds*3];
	for (int k = 0; k< 3*totalSeeds; k++){
		
		seedPtr[k] = positions[k];
		
	}
	//Then do streamlines with prepared seeds:
	bool rc = GenPathLines(positions, seedPtr, totalSeeds, maxPoints, startInjection, endInjection, injectionTimeIncrement, speeds);
	delete [] seedPtr;
	return rc;

}
//Generate path lines using previously prepared seeds:
//
bool VaporFlow::GenPathLines(float* positions,
							 float* seedPtr,
							 int seedNum,
							   int maxPoints, 
							   int startInjection, 
							   int endInjection,
							   int injectionTimeIncrement, 
							   float* speeds)
{
	animationTimeStepSize = animationTimeStepMultiplier;

	// create field object
	CVectorField* pField;
	Solution* pSolution;
	CartesianGrid* pCartesianGrid;
	float **pUData, **pVData, **pWData;
	int numInjections;						// times to inject new seeds
	int timeSteps;							// total "time steps"
	int totalXNum, totalYNum, totalZNum, totalNum;
	int realStartTime, realEndTime;
    totalXNum = (maxRegion[0]-minRegion[0] + 1)* dataMgr->GetMetadata()->GetBlockSize()[0];
	totalYNum = (maxRegion[1]-minRegion[1] + 1)* dataMgr->GetMetadata()->GetBlockSize()[1];
	totalZNum = (maxRegion[2]-minRegion[2] + 1)* dataMgr->GetMetadata()->GetBlockSize()[2];
	totalNum = totalXNum*totalYNum*totalZNum;
	numInjections = 1 + ((endInjection - startInjection)/injectionTimeIncrement);
	//realStartTime and realEndTime are actual limits of time steps for which positions
	//are calculated.
	realStartTime = (startInjection < startTimeStep)? startTimeStep: startInjection;
	//timeSteps is the number of sampled time steps:
	timeSteps = (endTimeStep - realStartTime)/timeStepIncrement + 1;
	realEndTime = realStartTime + (timeSteps-1)*timeStepIncrement;
	pUData = new float*[timeSteps];
	pVData = new float*[timeSteps];
	pWData = new float*[timeSteps];
	memset(pUData, 0, sizeof(float*)*timeSteps);
	memset(pVData, 0, sizeof(float*)*timeSteps);
	memset(pWData, 0, sizeof(float*)*timeSteps);
	pSolution = new Solution(pUData, pVData, pWData, totalNum, timeSteps);
	pSolution->SetTimeScaleFactor(userTimeStepMultiplier);
	pSolution->SetTimeIncrement(timeStepIncrement);
	pSolution->SetTime(realStartTime, realEndTime);
	pCartesianGrid = new CartesianGrid(totalXNum, totalYNum, totalZNum);

	// set the boundary of physical grid
	VECTOR3 minB, maxB;
	vector<double> vExtent;
	vExtent = dataMgr->GetMetadata()->GetExtents();
	minB.Set(vExtent[0], vExtent[1], vExtent[2]);
	maxB.Set(vExtent[3], vExtent[4], vExtent[5]);
	pCartesianGrid->SetBoundary(minB, maxB);
	pField = new CVectorField(pCartesianGrid, pSolution, timeSteps);
	
	// get the userTimeStep between two sampled timesteps
	int* pUserTimeSteps;
	int tIndex = 0;
	pUserTimeSteps = new int[timeSteps-1];
	//Calculate the time differences between successive sampled timesteps.
	//save that value in pUserTimeSteps, available in pField.
	for(int nSampledStep = realStartTime; nSampledStep < realEndTime; nSampledStep += timeStepIncrement)
	{
		int nextSampledStep = nSampledStep + timeStepIncrement;
		if(dataMgr->GetMetadata()->HasTSUserTime(nSampledStep)&&dataMgr->GetMetadata()->HasTSUserTime(nextSampledStep))
			pUserTimeSteps[tIndex++] = dataMgr->GetMetadata()->GetTSUserTime(nextSampledStep)[0] - 
									   dataMgr->GetMetadata()->GetTSUserTime(nSampledStep)[0];
		else
			pUserTimeSteps[tIndex++] = timeStepIncrement;
	}
	pField->SetUserTimeSteps(pUserTimeSteps);

	// initialize streak line
	vtCStreakLine* pStreakLine;
	float currentT = 0.0;
	pStreakLine = new vtCStreakLine(pField);
	pStreakLine->SetTimeDir(FORWARD);
	pStreakLine->setParticleLife(maxPoints);
	pStreakLine->setSeedPoints(seedPtr, seedNum, currentT);
	pStreakLine->SetSamplingRate(animationTimeStepSize);
	pStreakLine->SetInitStepSize(initialStepSize);
	pStreakLine->SetMaxStepSize(maxStepSize);
	pStreakLine->setIntegrationOrder(FOURTH);

	//AN: for memory debugging:
	int numInj = 1+(endInjection-startInjection)/injectionTimeIncrement;
	pStreakLine->fullArraySize = numInj*maxPoints*seedNum*3;

	// start to computer streakline
	unsigned int* pointers = new unsigned int[seedNum*numInjections];
	unsigned int* startPositions = new unsigned int[seedNum*numInjections];
	memset(pointers, 0, sizeof(unsigned int)*seedNum*numInjections);
	for(int iFor = 0; iFor < (seedNum*numInjections); iFor++)
		startPositions[iFor] = iFor * maxPoints * 3;
	
	int index = -1, iInjection = 0;
	bool bInject;
	for(int iFor = realStartTime; iFor < realEndTime; iFor++)
	{
		bInject = false;
		//index counts advections
		index++;								// index to solution instance
		//AN: Changed iTemp to start at 0
		//  10/05/05
		//
        int iTemp = (iFor-realStartTime)/timeStepIncrement;		// index to lower sampled time step

		// get usertimestep between current time step and previous sampled time step
		double diff = 0.0, curDiff = 0.0;
		if(dataMgr->GetMetadata()->HasTSUserTime(iFor))
		{
			int lowerT = realStartTime+iTemp*timeStepIncrement;
			for(; lowerT < iFor; lowerT++)
				//diff is the time from the previous sample to the current timestep
				//or is 0 if this is a sample timestep
				//This is needed for time interpolation
				diff += dataMgr->GetMetadata()->GetTSUserTime(lowerT+1)[0] - 
						dataMgr->GetMetadata()->GetTSUserTime(lowerT)[0];
			//curDiff is the time from the current timestep to the next timestep
			curDiff = dataMgr->GetMetadata()->GetTSUserTime(iFor+1)[0] - 
					  dataMgr->GetMetadata()->GetTSUserTime(iFor)[0];
		}
		else
		{
			diff = iFor-(realStartTime+iTemp*timeStepIncrement);
			diff = (diff < 0.0) ? 0.0 : diff;
			curDiff = 1.0;
		}
		pField->SetUserTimeStepInc((int)diff, (int)curDiff);

		// need get new data
		if((iFor%timeStepIncrement) == 0)
		{
			//For the first sample time, get data for current time (get the next sampled timestep in next line)
			if(iFor == realStartTime){
				//AN 10/19/05
				//Check for valid data, return false if invalid
				//
				float* xDataPtr = GetData(iFor,xVarName); 
				float* yDataPtr = GetData(iFor,yVarName); 
				float* zDataPtr = GetData(iFor,zVarName); 
				if (xDataPtr == 0 || yDataPtr == 0 || zDataPtr == 0){
					// release resources
					delete[] pUserTimeSteps;
					delete[] pointers;
					delete pStreakLine;
					delete pField;
					return false;
				}
				pField->SetSolutionData(iTemp,xDataPtr,yDataPtr,zDataPtr);
			}
			//otherwise just get next sampled timestep.
			float* xDataPtr = GetData(iFor+timeStepIncrement,xVarName); 
			float* yDataPtr = GetData(iFor+timeStepIncrement,yVarName); 
			float* zDataPtr = GetData(iFor+timeStepIncrement,zVarName); 
			if (!xDataPtr || !yDataPtr || !zDataPtr){
				// release resources
				delete[] pUserTimeSteps;
				delete[] pointers;
				delete pStreakLine;
				delete pField;
				return false;
			}
			pField->SetSolutionData(iTemp+1,xDataPtr,yDataPtr,zDataPtr); 
		}

		// whether inject new seeds this time?
		if((iFor >= startInjection) && (iFor <= endInjection) && ((index%injectionTimeIncrement) == 0))
			bInject = true;

		// execute streakline
		//AN:  Changed these so that (float)iFor (not (float)index ) is passed as the start time
		//  10/05/05
		if(bInject)				// inject new seeds
			pStreakLine->execute((float)iFor, positions, startPositions, pointers, true, iInjection++, speeds);
		else					// do not inject new seeds
			pStreakLine->execute((float)iFor, positions, startPositions, pointers, false, iInjection, speeds);
	}

	//Reset();
	// release resource
	delete[] pUserTimeSteps;
	delete[] pointers;
	delete pStreakLine;
	delete pField;
	return true;
}
