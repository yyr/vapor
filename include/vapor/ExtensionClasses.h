//This header file tells where to find all extension class definitions.
//All such extension classes must be derived from ParamsBase
//These can include Params classes, as well as components such as TransferFunctions
//
//The InstallExtension method installs the extension classes and  (invoked by ControlExecutive).

#include "../lib/params/arrowparams.h"
#include "../lib/params/viewpoint.h"
#include "../lib/params/Box.h"
#include "../apps/vaporgui/images/arrowrake.xpm"


namespace VAPoR {
	//For each extension class, insert the methods ParamsBase::RegisterParamsBaseClass 
	//Into the following method:
	static void InstallExtensions(){
           ParamsBase::RegisterParamsBaseClass(ArrowParams::_arrowParamsTag, ArrowParams::CreateDefaultInstance, true);
	}
	
};

