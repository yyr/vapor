#ifdef WIN32
#pragma warning(disable : 4244 4251 4267 4100 4996)
#endif

#include "vapor/VaporFlow.h"
#include "vapor/flowlinedata.h"
#include "vapor/errorcodes.h"
#include "Rake.h"
#include "VTFieldLine.h"
#include "math.h"


using namespace VetsUtil;
using namespace VAPoR;



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

	minPriorityVal = 0.f;
	maxPriorityVal = 1.e30f;
	seedDistBias = 0.f;

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
	steadyUserTimeStepMultiplier = 1.0;
	unsteadyUserTimeStepMultiplier = 1.0;
	steadyAnimationTimeStepMultiplier = 1.0;
	unsteadyAnimationTimeStepMultiplier = 1.0;
	
	bUseRandomSeeds = false;
	periodicDim[0]= periodicDim[1]= periodicDim[2]= false;

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
	

	userTimeStepSize = 1.0f;
	animationTimeStepSize = 1.0;
	
	steadyUserTimeStepMultiplier = 1.0;
	unsteadyUserTimeStepMultiplier = 1.0;
	steadyAnimationTimeStepMultiplier = 1.0;
	unsteadyAnimationTimeStepMultiplier = 1.0;

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
						  const size_t max_bdim[3],
						  size_t fullGridHeight)
{
	full_height = fullGridHeight;
	numXForms = num_xforms;
	size_t fullDims[3];
	const std::vector<double> extents = dataMgr->GetExtents();
	
	size_t fullDataSize[3];
	dataMgr->GetDim(fullDataSize,num_xforms);
	dataMgr->GetDim(fullDims,-1);
	for (int i = 0; i< 3; i++){
		minBlkRegion[i] = min_bdim[i];
		maxBlkRegion[i] = max_bdim[i];
		minRegion[i] = min[i];
		maxRegion[i] = max[i];
		if (min[i] == 0 && max[i] == (fullDataSize[i]-1)) fullInDim[i] = true; 
		else fullInDim[i] = false;
		//Establish the period, in case the data is periodic.
		flowPeriod[i] = (extents[i+3] - extents[i])*((float)fullDims[i])/((float)(fullDims[i]-1)); 
	}

	
	
}

//////////////////////////////////////////////////////////////////////////
// specify the spatial extent and resolution of the rake.  Needed for
// accessing fields that are bounded by the rake, such as the
// biased random rake
//////////////////////////////////////////////////////////////////////////
void VaporFlow::SetRakeRegion(const size_t min[3], 
						  const size_t max[3],
						  const size_t min_bdim[3],
						  const size_t max_bdim[3])
{
	for (int i = 0; i< 3; i++){
		minBlkRake[i] = min_bdim[i];
		maxBlkRake[i] = max_bdim[i];
		minRake[i] = min[i];
		maxRake[i] = max[i];
	}
}

//////////////////////////////////////////////////////////////////////////////////
//  Setup Time step list for unsteady integration.. Replaces above method
//////////////////////////////////////////////////////////////////
void VaporFlow::SetUnsteadyTimeSteps(int timeStepList[], size_t numSteps){
	unsteadyTimestepList = timeStepList;
	numUnsteadyTimesteps = numSteps;
}

//////////////////////////////////////////////////////////////////////////
// specify the userTimeStepSize and animationTimeStepSize as multiples
// of the userTimeStepSize
//////////////////////////////////////////////////////////////////////////
void VaporFlow::ScaleSteadyTimeStepSizes(double userTimeStepMultiplier, 
								   double animationTimeStepMultiplier)
{
	steadyUserTimeStepMultiplier = userTimeStepMultiplier;
	steadyAnimationTimeStepMultiplier = animationTimeStepMultiplier;
}
void VaporFlow::ScaleUnsteadyTimeStepSizes(double userTimeStepMultiplier, 
								   double animationTimeStepMultiplier)
{
	unsteadyUserTimeStepMultiplier = userTimeStepMultiplier;
	unsteadyAnimationTimeStepMultiplier = animationTimeStepMultiplier;
}

//////////////////////////////////////////////////////////////////////////
// specify a set of seed points randomly generated over the specified
// spatial interval. Points can be in axis aligned dimension 0, 1, 2, 3
// Obsolete, replaced by Distributed version below.
//////////////////////////////////////////////////////////////////////////
/*
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
*/
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
	
	float *regionData = dataMgr->GetRegion(ts, varName, (int)numXForms, minBlkRegion, maxBlkRegion,1);
	
	if (!regionData) {
		SetErrMsg(VAPOR_ERROR_FLOW,"Error obtaining field\ndata for timestep %d, variable %s",ts, varName);
		return 0;
	}
	return regionData;
}
//////////////////////////////////////////////////////////////////////////
// Obtain data for three components of a vector field. 
// Ignore if the variable name is "0"
// Resulting data array is assigned to float** arguments.
// If data is not obtained, unlock previously obtained data
//////////////////////////////////////////////////////////////////////////
bool VaporFlow::Get3Data(size_t ts, 
			const char* xVarName,const char* yVarName,const char* zVarName,
			float** pUData, float** pVData, float** pWData)
{
	bool zeroX = (strcmp(xVarName, "0") == 0); 
	bool zeroY = (strcmp(yVarName, "0") == 0); 
	bool zeroZ = (strcmp(zVarName, "0") == 0); 
	float* xDataPtr = 0, *yDataPtr = 0, *zDataPtr = 0;
	if(!zeroX) xDataPtr = GetData(ts,xVarName);
	if((zeroX || xDataPtr) && !zeroY) yDataPtr = GetData(ts,yVarName);
	if((zeroX || xDataPtr) && (zeroY || yDataPtr) && !zeroZ) zDataPtr = GetData(ts,zVarName);
	//Check if we failed:
	if ((!zeroX && (xDataPtr == 0)) || (!zeroY && (yDataPtr == 0)) || (!zeroZ && (zDataPtr == 0))){
					
		if (!zeroX && xDataPtr) dataMgr->UnlockRegion(xDataPtr);
		if (!zeroY && yDataPtr) dataMgr->UnlockRegion(yDataPtr);
		return false;
	}
	if(!zeroX) *pUData = xDataPtr;
	if(!zeroY) *pVData = yDataPtr;
	if(!zeroZ) *pWData = zDataPtr;
	return true;
}
//Generate the seeds for the rake.  If rake is random calculates distributed seeds 
int VaporFlow::GenRakeSeeds(float* seeds, int timeStep, unsigned int randomSeed, int stride){
	int seedNum;
	seedNum = (int)(numSeeds[0]*numSeeds[1]*numSeeds[2]);
	SeedGenerator* pSeedGenerator = new SeedGenerator(minRakeExt, maxRakeExt, numSeeds);
	if (bUseRandomSeeds && seedDistBias != 0.f){
		pSeedGenerator->SetSeedDistrib(seedDistBias, timeStep, numXForms,xSeedDistVarName, ySeedDistVarName, zSeedDistVarName);
	}
	pSeedGenerator->GetSeeds(this, seeds, bUseRandomSeeds, randomSeed, stride);
	delete pSeedGenerator;
	return seedNum;
}




