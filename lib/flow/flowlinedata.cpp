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
		if (speedLists) delete speedLists[i];
		if (flowRGBAs) delete flowRGBAs[i];
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
void PathLineData::setPointAtTime(int lineNum, float timeStep, float x, float y, float z){
	//Determine the (closest) index:
	int insertPosn = (int)(startTimeStep + 
		(timeStep - startTimeStep)*samplesPerTStep + 0.5f);
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

