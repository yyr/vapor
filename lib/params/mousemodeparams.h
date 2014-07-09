//************************************************************************
//																		*
//		     Copyright (C)  1024										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		mousemodeparams.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		April 2014
//
//	Description:	Defines the MouseModeParams class.
//		This Params class is global.  Specifies the current MouseMode.
//		Also has various static methods for controlling mouse modes 
//
#ifndef MOUSEMODEPARAMS_H
#define MOUSEMODEPARAMS_H


#include <vector>
#include <map>
#include <vapor/common.h>
#include "params.h"
#include <vapor/MyBase.h>


using namespace VetsUtil;

namespace VAPoR {


//! \class MouseModeParams
//! \ingroup Public_Params
//! \brief A class for describing mouse modes in use in VAPOR
//! \author Alan Norton
//! \version 3.0
//! \date    April 2014
//!
class PARAMS_API MouseModeParams : public BasicParams {
	
public: 
//! @name Internal
//! Internal methods not intended for general use
///@{

	//! Constructor
	//! \param[in] int winnum The window number, -1 since it's global
	MouseModeParams(XmlNode* parent, int winnum);

	//! Destructor
	virtual ~MouseModeParams();
	
	//! Method to validate all values in a MouseModeParams instance
	//! \param[in] bool default indicates whether or not to set to default values associated with the current DataMgr
	//! \sa DataMgr
	virtual void Validate(bool useDefault);
	//! Method to initialize a new MouseModeParams instance
	virtual void restart();
	

	//Enum describes various built-in mouse modes:
	enum mouseModeType {
		unknownMode=1000,
		navigateMode=0,
		regionMode=1,
		probeMode=3,
		twoDDataMode=4,
		twoDImageMode=5,
		rakeMode=2,
		barbMode=6,
		isolineMode=7
	};
	//! Static method identifies pixmap icon for each mode
	static const char* const * GetIcon(int modeIndex){
		return modeXPMIcon[modeIndex];
	}
	//! Required static method (for extensibility):
	//! \retval ParamsBase* pointer to a default Params instance
	static ParamsBase* CreateDefaultInstance() {return new MouseModeParams(0, -1);}
	//! Pure virtual method on Params. Provide a short name suitable for use in the GUI
	//! \retval string name
	const std::string getShortName() {return _shortName;}
	///@}

	//Static methods for controlling mouse modes
	//! Method to register a mouse mode.  Called during startup.
	//! \param[in] tag Params tag associated with this mouse mode
	//! \param[in] modeType integer identifying this mouse mode, used for selecting
	//! \param[in] modeName name of the mode, identifying it during selection
	//! \param[in] xpm Icon (as an xpm) used when displaying mode in GUI.
	static int RegisterMouseMode(std::string tag, int modeType, const char* modeName, const char* const xpmIcon[]);

	//! Static method used to add a new Mouse Mode to the list of available modes.
	//! \param[in] const string& tag associated with the Params class
	//! \param[in] int Manipulator type (1,2,or 3)
	//! \param[in] const char* name of mouse mode
	//! \retval int Resulting mouse mode
	static int AddMouseMode(const std::string paramsTag, int manipType, const char* name);

	//! Static method that indicates the manipulator type that is associated with a mouse mode.
	//! \sa VizWinMgr
	//! \param[in] modeIndex is a positive integer indexing the current mouse modes
	//! \retval int manipulator type is 1 (region box) 2 (2D box) or 3 (rotated 3D box).
	static int getModeManipType(int modeIndex){
		return manipFromMode[modeIndex];
	}

	//! Static method that returns the Params type associated with a mouse mode.
	//! \param[in] int modeIndex  The mouse mode.
	//! \retval ParamsBase::ParamsBaseType  The type of the params associated with the mouse mode.
	static ParamsBase::ParamsBaseType getModeParamType(int modeIndex){
		return paramsFromMode[modeIndex];
	}

	//! Static method that identifies the current mouse mode for a particular params type.
	//! \param[in] ParamsBase::ParamsBaseType t must be the type of a params with an associated mouse mode
	//! \retval int Mouse mode
	static int getModeFromParams(ParamsBase::ParamsBaseType t){return modeFromParams[t];}

	//! Static method that identifies the name associated with a mouse mode.
	//! This is the text that is displayed in the mouse mode selector.
	//! \param[in] int Mouse Mode
	//! \retval const string& Name associated with mode
	static const string getModeName(int index) {return modeName[index];}

	//! Static method called at startup to register all the built-in mouse modes,
	//! by calling RegisterMouseMode() for each built-in mode.
	//! Also calls InstallExtensionMouseModes() to register extension modes.
	static void RegisterMouseModes();

	//! Static method indicates the current mouse mode
	//! \retval mouseModeType current mouse mode
	static mouseModeType  GetCurrentMouseMode(){
		return ((MouseModeParams*)Params::GetParamsInstance(_mouseModeParamsTag))->getCurrentMouseMode();
	}
	//! Static method sets the current mouse mode
	//! \param[in] mouseModeType t current mouse mode
	static void SetCurrentMouseMode(mouseModeType t){
		((MouseModeParams*)Params::GetParamsInstance(_mouseModeParamsTag))->setCurrentMouseMode(t);
	}
	//! Static method indicates how many mouse modes are available.
	static int getNumMouseModes(){
		return modeName.size();
	}
	

#ifndef DOXYGEN_SKIP_THIS
	
	static const string _mouseModeParamsTag;

protected:
	//! non-static Set/get methods are not public.
	int setCurrentMouseMode(int mode){
		return SetValueLong(_mouseModeTag, "Set mouse mode",(long)mode);
	}
	mouseModeType getCurrentMouseMode(){
		return (mouseModeType)GetValueLong(_mouseModeTag);
	}
	static const string _shortName;
	static const string _mouseModeTag;
	//Mouse Mode tables.  Static
	static vector<ParamsBase::ParamsBaseType> paramsFromMode;
	static map<ParamsBase::ParamsBaseType, int> modeFromParams;
	static vector<int> manipFromMode;
	static vector<string> modeName;
	static map<int,const char* const *> modeXPMIcon;
	
	
#endif //DOXYGEN_SKIP_THIS
};

};
#endif //MOUSEMODEPARAMS_H 
