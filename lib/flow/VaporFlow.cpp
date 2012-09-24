#ifdef WIN32
#pragma warning(disable : 4244 4251 4267 4100 4996)
#endif

#include <vapor/VaporFlow.h>
#include <vapor/flowlinedata.h>
#include <vapor/errorcodes.h>
#include "Rake.h"
#include "VTFieldLine.h"
#include "math.h"

#define SMALLEST_MAX_STEP 0.25f
#define SMALLEST_MIN_STEP 0.01f
#define LARGEST_MAX_STEP 10.f
#define LARGEST_MIN_STEP 4.f

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
	compressLevel = 0;
	for (int i = 0; i< 3; i++){
		
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
	
	if(xSteadyVarName) delete [] xSteadyVarName;
	if(ySteadyVarName) delete [] ySteadyVarName;
	if(zSteadyVarName) delete [] zSteadyVarName;
	if(xUnsteadyVarName) delete [] xUnsteadyVarName;
	if(yUnsteadyVarName) delete [] yUnsteadyVarName;
	if(zUnsteadyVarName) delete [] zUnsteadyVarName;
	if(xPriorityVarName) delete [] xPriorityVarName;
	if(yPriorityVarName) delete [] yPriorityVarName;
	if(zPriorityVarName) delete [] zPriorityVarName;
	if(xSeedDistVarName) delete [] xSeedDistVarName;
	if(ySeedDistVarName) delete [] ySeedDistVarName;
	if(zSeedDistVarName) delete [] zSeedDistVarName;
	
	
}