//////////////////////////////////////////////////////////////////////////////////////
// Find the highest priority points in the steady flow.
// Uses the same region mapping as the field region.
// Also can be used for maximizing over multiple forward advections, by using
// pathContainer = 0.
// If the pathContainer is null, then the min/max priority values are ignored,
// the direction is assumed to be 1 but is changed to steadyFlowDirection
// the final values are put back into the FlowLineData at the integ. start position, and
// END_FLOW_FLAGS are bypassed (they don't end the search for max).
//
bool VaporFlow::prioritizeSeeds(FlowLineData* container, PathLineData* pathContainer, int timeStep){
	//Use the current Region, obtain the priority field.
	//First map the region to doubles:
	double minDouble[3], maxDouble[3];
	
	dataMgr->MapVoxToUser((size_t)-1,minRegion, minDouble, (int)numXForms);
	dataMgr->MapVoxToUser((size_t)-1,maxRegion, maxDouble, (int)numXForms);
	//Use the current region bounds, not the rake bounds...
	FieldData* fData = setupFieldData(xPriorityVarName, yPriorityVarName, zPriorityVarName, 
		false, (int)numXForms, timeStep, false);
	if (!fData) return false;
	//Note that pathContainer is 0 if we are prioritizing after advection.
	//In that case the search does not stop if minPriorityVal or maxPriorityVal are attained.
	//If we are doing multi-advection, create an array to hold the maximizing points
	float** maxPointHolder = 0;
	if (!pathContainer) {
		maxPointHolder = new float*[container->getNumLines()];
		for (int i = 0; i<container->getNumLines(); i++) maxPointHolder[i] = 0;
	}
	//Go through the seeds, calculate the prioritization magnitude at each point, starting
	//At the seed, going first backwards and then forwards
	
	for (int line = 0; line< container->getNumLines(); line++){
		float maxVal =  - 1.f;
		
		int startPos = container->getSeedPosition();
		float* maxPoint = 0;
		bool searchOver = false;
		
		float startMag = -1.f;
		float* startPt = 0;
		if (pathContainer){
			//Initialize with starting point, if pathContainer exists
			startPt = container->getFlowPoint(line,startPos);
			if (startPt[0] == END_FLOW_FLAG || startPt[0] == STATIONARY_STREAM_FLAG)
				continue; // if the seed is already out, the line is bad
			startMag = fData->getFieldMag(startPt);
		}
		//First search backwards (if pathContainer != 0)
		if (pathContainer && steadyFlowDirection <= 0){
			for (int ptindx = startPos; ptindx >= container->getStartIndex(line); ptindx--){
				float* pt = container->getFlowPoint(line, ptindx);
				if (pt[0] == END_FLOW_FLAG || pt[0] == STATIONARY_STREAM_FLAG) {
					if (pathContainer) break;
					else continue;
				}
				float val = fData->getFieldMag(pt);
				if(val < 0.f) {// point is out of region
					if (pathContainer) break;
					else continue;
				}
				if (pathContainer && val < minPriorityVal*startMag) {
					//If val is below min, don't search further this way:
					
					//shorten the steady flow line:
					container->setFlowStart(line, ptindx - startPos);
					break;
				}
				if (val > maxVal){ //establish a new max
					maxVal = val;
					maxPoint = pt;
				}
				if (pathContainer && val >= maxPriorityVal) {maxPoint = pt; searchOver = true; break;}
			}
		}
		if (!searchOver && ((!pathContainer) || steadyFlowDirection >= 0)){
			for (int ptindx = startPos; ptindx <= container->getEndIndex(line); ptindx++){
				float* pt = container->getFlowPoint(line, ptindx);
				if (pt[0] == END_FLOW_FLAG || pt[0] == STATIONARY_STREAM_FLAG)	{
					if (pathContainer) break;
					else continue;
				}
				float val = fData->getFieldMag(pt);
				if(val < 0.f) {
					if (pathContainer) break;
					else continue;
				}
				if (pathContainer && val < minPriorityVal*startMag) {
					//Shorten the steady field line to end at pt:
					container->setFlowEnd(line, ptindx - startPos);
					break;
				}
				if (val > maxVal){ //establish a new max
					maxVal = val;
					maxPoint = pt;
				}
				if (pathContainer && val >= maxPriorityVal) {maxPoint = pt; break;}
			}
		}
		//Now put the winner back into the steady flow data at position 0,
		//or into the path container.
		//Make appropriate modifications to that flowLine if necessary
		
		
		if (pathContainer){
			if (maxPoint)//If no winner, do nothing:
				pathContainer->setPointAtTime(line,(float)timeStep, maxPoint[0],maxPoint[1],maxPoint[2]);
		} else {
			//Insert the point, or a null if there is no point, this is tested-for
			//before inserting below.
			maxPointHolder[line] = maxPoint;
		}
	
		
	}//end loop over line
	//Now move the seeds in the container to the appropriate position for steady flow advection
	if (!pathContainer){
		container->setFlowDirection(steadyFlowDirection);
		for (int line = 0; line< container->getNumLines(); line++){
			container->setFlowStart(line, 0);
			container->setFlowEnd(line, 0);
			if (maxPointHolder[line])
				container->setFlowPoint(line, 0, 
					maxPointHolder[line][0],maxPointHolder[line][1],maxPointHolder[line][2]);
			else 
				container->setFlowPoint(line, 0, END_FLOW_FLAG,END_FLOW_FLAG,END_FLOW_FLAG);
		}
		delete [] maxPointHolder;
	}
	//Release the locks on the prioritization field..
	fData->releaseData(dataMgr);
	delete fData;
	
	return true;
}
void VaporFlow::SetPriorityField(const char* varx, const char* vary, const char* varz,
									   float minField , float maxField)
{
	if(!xPriorityVarName)
		xPriorityVarName = new char[260];
	if(!yPriorityVarName)
		yPriorityVarName = new char[260];
	if(!zPriorityVarName)
		zPriorityVarName = new char[260];
	strcpy(xPriorityVarName, varx);
	strcpy(yPriorityVarName, vary);
	strcpy(zPriorityVarName, varz); 
	minPriorityVal = minField;
	maxPriorityVal = maxField;
}					
void VaporFlow::SetDistributedSeedPoints(const float min[3], const float max[3], int numSeeds, 
	const char* varx, const char* vary, const char* varz, float bias)
{
	assert( bias >= -15.f && bias <= 15.f);
	for(int iFor = 0; iFor < 3; iFor++)
	{
		minRakeExt[iFor] = min[iFor];
		maxRakeExt[iFor] = max[iFor];
	}
	this->numSeeds[0] = numSeeds;
	this->numSeeds[1] = 1;
	this->numSeeds[2] = 1;

	bUseRandomSeeds = true;
	if(!xSeedDistVarName)
		xSeedDistVarName = new char[260];
	if(!ySeedDistVarName)
		ySeedDistVarName = new char[260];
	if(!zSeedDistVarName)
		zSeedDistVarName = new char[260];
	strcpy(xSeedDistVarName, varx);
	strcpy(ySeedDistVarName, vary);
	strcpy(zSeedDistVarName, varz); 
	
	seedDistBias = bias;
}			

