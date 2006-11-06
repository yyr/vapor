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

	
	xSteadyVarName = NULL;
	ySteadyVarName = NULL;
	zSteadyVarName = NULL;
	xUnsteadyVarName = NULL;
	yUnsteadyVarName = NULL;
	zUnsteadyVarName = NULL;
	xPriorityVarName = NULL;
	yPriorityVarName = NULL;
	zPriorityVarName = NULL;
	xSeedDistVarName = NULL;
	ySeedDistVarName = NULL;
	zSeedDistVarName = NULL;

	numXForms = 0;
	for (int i = 0; i< 3; i++){
		minBlkRegion[i] = 0;
		maxBlkRegion[i] = 0;
		minRegion[i] = 0;
		maxRegion[i] = 0;
		fullInDim[i] = false;
	}

	userTimeStepSize = 1.0;
	animationTimeStepSize = 1.0;
	userTimeStepMultiplier = 1.0;
	animationTimeStepMultiplier = 1.0;
	
	bUseRandomSeeds = false;
	periodicDim[0]= periodicDim[1]= periodicDim[2]= false;

#ifdef DEBUG
	fDebug = fopen("C:\\Liya\\debug.txt", "w");
#endif
}

VaporFlow::~VaporFlow()
{
	
	if(xSteadyVarName) delete xSteadyVarName;
	if(ySteadyVarName) delete ySteadyVarName;
	if(zSteadyVarName) delete zSteadyVarName;
	if(xUnsteadyVarName) delete xUnsteadyVarName;
	if(yUnsteadyVarName) delete yUnsteadyVarName;
	if(zUnsteadyVarName) delete zUnsteadyVarName;
	if(xPriorityVarName) delete xPriorityVarName;
	if(yPriorityVarName) delete yPriorityVarName;
	if(zPriorityVarName) delete zPriorityVarName;
	if(xSeedDistVarName) delete xSeedDistVarName;
	if(ySeedDistVarName) delete ySeedDistVarName;
	if(zSeedDistVarName) delete zSeedDistVarName;
	
#ifdef DEBUG
	fclose(fDebug);
#endif
}

void VaporFlow::Reset(void)
{
	numXForms = 0;
	for (int i = 0; i< 3; i++){
		minBlkRegion[i] = 0;
		maxBlkRegion[i] = 0;
		minRegion[i] = 0;
		maxRegion[i] = 0;
	}
	

	userTimeStepSize = 1.0;
	animationTimeStepSize = 1.0;
	userTimeStepMultiplier = 1.0;
	animationTimeStepMultiplier = 1.0;

	bUseRandomSeeds = false;
}


//////////////////////////////////////////////////////////////////////////
// specify the names of the three variables that define the components
// of the steady field
//////////////////////////////////////////////////////////////////////////
void VaporFlow::SetSteadyFieldComponents(const char* xvar,
								   const char* yvar,
								   const char* zvar)
{
	if(!xSteadyVarName)
		xSteadyVarName = new char[260];
	if(!ySteadyVarName)
		ySteadyVarName = new char[260];
	if(!zSteadyVarName)
		zSteadyVarName = new char[260];

	strcpy(xSteadyVarName, xvar);
	strcpy(ySteadyVarName, yvar);
	strcpy(zSteadyVarName, zvar);
}
//////////////////////////////////////////////////////////////////////////
// specify the names of the three variables that define the components
// of the unsteady vector field
//////////////////////////////////////////////////////////////////////////
void VaporFlow::SetUnsteadyFieldComponents(const char* xvar,
								   const char* yvar,
								   const char* zvar)
{
	if(!xUnsteadyVarName)
		xUnsteadyVarName = new char[260];
	if(!yUnsteadyVarName)
		yUnsteadyVarName = new char[260];
	if(!zUnsteadyVarName)
		zUnsteadyVarName = new char[260];

	strcpy(xUnsteadyVarName, xvar);
	strcpy(yUnsteadyVarName, yvar);
	strcpy(zUnsteadyVarName, zvar);
}

