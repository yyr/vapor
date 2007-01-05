#ifdef WIN32
#pragma warning(disable : 4251 4100)
#endif

#include "vapor/VaporFlow.h"
#include "Rake.h"
#include "VTFieldLine.h"
#include "vapor/flowlinedata.h"
using namespace VetsUtil;
using namespace VAPoR;

#ifdef DEBUG
	FILE* fDebug;
#endif




//Methods on FlowLineData and subclass PathlineData

FlowLineData::FlowLineData(int numLines, int maxPoints, bool useSpeeds,  int direction, bool doRGBAs){
	nmLines = numLines;
	mxPoints = maxPoints;
	flowDirection = direction;
	if(direction < 0 ) integrationStartPosn = mxPoints -1;
	else if (direction > 0) integrationStartPosn = 0;
	else integrationStartPosn = mxPoints/2;
	startIndices = new int[numLines];
	
	lineLengths = new int[numLines];
	flowLineLists = new float*[numLines];
	if (doRGBAs) flowRGBAs = new float*[numLines];
	else flowRGBAs = 0;
	if (useSpeeds) speedLists = new float*[numLines]; 
	else speedLists = 0;

	for (int i = 0; i<numLines; i++){
		flowLineLists[i] = new float[3*mxPoints];
		if (flowRGBAs) flowRGBAs[i] = new float[4*mxPoints];
		if (useSpeeds) speedLists[i] = new float[mxPoints];
		if (flowDirection > 0) startIndices[i] = 0;
		else if (flowDirection < 0) startIndices[i] = mxPoints -1;
		else startIndices[i] = -1; //Invalid value, must be reset to use.
		lineLengths[i] = -1;
	}
}
FlowLineData::~FlowLineData() {
	delete startIndices;
	delete lineLengths;
	
	for (int i = 0; i<nmLines; i++){
		delete flowLineLists[i];
		if (speedLists && speedLists[i]) delete speedLists[i];
		if (flowRGBAs && flowRGBAs[i]) delete flowRGBAs[i];
	}
	delete flowLineLists;
	if (speedLists) delete speedLists;
	if (flowRGBAs) delete flowRGBAs;
}
//Establish an end (rendering order) position for a flow line, after start already established.
void FlowLineData::
setFlowEnd(int lineNum, int integPos){
	assert(integPos>= 0);
	if (flowDirection > 0) {
		assert(integPos >= 0 && integPos < mxPoints);
		lineLengths[lineNum] = integPos+1;
	}
	//Note: must have set start before calling this.
	assert( startIndices[lineNum] >= 0);
	if (flowDirection == 0) lineLengths[lineNum] = 
		integrationStartPosn - startIndices[lineNum] + integPos + 1;
	if (flowDirection < 0) { 
		//The start position can be anything before the end.
		//But the flow line must end at integration position 0 
		assert (integPos == 0); 
		lineLengths[lineNum] = integrationStartPosn + 1 +integPos - startIndices[lineNum];
	}
}
int FlowLineData::
getMaxLength(int dir){//How far we can integrate in a direction:
	if ((flowDirection != 0) && (flowDirection == dir)) return mxPoints; //full data size
	if (flowDirection != 0) return 0; //if it wasn't bidirectional
	//otherwise max length is shortest distance to end (forward or backward)
	return (Min(1+integrationStartPosn, mxPoints - integrationStartPosn));
}
//Method for resetting the value of a point in unsteady flow (after reprioritization).
//Also can be used to append a point to the start or end of an existing unsteady flow line.
void PathLineData::setPointAtTime(int lineNum, float timeStep, float x, float y, float z){
	//Determine the (closest) index:
	int insertPosn = (int)((timeStep - startTimeStep)*samplesPerTStep + 0.5f);
	assert(insertPosn >= 0 && insertPosn < mxPoints);
	*(flowLineLists[lineNum]+ 3*insertPosn) = x;
	*(flowLineLists[lineNum]+ 3*insertPosn+1) = y;
	*(flowLineLists[lineNum]+ 3*insertPosn+2) = z;
	//Update start/end pointers.  Should only extend by one at a time.
	if (insertPosn < startIndices[lineNum]){
		assert (insertPosn == startIndices[lineNum]-1);
		startIndices[lineNum] = insertPosn;
		lineLengths[lineNum]++;
	}
	else if (insertPosn > startIndices[lineNum]+lineLengths[lineNum]-1){
		assert (insertPosn == startIndices[lineNum]+lineLengths[lineNum]);
		lineLengths[lineNum]++;
	}
}
//Method for resetting the value of a point in unsteady flow (after reprioritization).
//Also can be used to append a point to the start or end of an existing unsteady flow line.
void PathLineData::setSpeedAtTime(int lineNum, float timeStep, float speed){
	//Determine the (closest) index:
	int insertPosn = (int)((timeStep - startTimeStep)*samplesPerTStep + 0.5f);
	assert(insertPosn >= 0 && insertPosn < mxPoints);
	*(speedLists[lineNum]+ insertPosn) = speed;
}
//Methods for (progressively) designating the start or end of an unsteady flow line.
//This is based on left-to-right ordering, not integration order.
//
void PathLineData::setFlowStartAtTime(int lineNum, float time){
	//Determine the (closest) index:
	int insertPosn = (int)((time - startTimeStep)*samplesPerTStep + 0.5f);
	assert(insertPosn >= 0 && insertPosn < mxPoints);
	int prevIndex = startIndices[lineNum];
	startIndices[lineNum] = insertPosn;
	if (prevIndex >= 0) {  //Correct the line length
		lineLengths[lineNum] = lineLengths[lineNum] + prevIndex - insertPosn;
	}
}
void PathLineData::setFlowEndAtTime(int lineNum, float time){
	//Determine the (closest) index:
	int insertPosn = (int)((time - startTimeStep)*samplesPerTStep + 0.5f);
	assert(insertPosn >= 0 && insertPosn < mxPoints);
	lineLengths[lineNum] = insertPosn - startIndices[lineNum];
}
//Initialize a path line with a seed point, seedIndex.
//The time step must be an integer
void PathLineData::insertSeedAtTime(int seedIndex, int timeStep,  float x, float y, float z){
	assert(actualNumLines < nmLines);
	//Determine the (closest) index:
	int insertPosn = (int)((timeStep - startTimeStep)*samplesPerTStep + 0.5f);
	assert(insertPosn >= 0 && insertPosn < mxPoints);
	*(flowLineLists[actualNumLines]+ 3*insertPosn) = x;
	*(flowLineLists[actualNumLines]+ 3*insertPosn+1) = y;
	*(flowLineLists[actualNumLines]+ 3*insertPosn+2) = z;
	//Set start/end pointers.  
	startIndices[actualNumLines] = insertPosn;
	lineLengths[actualNumLines] = 1;
	seedIndices[actualNumLines] = seedIndex;
	//seedTimes[actualNumLines] = insertPosn;
	actualNumLines++;
}
//Determine how many lines exist at a given time step
int PathLineData::getNumLinesAtTime(int timeStep){
	int count = 0;
	for (int i = 0; i<actualNumLines; i++){
		if (getFirstTimestep(i) <= timeStep && getLastTimestep(i) >= timeStep) count++;
	}
	return count;
}
float* PathLineData::
getPointAtTime(int lineNum, float timeStep){
	//Determine the (closest) index:
	int posn = (int)((timeStep - startTimeStep)*samplesPerTStep + 0.5f);
	if (posn < getStartIndex(lineNum) || posn > getEndIndex(lineNum)) {
		return 0;
	}
	return flowLineLists[lineNum]+ 3*posn;
}
