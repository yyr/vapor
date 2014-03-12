
#ifndef ARROWPARAMS_H
#define ARROWPARAMS_H

#include "vapor/ParamNode.h"
#include "params.h"
#include "command.h"
#include "datastatus.h"

namespace VAPoR {

//! \class ArrowParams
//! \brief Class that supports drawing Barbs based on 2D or 3D vector field
//! \author Alan Norton
//! \version 3.0
//! \date February 2014
class PARAMS_API ArrowParams : public RenderParams {
public:
	ArrowParams(XmlNode *parent, int winnum);
	static ParamsBase* CreateDefaultInstance() {
		return new ArrowParams(0, -1);
	}
	const std::string& getShortName() {return _shortName;}
	virtual ~ArrowParams();
	virtual void restart();
	//! Method to retrieve the Box describing the rake extents
	//! \retval Box*
	virtual Box* GetBox() {
		ParamNode* pNode = GetRootNode()->GetNode(Box::_boxTag);
		if (pNode) return (Box*)pNode->GetParamsBase();
		Box* box = new Box();
		GetRootNode()->AddNode(Box::_boxTag, box->GetRootNode());
		return box;
	}

	//! Validate current settings
	//! \param[in] bool useDefault determines whether or not to set values to default.
	virtual void Validate(bool useDefault);
	virtual bool IsOpaque() {return true;}
	//!
	//! Determine if the specified variable is being used
	//!
	virtual bool usingVariable(const std::string& varname){
		for (int i = 0; i<3; i++){
			if (GetFieldVariableName(i) == varname) return true;
		}
		if (IsTerrainMapped() && (varname == GetHeightVariableName())) return true;
		return false;
	}


	//Get/Set methods based on XML representation
	//! Determine the local extents of the rake
	//! \retval vector<double>& local extents
	const vector<double>& GetRakeLocalExtents(){
		return (GetBox()->GetLocalExtents());
	}
	//! Determine the local extents of the rake
	//! \param[out] double exts[6] local extents
	void GetRakeLocalExtents(double exts[6]){
		GetBox()->GetLocalExtents(exts);
	}
	//! Set the local extents of the rake
	//! \param[in] vector<double>&exts local extents
	//! \retval int zero if OK.
	int SetRakeLocalExtents(const vector<double>&exts);
		
	//! Determine the size of the rake grid
	//! \retval vector<long>& grid
	const vector<long>& GetRakeGrid(){
		const vector<long> defaultGrid(3,1);
		return (GetValueLongVec(_rakeGridTag,defaultGrid));
	}
	//! Specify the rake grid
	//! \param[in] const int grid[3] Dimensions of grid.
	//! \retval int 0 if successful
	int SetRakeGrid(const int grid[3]);

	//! Determine line thickness in voxels
	//! \retval double line thickness
	double GetLineThickness(){
		const vector<double> one(1,1.);
		return (GetValueDouble(_lineThicknessTag,one));
	}
	//! Set the line thickness
	//! \param[in] double thickness
	//! \retval int 0 if success
	int SetLineThickness(double val){
		
		return SetValueDouble(_lineThicknessTag,"Set barb thickness",val);
	}
	//! Get the scale factor for vector length. 1.0 is (scene diameter)/100.
	//! \retval double scale factor
	double GetVectorScale(){
		const vector<double>defaultScale(1,1.);
		return ((float)GetValueDouble(_vectorScaleTag,defaultScale));
	}
	//! Set the scale factor for vector length
	//! \param[in] double scale factor
	//! \retval 0 if successful
	int SetVectorScale(double val);
	