bool VaporFlow::GenStreamLines(FlowLineData* container, unsigned int randomSeed){
	// first generate seeds
	float* seedPtr;
	int seedNum;
	seedNum = (int)(numSeeds[0]*numSeeds[1]*numSeeds[2]);
	assert(seedNum == container->getNumLines());
	seedPtr = new float[seedNum*3];
	SeedGenerator* pSeedGenerator = new SeedGenerator(minRakeExt, maxRakeExt, numSeeds);
	if (bUseRandomSeeds && seedDistBias != 0.f)
		pSeedGenerator->SetSeedDistrib(seedDistBias, steadyStartTimeStep, (int)numXForms,
			xSeedDistVarName,ySeedDistVarName,zSeedDistVarName);
	bool rc = pSeedGenerator->GetSeeds(this, seedPtr, bUseRandomSeeds, randomSeed);
	delete pSeedGenerator;
	if (!rc) return false;
	//Then do streamlines with prepared seeds:
	rc = GenStreamLinesNoRake(container, seedPtr);
	delete [] seedPtr;
	return rc;

}
//General new version of GenStreamLines, uses FlowLineData
bool VaporFlow::GenStreamLinesNoRake(FlowLineData* container,
							   float* seedPtr)
{
	int numSeeds = container->getNumLines();
	
	// scale animationTimeStep and userTimeStep.  
	
	userTimeStepSize = steadyUserTimeStepMultiplier;
	animationTimeStepSize = steadyAnimationTimeStepMultiplier;
	
	// create field object
	CVectorField* pField;
	Solution* pSolution;
	CartesianGrid* pCartesianGrid;
	float **pWData;
	float **pVData;
	float **pUData;
	int totalXNum = (int)(maxBlkRegion[0]-minBlkRegion[0]+1)* dataMgr->GetBlockSize()[0];
	int totalYNum = (int)(maxBlkRegion[1]-minBlkRegion[1]+1)* dataMgr->GetBlockSize()[1];
	int totalZNum = (int)(maxBlkRegion[2]-minBlkRegion[2]+1)* dataMgr->GetBlockSize()[2];
	int totalNum = totalXNum*totalYNum*totalZNum;
	bool zeroX = (strcmp(xSteadyVarName, "0") == 0); 
	bool zeroY = (strcmp(ySteadyVarName, "0") == 0); 
	bool zeroZ = (strcmp(zSteadyVarName, "0") == 0); 
	if(!zeroX) pUData = new float*[1];
	else pUData = 0;
	if(!zeroY) pVData = new float*[1];
	else pVData = 0;
	if(!zeroZ) pWData = new float*[1];
	else pWData = 0;

	bool gotData = Get3Data(steadyStartTimeStep, xSteadyVarName, ySteadyVarName,
		zSteadyVarName, pUData, pVData, pWData);
	
	if (!gotData) 
		return false;
	
	pSolution = new Solution(pUData, pVData, pWData, totalNum, 1);
	pSolution->SetTimeScaleFactor((float)steadyUserTimeStepMultiplier);
	pSolution->SetTime((int)steadyStartTimeStep, (int)steadyStartTimeStep);
	pCartesianGrid = new CartesianGrid(totalXNum, totalYNum, totalZNum, 
		regionPeriodicDim(0),regionPeriodicDim(1),regionPeriodicDim(2),maxRegion);
	pCartesianGrid->setPeriod(flowPeriod);
	// set the boundary of physical grid
	VECTOR3 minB, maxB, minR, maxR;
	double minUser[3], maxUser[3], regMin[3],regMax[3];
	size_t blockRegionMin[3],blockRegionMax[3];
	const size_t* bs = dataMgr->GetBlockSize();
	for (int i = 0; i< 3; i++){
		blockRegionMin[i] = bs[i]*minBlkRegion[i];
		blockRegionMax[i] = bs[i]*(maxBlkRegion[i]+1)-1;
	}
	dataMgr->MapVoxToUser((size_t)-1, blockRegionMin, minUser, (int)numXForms);
	dataMgr->MapVoxToUser((size_t)-1, blockRegionMax, maxUser, (int)numXForms);
	dataMgr->MapVoxToUser((size_t)-1, minRegion, regMin, (int)numXForms);
	dataMgr->MapVoxToUser((size_t)-1, maxRegion, regMax, (int)numXForms);
	
	//Use current region to determine coords of grid boundary:
	
	//Now adjust minB, maxB to block region extents:
	
	for (int i = 0; i< 3; i++){
		minB[i] = (float)minUser[i];
		maxB[i] = (float)maxUser[i];
		minR[i] = (float)regMin[i];
		maxR[i] = (float)regMax[i];
	}
	
	pCartesianGrid->SetRegionExtents(minR,maxR);
	pCartesianGrid->SetBoundary(minB, maxB);
	pField = new CVectorField(pCartesianGrid, pSolution, 1);
	pField->SetUserTimeStepInc(0);
	
	// create streamline
	vtCStreamLine* pStreamLine = new vtCStreamLine(pField);

	float currentT = (float)steadyStartTimeStep;

	//AN: for memory debugging:
	//pStreamLine->fullArraySize = container->getMaxPoints()*seedNum*3;

	// by default, tracing is bidirectional
	if (steadyFlowDirection > 0)
		pStreamLine->setBackwardTracing(false);
	else if (steadyFlowDirection < 0)
		pStreamLine->setForwardTracing(false);

	pStreamLine->setMaxPoints(container->getMaxPoints());
	//test the seeds, but don't count the ones that are already outside from previous advection
	int seedsInRegion = 0;
	int totalSeeds = numSeeds;
	for (int i = 0; i<numSeeds; i++){
		if (seedPtr[3*i] == END_FLOW_FLAG){
			totalSeeds--;
			continue;
		}
		int j; 
		for (j = 0; j< 3; j++){
			if (regionPeriodicDim(j)) continue;
			if (seedPtr[3*i+j] < regMin[j] || seedPtr[3*i+j] > regMax[j]){
				break;
			}
		}
		if(j==3) seedsInRegion++;
	}
	if (seedsInRegion == 0){
		MyBase::SetErrMsg(VAPOR_WARNING_FLOW,
			" No seed points are in region at timestep %d",
			steadyStartTimeStep);
		return false;
	} else if (seedsInRegion < totalSeeds){
		MyBase::SetErrMsg(VAPOR_WARNING_FLOW,
			"Only %d of %d seeds are in region\nat timestep %d:",
			seedsInRegion, numSeeds, steadyStartTimeStep);
	}
	pStreamLine->setSeedPoints(seedPtr, numSeeds, currentT);
	pStreamLine->SetSamplingRate((float)animationTimeStepSize);
	pStreamLine->SetInitStepSize(initialStepSize);
	pStreamLine->SetMaxStepSize(maxStepSize);
	pStreamLine->setIntegrationOrder(FOURTH);
	pStreamLine->SetStationaryCutoff((0.1f*pCartesianGrid->GetGridSpacing(0))/(container->getMaxPoints()*(float)steadyAnimationTimeStepMultiplier));
	pStreamLine->computeStreamLine(currentT, container);
	
	//AN: Removed a call to Reset() here. This requires all vapor flow state to be
	// reestablished each time flow lines are generated.
	// release resource
	delete pStreamLine;
	delete pField;
	//Unlock region:

	if(pUData)dataMgr->UnlockRegion(pUData[0]);
	if(pVData)dataMgr->UnlockRegion(pVData[0]);
	if(pWData)dataMgr->UnlockRegion(pWData[0]);
		
	return true;
}