void VaporFlow::Reset(void)
{
	numXForms = 0;
	for (int i = 0; i< 3; i++){
		
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
void VaporFlow::SetLocalRegion(size_t num_xforms,
						  int clevel,
						  const size_t min[3], 
						  const size_t max[3],
						  const double regLocalExts[6])
{
	
	numXForms = num_xforms;
	compressLevel = clevel;
	size_t fullDims[3];
	//extents are just used to obtain domain size:
	const std::vector<double> extents = dataMgr->GetExtents();
	
	size_t fullDataSize[3];
	dataMgr->GetDim(fullDataSize,num_xforms);
	dataMgr->GetDim(fullDims,-1);
	for (int i = 0; i< 3; i++){
		minRegion[i] = min[i];
		maxRegion[i] = max[i];
		regionLocalExtents[i] = regLocalExts[i];
		regionLocalExtents[i+3] = regLocalExts[i+3];
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
void VaporFlow::SetLocalRakeRegion(const double rakeLocalExtents[6])
{
	for (int i = 0; i< 3; i++){
		minLocalRakeExt[i] = rakeLocalExtents[i];
		maxLocalRakeExt[i] = rakeLocalExtents[i+3];
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
// specify a set of seed points regularly generated over the specified
// spatial interval. Points can be in axis aligned dimension 0, 1, 2, 3
//////////////////////////////////////////////////////////////////////////
void VaporFlow::SetRegularSeedPoints(const double min[3], 
									const double max[3], 
									const size_t numSeeds[3])
{
	for(int iFor = 0; iFor < 3; iFor++)
	{
		minLocalRakeExt[iFor] = min[iFor];
		maxLocalRakeExt[iFor] = max[iFor];
	}
	this->numSeeds[0] = (unsigned int)numSeeds[0];
	this->numSeeds[1] = (unsigned int)numSeeds[1];
	this->numSeeds[2] = (unsigned int)numSeeds[2];

	bUseRandomSeeds = false;
}



//////////////////////////////////////////////////////////////////////////
// Obtain data for three components of a vector field. 
// Ignore if the variable name is "0"
// Resulting data array is assigned to float** arguments.
// If data is not obtained, unlock previously obtained data
//////////////////////////////////////////////////////////////////////////
bool VaporFlow::Get3GridData(size_t ts, 
			const char* xVarName,const char* yVarName,const char* zVarName,	 size_t minExt[3], size_t maxExt[3],
			RegularGrid** pxGrid, RegularGrid** pyGrid, RegularGrid** pzGrid)
{
	bool zeroX = (strcmp(xVarName, "0") == 0); 
	bool zeroY = (strcmp(yVarName, "0") == 0); 
	bool zeroZ = (strcmp(zVarName, "0") == 0); 
	*pxGrid = 0; *pyGrid = 0; *pzGrid = 0;
	if(!zeroX) *pxGrid = dataMgr->GetGrid(ts, xVarName, (int)numXForms, compressLevel, minExt, maxExt,1);
	if((zeroX || *pxGrid) && !zeroY) *pyGrid = dataMgr->GetGrid(ts, yVarName, (int)numXForms, compressLevel, minExt, maxExt,1);
	if((zeroX || *pxGrid) && (zeroY || *pyGrid) && !zeroZ) *pzGrid = dataMgr->GetGrid(ts, zVarName, (int)numXForms, compressLevel, minExt, maxExt,1);
	//Check if we failed:
	if ((!zeroX && (*pxGrid == 0)) || (!zeroY && (*pyGrid == 0)) || (!zeroZ && (*pzGrid == 0))){
					
		if (!zeroX && *pxGrid) dataMgr->UnlockGrid(*pxGrid);
		if (!zeroY && *pyGrid) dataMgr->UnlockGrid(*pyGrid);
		return false;
	}
	return true;
}
//Generate the seeds for the rake.  If rake is random calculates distributed seeds 
int VaporFlow::GenRakeSeeds(float* seeds, int timeStep, unsigned int randomSeed, int stride){
	int seedNum;
	seedNum = (int)(numSeeds[0]*numSeeds[1]*numSeeds[2]);
	SeedGenerator* pSeedGenerator = new SeedGenerator(minLocalRakeExt, maxLocalRakeExt, numSeeds);
	if (bUseRandomSeeds && seedDistBias != 0.f){
		pSeedGenerator->SetSeedDistrib(seedDistBias, timeStep, numXForms,xSeedDistVarName, ySeedDistVarName, zSeedDistVarName);
	}
	pSeedGenerator->GetSeeds(timeStep, this, seeds, bUseRandomSeeds, randomSeed, stride);
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
	
	//Use the current region bounds, not the rake bounds...
	vector<string> varnames;
	varnames.push_back(xPriorityVarName);
	varnames.push_back(yPriorityVarName);
	varnames.push_back(zPriorityVarName);
	FieldData* fData = setupFieldData(varnames, 
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
void VaporFlow::SetDistributedSeedPoints(const double min[3], const double max[3], int numSeeds, 
	const char* varx, const char* vary, const char* varz, float bias)
{
	assert( bias >= -15.f && bias <= 15.f);
	for(int iFor = 0; iFor < 3; iFor++)
	{
		minLocalRakeExt[iFor] = min[iFor];
		maxLocalRakeExt[iFor] = max[iFor];
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

bool VaporFlow::GenStreamLines(int timestep, FlowLineData* container, unsigned int randomSeed){
	// first generate seeds
	float* seedPtr;
	int seedNum;
	seedNum = (int)(numSeeds[0]*numSeeds[1]*numSeeds[2]);
	assert(seedNum == container->getNumLines());
	seedPtr = new float[seedNum*3];
	SeedGenerator* pSeedGenerator = new SeedGenerator(minLocalRakeExt, maxLocalRakeExt, numSeeds);
	if (bUseRandomSeeds && seedDistBias != 0.f)
		pSeedGenerator->SetSeedDistrib(seedDistBias, steadyStartTimeStep, (int)numXForms,
			xSeedDistVarName,ySeedDistVarName,zSeedDistVarName);
	bool rc = pSeedGenerator->GetSeeds(timestep, this, seedPtr, bUseRandomSeeds, randomSeed);
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

	
	RegularGrid *pUGrid[1], *pVGrid[1], *pWGrid[1];
	
	bool gotData = Get3GridData(steadyStartTimeStep, xSteadyVarName, ySteadyVarName,
		zSteadyVarName, minRegion, maxRegion, &pUGrid[0],  &pVGrid[0],  &pWGrid[0]);
	
	if (!gotData) 
		return false;
	int totalXNum = (int)(maxRegion[0]-minRegion[0]+1);
	int totalYNum = (int)(maxRegion[1]-minRegion[1]+1);
	int totalZNum = (int)(maxRegion[2]-minRegion[2]+1);
	
	pSolution = new Solution(pUGrid,pVGrid,pWGrid, 1, periodicDim);
	pSolution->SetTimeScaleFactor((float)steadyUserTimeStepMultiplier);
	pSolution->SetTime((int)steadyStartTimeStep, (int)steadyStartTimeStep);
	pCartesianGrid = new CartesianGrid(totalXNum, totalYNum, totalZNum, 
		regionPeriodicDim(0),regionPeriodicDim(1),regionPeriodicDim(2),maxRegion);
	pCartesianGrid->setPeriod(flowPeriod);
	
	//The region extents must be converted based on time-varying extents
	const vector<double>& usrExts = dataMgr->GetExtents(steadyStartTimeStep);
	double rMin[3],rMax[3];
	for (int i = 0; i<3; i++){
		rMin[i] = usrExts[i]+regionLocalExtents[i];
		rMax[i] = usrExts[i]+regionLocalExtents[i+3];
	}
	VECTOR3 minR(rMin);
	VECTOR3 maxR(rMax);
	pCartesianGrid->SetRegionExtents(minR,maxR);
	
	
	//Use current region to determine coords of grid boundary:
	
	pField = new CVectorField(pCartesianGrid, pSolution, 1);
	pField->SetUserTimeStepInc(0);
	
	// create streamline
	vtCStreamLine* pStreamLine = new vtCStreamLine(pField);

	float currentT = (float)steadyStartTimeStep;


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
			if (seedPtr[3*i+j] < rMin[j] || seedPtr[3*i+j] > rMax[j]){
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
	
	double minspacing[3];
	pSolution->getMinGridSpacing(0,minspacing);
	pStreamLine->SetInitStepSize(getInitStepSize(minspacing));
	pStreamLine->SetMaxStepSize(getMaxStepSize(minspacing));
	float mmspacing = Min(minspacing[0],Min(minspacing[1],minspacing[2]));
	pStreamLine->SetStationaryCutoff(0.1f*mmspacing/(container->getMaxPoints()*(float)steadyAnimationTimeStepMultiplier));
	pStreamLine->computeStreamLine(currentT, container);
	
	//AN: Removed a call to Reset() here. This requires all vapor flow state to be
	// reestablished each time flow lines are generated.
	// release resource
	delete pStreamLine;
	delete pField;
	//Unlock region:

	if(pUGrid[0])dataMgr->UnlockGrid(pUGrid[0]);
	if(pVGrid[0])dataMgr->UnlockGrid(pVGrid[0]);
	if(pWGrid[0])dataMgr->UnlockGrid(pWGrid[0]);
		
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
bool VaporFlow::GenStreamLines (int timeStep, FlowLineData* steadyContainer, PathLineData* unsteadyContainer, bool prioritize){
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
	delete [] seedList;
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
	RegularGrid **pUGrid, **pVGrid, **pWGrid;
	
	int totalXNum, totalYNum, totalZNum;
	
    totalXNum = (int)(maxRegion[0]-minRegion[0] + 1);
	totalYNum = (int)(maxRegion[1]-minRegion[1] + 1);
	totalZNum = (int)(maxRegion[2]-minRegion[2] + 1);
	
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
	if (zeroX) pUGrid = 0;
	else {
		pUGrid = new RegularGrid*[numTimeSamples];
		memset(pUGrid, 0, sizeof(float*)*numTimeSamples);
	}
	if (zeroY) pVGrid = 0;
	else {
		pVGrid = new RegularGrid*[numTimeSamples];
		memset(pVGrid, 0, sizeof(float*)*numTimeSamples);
	}
	if (zeroZ) pWGrid = 0;
	else {
		pWGrid = new RegularGrid*[numTimeSamples];
		memset(pWGrid, 0, sizeof(float*)*numTimeSamples);
	}
	pSolution = new Solution(pUGrid, pVGrid, pWGrid, numTimesteps, periodicDim);
	pSolution->SetTimeScaleFactor((float)unsteadyUserTimeStepMultiplier);
	
	pSolution->SetTime(startTimeStep, endTimeStep);
	pCartesianGrid = new CartesianGrid(totalXNum, totalYNum, totalZNum, 
		regionPeriodicDim(0),regionPeriodicDim(1),regionPeriodicDim(2),maxRegion);
	pCartesianGrid->setPeriod(flowPeriod);
	
	// set the boundary of physical grid
	
	VECTOR3 minR, maxR;
	double regMin[3],regMax[3];
	
	dataMgr->MapVoxToUser(0, minRegion, regMin, (int)numXForms);
	dataMgr->MapVoxToUser(0, maxRegion, regMax, (int)numXForms);
	
	minR.Set((float)regMin[0], (float)regMin[1], (float)regMin[2]);
	maxR.Set((float)regMax[0], (float)regMax[1], (float)regMax[2]);
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
		double prevTime = dataMgr->GetTSUserTime(prevSampledStep);
		double nextTime = dataMgr->GetTSUserTime(nextSampledStep);
		pUserTimeSteps[tIndex] = (float)(nextTime - prevTime);

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
	double minspacing[3];
	
	


	//Keep first and next grid pointers.  They must be released as we go:
	RegularGrid *xGridPtr =0, *yGridPtr = 0, *zGridPtr =0;
	RegularGrid *xGridPtr2 =0, *yGridPtr2 = 0, *zGridPtr2 =0;

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
		double iforTime = dataMgr->GetTSUserTime(iFor);
		double prevTime = dataMgr->GetTSUserTime(prevSample);
		double nextTime = dataMgr->GetTSUserTime(nextSample);
		
		diff = iforTime - prevTime;
		curDiff = nextTime - iforTime; 
			
		
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
				bool gotData = Get3GridData(iFor, xUnsteadyVarName, yUnsteadyVarName,
					zUnsteadyVarName, minRegion, maxRegion, &xGridPtr, &yGridPtr, &zGridPtr);
				if(!gotData){
					delete[] pUserTimeSteps;
					delete pStreakLine;
					delete pField;
					return false;
				}
				pField->SetSolutionGrid(tsIndex,&xGridPtr,&yGridPtr,&zGridPtr, periodicDim);
				pSolution->getMinGridSpacing(tsIndex, minspacing);
				pStreakLine->SetInitStepSize(getInitStepSize(minspacing));
				pStreakLine->SetMaxStepSize(getMaxStepSize(minspacing));
			} else { //Changing time sample..
				//If it's not the very first time, need to release data for previous 
				//time step, and move end ptrs to start:
				//Now can release first pointers:
				if(!zeroX)dataMgr->UnlockGrid(xGridPtr);
				if(!zeroY)dataMgr->UnlockGrid(yGridPtr);
				if(!zeroZ)dataMgr->UnlockGrid(zGridPtr);
				//And use them to save the second pointers:
				xGridPtr = xGridPtr2;
				yGridPtr = yGridPtr2;
				zGridPtr = zGridPtr2;
				//Also nullify pointers to released timestep data:
				pField->ClearSolutionGrid(tsIndex-1);
			}
			//now get data for second ( next) sampled timestep. 
			bool gotData = Get3GridData(nextSample, xUnsteadyVarName,yUnsteadyVarName, zUnsteadyVarName, 
				minRegion, maxRegion, &xGridPtr2, &yGridPtr2,&zGridPtr2);
			if(!gotData){
				// if we failed:  release resources for 2 time steps:
				delete[] pUserTimeSteps;
				delete pStreakLine;
				delete pField;
				if (!zeroX && xGridPtr) dataMgr->UnlockGrid(xGridPtr);
				if (!zeroY && yGridPtr) dataMgr->UnlockGrid(yGridPtr);
				if (!zeroZ && zGridPtr) dataMgr->UnlockGrid(zGridPtr);
				return false;
			}
			pField->SetSolutionGrid(tsIndex+1,&xGridPtr2,&yGridPtr2, &zGridPtr2, periodicDim); 
		}

			
		//Set up the seeds for this time step.  All points that are already specified at the time step
		//are taken to be seeds for ExtendPathLines
		int nseeds = pStreakLine->addSeeds(iFor, container);
		bInject = (nseeds > 0);
		
		pStreakLine->execute((float)iFor, container, bInject, doingFLA);
		
	
	}

	
	// release resources.  we always have valid start and end pointers
	// at this point.
	if(!zeroX) dataMgr->UnlockGrid(xGridPtr);
	if(!zeroX) dataMgr->UnlockGrid(yGridPtr);
	if(!zeroX) dataMgr->UnlockGrid(zGridPtr);
	if(!zeroX) dataMgr->UnlockGrid(xGridPtr2);
	if(!zeroX) dataMgr->UnlockGrid(yGridPtr2);
	if(!zeroX) dataMgr->UnlockGrid(zGridPtr2);
	
	pField->ClearSolutionGrid(tsIndex);
	pField->ClearSolutionGrid(tsIndex+1);
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
	RegularGrid** pUGrid, **pVGrid, **pWGrid;
	
	int totalXNum, totalYNum, totalZNum;
	
    totalXNum = (maxRegion[0]-minRegion[0] + 1);
	totalYNum = (maxRegion[1]-minRegion[1] + 1);
	totalZNum = (maxRegion[2]-minRegion[2] + 1);
	
	
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
	if (zeroX) pUGrid = 0;
	else {
		pUGrid = new RegularGrid*[numTimeSamples];
		memset(pUGrid, 0, sizeof(float*)*numTimeSamples);
	}
	if (zeroY) pVGrid = 0;
	else {
		pVGrid = new RegularGrid*[numTimeSamples];
		memset(pVGrid, 0, sizeof(float*)*numTimeSamples);
	}
	if (zeroZ) pWGrid = 0;
	else {
		pWGrid = new RegularGrid*[numTimeSamples];
		memset(pWGrid, 0, sizeof(float*)*numTimeSamples);
	}
	pSolution = new Solution(pUGrid, pVGrid, pWGrid, numTimesteps, periodicDim);
	
	pSolution->SetTimeScaleFactor(unsteadyUserTimeStepMultiplier);
	
	pSolution->SetTime(startTimeStep, endTimeStep);
	pCartesianGrid = new CartesianGrid(totalXNum, totalYNum, totalZNum, 
		regionPeriodicDim(0),regionPeriodicDim(1),regionPeriodicDim(2),maxRegion);
	pCartesianGrid->setPeriod(flowPeriod);
	
	// set the boundary of physical grid
	
	VECTOR3 minR, maxR;
	double regMin[3],regMax[3];
	
	dataMgr->MapVoxToUser(0, minRegion, regMin, numXForms);
	dataMgr->MapVoxToUser(0, maxRegion, regMax, numXForms);
	
	minR.Set(regMin[0], regMin[1], regMin[2]);
	maxR.Set(regMax[0], regMax[1], regMax[2]);
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
		
		double prevTime = dataMgr->GetTSUserTime(prevSampledStep);
		double nextTime = dataMgr->GetTSUserTime(nextSampledStep);
		pUserTimeSteps[tIndex] = nextTime - prevTime;	

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
	double minspacing[3];
	pSolution->getMinGridSpacing(startTimeStep,minspacing);
	pStreakLine->SetInitStepSize(getInitStepSize(minspacing));
	pStreakLine->SetMaxStepSize(getMaxStepSize(minspacing));
	
	

	//Insert all the seeds at the first time step:
	int nseeds = pStreakLine->addFLASeeds(startTimeStep, flArray[startTimeStep], maxNumSamples);

	if (nseeds <= 0) {
		delete pStreakLine;
		delete pField;
		return false;
	}

	//Keep first and next Grid pointers.  They must be released as we go:
	
	RegularGrid *xGridPtr = 0,	*yGridPtr = 0,*zGridPtr = 0;
	RegularGrid *xGridPtr2 = 0,	*yGridPtr2 = 0,*zGridPtr2 = 0;


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
		double iforTime = dataMgr->GetTSUserTime(iFor);
		double prevTime = dataMgr->GetTSUserTime(prevSample);
		double nextTime = dataMgr->GetTSUserTime(nextSample);
		
		diff = iforTime - prevTime;
		curDiff = nextTime - iforTime;
					
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
				bool gotData = Get3GridData(iFor, xUnsteadyVarName, yUnsteadyVarName,
					zUnsteadyVarName, minRegion, maxRegion, &xGridPtr, &yGridPtr, &zGridPtr);
				if(!gotData){
					// release resources
					delete[] pUserTimeSteps;
					delete pStreakLine;
					delete pField;
					if (!zeroX && xGridPtr) dataMgr->UnlockGrid(xGridPtr);
					if (!zeroY && yGridPtr) dataMgr->UnlockGrid(yGridPtr);
					return false;
				}
				pField->SetSolutionGrid(tsIndex,&xGridPtr,&yGridPtr,&zGridPtr, periodicDim);
			} else { //Changing time sample..
				//If it's not the very first time, need to release data for previous 
				//time step, and move end ptrs to start:
				//Now can release first pointers:
				if(!zeroX) dataMgr->UnlockGrid(xGridPtr);
				if(!zeroY) dataMgr->UnlockGrid(yGridPtr);
				if(!zeroZ) dataMgr->UnlockGrid(zGridPtr);
				//And use them to save the second pointers:
				xGridPtr = xGridPtr2;
				yGridPtr = yGridPtr2;
				zGridPtr = zGridPtr2;
				//Also nullify pointers to released timestep data:
				pField->ClearSolutionGrid(tsIndex-1);
			}
			//now get data for second ( next) sampled timestep.
			bool gotData = Get3GridData(nextSample, xUnsteadyVarName, yUnsteadyVarName,
				zUnsteadyVarName, minRegion, maxRegion, &xGridPtr2, &yGridPtr2, &zGridPtr2);
			if (!gotData){
				// if we failed:  release resources
				delete[] pUserTimeSteps;
				delete pStreakLine;
				delete pField;
				if (!zeroX && xGridPtr) dataMgr->UnlockGrid(xGridPtr);
				if (!zeroY && yGridPtr) dataMgr->UnlockGrid(yGridPtr);
				if (!zeroZ && zGridPtr) dataMgr->UnlockGrid(zGridPtr);
				return false;
			}
			pField->SetSolutionGrid(tsIndex+1,&xGridPtr2,&yGridPtr2,&zGridPtr2,periodicDim); 
		}
		// advect for one timestep.  Inject seeds only at the first timestep
		pStreakLine->advectFLAPoints(iFor, timeDir, flArray, (iFor == startTimeStep));
	}

	
	// release resources.  we always have valid start and end pointers
	// at this point.
	if(!zeroX)dataMgr->UnlockGrid(xGridPtr);
	if(!zeroY)dataMgr->UnlockGrid(yGridPtr);
	if(!zeroZ)dataMgr->UnlockGrid(zGridPtr);
	if(!zeroX)dataMgr->UnlockGrid(xGridPtr2);
	if(!zeroY)dataMgr->UnlockGrid(yGridPtr2);
	if(!zeroZ)dataMgr->UnlockGrid(zGridPtr2);
	
	pField->ClearSolutionGrid(tsIndex);
	pField->ClearSolutionGrid(tsIndex+1);
	delete[] pUserTimeSteps;
	delete pStreakLine;
	delete pField;
	
	return true;
}
//Prepare for obtaining values from a vector field.  Uses integer region bounds or
//rake bounds.  numRefinements does not need to be same as region numrefinements.
//scaleField indicates whether or not the field is scaled by current steady field scale factor.
FieldData* VaporFlow::
setupFieldData(const vector<string>& varnames, 
			   bool useRakeBounds, int numRefinements, int timestep, bool scaleField){
	
	size_t minInt[3], maxInt[3];
	size_t ts = (size_t) timestep;
	const vector<double>& userExts = dataMgr->GetExtents(ts);
	double rakeExts[6];
	for (int i = 0; i<3; i++){
		rakeExts[i] = minLocalRakeExt[i]+userExts[i];
		rakeExts[i+3] = maxLocalRakeExt[i]+userExts[i];
	}
	if (useRakeBounds){
		dataMgr->GetEnclosingRegion(ts, rakeExts, rakeExts+3, minInt, maxInt, numRefinements);
	} else {
		for (int i = 0; i< 3; i++) {
			minInt[i] = minRegion[i];
			maxInt[i] = maxRegion[i];
		}
	}
	
	// get the field data, lock it in place:
	// create field object
	
	CVectorField* pField;
	Solution* pSolution;
	CartesianGrid* pCartesianGrid;

	RegularGrid **pUGrid = new RegularGrid*[1];
	RegularGrid **pVGrid = new RegularGrid*[1];
	RegularGrid **pWGrid = new RegularGrid*[1];
	
	bool gotData = Get3GridData(ts, varnames[0].c_str(),varnames[1].c_str(),varnames[2].c_str(),
		minInt, maxInt, &pUGrid[0],  &pVGrid[0],  &pWGrid[0]);
	
	if (!gotData) 
		return false;
	
	pSolution = new Solution(pUGrid,pVGrid,pWGrid, 1, periodicDim);
	if (scaleField)
		pSolution->SetTimeScaleFactor(steadyUserTimeStepMultiplier);
	else
		pSolution->SetTimeScaleFactor(1.f);
	pSolution->SetTime(timestep, timestep);
	int totalXNum = (maxInt[0]-minInt[0]+1);
	int totalYNum = (maxInt[1]-minInt[1]+1);
	int totalZNum = (maxInt[2]-minInt[2]+1);
	pCartesianGrid = new CartesianGrid(totalXNum, totalYNum, totalZNum, 
		regionPeriodicDim(0),regionPeriodicDim(1),regionPeriodicDim(2),maxInt);
	pCartesianGrid->setPeriod(flowPeriod);
	//The region extents must be set to be consistent with the current refinement level, by
	//converting from the integer extents
	double rMin[3],rMax[3];
	dataMgr->MapVoxToUser(steadyStartTimeStep, minRegion, rMin, numXForms);
	dataMgr->MapVoxToUser(steadyStartTimeStep,maxRegion, rMax, numXForms);
	VECTOR3 minR(rMin);
	VECTOR3 maxR(rMax);
	pCartesianGrid->SetRegionExtents(minR,maxR);
	
	pField = new CVectorField(pCartesianGrid, pSolution, 1);
	pField->SetUserTimeStepInc(0.f);

	FieldData* fData = new FieldData();
	fData->setup(pField, pCartesianGrid, pUGrid,pVGrid,pWGrid, timestep, periodicDim);
	return fData;
}

//Obtain bounds on field magnitude, using voxel bounds on either rake or 
//region.
bool VaporFlow::
getFieldMagBounds(float* minVal, float* maxVal,const vector<string>& varnames, 
	bool useRakeBounds, int numRefinements, int timestep){
			 
	size_t minInt[3], maxInt[3];
	size_t ts = (size_t) timestep;
	
	if (useRakeBounds){
		const vector<double>& userExts = dataMgr->GetExtents(ts);
		double tvExts[6];
		for (int i = 0; i<3; i++){
			tvExts[i] = minLocalRakeExt[i]+userExts[i];
			tvExts[i+3] = maxLocalRakeExt[i]+userExts[i];
		}
		dataMgr->GetEnclosingRegion(ts, tvExts, tvExts+3, minInt, maxInt, numRefinements);
	} else {
		for (int i = 0; i< 3; i++) {
			minInt[i] = minRegion[i];
			maxInt[i] = maxRegion[i];
		}
	}

	const RegularGrid* varData[3];
	
	//Obtain the variables from the dataMgr:
	for (int var = 0; var<3; var++){
		if (varnames[var] == "0") varData[var] = 0;
		else {
			int lod = compressLevel;
			
			varData[var] = dataMgr->GetGrid(ts,
				varnames[var].c_str(),
				numRefinements, lod, minInt, maxInt,  1);
			if (!varData[var]) {
				dataMgr->SetErrCode(0);
				//release currently locked regions:
				for (int k = 0; k<var; k++){
					if (varData[k])
						dataMgr->UnlockGrid(varData[k]);
				}
				return false;
			}	
		}
	}
	
	int numPts = 0;
	float minData = 1.e30, maxData = -1.e30;
	//push down the zero grids.
	for (int i = 0; i<2; i++){
		if (!varData[0]){
			varData[0] = varData[1];
			varData[1] = varData[2];
			varData[2] = 0;
		}
		if (!varData[1]){
			varData[1] = varData[2];
			varData[2] = 0;
		}
	}
	if (!varData[0]) return false;  //No grids
		
	RegularGrid::ConstIterator itr0, itr1, itr2;
	itr0 = varData[0]->begin();
	
	float mv = varData[0]->GetMissingValue();
	
	for (itr0 = varData[0]->begin(),itr2 = varData[2]->begin(),itr1 = varData[1]->begin(); itr0 != varData[0]->end(); ++itr0){
		float val[3] = { 0.f, 0.f, 0.f};
		val[0] = *itr0;
		if(varData[1]) val[1] = *itr1;
		if(varData[2]) val[2] = *itr2;
		//Ignore if first variable is mv
		if (val[0] != mv) {
			//Note:  we assume that all variables have missing values in same place
			assert (val[1] != mv && val[2] != mv);
			float mag = sqrt(val[0]*val[0]+val[1]*val[1]+val[2]*val[2]);
			if (minData > mag) minData = mag;
			if (maxData < mag) maxData = mag;
			numPts++;
		}
		if (varData[1]) ++itr1;
		if (varData[2]) ++itr2;
	}
		
	for (int k = 0; k<3; k++){
		if(varData[k]) dataMgr->UnlockGrid(varData[k]);
	}
	
	if (numPts == 0) return false;
	
	*minVal = minData;
	*maxVal = maxData;
	
	return true;
}
double VaporFlow::getInitStepSize(double minspacing[3]){
	
	double initstep;
	double acc = integrationAccuracy;
	double minmin = Min(minspacing[0],Min(minspacing[1],minspacing[2]));
	double maxmin = Max(minspacing[0],Max(minspacing[1],minspacing[2]));
	//Multiply minmin/maxmin times smallest min step, so that will use the smallest dimension
	//of the smallest cell at highest accuracy.  Note that the result of this function
	//is multiplied by maxmin to determine actual min step size.
	initstep = SMALLEST_MIN_STEP*acc*minmin/maxmin + (1.f - acc)*LARGEST_MIN_STEP;
	
	return initstep;
}
double VaporFlow::getMaxStepSize(double minspacing[3]){
	
	double maxstep;
	double acc = integrationAccuracy;
	//Multiply minmin/maxmin times smallest max step, so that will use the smallest dimension
	//of the smallest cell when at highest accuracy
	double minmin = Min(minspacing[0],Min(minspacing[1],minspacing[2]));
	double maxmin = Max(minspacing[0],Max(minspacing[1],minspacing[2]));
	maxstep = SMALLEST_MAX_STEP*acc*minmin/maxmin + (1.f - acc)*LARGEST_MAX_STEP;
	
	return maxstep;
}