//////////////////////////////////////////////////////////////////////////
// specify the spatial extent and resolution of the region that will be
// used for the flow calculations
//////////////////////////////////////////////////////////////////////////
void VaporFlow::SetRegion(size_t num_xforms, 
						  const size_t min[3], 
						  const size_t max[3],
						  const size_t min_bdim[3],
						  const size_t max_bdim[3])
{
	numXForms = num_xforms;
	const size_t* fullDims = dataMgr->GetMetadata()->GetDimension();
	const std::vector<double> extents = dataMgr->GetMetadata()->GetExtents();
	int max_xforms = dataMgr->GetMetadata()->GetNumTransforms();
	for (int i = 0; i< 3; i++){
		minBlkRegion[i] = min_bdim[i];
		maxBlkRegion[i] = max_bdim[i];
		minRegion[i] = min[i];
		maxRegion[i] = max[i];
		if (min[i] == 0 && max[i] == ((fullDims[i]>>(max_xforms-num_xforms))-1)) fullInDim[i] = true; 
		else fullInDim[i] = false;
		//Establish the period, in case the data is periodic.
		flowPeriod[i] = (extents[i+3] - extents[i])*((float)fullDims[i])/((float)(fullDims[i]-1)); 
	}

	
	
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
	float *regionData = dataMgr->GetRegion(ts, varName, numXForms, minBlkRegion, maxBlkRegion,1);
	SetErrMsgCB(errorCallback);
	if (!regionData) {
		SetErrMsg(103,"Error obtaining field data for timestep %d, variable %s",ts, varName);
		return 0;
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
	int totalXNum = (maxBlkRegion[0]-minBlkRegion[0]+1)* dataMgr->GetMetadata()->GetBlockSize()[0];
	int totalYNum = (maxBlkRegion[1]-minBlkRegion[1]+1)* dataMgr->GetMetadata()->GetBlockSize()[1];
	int totalZNum = (maxBlkRegion[2]-minBlkRegion[2]+1)* dataMgr->GetMetadata()->GetBlockSize()[2];
	int totalNum = totalXNum*totalYNum*totalZNum;
	pUData = new float*[1];
	pVData = new float*[1];
	pWData = new float*[1];
	pUData[0] = GetData(startTimeStep, xSteadyVarName);
	if (pUData[0]== 0)
		return false;

	pVData[0] = GetData(startTimeStep, ySteadyVarName);
	if (pVData[0] == 0) {
		dataMgr->UnlockRegion(pUData[0]);
		return false;
	}
	pWData[0] = GetData(startTimeStep, zSteadyVarName);
	if (pWData[0] == 0) {
		dataMgr->UnlockRegion(pUData[0]);
		dataMgr->UnlockRegion(pVData[0]);
		return false;
	}

	pSolution = new Solution(pUData, pVData, pWData, totalNum, 1);
	pSolution->SetTimeScaleFactor(userTimeStepMultiplier);
	pSolution->SetTime(startTimeStep, startTimeStep);
	pCartesianGrid = new CartesianGrid(totalXNum, totalYNum, totalZNum, regionPeriodicDim(0),regionPeriodicDim(1),regionPeriodicDim(2));
	pCartesianGrid->setPeriod(flowPeriod);
	// set the boundary of physical grid
	VDFIOBase* myReader = (VDFIOBase*)dataMgr->GetRegionReader();
	VECTOR3 minB, maxB, minR, maxR;
	double minUser[3], maxUser[3], regMin[3],regMax[3];
	size_t blockRegionMin[3],blockRegionMax[3];
	const size_t* bs = dataMgr->GetMetadata()->GetBlockSize();
	for (int i = 0; i< 3; i++){
		blockRegionMin[i] = bs[i]*minBlkRegion[i];
		blockRegionMax[i] = bs[i]*(maxBlkRegion[i]+1)-1;
	}
	myReader->MapVoxToUser(startTimeStep, blockRegionMin, minUser, numXForms);
	myReader->MapVoxToUser(startTimeStep, blockRegionMax, maxUser, numXForms);
	myReader->MapVoxToUser(startTimeStep, minRegion, regMin, numXForms);
	myReader->MapVoxToUser(startTimeStep, maxRegion, regMax, numXForms);
	
	//Use current region to determine coords of grid boundary:
	
	//Now adjust minB, maxB to block region extents:
	
	for (int i = 0; i< 3; i++){
		minB[i] = minUser[i];
		maxB[i] = maxUser[i];
		minR[i] = regMin[i];
		maxR[i] = regMax[i];
	}
	
	pCartesianGrid->SetRegionExtents(minR,maxR);
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
	pStreamLine->SetStationaryCutoff((0.1f*pCartesianGrid->GetGridSpacing(0))/(maxPoints*animationTimeStepMultiplier));
	pStreamLine->execute((void *)&currentT, positions, speeds);
	
	//AN: Removed a call to Reset() here. This requires all vapor flow state to be
	// reestablished each time flow lines are generated.
	// release resource
	delete pStreamLine;
	delete pField;
	//Unlock region:

	dataMgr->UnlockRegion(pUData[0]);
	dataMgr->UnlockRegion(pVData[0]);
	dataMgr->UnlockRegion(pWData[0]);
		
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
void VaporFlow::SetPeriodicDimensions(bool xdim, bool ydim, bool zdim){
	periodicDim[0] = xdim;
	periodicDim[1] = ydim;
	periodicDim[2] = zdim;
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
    totalXNum = (maxBlkRegion[0]-minBlkRegion[0] + 1)* dataMgr->GetMetadata()->GetBlockSize()[0];
	totalYNum = (maxBlkRegion[1]-minBlkRegion[1] + 1)* dataMgr->GetMetadata()->GetBlockSize()[1];
	totalZNum = (maxBlkRegion[2]-minBlkRegion[2] + 1)* dataMgr->GetMetadata()->GetBlockSize()[2];
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
	pCartesianGrid = new CartesianGrid(totalXNum, totalYNum, totalZNum, regionPeriodicDim(0),regionPeriodicDim(1),regionPeriodicDim(2));
	pCartesianGrid->setPeriod(flowPeriod);
	// set the boundary of physical grid
	
	VDFIOBase* myReader = (VDFIOBase*)dataMgr->GetRegionReader();
	const size_t* bs = dataMgr->GetMetadata()->GetBlockSize();
	VECTOR3 minB, maxB, minR, maxR;
	double minUser[3], maxUser[3], regMin[3],regMax[3];
	size_t blockRegionMin[3],blockRegionMax[3];
	for (int i = 0; i< 3; i++){
		blockRegionMin[i] = bs[i]*minBlkRegion[i];
		blockRegionMax[i] = bs[i]*(maxBlkRegion[i]+1)-1;
	}
	myReader->MapVoxToUser(realStartTime, blockRegionMin, minUser, numXForms);
	myReader->MapVoxToUser(realStartTime, blockRegionMax, maxUser, numXForms);
	myReader->MapVoxToUser(realStartTime, minRegion, regMin, numXForms);
	myReader->MapVoxToUser(realStartTime, maxRegion, regMax, numXForms);
	
	minB.Set(minUser[0], minUser[1], minUser[2]);
	maxB.Set(maxUser[0], maxUser[1], maxUser[2]);
	minR.Set(regMin[0], regMin[1], regMin[2]);
	maxR.Set(regMax[0], regMax[1], regMax[2]);
	pCartesianGrid->SetBoundary(minB, maxB);
	pCartesianGrid->SetRegionExtents(minR,maxR);
	
	//Now set the region bound:

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

	//Keep first and next region pointers.  They must be released as we go:
	float *xDataPtr =0, *yDataPtr = 0, *zDataPtr =0;
	float *xDataPtr2 =0, *yDataPtr2 = 0, *zDataPtr2 =0;

	// start to compute streakline
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

		// need get new data ?
		if(((iFor-realStartTime)%timeStepIncrement) == 0)
		{
			//For the very first sample time, get data for current time (get the next sampled timestep in next line)
			if(iFor == realStartTime){
				//AN 10/19/05
				//Check for valid data, return false if invalid
				//
				
				xDataPtr = GetData(iFor,xUnsteadyVarName);
				
				if(xDataPtr) yDataPtr = GetData(iFor,yUnsteadyVarName);
				
				if(yDataPtr) zDataPtr = GetData(iFor,zUnsteadyVarName);
				
				if (xDataPtr == 0 || yDataPtr == 0 || zDataPtr == 0){
					// release resources
					delete[] pUserTimeSteps;
					delete[] pointers;
					delete pStreakLine;
					delete pField;
					if (xDataPtr) dataMgr->UnlockRegion(xDataPtr);
					if (yDataPtr) dataMgr->UnlockRegion(yDataPtr);
					return false;
				}
				pField->SetSolutionData(iTemp,xDataPtr,yDataPtr,zDataPtr);
			} else {
				//If it's not the very first time, need to release data for first 
				//time step, and move end ptrs to start:
				//Now can release first pointers:
				dataMgr->UnlockRegion(xDataPtr);
				dataMgr->UnlockRegion(yDataPtr);
				dataMgr->UnlockRegion(zDataPtr);
				//And use them to save the second pointers:
				xDataPtr = xDataPtr2;
				yDataPtr = yDataPtr2;
				zDataPtr = zDataPtr2;
			}
			//now get data for second ( next) sampled timestep.  
			yDataPtr2 = zDataPtr2 = 0;
			xDataPtr2 = GetData(iFor+timeStepIncrement,xUnsteadyVarName); 
			if(xDataPtr2) yDataPtr2 = GetData(iFor+timeStepIncrement,yUnsteadyVarName); 
			if(yDataPtr2) zDataPtr2 = GetData(iFor+timeStepIncrement,zUnsteadyVarName); 
			if (!xDataPtr2 || !yDataPtr2 || !zDataPtr2){
				// if we failed:  release resources
				delete[] pUserTimeSteps;
				delete[] pointers;
				delete pStreakLine;
				delete pField;
				if (xDataPtr) dataMgr->UnlockRegion(xDataPtr);
				if (yDataPtr) dataMgr->UnlockRegion(yDataPtr);
				if (zDataPtr) dataMgr->UnlockRegion(zDataPtr);
				if (xDataPtr2) dataMgr->UnlockRegion(xDataPtr2);
				if (yDataPtr2) dataMgr->UnlockRegion(yDataPtr2);
				return false;
			}
			pField->SetSolutionData(iTemp+1,xDataPtr2,yDataPtr2,zDataPtr2); 
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
	// release resources.  we always have valid start and end pointers
	// at this point.
	dataMgr->UnlockRegion(xDataPtr);
	dataMgr->UnlockRegion(yDataPtr);
	dataMgr->UnlockRegion(zDataPtr);
	dataMgr->UnlockRegion(xDataPtr2);
	dataMgr->UnlockRegion(yDataPtr2);
	dataMgr->UnlockRegion(zDataPtr2);
	delete[] pUserTimeSteps;
	delete[] pointers;
	delete pStreakLine;
	delete pField;
	return true;
}
