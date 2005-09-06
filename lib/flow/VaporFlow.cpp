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
float* VaporFlow::GetData(size_t ts, const char* varName, const int numNode)
{
	float *lowPtr, *highPtr;
	// find low and high ts to sample data
	size_t lowT, highT;
	float ratio;
	lowT = (ts - startTimeStep)/timeStepIncrement;
	highT = lowT + 1;
	ratio = (float)((ts - startTimeStep)%timeStepIncrement)/(float)timeStepIncrement;
	
	// get low data
	lowPtr = dataMgr->GetRegion(lowT, varName, numXForms, minRegion, maxRegion);
		
	// get high data and interpolate
	if(ratio != 0.0)
	{
		highPtr = dataMgr->GetRegion(highT, varName, numXForms, minRegion, maxRegion);
		for(int iFor = 0; iFor < numNode; iFor++)
			lowPtr[iFor] = Lerp(lowPtr[iFor], highPtr[iFor], ratio);
	}
	
	return lowPtr;
}

//////////////////////////////////////////////////////////////////////////
// generate streamlines
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
	
	// scale animationTimeStep and userTimeStep
	if(dataMgr->GetMetadata()->HasTSUserTime(startTimeStep))
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
	int totalXNum = (maxRegion[0]-minRegion[0]+1)* dataMgr->GetMetadata()->GetBlockSize();
	int totalYNum = (maxRegion[1]-minRegion[1]+1)* dataMgr->GetMetadata()->GetBlockSize();
	int totalZNum = (maxRegion[2]-minRegion[2]+1)* dataMgr->GetMetadata()->GetBlockSize();
	int totalNum = totalXNum*totalYNum*totalZNum;
	pUData = new float*[1];
	pVData = new float*[1];
	pWData = new float*[1];
	pUData[0] = GetData(startTimeStep, xVarName, totalNum);
	pVData[0] = GetData(startTimeStep, yVarName, totalNum);
	pWData[0] = GetData(startTimeStep, zVarName, totalNum);
	pSolution = new Solution(pUData, pVData, pWData, totalNum, 1);
	pSolution->SetTimeScaleFactor(userTimeStepMultiplier);
	pSolution->SetTime(startTimeStep, startTimeStep);
	pSolution->SetTimeInc(1);
	pCartesianGrid = new CartesianGrid(totalXNum, totalYNum, totalZNum);
	
	// set the boundary of physical grid
	VECTOR3 minB, maxB;
	vector<double> vExtent;
	vExtent = dataMgr->GetMetadata()->GetExtents();
	minB.Set(vExtent[0], vExtent[1], vExtent[2]);
	maxB.Set(vExtent[3], vExtent[4], vExtent[5]);
	pCartesianGrid->SetBoundary(minB, maxB);
	pField = new CVectorField(pCartesianGrid, pSolution, 1);
	
	// execute streamline
	vtCStreamLine* pStreamLine;
	float currentT = 0.0;
	pStreamLine = new vtCStreamLine(pField);
	pStreamLine->setBackwardTracing(false);
	pStreamLine->setMaxPoints(maxPoints);
	pStreamLine->setSeedPoints(seedPtr, seedNum, currentT);
	pStreamLine->SetSamplingRate(animationTimeStepSize);
	pStreamLine->SetInitStepSize(initialStepSize);
	pStreamLine->SetMaxStepSize(maxStepSize);
	pStreamLine->setIntegrationOrder(FOURTH);
	pStreamLine->execute((void *)&currentT, positions, speeds);
	
	Reset();
	// release resource
	delete[] seedPtr;
	delete pStreamLine;
	delete pField;
	return true;
}

//////////////////////////////////////////////////////////////////////////
// generate streaklines
//////////////////////////////////////////////////////////////////////////
bool VaporFlow::GenStreakLines(float* positions,
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
    totalXNum = (maxRegion[0]-minRegion[0] + 1)* dataMgr->GetMetadata()->GetBlockSize();
	totalYNum = (maxRegion[1]-minRegion[1] + 1)* dataMgr->GetMetadata()->GetBlockSize();
	totalZNum = (maxRegion[2]-minRegion[2] + 1)* dataMgr->GetMetadata()->GetBlockSize();
	totalNum = totalXNum*totalYNum*totalZNum;
	numInjections = 1 + ((endInjection - startInjection)/injectionTimeIncrement);
	realStartTime = (startInjection < startTimeStep)? startTimeStep: startInjection;
	realEndTime = endTimeStep;
	timeSteps = realEndTime - realStartTime + 1;
	pUData = new float*[timeSteps];
	pVData = new float*[timeSteps];
	pWData = new float*[timeSteps];
	memset(pUData, 0, sizeof(float*)*timeSteps);
	memset(pVData, 0, sizeof(float*)*timeSteps);
	memset(pWData, 0, sizeof(float*)*timeSteps);
	pSolution = new Solution(pUData, pVData, pWData, totalNum, timeSteps);
	pSolution->SetTimeScaleFactor(userTimeStepMultiplier);

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

	// start to computer streakline
	unsigned int* pointers = new unsigned int[seedNum*numInjections];
	memset(pointers, 0, sizeof(unsigned int)*seedNum*numInjections);
	int index, iInjection = 0;
	bool bInject;
	double diff;
	for(int iFor = realStartTime; iFor < realEndTime; iFor++)
	{
		index = iFor - realStartTime;				// index to solution instance

		// scale animationTimeStep and userTimeStep
		if(dataMgr->GetMetadata()->HasTSUserTime(iFor))
			diff = dataMgr->GetMetadata()->GetTSUserTime(iFor+1)[0] - dataMgr->GetMetadata()->GetTSUserTime(iFor)[0];
		else
			diff = 1.0;
		pSolution->SetTimeInc((int)diff);
		bInject = false;

		// need get new data
		if(iFor == realStartTime)
		{
			pField->SetSolutionData(index, 
									GetData(iFor, xVarName, totalNum),
									GetData(iFor, yVarName, totalNum),
									GetData(iFor, zVarName, totalNum));
			pField->SetSolutionData(index+1, 
									GetData(iFor+1, xVarName, totalNum),
									GetData(iFor+1, yVarName, totalNum),
									GetData(iFor+1, zVarName, totalNum));
		}
		else
		{
			pField->SetSolutionData(index+1, 
									GetData(iFor+1, xVarName, totalNum),
									GetData(iFor+1, yVarName, totalNum),
									GetData(iFor+1, zVarName, totalNum));
		}

		// whether inject new seeds this time?
		if((iFor >= startInjection) && (iFor <= endInjection) && ((index%injectionTimeIncrement) == 0))
			bInject = true;

		// execute streakline
		if(bInject)				// inject new seeds
		{
			pStreakLine->execute((void *)&iFor, positions, pointers, true, iInjection, speeds);
			iInjection++;
		}	
		else					// do not inject new seeds
		{
			pStreakLine->execute((void *)&iFor, positions, pointers, false, iInjection, speeds);
		}
	}

	Reset();
	// release resource
	delete[] pointers;
	delete[] seedPtr;
	delete pStreakLine;
	delete pField;
	return true;
}