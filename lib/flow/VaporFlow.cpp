#ifdef WIN32
#pragma warning(disable : 4251 4100)
#endif

#include "vapor/VaporFlow.h"
#include "Rake.h"
#include "VTFieldLine.h"

using namespace VetsUtil;
using namespace VAPoR;

VaporFlow::VaporFlow(DataMgr* dm)
{
	dataMgr = dm;

	xVarName = NULL;
	yVarName = NULL;
	zVarName = NULL;

	numXForms = 0;
	minRegion[0] = minRegion[1] = minRegion[2] = 0;
	maxRegion[0] = maxRegion[1] = maxRegion[2] = 0;

	bUseRandomSeeds = true;
}

VaporFlow::~VaporFlow()
{
	//Don't delete dataMgr, it can still be used by calling program.
	//if(dataMgr != NULL)
		//delete dataMgr;

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
void VaporFlow::GetUserTimeStepSize(int frameNum)
{
//	userTimeStepSize = dataMgr->GetMetadata()->GetTSUserTime(frameNum);
}
void VaporFlow::ScaleTimeStepSizes(double userTimeStepMultiplier, 
								   double animationTimeStepMultiplier)
{
	userTimeStepSize *= userTimeStepMultiplier;
	animationTimeStepSize *= animationTimeStepMultiplier;
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
void VaporFlow::GetData(size_t ts, VECTOR3* pData, const int numNode)
{
	float *vx, *vy, *vz;

	// find low and high ts to sample data
	size_t lowT, highT;
	float ratio;
	lowT = (ts - startTimeStep)/timeStepIncrement;
	highT = lowT = 1;
	ratio = (float)((ts - startTimeStep)%timeStepIncrement)/(float)timeStepIncrement;
	
	// get low data
	vx = dataMgr->GetRegion(lowT, xVarName, numXForms, minRegion, maxRegion);
	vy = dataMgr->GetRegion(lowT, yVarName, numXForms, minRegion, maxRegion);
	vz = dataMgr->GetRegion(lowT, zVarName, numXForms, minRegion, maxRegion);
	for(int iFor = 0; iFor < numNode; iFor++)
		pData[iFor].Set(vx[iFor], vy[iFor], vz[iFor]);

	// get high data and interpolate
	vx = dataMgr->GetRegion(highT, xVarName, numXForms, minRegion, maxRegion);
	vy = dataMgr->GetRegion(highT, yVarName, numXForms, minRegion, maxRegion);
	vz = dataMgr->GetRegion(highT, zVarName, numXForms, minRegion, maxRegion);
	for(int iFor = 0; iFor < numNode; iFor++)
		pData[iFor].Set(Lerp(pData[iFor][0], vx[iFor], ratio),
						Lerp(pData[iFor][1], vy[iFor], ratio),
						Lerp(pData[iFor][2], vz[iFor], ratio));
}

//////////////////////////////////////////////////////////////////////////
// generate streamlines
//////////////////////////////////////////////////////////////////////////
bool VaporFlow::GenStreamLines(float* positions, 
							   int maxPoints, 
							   unsigned int randomSeed)
{
	//first generate seeds
	float* seedPtr;
	int seedNum;
	seedNum = numSeeds[0]*numSeeds[1]*numSeeds[2];
	seedPtr = new float[seedNum*3];
	SeedGenerator* pSeedGenerator = new SeedGenerator(minRakeExt, maxRakeExt, numSeeds);
	pSeedGenerator->GetSeeds(seedPtr, bUseRandomSeeds, randomSeed);
	delete pSeedGenerator;

	// create field object
	CVectorField* pField;
	Solution* pSolution;
	CartesianGrid* pCartesianGrid;
	VECTOR3** ppVector;
	int totalNum = (int)(maxRegion[0]-minRegion[0]+1)*(int)(maxRegion[1]-minRegion[1]+1)*(int)(maxRegion[2]-minRegion[2]+1);
	ppVector = new VECTOR3*[1];
	ppVector[0] = new VECTOR3[totalNum];
	GetData(0, ppVector[0], totalNum);
	pSolution = new Solution(ppVector, totalNum, 1);
	pCartesianGrid = new CartesianGrid( (int)(maxRegion[0]-minRegion[0]+1), 
										(int)(maxRegion[1]-minRegion[1]+1), 
										(int)(maxRegion[2]-minRegion[2]+1));
	pCartesianGrid->ComputeBBox();
	pField = new CVectorField(pCartesianGrid, pSolution, 1);
	
	// execute streamline
	vtCStreamLine* pStreamLine;
	float currentT = 0.0;
	pStreamLine = new vtCStreamLine(pField);
	pStreamLine->setBackwardTracing(false);
	pStreamLine->SetLowerUpperAngle(3.0, 15.0);
	pStreamLine->setMaxPoints(maxPoints);
	pStreamLine->SetMaxStepSize(2.0);
	pStreamLine->setSeedPoints(seedPtr, seedNum, currentT);
	pStreamLine->SetInitStepSize(initialStepSize);
	pStreamLine->setIntegrationOrder(FOURTH);
	pStreamLine->execute((void *)&currentT, positions);
	
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
							   int injectionTimeIncrement)
{
	//first generate seeds
	float* seedPtr;
	int seedNum;
	seedNum = numSeeds[0]*numSeeds[1]*numSeeds[2];
	seedPtr = new float[seedNum*3];
	SeedGenerator* pSeedGenerator = new SeedGenerator(minRakeExt, maxRakeExt, numSeeds);
	pSeedGenerator->GetSeeds(seedPtr, bUseRandomSeeds, randomSeed);
	delete pSeedGenerator;

	// create field object
	CVectorField* pField;
	Solution* pSolution;
	CartesianGrid* pCartesianGrid;
	VECTOR3** ppVector;
	int numInjections;						// times to inject new seeds
	int timeSteps;							// total "time steps"
	int totalNum = (int)(maxRegion[0]-minRegion[0]+1)*(int)(maxRegion[1]-minRegion[1]+1)*(int)(maxRegion[2]-minRegion[2]+1);
	numInjections = 1 + ((endInjection - startInjection)/injectionTimeIncrement);
	timeSteps = numInjections+1;
	ppVector = new VECTOR3*[timeSteps];
	for(int iFor = 0; iFor < timeSteps; iFor++)		ppVector[iFor] = NULL;
	pSolution = new Solution(ppVector, totalNum, timeSteps);
	int temp = (endInjection+injectionTimeIncrement)<endTimeStep?(endInjection+injectionTimeIncrement):endTimeStep;
	pSolution->SetTime(startInjection, temp, injectionTimeIncrement);
	pCartesianGrid = new CartesianGrid((int)(maxRegion[0]-minRegion[0]+1), 
									   (int)(maxRegion[1]-minRegion[1]+1), 
									   (int)(maxRegion[2]-minRegion[2]+1));
	pCartesianGrid->ComputeBBox();
	pField = new CVectorField(pCartesianGrid, pSolution, timeSteps);

	// initialize streak line
	vtCStreakLine* pStreakLine;
	float currentT = 0.0;
	pStreakLine = new vtCStreakLine(pField);
	pStreakLine->SetTimeDir(FORWARD);
	pStreakLine->setParticleLife(maxPoints);
	pStreakLine->SetLowerUpperAngle(3.0, 15.0);
	pStreakLine->SetMaxStepSize(2.0);
	pStreakLine->setSeedPoints(seedPtr, seedNum, currentT);
	pStreakLine->setIntegrationOrder(FOURTH);

	// start to computer streakline
	int iInjection = 0;
	for(int iFor = startInjection; iFor < endInjection; iFor++)
	{
		int index = iFor - startInjection;

		// need get new data
		if((iFor/injectionTimeIncrement) == 0)
		{
			if(index == 0)
			{
				VECTOR3* pData1 = new VECTOR3[totalNum];
				VECTOR3* pData2 = new VECTOR3[totalNum];
				GetData(iFor, pData1, totalNum);
				GetData(iFor+injectionTimeIncrement, pData2, totalNum);
				pField->SetSolutionData(iInjection, pData1);
				pField->SetSolutionData(iInjection+1, pData2);
			}
			else if((index/injectionTimeIncrement) == 0)
			{
				VECTOR3* pData2 = new VECTOR3[totalNum];
				GetData(iFor+injectionTimeIncrement, pData2, totalNum);
				pField->SetSolutionData(iInjection+1, pData2);
			}
		}
		
		// execute streakline
		if((iFor/injectionTimeIncrement) == 0)
		{
			pStreakLine->execute((void *)&iFor, positions, true, iInjection);
			iInjection++;
		}
		else
			pStreakLine->execute((void *)&iFor, positions, false, iInjection);

		// release previous memory for original data
	}

	return true;
}