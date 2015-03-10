''' The vapor module is internal to vaporgui, so its methods and variables can only be accessed in the python scripts used to calculate derived variables inside vaporgui.  
	This module includes the following methods:
	vapor.MapVoxToUser - map voxel coordinates to user coordinates
	vapor.MapUserToVox - map user coordinates to voxel coordinates
	vapor.GetValidRegionMax - determine maximum valid voxel coordinates of a variable
	vapor.GetValidRegionMin - determine minimum valid voxel coordinates of a variable
	vapor.Get3DVariable - Obtain a 3 dimensional array of variable data from the VDC
	vapor.Get2DVariable - Obtain a 2 dimensional array of variable data from the VDC
	vapor.VariableExists - Determine whether a specified variable exists at a timestep
	vapor.Get3DMask - Obtain a 3D mask variable indicating where a variable is not equal to its missing value
	vapor.Get2DMask - Obtain a 2D mask variable indicating where a variable is not equal to its missing value.

	This module also includes the following variables
	vapor.TIMESTEP - The integer time step of the output variable(s) being calculated.
	vapor.REFINEMENT - the integer refinement level of the output variable(s) being calculated. 
	vapor.LOD - The integer level-of-detail (compression level) of the output variable(s) being calculated.
	vapor.BOUNDS - The integer 6-tuple specifying the integer extents of the Z, Y, and X coordinates 
	of the variable being calculated, inside the full 3D domain of the current VDC. '''

def MapVoxToUser(voxcoord, refinementLevel ,lod): 
	''' Convert voxel coordinates to user coordinates, at a given refinement level and level of detail.
	Calling sequence: userCoord = MapVoxToUser(voxcoord, refLevel ,lod) 
	Where: voxcoord is an integer 3-tuple indicating voxel coordinates to convert.
	refLevel is the integer refinement level
	lod (optional, defaults to 0) is the integer compression level 
	Result userCoord is the returned float32 3-tuple of user coordinates.''' 
	
def MapUserToVox(userCoord, refinementLevel ,lod): 
	''' Convert user coordinates to voxel coordinates, at a given refinement level and level of detail.
	Calling sequence: voxCoord = MapUserToVox(userCoord, refLevel [,lod]) 
	Where: userCoord is a float3 3-tuple indicating user coordinates to convert.
	refLevel is the integer refinement level
	lod (optional, defaults to 0) is the integer compression level 
	Result voxCoord is the returned integer 3-tuple of voxel coordinates.''' 
	
def GetValidRegionMax(timestep, variableName, refinementLevel):
	''' Determine the maximum voxel coordinates of a variable at a timestep and refinement level.
	Calling sequence: maxCoord = vapor.GetValidRegionMax(timestep,variableName,refinementLevel)
	Where: timestep is an integer time step in the VDC.
	variableName is a string variable name in the VDC
	refinementLevel is an integer refinement level in the VDC
	returns maxCoord, an integer 3-tuple, the maximum coordinates of the variable at the timestep and refinement level.'''

def GetValidRegionMin(timestep, variableName, refinementLevel):
	''' Determine the minimum voxel coordinates of a variable at a timestep and refinement level.
	Calling sequence: minCoord = vapor.GetValidRegionMin(timestep,variableName,refinementLevel)
	Where: timestep is an integer time step in the VDC.
	variableName is a string variable name in the VDC
	refinementLevel is an integer refinement level in the VDC
	returns minCoord, an integer 3-tuple, the minimum coordinates of the variable at the timestep and refinement level.'''

def Get3DVariable(timestep, variableName, refinement, lod, bounds):
	''' Obtain a 3-dimensional array of variable data from the VDC, using:
	timestep (integer time step)
	variableName (the variable name, a string)
	refinement (integer refinement level)
	lod (integer compression level)
	bounds (6-tuple integer extents identifying the location of the data in the full domain, 
	as with vapor.BOUNDS. 
	Returns a 3-dimensional float32 array of variable values.'''

def Get2DVariable(timestep, variableName, refinement, lod, bounds):
	''' Obtain a 2-dimensional array of variable data from the VDC, using:
	timestep (integer time step)
	variableName (the variable name, a string)
	refinement (integer refinement level)
	lod (integer compression level)
	bounds (6-tuple integer extents identifying the location of the data in the full domain, 
	as with vapor.BOUNDS, however the first and fourth element of this tuple are ignored. 
	Returns a 2-dimensional float32 array of variable values.'''

def VariableExists(timestep, varname ,refinement ,lod):
	''' Determine if a variable is present in the VDC at specified timestep, refinement, and lod.  Arguments are:
	timestep (integer time step)
	varname (the variable name, a string)
	refinement (integer refinement level, optional, defaults to 0)
	lod (integer compression level, optional, defaults to 0)
	Returns: boolean value, true if the variable exists.'''
	
def Get3DMask(timestep, varname, refinement, lod, bounds):
	''' Obtain the boolean 3D mask associated with a 3D variable at a timestep, with specified refinement, lod, and bounds. Arguments are:
	timestep (integer time step)
	varname (string variable name)
	refinement (integer refinement level)
	lod (integer compression level)
	bounds (6-tuple integer extents identifying the location of the data in the full domain, as with vapor.BOUNDS.
	Returns:  a 3-dimensional boolean array, which is the requested mask.'''
	
def Get2DMask(timestep, varname, refinement, lod, bounds):
	''' Obtain the boolean 2D mask associated with a 2D variable at a timestep, 
	with specified refinement, lod, and bounds. Arguments are:
	timestep (integer time step)
	varname (string variable name)
	refinement (integer refinement level)
	lod (integer compression level)
	bounds (6-tuple integer extents identifying the location of the data in the full domain, 
	as with vapor.BOUNDS.  The first and fourth entry are ignored.
	Returns:  a 2-dimensional boolean array, which is the requested mask.'''
	