void VaporFlow::SetPeriodicDimensions(bool xdim, bool ydim, bool zdim){
	periodicDim[0] = xdim;
	periodicDim[1] = ydim;
	periodicDim[2] = zdim;
}

/////////////////////////////////////////////////////////////////
//Version of GenStreamLines to be used with field line advection.
//Gets the seeds from the unsteady container.  Resets the seed value
//if prioritize is true.  Eventually should handle timeSteps that are
//not in the set of unsteady sample times.  Expects a sample timestep
///////////////////////////////////////////////////////////////
bool VaporFlow::GenStreamLines (FlowLineData* steadyContainer, PathLineData* unsteadyContainer, int timeStep, bool prioritize){
	//First make a list of the seeds from the unsteady Container:
	int numSeeds = unsteadyContainer->getNumLines();
	assert(steadyContainer->getNumLines() == unsteadyContainer->getNumLines());
	float* seedList = new float[3*numSeeds];
	for (int i = 0; i< numSeeds; i++){
		float *seed = unsteadyContainer->getPointAtTime(i, (float)timeStep);
		if (!seed) {//put in dummy values:
			for (int j = 0; j< 3; j++){
				seedList[3*i+j] = END_FLOW_FLAG;
			}
		} else {
			for (int j = 0; j< 3; j++){
				seedList[3*i+j] = seed[j];
			}
		}
	}
	GenStreamLinesNoRake(steadyContainer, seedList);
	delete seedList;
	if (prioritize){
		//This had better be a sample time!
		prioritizeSeeds(steadyContainer, unsteadyContainer, timeStep);
	}
	return true;
}
/////////////////////////////////////////////////////////////////////////////////////////
//Perform unsteady integration to extend pathlines from one time step to another.
//Seeds are obtained from the startTimeStep, and integrated to endTimeStep.
//(Eventually seeds will be added enroute?)
//This can be either backwards or forwards in time.
//The doingFLA argument is passed to sampleFieldLines, so that unsteady field lines 
//that end before the next timestep are not inserted into that next timeStep
/////////////////////////////////////////////////////////////////////////////////////
bool VaporFlow::ExtendPathLines(PathLineData* container, int startTimeStep, int endTimeStep
								,bool doingFLA){
	animationTimeStepSize = unsteadyAnimationTimeStepMultiplier;

	//Make the container aware of the correct direction (note that this can change)
	TIME_DIR timeDir;
	assert (endTimeStep != startTimeStep);
	if (startTimeStep < endTimeStep) {
		timeDir = FORWARD;
		container->setPathDirection(1);
	}
	else {
		timeDir = BACKWARD;
		container->setPathDirection(-1);
	}
	// create field object
	CVectorField* pField;
	Solution* pSolution;
	CartesianGrid* pCartesianGrid;
	float **pUData, **pVData, **pWData;
	
	int totalXNum, totalYNum, totalZNum, totalNum;
	
    totalXNum = (int)(maxBlkRegion[0]-minBlkRegion[0] + 1)* dataMgr->GetBlockSize()[0];
	totalYNum = (int)(maxBlkRegion[1]-minBlkRegion[1] + 1)* dataMgr->GetBlockSize()[1];
	totalZNum = (int)(maxBlkRegion[2]-minBlkRegion[2] + 1)* dataMgr->GetBlockSize()[2];
	totalNum = (int)(totalXNum*totalYNum*totalZNum);
	
	//realStartTime and realEndTime are actual limits of time steps for which positions
	//are calculated.
	
	
	//Find the start/end index into the time step sample list.
	//Note that the unsteadyTimestepList is in forward time order
	int sampleStartIndex = -1;
	int sampleEndIndex = -1;
	for (int indx = 0; indx< (int)numUnsteadyTimesteps; indx++){
		if (unsteadyTimestepList[indx] == startTimeStep) sampleStartIndex = indx;
		if (unsteadyTimestepList[indx] == endTimeStep) sampleEndIndex = indx;
	}
	assert(sampleStartIndex >= 0 && sampleEndIndex >= 0);
	//numTimesteps is the number of time steps needed
	int numTimesteps = abs(endTimeStep - startTimeStep) + 1;
	int numTimeSamples = abs(sampleStartIndex - sampleEndIndex) + 1;
	bool zeroX = (strcmp(xUnsteadyVarName, "0") == 0); 
	bool zeroY = (strcmp(yUnsteadyVarName, "0") == 0); 
	bool zeroZ = (strcmp(zUnsteadyVarName, "0") == 0); 
	if (zeroX) pUData = 0;
	else {
		pUData = new float*[numTimeSamples];
		memset(pUData, 0, sizeof(float*)*numTimeSamples);
	}
	if (zeroY) pVData = 0;
	else {
		pVData = new float*[numTimeSamples];
		memset(pVData, 0, sizeof(float*)*numTimeSamples);
	}
	if (zeroZ) pWData = 0;
	else {
		pWData = new float*[numTimeSamples];
		memset(pWData, 0, sizeof(float*)*numTimeSamples);
	}
	pSolution = new Solution(pUData, pVData, pWData, totalNum, numTimesteps);
	pSolution->SetTimeScaleFactor((float)unsteadyUserTimeStepMultiplier);
	
	pSolution->SetTime(startTimeStep, endTimeStep);
	pCartesianGrid = new CartesianGrid(totalXNum, totalYNum, totalZNum, 
		regionPeriodicDim(0),regionPeriodicDim(1),regionPeriodicDim(2),maxRegion);
	pCartesianGrid->setPeriod(flowPeriod);
	
	// set the boundary of physical grid
	
	const size_t* bs = dataMgr->GetBlockSize();
	VECTOR3 minB, maxB, minR, maxR;
	double minUser[3], maxUser[3], regMin[3],regMax[3];
	size_t blockRegionMin[3],blockRegionMax[3];
	for (int i = 0; i< 3; i++){
		blockRegionMin[i] = bs[i]*minBlkRegion[i];
		blockRegionMax[i] = bs[i]*(maxBlkRegion[i]+1)-1;
	}
	dataMgr->MapVoxToUser((size_t)-1, blockRegionMin, minUser, (int)numXForms);
	dataMgr->MapVoxToUser((size_t)-1, blockRegionMax, maxUser, (int)numXForms);
	dataMgr->MapVoxToUser((size_t)-1, minRegion, regMin, (int)numXForms);
	dataMgr->MapVoxToUser((size_t)-1, maxRegion, regMax, (int)numXForms);
	
	minB.Set((float)minUser[0], (float)minUser[1], (float)minUser[2]);
	maxB.Set((float)maxUser[0], (float)maxUser[1], (float)maxUser[2]);
	minR.Set((float)regMin[0], (float)regMin[1], (float)regMin[2]);
	maxR.Set((float)regMax[0], (float)regMax[1], (float)regMax[2]);
	pCartesianGrid->SetBoundary(minB, maxB);
	pCartesianGrid->SetRegionExtents(minR,maxR);
	
	//Now set the region bound:

	pField = new CVectorField(pCartesianGrid, pSolution, numTimesteps);
	
	// get the userTimeStep between pairs of timesteps
	float* pUserTimeSteps;
	int tIndex = 0;
	pUserTimeSteps = new float[numTimesteps];

	//Calculate the time differences between successive sampled timesteps in
	//the interval we are working on.
	//save that value in pUserTimeSteps, available in pField.
	//These are negative if we are advecting backwards in time!!

	for(int sampleIndex = sampleStartIndex; sampleIndex != sampleEndIndex; sampleIndex += timeDir)
	{
		int prevSampledStep = unsteadyTimestepList[sampleIndex];
		int nextSampledStep = unsteadyTimestepList[sampleIndex+timeDir];
		const vector<double>& prevTime = dataMgr->GetTSUserTime(prevSampledStep);
		const vector<double>& nextTime = dataMgr->GetTSUserTime(nextSampledStep);
		if (prevTime.size()> 0 && nextTime.size()> 0)
			pUserTimeSteps[tIndex] = (float)(nextTime[0] - prevTime[0]);
		else
			pUserTimeSteps[tIndex] = (float)(nextSampledStep - prevSampledStep);
		//following should make the value always positive
		pUserTimeSteps[tIndex++] *= timeDir;
		assert(pUserTimeSteps[tIndex-1] >= 0.f);
	}
	pField->SetUserTimeSteps(pUserTimeSteps);

	// initialize streak line
	vtCStreakLine* pStreakLine;
	
	pStreakLine = new vtCStreakLine(pField);
	
	pStreakLine->SetTimeDir(timeDir);
	
	pStreakLine->setParticleLife(container->getMaxPoints());
	//The sampling rate is the time-difference between samples.
	//AnimationTimeStepSize is the samples per timeStep
	pStreakLine->SetSamplingRate(1.f/animationTimeStepSize);
	pStreakLine->SetInitStepSize(initialStepSize);
	pStreakLine->SetMaxStepSize(maxStepSize);
	pStreakLine->setIntegrationOrder(FOURTH);


	//Keep first and next region pointers.  They must be released as we go:
	float *xDataPtr =0, *yDataPtr = 0, *zDataPtr =0;
	float *xDataPtr2 =0, *yDataPtr2 = 0, *zDataPtr2 =0;
	

	bool bInject;

	int currIndex = sampleStartIndex;
	int prevSample = startTimeStep;
	int nextSample = unsteadyTimestepList[sampleStartIndex+timeDir];
	int tsIndex=0; //Index that counts the sampled timesteps from 0
	// Go through all the timesteps, one at a time. Ifor is the time Step.
	// For each pair (prevSample, nextSample) integrate between them.
	for(int iFor = startTimeStep; iFor != endTimeStep; iFor += timeDir)
	{

		//Following sets prevSample, nextSample, currIndex
		if (iFor == nextSample) {//bump up the samples
			currIndex++;
			prevSample = nextSample;
			nextSample = unsteadyTimestepList[currIndex+timeDir];
		}
		
		tsIndex = abs(currIndex - sampleStartIndex);
		
		// get usertimestep differences between the current time step and previous and next sampled time steps
		//Currently only one vapor time step is integrated at a time.
		double diff = 0.0, curDiff = 0.0;
		const vector<double>& iforTime = dataMgr->GetTSUserTime(iFor);
		const vector<double>& prevTime = dataMgr->GetTSUserTime(prevSample);
		const vector<double>& nextTime = dataMgr->GetTSUserTime(nextSample);
		if (iforTime.size()>0 && prevTime.size()>0 && nextTime.size()> 0){
		
			diff = iforTime[0] -prevTime[0];
			curDiff = nextTime[0] - iforTime[0]; 
			
		} else {
			diff = (iFor - prevSample);
			curDiff = (nextSample - iFor);
			
		}
		
		pField->SetUserTimeStepInc((float)diff);
		float vaporTimeDiff = (float)(numTimesteps -1)/(float)(numTimeSamples -1);
		pField->SetUserTimePerVaporTS(abs((diff+curDiff)/vaporTimeDiff));
		//specify the number of time steps between time samples
		pSolution->SetTimeIncrement((numTimesteps -1)/(numTimeSamples -1), timeDir);
		// need get new field data?
		
		if (iFor == prevSample)
		{
			//For the very first sample time, get data for current time (get the next sampled timestep in next line)
			if(iFor == startTimeStep){
				bool gotData = Get3Data(iFor, xUnsteadyVarName, yUnsteadyVarName,
					zUnsteadyVarName, &xDataPtr, &yDataPtr, &zDataPtr);
				if(!gotData){
					delete[] pUserTimeSteps;
					delete pStreakLine;
					delete pField;
					return false;
				}
				pField->SetSolutionData(tsIndex,xDataPtr,yDataPtr,zDataPtr);
			} else { //Changing time sample..
				//If it's not the very first time, need to release data for previous 
				//time step, and move end ptrs to start:
				//Now can release first pointers:
				if(!zeroX)dataMgr->UnlockRegion(xDataPtr);
				if(!zeroY)dataMgr->UnlockRegion(yDataPtr);
				if(!zeroZ)dataMgr->UnlockRegion(zDataPtr);
				//And use them to save the second pointers:
				xDataPtr = xDataPtr2;
				yDataPtr = yDataPtr2;
				zDataPtr = zDataPtr2;
				//Also nullify pointers to released timestep data:
				pField->SetSolutionData(tsIndex-1,0,0,0);
			}
			//now get data for second ( next) sampled timestep. 
			bool gotData = Get3Data(nextSample, xUnsteadyVarName,
				yUnsteadyVarName, zUnsteadyVarName, &xDataPtr2, &yDataPtr2,
				&zDataPtr2);
			if(!gotData){
				// if we failed:  release resources for 2 time steps:
				delete[] pUserTimeSteps;
				delete pStreakLine;
				delete pField;
				if (!zeroX && xDataPtr) dataMgr->UnlockRegion(xDataPtr);
				if (!zeroY && yDataPtr) dataMgr->UnlockRegion(yDataPtr);
				if (!zeroZ && zDataPtr) dataMgr->UnlockRegion(zDataPtr);
				return false;
			}
			pField->SetSolutionData(tsIndex+1,xDataPtr2,yDataPtr2,zDataPtr2); 
		}

			
		//Set up the seeds for this time step.  All points that are already specified at the time step
		//are taken to be seeds for ExtendPathLines
		int nseeds = pStreakLine->addSeeds(iFor, container);
		bInject = (nseeds > 0);
		
		pStreakLine->execute((float)iFor, container, bInject, doingFLA);
		
	
	}

	
	// release resources.  we always have valid start and end pointers
	// at this point.
	if(!zeroX) dataMgr->UnlockRegion(xDataPtr);
	if(!zeroY) dataMgr->UnlockRegion(yDataPtr);
	if(!zeroZ) dataMgr->UnlockRegion(zDataPtr);
	if(!zeroX) dataMgr->UnlockRegion(xDataPtr2);
	if(!zeroY) dataMgr->UnlockRegion(yDataPtr2);
	if(!zeroZ) dataMgr->UnlockRegion(zDataPtr2);
	pField->SetSolutionData(tsIndex,0,0,0);
	pField->SetSolutionData(tsIndex+1,0,0,0);
	delete[] pUserTimeSteps;
	delete pStreakLine;
	delete pField;
	
	return true;
}
/////////////////////////////////////////////////////////////////////////////////////////
//Perform unsteady integration to extend fieldlines from one time step to another.
//Seeds are obtained by resampling from flArray[startTimeStep] up to maxNumSamples
//from each field line.
//These are then integrated to endTimeStep, placing intermediate points into the
//intermediate flowLineData's flArray[I], up through I = endTimeStep. 
//This can be either backwards or forwards in time.
//Like extendPathLines, but uses array of FlowLineData*'s as both a source and
//target.
/////////////////////////////////////////////////////////////////////////////////////
bool VaporFlow::AdvectFieldLines(FlowLineData** flArray, int startTimeStep, int endTimeStep, int maxNumSamples)
{
	animationTimeStepSize = unsteadyAnimationTimeStepMultiplier;

	//Make the container aware of the correct direction (note that this can change)
	TIME_DIR timeDir;
	assert (endTimeStep != startTimeStep);
	if (startTimeStep < endTimeStep) {
		timeDir = FORWARD;
	}
	else {
		timeDir = BACKWARD;
	}
	
	// create field object
	CVectorField* pField;
	Solution* pSolution;
	CartesianGrid* pCartesianGrid;
	float **pUData = 0, **pVData = 0, **pWData = 0;
	
	int totalXNum, totalYNum, totalZNum, totalNum;
	
    totalXNum = (maxBlkRegion[0]-minBlkRegion[0] + 1)* dataMgr->GetBlockSize()[0];
	totalYNum = (maxBlkRegion[1]-minBlkRegion[1] + 1)* dataMgr->GetBlockSize()[1];
	totalZNum = (maxBlkRegion[2]-minBlkRegion[2] + 1)* dataMgr->GetBlockSize()[2];
	totalNum = totalXNum*totalYNum*totalZNum;
	
	//realStartTime and realEndTime are actual limits of time steps for which positions
	//are calculated.
	
	
	//Find the start/end index into the time step sample list.
	//Note that the unsteadyTimestepList is in forward time order
	int sampleStartIndex = -1;
	int sampleEndIndex = -1;
	for (int indx = 0; indx< (int)numUnsteadyTimesteps; indx++){
		if (unsteadyTimestepList[indx] == startTimeStep) sampleStartIndex = indx;
		if (unsteadyTimestepList[indx] == endTimeStep) sampleEndIndex = indx;
	}
	assert(sampleStartIndex >= 0 && sampleEndIndex >= 0);
	//numTimesteps is the number of time steps needed
	int numTimesteps = abs(endTimeStep - startTimeStep) + 1;
	int numTimeSamples = abs(sampleStartIndex - sampleEndIndex) + 1;
	
	bool zeroX = (strcmp(xUnsteadyVarName, "0") == 0); 
	bool zeroY = (strcmp(yUnsteadyVarName, "0") == 0); 
	bool zeroZ = (strcmp(zUnsteadyVarName, "0") == 0); 

	if(!zeroX){
		pUData = new float*[numTimeSamples];
		memset(pUData, 0, sizeof(float*)*numTimeSamples);
	}
	if(!zeroY){
		pVData = new float*[numTimeSamples];
		memset(pVData, 0, sizeof(float*)*numTimeSamples);
	}
	if(!zeroZ){
		pWData = new float*[numTimeSamples];
		memset(pWData, 0, sizeof(float*)*numTimeSamples);
	}
	
	pSolution = new Solution(pUData, pVData, pWData, totalNum, numTimesteps);
	pSolution->SetTimeScaleFactor(unsteadyUserTimeStepMultiplier);
	
	pSolution->SetTime(startTimeStep, endTimeStep);
	pCartesianGrid = new CartesianGrid(totalXNum, totalYNum, totalZNum, 
		regionPeriodicDim(0),regionPeriodicDim(1),regionPeriodicDim(2),maxRegion);
	pCartesianGrid->setPeriod(flowPeriod);
	
	// set the boundary of physical grid
	
	const size_t* bs = dataMgr->GetBlockSize();
	VECTOR3 minB, maxB, minR, maxR;
	double minUser[3], maxUser[3], regMin[3],regMax[3];
	size_t blockRegionMin[3],blockRegionMax[3];
	for (int i = 0; i< 3; i++){
		blockRegionMin[i] = bs[i]*minBlkRegion[i];
		blockRegionMax[i] = bs[i]*(maxBlkRegion[i]+1)-1;
	}
	dataMgr->MapVoxToUser((size_t)-1, blockRegionMin, minUser, numXForms);
	dataMgr->MapVoxToUser((size_t)-1, blockRegionMax, maxUser, numXForms);
	dataMgr->MapVoxToUser((size_t)-1, minRegion, regMin, numXForms);
	dataMgr->MapVoxToUser((size_t)-1, maxRegion, regMax, numXForms);
	
	minB.Set(minUser[0], minUser[1], minUser[2]);
	maxB.Set(maxUser[0], maxUser[1], maxUser[2]);
	minR.Set(regMin[0], regMin[1], regMin[2]);
	maxR.Set(regMax[0], regMax[1], regMax[2]);
	pCartesianGrid->SetBoundary(minB, maxB);
	pCartesianGrid->SetRegionExtents(minR,maxR);
	
	//Now set the region bound:

	pField = new CVectorField(pCartesianGrid, pSolution, numTimesteps);
	
	// get the userTimeStep between pairs of timesteps
	float* pUserTimeSteps;
	int tIndex = 0;
	pUserTimeSteps = new float[numTimesteps];

	//Calculate the time differences between successive sampled timesteps in
	//the interval we are working on.
	//save that value in pUserTimeSteps, available in pField.
	//These are negative if we are advecting backwards in time!!

	for(int sampleIndex = sampleStartIndex; sampleIndex != sampleEndIndex; sampleIndex += timeDir)
	{
		int prevSampledStep = unsteadyTimestepList[sampleIndex];
		int nextSampledStep = unsteadyTimestepList[sampleIndex+timeDir];
		
		const vector<double>& prevTime = dataMgr->GetTSUserTime(prevSampledStep);
		const vector<double>& nextTime = dataMgr->GetTSUserTime(nextSampledStep);
		if (nextTime.size()> 0 && prevTime.size()>0)
			pUserTimeSteps[tIndex] = nextTime[0] - prevTime[0];	
		else
			pUserTimeSteps[tIndex] = (float)(nextSampledStep - prevSampledStep);
		//following should make the value always positive
		pUserTimeSteps[tIndex++] *= timeDir;
		assert(pUserTimeSteps[tIndex-1] >= 0.f);
	}
	pField->SetUserTimeSteps(pUserTimeSteps);

	// initialize streak line
	vtCStreakLine* pStreakLine;
	
	pStreakLine = new vtCStreakLine(pField);
	
	pStreakLine->SetTimeDir(timeDir);
	
	pStreakLine->setParticleLife(endTimeStep - startTimeStep + 1);
	//The sampling rate is the time-difference between samples.
	//AnimationTimeStepSize is the samples per timeStep
	pStreakLine->SetSamplingRate(1.f/animationTimeStepSize);
	pStreakLine->SetInitStepSize(initialStepSize);
	pStreakLine->SetMaxStepSize(maxStepSize);
	pStreakLine->setIntegrationOrder(FOURTH);

	//Insert all the seeds at the first time step:
	int nseeds = pStreakLine->addFLASeeds(startTimeStep, flArray[startTimeStep], maxNumSamples);

	if (nseeds <= 0) {
		delete pStreakLine;
		delete pField;
		return false;
	}

	//Keep first and next region pointers.  They must be released as we go:
	float *xDataPtr =0, *yDataPtr = 0, *zDataPtr =0;
	float *xDataPtr2 =0, *yDataPtr2 = 0, *zDataPtr2 =0;


	int currIndex = sampleStartIndex;
	int prevSample = startTimeStep;
	int nextSample = unsteadyTimestepList[sampleStartIndex+timeDir];
	int tsIndex=0; //Index that counts the sampled timesteps from 0
	// Go through all the timesteps, one at a time. Ifor is the time Step.
	// For each pair (prevSample, nextSample) integrate between them.
	// Actually, this will probably only be used when there are exactly 2 sampled timestep,
	// startTimeStep and endTimeStep; however the code for intermediate samples
	// is being left in just in case it will be useful later.
	for(int iFor = startTimeStep; iFor != endTimeStep; iFor += timeDir)
	{

		//Following sets prevSample, nextSample, currIndex
		if (iFor == nextSample) {//bump up the samples
			currIndex++;
			prevSample = nextSample;
			nextSample = unsteadyTimestepList[currIndex+timeDir];
		}
		
		tsIndex = abs(currIndex - sampleStartIndex);
		
		// get usertimestep differences between the current time step and previous and next sampled time steps
		double diff = 0.0, curDiff = 0.0;
		const vector<double>& iforTime = dataMgr->GetTSUserTime(iFor);
		const vector<double>& prevTime = dataMgr->GetTSUserTime(prevSample);
		const vector<double>& nextTime = dataMgr->GetTSUserTime(nextSample);
		if(iforTime.size()> 0 && prevTime.size()> 0 && nextTime.size()> 0){
		
			diff = iforTime[0] - prevTime[0];
				
			curDiff = nextTime[0] - iforTime[0];
					
		} else {
			diff = (iFor - prevSample);
			curDiff = (nextSample - iFor);
		}
		float vaporTimeDiff = (float)(numTimesteps -1)/(float)(numTimeSamples -1);
		pField->SetUserTimePerVaporTS(abs((diff+curDiff)/vaporTimeDiff));
		pField->SetUserTimeStepInc((float)diff);
	
		//specify the number of time steps between time samples
		pSolution->SetTimeIncrement((numTimesteps -1)/(numTimeSamples -1), timeDir);
		// need get new field data?
		
		if (iFor == prevSample)
		{
			//For the very first sample time, get data for current time (get the next sampled timestep in next line)
			if(iFor == startTimeStep){
				bool gotData = Get3Data(iFor, xUnsteadyVarName, yUnsteadyVarName,
					zUnsteadyVarName, &xDataPtr, &yDataPtr, &zDataPtr);
				if(!gotData){
					// release resources
					delete[] pUserTimeSteps;
					delete pStreakLine;
					delete pField;
					if (!zeroX && xDataPtr) dataMgr->UnlockRegion(xDataPtr);
					if (!zeroY && yDataPtr) dataMgr->UnlockRegion(yDataPtr);
					return false;
				}
				pField->SetSolutionData(tsIndex,xDataPtr,yDataPtr,zDataPtr);
			} else { //Changing time sample..
				//If it's not the very first time, need to release data for previous 
				//time step, and move end ptrs to start:
				//Now can release first pointers:
				if(!zeroX) dataMgr->UnlockRegion(xDataPtr);
				if(!zeroY) dataMgr->UnlockRegion(yDataPtr);
				if(!zeroZ) dataMgr->UnlockRegion(zDataPtr);
				//And use them to save the second pointers:
				xDataPtr = xDataPtr2;
				yDataPtr = yDataPtr2;
				zDataPtr = zDataPtr2;
				//Also nullify pointers to released timestep data:
				pField->SetSolutionData(tsIndex-1,0,0,0);
			}
			//now get data for second ( next) sampled timestep.
			bool gotData = Get3Data(nextSample, xUnsteadyVarName, yUnsteadyVarName,
				zUnsteadyVarName, &xDataPtr2, &yDataPtr2, &zDataPtr2);
			if (!gotData){
				// if we failed:  release resources
				delete[] pUserTimeSteps;
				delete pStreakLine;
				delete pField;
				if (!zeroX && xDataPtr) dataMgr->UnlockRegion(xDataPtr);
				if (!zeroY && yDataPtr) dataMgr->UnlockRegion(yDataPtr);
				if (!zeroZ && zDataPtr) dataMgr->UnlockRegion(zDataPtr);
				return false;
			}
			pField->SetSolutionData(tsIndex+1,xDataPtr2,yDataPtr2,zDataPtr2); 
		}
		// advect for one timestep.  Inject seeds only at the first timestep
		pStreakLine->advectFLAPoints(iFor, timeDir, flArray, (iFor == startTimeStep));
	}

	
	// release resources.  we always have valid start and end pointers
	// at this point.
	if(!zeroX)dataMgr->UnlockRegion(xDataPtr);
	if(!zeroY)dataMgr->UnlockRegion(yDataPtr);
	if(!zeroZ)dataMgr->UnlockRegion(zDataPtr);
	if(!zeroX)dataMgr->UnlockRegion(xDataPtr2);
	if(!zeroY)dataMgr->UnlockRegion(yDataPtr2);
	if(!zeroZ)dataMgr->UnlockRegion(zDataPtr2);
	pField->SetSolutionData(tsIndex,0,0,0);
	pField->SetSolutionData(tsIndex+1,0,0,0);
	delete[] pUserTimeSteps;
	delete pStreakLine;
	delete pField;
	
	return true;
}
//Prepare for obtaining values from a vector field.  Uses integer region bounds or
//rake bounds.  numRefinements does not need to be same as region numrefinements.
//scaleField indicates whether or not the field is scaled by current steady field scale factor.
FieldData* VaporFlow::
setupFieldData(const char* varx, const char* vary, const char* varz, 
			   bool useRakeBounds, int numRefinements, int timestep, bool scaleField){
	
	size_t minInt[3], maxInt[3];
	size_t minBlk[3], maxBlk[3];
	size_t blockRegionMin[3],blockRegionMax[3];
	
	double minUser[3], maxUser[3]; //coords we will use for mapping (full block bounds)
	if (useRakeBounds){
		for (int i = 0; i< 3; i++) {
			minInt[i] = minRake[i];
			maxInt[i] = maxRake[i];
			minBlk[i] = minBlkRake[i];
			maxBlk[i] = maxBlkRake[i];
		}
	} else {
		for (int i = 0; i< 3; i++) {
			minInt[i] = minRegion[i];
			maxInt[i] = maxRegion[i];
			minBlk[i] = minBlkRegion[i];
			maxBlk[i] = maxBlkRegion[i];
		}
	}
	
	
	const size_t* bs = dataMgr->GetBlockSize();
	// get the field data, lock it in place:
	// create field object
	CVectorField* pField;
	Solution* pSolution;
	CartesianGrid* pCartesianGrid;
	float **pUData, **pVData, **pWData;
	int totalXNum = (maxBlk[0]-minBlk[0]+1)* bs[0];
	int totalYNum = (maxBlk[1]-minBlk[1]+1)* bs[1];
	int totalZNum = (maxBlk[2]-minBlk[2]+1)* bs[2];
	int totalNum = totalXNum*totalYNum*totalZNum;

	//Now get the data from the dataMgr:
	pUData = new float*[1];
	pVData = new float*[1];
	pWData = new float*[1];
	if (strcmp(varx,"0")== 0) pUData[0] = 0;
	else {
		pUData[0] = dataMgr->GetRegion(timestep, varx, numRefinements, minBlk, maxBlk, 1);
		if (pUData[0]== 0)
			return 0;
	}
	if (strcmp(vary,"0")== 0) pVData[0] = 0;
	else {
		pVData[0] = dataMgr->GetRegion(timestep, vary, numRefinements, minBlk, maxBlk, 1);
		if (pVData[0] == 0 && pUData[0]) {
			dataMgr->UnlockRegion(pUData[0]);
			return 0;
		}
	}
	if (strcmp(varz,"0")== 0) pWData[0] = 0;
	else {
		pWData[0] = dataMgr->GetRegion(timestep, varz, numRefinements, minBlk, maxBlk,  1);
		if (pWData[0] == 0) {
			if(pUData[0])dataMgr->UnlockRegion(pUData[0]);
			if(pVData[0])dataMgr->UnlockRegion(pVData[0]);
			return 0;
		}
	}

	pSolution = new Solution(pUData, pVData, pWData, totalNum, 1);
	//Note:  We optionally use the scale factor here
	if (scaleField)
		pSolution->SetTimeScaleFactor(steadyUserTimeStepMultiplier);
	else
		pSolution->SetTimeScaleFactor(1.f);
	pSolution->SetTime(timestep, timestep);
	
	pCartesianGrid = new CartesianGrid(totalXNum, totalYNum, totalZNum, 
		regionPeriodicDim(0),regionPeriodicDim(1),regionPeriodicDim(2),maxInt);
	pCartesianGrid->setPeriod(flowPeriod);
	
	// set the boundary of physical grid
	
	VECTOR3 minB, maxB, minR, maxR;
	double regMin[3],regMax[3];
	
	//Determine the bounds of the full block region (it's what is used for 
	// coordinate mapping)
	//Convert block extents to integer coords:
	for (int i = 0; i<3; i++){
		blockRegionMin[i] = bs[i]*minBlk[i];
		blockRegionMax[i] = bs[i]*(maxBlk[i]+1)-1;
	}
	dataMgr->MapVoxToUser((size_t)-1, blockRegionMin, minUser, numRefinements);
	dataMgr->MapVoxToUser((size_t)-1, blockRegionMax, maxUser, numRefinements);
	//Also, map the region extents (needed for in/out testing):
	dataMgr->MapVoxToUser((size_t)-1, minInt, regMin, numRefinements);
	dataMgr->MapVoxToUser((size_t)-1, maxInt, regMax, numRefinements);
	
	
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
	pField->SetUserTimeStepInc(0.f);

	FieldData* fData = new FieldData();
	fData->setup(pField, pCartesianGrid, pUData, pVData, pWData, timestep);
	return fData;
}

