//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************///
//	File:		renderer.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		September 2004
//
//	Description:	Defines the Renderer class.
//		A pure virtual class that is implemented for each renderer.
//		Methods are called by the visualizer class as needed.
//

#ifndef RENDERER_H
#define RENDERER_H


#include <vapor/MyBase.h>
#include <vapor/common.h>
#include "params.h"
#include "visualizer.h"

using namespace VetsUtil;

namespace VAPoR {
class DataMgr;
class Metadata;

//! \class Renderer
//! \ingroup Public_Render
//! \brief A class that performs rendering in a Visualizer
//! \author Alan Norton
//! \version $Revision$
//! \date    $Date$
//!
//! Renderer class is a pure virtual class that supports 
//! OpenGL rendering in the VAPOR visualizer window.
//! All renderers must derive from this class.
class RENDER_API Renderer: public MyBase
{
	
public:
	//! Constructor should be invoked by any derived renderers.
	//! It is invoked when the user enables a renderer.
	//! Provides any needed setup of renderer state, but not of OpenGL state.
	//
	Renderer(Visualizer* vw, RenderParams* rp, string name);
	virtual ~Renderer();
	//! Pure virtual method
	//! Any OpenGL initialization is performed in initializeGL
	//! It will be called from an OpenGL rendering context.
	//! Sets initialized to true if successful.
    virtual void	initializeGL();

	//! All OpenGL rendering is performed in the pure virtual paintGL method.
    virtual void		paintGL();
	
	//! Whenever the Params associated with the renderer is changed, setRenderParams must be called.
	//! This virtual method should be overridden if there are any dirty flags registered by the renderer.
	//! \param[in] RenderParams* rp Pointer to the current RenderParams instance
	virtual void setRenderParams(RenderParams* rp) {currentRenderParams = rp;}

	//! Obtain the current RenderParams instance
	//! \retval RenderParams* current render params
	RenderParams* getRenderParams(){return currentRenderParams;}

	//! Identify whether the renderer has been initialized 
	//! \retval bool initialized
	bool isInitialized() {return initialized;}

	//! Call setBypass to indicate that the renderer will not work until the state of the params is changed
	//! This will result in the renderer not being invoked for the specified timestep
	//! \param[in] int timestep The timestep when the renderer fails
	void setBypass(int timestep) {if(currentRenderParams)currentRenderParams->setBypass(timestep);}

	//! Partial bypass is currently only used by DVR and Isosurface renderers.
	//! Indicates a renderer that should be bypassed at
	//! full resolution but not at interactive resolution.
	//! \param[in] timestep Time step that should be bypassed
	void setPartialBypass(int timestep) {if(currentRenderParams)currentRenderParams->setPartialBypass(timestep);}

	//! SetAllBypass is set to indicate all (or no) timesteps should be bypassed
	//! Should be set true when render failure is independent of timestep
	//! Should be set false when state changes and rendering should be reattempted.
	//! \param[in] val indicates whether it is being turned on or off. 
	void setAllBypass(bool val){if (currentRenderParams) currentRenderParams->setAllBypass(val);}

	//! doBypass indicates the state of the bypass flag.
	//! \param[in] timestep indicates the timestep being checked
	//! \retval bool indicates that the rendering at the timestep should be bypassed.
	bool doBypass(int timestep) {return (currentRenderParams && currentRenderParams->doBypass(timestep));}

	//! doAlwaysBypass is used in the presence of partial bypass.
	//! Indicates that the rendering should be bypassed at any resolution
	//! \param[in] int timestep Time step
	//! \retval bool value of flag
	bool doAlwaysBypass(int timestep) {
		return (currentRenderParams && currentRenderParams->doAlwaysBypass(timestep));}

	//! General method to force a renderer to re-obtain its data.  
	//! Must be implemented, although does not need to do anything if
	//! no state or cache is retained between renderings.
	virtual void setAllDataDirty() = 0;

	//! @name Internal
	//! Internal methods not intended for general use
	//!
	///@{

	//! Static method invoked during Undo and Redo of Renderer enable or disable.
	//! This function must be passed in Command::CaptureStart for enable and disable rendering.
	//! It causes the Undo and Redo operations on enable/disable renderer to actually create
	//! or destroy the renderer.
	//! \sa UndoRedoHelpCB_T
	//! \param[in] isUndo indicates whether an Undo or Redo is being performed
	//! \param[in] instance indicates the RenderParams instance that is enabled or disabled
	//! \param[in] beforeP is a copy of the InstanceParams at the start of the Command
	//! \param[in] afterP is a copy of the InstanceParams at the end of the Command 
	static void UndoRedo(bool isUndo, int instance, Params* beforeP, Params* afterP);
	

	//! Identify the name of the current renderer (for error messages)
	//! \retval name (i.e. tag) of renderer
	const string getMyName() const {return(_myName);};
	
	///@}
	

protected:

	//! Pure virtual method
	//! Any OpenGL initialization is performed in initializeGL
	//! It will be called from an OpenGL rendering context.
    virtual int	_initializeGL() = 0;

	//! All OpenGL rendering is performed in the pure virtual paintGL method.
    virtual int	_paintGL() = 0;

	//! Enable specified clipping planes during the GL rendering
	//! Must be invoked during paintGL()
	//! \sa disableClippingPlanes
	//! \param[in] extents Specifies the extents of the clipping bounds
	void enableClippingPlanes(const double extents[6]);
	//! Enable clipping planes associated with the full 3D data domain
	//! \sa disableClippingPlanes
	void enableFullClippingPlanes();
	//! Enable clipping planes associated with the current RegionParams extents
	//! \sa disableClippingPlanes
	void enableRegionClippingPlanes();
	//! Enable clipping planes associated with the current TwoDDataParams extents
	//! \sa disableClippingPlanes
	void enable2DClippingPlanes();
	//! Disable the clipping planes that were previously enabled during
	//! enableClippingPlanes(), enableFullClippingPlanes(), enableRegionClippingPlanes, or enable2DClippingPlanes()
	void disableClippingPlanes();
	

#ifndef DOXYGEN_SKIP_THIS
	Visualizer* myVisualizer;
	RenderParams* currentRenderParams;
	bool initialized;
	
private:
	string _myName;
#endif //DOXYGEN_SKIP_THIS
};
};

#endif // RENDERER_H