	//! Determine if barbs are mapped to terrain
	//! \retval bool true if terrain mapping is used
	bool IsTerrainMapped(){
		const vector<long>off(1,0);
		return ((bool)GetValueLong(_terrainMapTag,off));
	}
	//! Set terrain mapping off or on
	//! \param[in] bool true if terrain mapping used
	//! \retval int 0 if successful
	int SetTerrainMapped(bool val) {
		int rc = SetValueLong(_terrainMapTag, "Set barb terrain-mapping", (long)val);
		setAllBypass(false);
		return rc;
	}
	//! Specify the color of barbs.  Color rgb values between 0 and 1
	//! \param[in] const double rgb[3]
	//! \retval 0 if successful
	int SetConstantColor(const double rgb[3]);

	//! Determine the current color (in r,g,b)
	//! \retval const double rgb[3];
	const vector<double>& GetConstantColor();

	//! Set a field variable name, and also recalculate default vector scale
	//! \param[in] int coordinate
	//! \param[in] string variable name (or "0")
	//! \retval 0 if OK
	int SetFieldVariableName(int i, const string& varName);

	//! Determine a field variable name
	//! \param[in] int coordinate
	//! \retval const string& variable name
	const string& GetFieldVariableName(int i);

	//! Specify the variable being used for height
	//! \param[in] const string& variable name
	//! \retval int 0 if successful
	int SetHeightVariableName(const string& varName);

	//! Determine variable name being used for height
	//! \retval const string& variable name
	const string& GetHeightVariableName();

	//! Specify whether 3D or 2D field variables are being used
	//! \param[in] bool true if 3D, false if 2D.
	//! \retval true if successful;
	int SetVariables3D(bool val) {
		if (val == VariablesAre3D()) return 0;
		int rc = SetValueLong(_variableDimensionTag,"Set barb var dimensions",(long)(val ? 3:2));
		setAllBypass(false);
		return rc;
	}
	//! Determine whether 3D field variables are being used.
	//! \retval bool true if 3D, false if 2D
	bool VariablesAre3D() {
		const vector<long>three(1,3);
		return (GetValueLong(_variableDimensionTag,three) == 3);
	}
	//! Specify that the rake grid is aligned to the data grid
	//! \param[in] bool true if grid is data-aligned
	//! \retval bool true if successful
	int AlignGridToData(bool val) {
		if (IsAlignedToData() == val) return 0;
		return SetValueLong(_alignGridTag, "Set barb grid alignment",(val ? 1:0));
	}
	//! Determine if rake grid is aligned to data grid
	//! \retval bool true if aligned.
	bool IsAlignedToData() {
		const vector<long> notAligned(1,0);
		return ((bool)GetValueLong(_alignGridTag,notAligned));
	}
	//! Determine the strides used in horizontal grid-alignment
	//! Stride is the number of data grid cells between rake grid.
	//! \retval vector<long> 2-vector of strides
	const vector<long>& GetGridAlignStrides(){
		const vector<long> defaultStrides(3,10);
		return GetValueLongVec(_alignGridStridesTag,defaultStrides);
	}
	//! Specify the strides used in horizontal grid-alignment
	//! Stride is the number of data grid cells between rake grid.
	//! \param[in] vector<long> vector of 2 strides.
	//! \retval int 0 if successful
	int SetGridAlignStrides(const vector<long>& strides);
#ifndef DOXYGEN_SKIP_THIS
	//Utility function to find rake when it is aligned to data:
	void calcDataAlignment(double rakeExts[6], int rakeGrid[3], size_t timestep);
	//Utility function to recalculate vector scale factor
	double calcDefaultScale();
	
	static const string _arrowParamsTag;

protected:
static const string _shortName;
static const string _constantColorTag;
static const string _rakeGridTag;
static const string _lineThicknessTag;
static const string _vectorScaleTag;
static const string _terrainMapTag;
static const string _variableDimensionTag;
static const string _alignGridTag;
static const string _alignGridStridesTag;
static const string _heightVariableNameTag;
double _constcolorbuf[4];

#endif //DOXYGEN_SKIP_THIS

}; //End of Class ArrowParams
};
#endif //ARROWPARAMS_H