//Obtain bounds on field magnitude, using voxel bounds on either rake or 
//region.
bool VaporFlow::
getFieldMagBounds(float* minVal, float* maxVal,const char* varx, const char* vary, const char* varz, 
	bool useRakeBounds, int numRefinements, int timestep){
			 
	//As in the above method, set up the mapping and get the data.
	size_t minInt[3], maxInt[3];
	size_t minBlk[3], maxBlk[3];
	
	if (useRakeBounds){
		for (int i = 0; i< 3; i++) {
			minInt[i] = minRake[i];
			maxInt[i] = maxRake[i];
			minBlk[i] = minBlkRake[i];
			maxBlk[i] = maxBlkRake[i];
		}
			
	} else {
		for (int i = 0; i< 3; i++) {
			minInt[i] = minRegion[i];
			maxInt[i] = maxRegion[i];
			minBlk[i] = minBlkRegion[i];
			maxBlk[i] = maxBlkRegion[i];
		}
	}
	
	
	const size_t* bs = dataMgr->GetBlockSize();
	
	float **pUData, **pVData, **pWData;
	
	//Now get the data from the dataMgr:
	pUData = new float*[1];
	pVData = new float*[1];
	pWData = new float*[1];
	if (strcmp(varx,"0")== 0) pUData[0] = 0;
	else {
		pUData[0] = dataMgr->GetRegion(timestep, varx, numRefinements, minBlk, maxBlk, 1);
		if (pUData[0]== 0)
			return false;
	}
	if (strcmp(vary,"0")== 0) pVData[0] = 0;
	else {
		pVData[0] = dataMgr->GetRegion(timestep, vary, numRefinements, minBlk, maxBlk,  1);
		if (pVData[0] == 0) {
			if(pUData[0]) dataMgr->UnlockRegion(pUData[0]);
			return false;
		}
	}
	if (strcmp(varz,"0")== 0) pWData[0] = 0;
	else {
		pWData[0] = dataMgr->GetRegion(timestep, varz, numRefinements, minBlk, maxBlk,  1);
		if (pWData[0] == 0) {
			if(pUData[0])dataMgr->UnlockRegion(pUData[0]);
			if(pVData[0])dataMgr->UnlockRegion(pVData[0]);
			return false;
		}
	}

	
	int numPts = 0;
	*minVal = 1.e30f;
	*maxVal = -1.f;
	//OK, we got the data. find the rms:
	for (size_t i = minInt[0]; i<=maxInt[0]; i++){
		for (size_t j = minInt[1]; j<=maxInt[1]; j++){
			for (size_t k = minInt[2]; k<=maxInt[2]; k++){
				int xyzCoord = (i - minBlk[0]*bs[0]) +
					(j - minBlk[1]*bs[1])*(bs[0]*(maxBlk[0]-minBlk[0]+1)) +
					(k - minBlk[2]*bs[2])*(bs[1]*(maxBlk[1]-minBlk[1]+1))*(bs[0]*(maxBlk[0]-minBlk[0]+1));
				float dataX = 0.f;
				if (pUData[0]) dataX = pUData[0][xyzCoord];
				float dataY = 0.f;
				if (pVData[0]) dataY = pVData[0][xyzCoord];
				float dataZ = 0.f;
				if (pWData[0]) dataZ = pWData[0][xyzCoord];

				float sumsq = dataX*dataX+dataY*dataY+dataZ*dataZ;
				float dataVal = sqrt(sumsq);
				if (*minVal > dataVal) 
					*minVal = dataVal;
				if (*maxVal < dataVal) 
					*maxVal = dataVal;
				numPts++;
			}
		}
	}
	if(pUData[0])dataMgr->UnlockRegion(pUData[0]);
	if(pVData[0])dataMgr->UnlockRegion(pVData[0]);
	if(pWData[0])dataMgr->UnlockRegion(pWData[0]);
	
	if (numPts == 0) return false;
	return true;
}
