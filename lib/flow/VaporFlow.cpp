#ifdef WIN32
#pragma warning(disable : 4251 4100)
#endif

#include "vapor/VaporFlow.h"
#include "vapor/flowlinedata.h"
#include "vapor/errorcodes.h"
#include "Rake.h"
#include "VTFieldLine.h"
#include "math.h"


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

	minPriorityVal = 0.f;
	maxPriorityVal = 1.e30;
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
						  const size_t max_bdim[3])
{
	numXForms = num_xforms;
	const size_t* fullDims = dataMgr->GetMetadata()->GetDimension();
	const std::vector<double> extents = dataMgr->GetMetadata()->GetExtents();
	int max_xforms = dataMgr->GetMetadata()->GetNumTransforms();
	size_t fullDataSize[3];
	dataMgr->GetRegionReader()->GetDim(fullDataSize,num_xforms);
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
		SetErrMsg("Error obtaining field data for timestep %d, variable %s",ts, varName);
		return 0;
	}
	return regionData;
}
//Generate the seeds for the rake.  If rake is random calculates distributed seeds 
int VaporFlow::GenRakeSeeds(float* seeds, int timeStep, unsigned int randomSeed, int stride){
	int seedNum;
	seedNum = numSeeds[0]*numSeeds[1]*numSeeds[2];
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
// Uses the same region mapping as the field region
bool VaporFlow::prioritizeSeeds(FlowLineData* container, PathLineData* pathContainer, int timeStep){
	//Use the current Region, obtain the priority field.
	//First map the region to doubles:
	double minDouble[3], maxDouble[3];
	const VDFIOBase* myReader = dataMgr->GetRegionReader();
	myReader->MapVoxToUser(timeStep,minRegion, minDouble, numXForms);
	myReader->MapVoxToUser(timeStep,maxRegion, maxDouble, numXForms);
	//Use the current region bounds, not the rake bounds...
	FieldData* fData = setupFieldData(xPriorityVarName, yPriorityVarName, zPriorityVarName, 
		false, numXForms, timeStep, false);
	if (!fData) return false;
	
	//Go through the seeds, calculate the prioritization magnitude at each point, starting
	//At the seed, going first backwards and then forwards
	for (int line = 0; line< container->getNumLines(); line++){
		float maxVal = minPriorityVal -1.f;
		int startPos = container->getSeedPosition();
		float* maxPoint = 0;
		if (steadyFlowDirection <= 0){
			for (int ptindx = startPos; ptindx >= container->getStartIndex(line); ptindx--){
				float* pt = container->getFlowPoint(line, ptindx);
				if (pt[0] == END_FLOW_FLAG) break;
				float val = fData->getFieldMag(pt);
				if(val < 0.f) break; // point is out of region
				if (val < minPriorityVal) break;
				if (val > maxVal){ //establish a new max
					maxVal = val;
					maxPoint = pt;
				}
				if (val >= maxPriorityVal) break;
			}
		}
		if (steadyFlowDirection >= 0){
			for (int ptindx = startPos; ptindx <= container->getEndIndex(line); ptindx++){
				float* pt = container->getFlowPoint(line, ptindx);
				if (pt[0] == END_FLOW_FLAG) break;
				float val = fData->getFieldMag(pt);
				if(val < 0.f) break; 
				if (val < minPriorityVal) break;
				if (val > maxVal){ //establish a new max
					maxVal = val;
					maxPoint = pt;
				}
				if (val >= maxPriorityVal) break;
			}
		}
		//Now put the winner back into the unsteady advection array.
		//If no winner, it's probably END_FLOW_FLAG.  do nothing.
		if (maxPoint)
			pathContainer->setPointAtTime(line,(float)timeStep, maxPoint[0],maxPoint[1],maxPoint[2]);
		
	}//end loop over line

	//Release the locks on the prioritization field..
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
	seedNum = numSeeds[0]*numSeeds[1]*numSeeds[2];
	assert(seedNum == container->getNumLines());
	seedPtr = new float[seedNum*3];
	SeedGenerator* pSeedGenerator = new SeedGenerator(minRakeExt, maxRakeExt, numSeeds);
	if (bUseRandomSeeds && seedDistBias != 0.f)
		pSeedGenerator->SetSeedDistrib(seedDistBias, steadyStartTimeStep, numXForms,
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
	
	// scale animationTimeStep and userTimeStep
	if((dataMgr->GetMetadata()->HasTSUserTime(steadyStartTimeStep))&&(dataMgr->GetMetadata()->HasTSUserTime(steadyStartTimeStep+1)))
	{
		double diff;
		diff =  dataMgr->GetMetadata()->GetTSUserTime(steadyStartTimeStep+1)[0] - dataMgr->GetMetadata()->GetTSUserTime(steadyStartTimeStep)[0];
		userTimeStepSize = diff*steadyUserTimeStepMultiplier;
		animationTimeStepSize = diff*steadyAnimationTimeStepMultiplier;
	}
	else
	{
		userTimeStepSize = steadyUserTimeStepMultiplier;
		animationTimeStepSize = steadyAnimationTimeStepMultiplier;
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
	pUData[0] = GetData(steadyStartTimeStep, xSteadyVarName);
	if (pUData[0]== 0)
		return false;

	pVData[0] = GetData(steadyStartTimeStep, ySteadyVarName);
	if (pVData[0] == 0) {
		dataMgr->UnlockRegion(pUData[0]);
		return false;
	}
	pWData[0] = GetData(steadyStartTimeStep, zSteadyVarName);
	if (pWData[0] == 0) {
		dataMgr->UnlockRegion(pUData[0]);
		dataMgr->UnlockRegion(pVData[0]);
		return false;
	}

	pSolution = new Solution(pUData, pVData, pWData, totalNum, 1);
	pSolution->SetTimeScaleFactor(steadyUserTimeStepMultiplier);
	pSolution->SetTime(steadyStartTimeStep, steadyStartTimeStep);
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
	myReader->MapVoxToUser(steadyStartTimeStep, blockRegionMin, minUser, numXForms);
	myReader->MapVoxToUser(steadyStartTimeStep, blockRegionMax, maxUser, numXForms);
	myReader->MapVoxToUser(steadyStartTimeStep, minRegion, regMin, numXForms);
	myReader->MapVoxToUser(steadyStartTimeStep, maxRegion, regMax, numXForms);
	
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
	//test the seeds:
	int seedsInRegion = 0;
	for (int i = 0; i<numSeeds; i++){
		bool out = false;
		for (int j = 0; j< 3; j++){
			if (seedPtr[3*i+j] < regMin[j] || seedPtr[3*i+j] > regMax[j]){
				out = true;
				break;
			}
		}
		if (!out) seedsInRegion++;
	}
		
	if (seedsInRegion < numSeeds){
		MyBase::SetErrMsg(VAPOR_WARNING_FLOW,
			"Flow seeds are outside region: \n %d seeds outside of total %d seeds.",numSeeds - seedsInRegion, numSeeds);
	}
	pStreamLine->setSeedPoints(seedPtr, numSeeds, currentT);
	pStreamLine->SetSamplingRate(animationTimeStepSize);
	pStreamLine->SetInitStepSize(initialStepSize);
	pStreamLine->SetMaxStepSize(maxStepSize);
	pStreamLine->setIntegrationOrder(FOURTH);
	pStreamLine->SetStationaryCutoff((0.1f*pCartesianGrid->GetGridSpacing(0))/(container->getMaxPoints()*steadyAnimationTimeStepMultiplier));
	pStreamLine->computeStreamLine(currentT, container);
	
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
//(Eventually seeds will be added enroute)
//This can be either backwards or forwards in time.
/////////////////////////////////////////////////////////////////////////////////////
bool VaporFlow::ExtendPathLines(PathLineData* container, int startTimeStep, int endTimeStep){
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
	
    totalXNum = (maxBlkRegion[0]-minBlkRegion[0] + 1)* dataMgr->GetMetadata()->GetBlockSize()[0];
	totalYNum = (maxBlkRegion[1]-minBlkRegion[1] + 1)* dataMgr->GetMetadata()->GetBlockSize()[1];
	totalZNum = (maxBlkRegion[2]-minBlkRegion[2] + 1)* dataMgr->GetMetadata()->GetBlockSize()[2];
	totalNum = totalXNum*totalYNum*totalZNum;
	
	//realStartTime and realEndTime are actual limits of time steps for which positions
	//are calculated.
	
	
	//Find the start/end index into the time step sample list.
	//Note that the unsteadyTimestepList is in forward time order
	int sampleStartIndex = -1;
	int sampleEndIndex = -1;
	for (int indx = 0; indx< numUnsteadyTimesteps; indx++){
		if (unsteadyTimestepList[indx] == startTimeStep) sampleStartIndex = indx;
		if (unsteadyTimestepList[indx] == endTimeStep) sampleEndIndex = indx;
	}
	assert(sampleStartIndex >= 0 && sampleEndIndex >= 0);
	//numTimesteps is the number of time steps needed
	int numTimesteps = abs(endTimeStep - startTimeStep) + 1;
	int numTimeSamples = abs(sampleStartIndex - sampleEndIndex) + 1;
	
	pUData = new float*[numTimeSamples];
	pVData = new float*[numTimeSamples];
	pWData = new float*[numTimeSamples];
	memset(pUData, 0, sizeof(float*)*numTimeSamples);
	memset(pVData, 0, sizeof(float*)*numTimeSamples);
	memset(pWData, 0, sizeof(float*)*numTimeSamples);
	pSolution = new Solution(pUData, pVData, pWData, totalNum, numTimesteps);
	pSolution->SetTimeScaleFactor(unsteadyUserTimeStepMultiplier);
	
	pSolution->SetTime(startTimeStep, endTimeStep);
	pCartesianGrid = new CartesianGrid(totalXNum, totalYNum, totalZNum, regionPeriodicDim(0),regionPeriodicDim(1),regionPeriodicDim(2));
	
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
	myReader->MapVoxToUser(startTimeStep, blockRegionMin, minUser, numXForms);
	myReader->MapVoxToUser(startTimeStep, blockRegionMax, maxUser, numXForms);
	myReader->MapVoxToUser(startTimeStep, minRegion, regMin, numXForms);
	myReader->MapVoxToUser(startTimeStep, maxRegion, regMax, numXForms);
	
	minB.Set(minUser[0], minUser[1], minUser[2]);
	maxB.Set(maxUser[0], maxUser[1], maxUser[2]);
	minR.Set(regMin[0], regMin[1], regMin[2]);
	maxR.Set(regMax[0], regMax[1], regMax[2]);
	pCartesianGrid->SetBoundary(minB, maxB);
	pCartesianGrid->SetRegionExtents(minR,maxR);
	
	//Now set the region bound:

	pField = new CVectorField(pCartesianGrid, pSolution, numTimesteps);
	
	// get the userTimeStep between pairs of timesteps
	int* pUserTimeSteps;
	int tIndex = 0;
	pUserTimeSteps = new int[numTimesteps];

	//Calculate the time differences between successive sampled timesteps in
	//the interval we are working on.
	//save that value in pUserTimeSteps, available in pField.
	//These are negative if we are advecting backwards in time!!

	for(int sampleIndex = sampleStartIndex; sampleIndex != sampleEndIndex; sampleIndex += timeDir)
	{
		int prevSampledStep = unsteadyTimestepList[sampleIndex];
		int nextSampledStep = unsteadyTimestepList[sampleIndex+timeDir];
		
		if(dataMgr->GetMetadata()->HasTSUserTime(prevSampledStep)&&dataMgr->GetMetadata()->HasTSUserTime(nextSampledStep))
			pUserTimeSteps[tIndex] = dataMgr->GetMetadata()->GetTSUserTime(nextSampledStep)[0] - 
									   dataMgr->GetMetadata()->GetTSUserTime(prevSampledStep)[0];
		else
			pUserTimeSteps[tIndex] = nextSampledStep - prevSampledStep;
		//following should make the value always positive
		pUserTimeSteps[tIndex++] *= timeDir;
		assert(pUserTimeSteps[tIndex-1] >= 0);
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
	int tsIndex; //Index that counts the sampled timesteps from 0
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
		double diff = 0.0, curDiff = 0.0;
	
		if(dataMgr->GetMetadata()->HasTSUserTime(iFor)){
			diff = dataMgr->GetMetadata()->GetTSUserTime(iFor)[0] -
				dataMgr->GetMetadata()->GetTSUserTime(prevSample)[0];
			curDiff = dataMgr->GetMetadata()->GetTSUserTime(nextSample)[0] - 
					  dataMgr->GetMetadata()->GetTSUserTime(iFor)[0];
		} else {
			diff = (iFor - prevSample);
			curDiff = (nextSample - iFor);
		}
		
		pField->SetUserTimeStepInc((int)diff, (int)curDiff);
		//specify the number of time steps between time samples
		pSolution->SetTimeIncrement((numTimesteps -1)/(numTimeSamples -1), timeDir);
		// need get new field data?
		
		if (iFor == prevSample)
		{
			//For the very first sample time, get data for current time (get the next sampled timestep in next line)
			if(iFor == startTimeStep){
				
				xDataPtr = GetData(iFor,xUnsteadyVarName);
				
				if(xDataPtr) yDataPtr = GetData(iFor,yUnsteadyVarName);
				if(yDataPtr) zDataPtr = GetData(iFor,zUnsteadyVarName);
				if (xDataPtr == 0 || yDataPtr == 0 || zDataPtr == 0){
					// release resources
					delete[] pUserTimeSteps;
					delete pStreakLine;
					delete pField;
					if (xDataPtr) dataMgr->UnlockRegion(xDataPtr);
					if (yDataPtr) dataMgr->UnlockRegion(yDataPtr);
					return false;
				}
				pField->SetSolutionData(tsIndex,xDataPtr,yDataPtr,zDataPtr);
			} else { //Changing time sample..
				//If it's not the very first time, need to release data for previous 
				//time step, and move end ptrs to start:
				//Now can release first pointers:
				dataMgr->UnlockRegion(xDataPtr);
				dataMgr->UnlockRegion(yDataPtr);
				dataMgr->UnlockRegion(zDataPtr);
				//And use them to save the second pointers:
				xDataPtr = xDataPtr2;
				yDataPtr = yDataPtr2;
				zDataPtr = zDataPtr2;
				//Also nullify pointers to released timestep data:
				pField->SetSolutionData(tsIndex-1,0,0,0);
			}
			//now get data for second ( next) sampled timestep.  
			yDataPtr2 = zDataPtr2 = 0;
			xDataPtr2 = GetData(nextSample,xUnsteadyVarName); 
			if(xDataPtr2) yDataPtr2 = GetData(nextSample,yUnsteadyVarName); 
			if(yDataPtr2) zDataPtr2 = GetData(nextSample,zUnsteadyVarName); 
			if (!xDataPtr2 || !yDataPtr2 || !zDataPtr2){
				// if we failed:  release resources
				delete[] pUserTimeSteps;
				delete pStreakLine;
				delete pField;
				if (xDataPtr) dataMgr->UnlockRegion(xDataPtr);
				if (yDataPtr) dataMgr->UnlockRegion(yDataPtr);
				if (zDataPtr) dataMgr->UnlockRegion(zDataPtr);
				if (xDataPtr2) dataMgr->UnlockRegion(xDataPtr2);
				if (yDataPtr2) dataMgr->UnlockRegion(yDataPtr2);
				return false;
			}
			pField->SetSolutionData(tsIndex+1,xDataPtr2,yDataPtr2,zDataPtr2); 
		}

			
		//Set up the seeds for this time step.  All points that are already specified at the time step
		//are taken to be seeds for ExtendPathLines
		int nseeds = pStreakLine->addSeeds(iFor, container);
		bInject = (nseeds > 0);
		
		pStreakLine->execute((float)iFor, container, bInject);
		
	
	}

	
	// release resources.  we always have valid start and end pointers
	// at this point.
	dataMgr->UnlockRegion(xDataPtr);
	dataMgr->UnlockRegion(yDataPtr);
	dataMgr->UnlockRegion(zDataPtr);
	dataMgr->UnlockRegion(xDataPtr2);
	dataMgr->UnlockRegion(yDataPtr2);
	dataMgr->UnlockRegion(zDataPtr2);
	pField->SetSolutionData(tsIndex,0,0,0);
	pField->SetSolutionData(tsIndex+1,0,0,0);
	delete[] pUserTimeSteps;
	delete pStreakLine;
	delete pField;
	
	return true;
}
//Prepare for obtaining values from a vector field.  Uses either region bounds or
//rake bounds.  numRefinements does not need to be same as region numrefinements.
//scaleField indicates whether or not the field is scaled by current steady field scale factor.
FieldData* VaporFlow::
setupFieldData(const char* varx, const char* vary, const char* varz, 
			   bool useRakeBounds, int numRefinements, int timestep, bool scaleField){
	
	size_t minInt[3], maxInt[3];
	size_t minBlk[3], maxBlk[3];
	double minExt[3], maxExt[3];
	double minUser[3], maxUser[3]; //coords we will use for mapping (full block bounds)
	if (useRakeBounds){
		for (int i = 0; i< 3; i++) { minExt[i] = minRake[i]; maxExt[i] = maxRake[i];}
	} else {
		for (int i = 0; i< 3; i++) { minExt[i] = minRegion[i]; maxExt[i] = maxRegion[i];}
	}
	
	const VDFIOBase* myReader = dataMgr->GetRegionReader();
	myReader->MapUserToVox((size_t)timestep, minExt, minInt, numRefinements);
	myReader->MapUserToVox((size_t)timestep, maxExt, maxInt, numRefinements);
	myReader->MapUserToBlk((size_t)timestep, minExt, minBlk, numRefinements);
	myReader->MapUserToBlk((size_t)timestep, maxExt, maxBlk, numRefinements);
	const size_t* bs = dataMgr->GetMetadata()->GetBlockSize();
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
	pUData[0] = dataMgr->GetRegion(timestep, varx, numRefinements, minBlk, maxBlk, 1);
	if (pUData[0]== 0)
		return 0;
	pVData[0] = dataMgr->GetRegion(timestep, vary, numRefinements, minBlk, maxBlk, 1);
	if (pVData[0] == 0) {
		dataMgr->UnlockRegion(pUData[0]);
		return 0;
	}
	pWData[0] = dataMgr->GetRegion(timestep, varz, numRefinements, minBlk, maxBlk, 1);
	if (pWData[0] == 0) {
		dataMgr->UnlockRegion(pUData[0]);
		dataMgr->UnlockRegion(pVData[0]);
		return 0;
	}

	pSolution = new Solution(pUData, pVData, pWData, totalNum, 1);
	//Note:  We optionally use the scale factor here
	if (scaleField)
		pSolution->SetTimeScaleFactor(steadyUserTimeStepMultiplier);
	else
		pSolution->SetTimeScaleFactor(1.f);
	pSolution->SetTime(timestep, timestep);
	pCartesianGrid = new CartesianGrid(totalXNum, totalYNum, totalZNum, false, false, false);
	
	// set the boundary of physical grid
	
	VECTOR3 minB, maxB, minR, maxR;
	double regMin[3],regMax[3];
	size_t blockRegionMin[3],blockRegionMax[3];
	//Determine the bounds of the full block region (it's what is used for mapping)
	if (useRakeBounds){
		for (int i = 0; i< 3; i++){
		blockRegionMin[i] = bs[i]*minBlkRake[i];
		blockRegionMax[i] = bs[i]*(maxBlkRake[i]+1)-1;
		}
	} else {
		for (int i = 0; i< 3; i++){
			blockRegionMin[i] = bs[i]*minBlkRegion[i];
			blockRegionMax[i] = bs[i]*(maxBlkRegion[i]+1)-1;
		}
	}
	myReader->MapVoxToUser(timestep, blockRegionMin, minUser, numRefinements);
	myReader->MapVoxToUser(timestep, blockRegionMax, maxUser, numRefinements);
	//Also, map the region extents (needed for in/out testing):
	myReader->MapVoxToUser(timestep, minInt, regMin, numRefinements);
	myReader->MapVoxToUser(timestep, maxInt, regMax, numRefinements);
	
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

	FieldData* fData = new FieldData();
	fData->setup(pField, pCartesianGrid, pUData, pVData, pWData, timestep);
	return fData;
}
//Obtain bounds on field magnitude
bool VaporFlow::
getFieldMagBounds(float* minVal, float* maxVal,const char* varx, const char* vary, const char* varz, 
	bool useRakeBounds, int numRefinements, int timestep){
			 
	//As in the above method, set up the mapping and get the data.
	size_t minInt[3], maxInt[3];
	size_t minBlk[3], maxBlk[3];
	double maxExt[3], minExt[3];
	if (useRakeBounds){
		for (int i = 0; i< 3; i++) {minExt[i] = minRake[i]; maxExt[i] = maxRake[i];}
	} else {
		for (int i = 0; i< 3; i++) {minExt[i] = minRegion[i]; maxExt[i] = maxRegion[i];}
	}
	
	const VDFIOBase* myReader = dataMgr->GetRegionReader();
	myReader->MapUserToVox((size_t)timestep, minExt, minInt, numRefinements);
	myReader->MapUserToVox((size_t)timestep, maxExt, maxInt, numRefinements);
	myReader->MapUserToBlk((size_t)timestep, minExt, minBlk, numRefinements);
	myReader->MapUserToBlk((size_t)timestep, maxExt, maxBlk, numRefinements);
	const size_t* bs = dataMgr->GetMetadata()->GetBlockSize();
	
	float **pUData, **pVData, **pWData;
	int totalXNum = (maxBlk[0]-minBlk[0]+1)* bs[0];
	int totalYNum = (maxBlk[1]-minBlk[1]+1)* bs[1];
	int totalZNum = (maxBlk[2]-minBlk[2]+1)* bs[2];
	
	//Now get the data from the dataMgr:
	pUData = new float*[1];
	pVData = new float*[1];
	pWData = new float*[1];
	pUData[0] = dataMgr->GetRegion(timestep, varx, numRefinements, minBlk, maxBlk, 1);
	if (pUData[0]== 0)
		return false;
	pVData[0] = dataMgr->GetRegion(timestep, vary, numRefinements, minBlk, maxBlk, 1);
	if (pVData[0] == 0) {
		dataMgr->UnlockRegion(pUData[0]);
		return false;
	}
	pWData[0] = dataMgr->GetRegion(timestep, varz, numRefinements, minBlk, maxBlk, 1);
	if (pWData[0] == 0) {
		dataMgr->UnlockRegion(pUData[0]);
		dataMgr->UnlockRegion(pVData[0]);
		return false;
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
				float sumsq = pUData[0][xyzCoord]*pUData[0][xyzCoord]+pVData[0][xyzCoord]*pVData[0][xyzCoord]+pWData[0][xyzCoord]*pWData[0][xyzCoord];
				float dataVal = sqrt(sumsq);
				if (*minVal > dataVal) 
					*minVal = dataVal;
				if (*maxVal < dataVal) 
					*maxVal = dataVal;
				numPts++;
			}
		}
	}
	dataMgr->UnlockRegion(pUData[0]);
	dataMgr->UnlockRegion(pVData[0]);
	dataMgr->UnlockRegion(pWData[0]);
	
	if (numPts == 0) return false;
	return true;
}
