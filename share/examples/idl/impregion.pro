;+
; NAME:
;       IMPREGION
;
; PURPOSE:
;		Import a volume subregion from VAPoR. The subregion must
;		first have been exported by VAPoR
;       endian or big endian format. 
;
; CALLING SEQUENCE:
;       IMPREGION
;
; KEYWORD PARAMETERS:
;       REFLEVEL:    The desired refinement levele. If this keyword is 
;			not set, the subregion will be imported at full resolution
;
; OUTPUTS:
;       The region is returned
;
; COMMON BLOCKS:
;       None.
;
;-
function impregion, REFLEVEL=reflevel


on_error,2                      ;Return to caller if an error occurs
if keyword_set(reflevel) eq 0 then reflevel = -1

	; Get the state info exported from VAPoR that describes the 
	; region of interest. vaporimport() returns a structure with 
	; the following fields:
	;	VDFPATH		: Path to the .vdf file
	;	TIMESTEP	: Time step of the volume subregion of interest
	;	VARNAME		: Variable name of volume subregion of interest
	;	MINRANGE	: Minimum extents of region of interest in volume
	;				coordinates specified relative to the finest resolution
	;	MAXRANGE	: Maximum extents of region of interest in volume
	;				coordinates specified relative to the finest resolution
	stateinfo = vaporimport()

	;
	;   Create a "Buffered Read" object to read the data, passing the
	;   metadata object handle created by vdf_create() as an argument
	;
	dfd = vdc_regreadcreate(stateinfo.vdfpath)

	;
	; Transform coordinates from finest resolution to resolution
	; of interest. Note, the coordinates returned in 'stateinfo' 
	; are in voxel coordinates relative to the finest resolution level.
	; We first convert voxel coordinates to user coordinates, then 
	; convert user coordinates back to voxel coordinates, but at 
	; the requested refinement level. 
	;
	minu = vdc_mapvox2user(dfd, -1, stateinfo.timestep,stateinfo.minrange)
	min = vdc_mapuser2vox(dfd,reflevel,stateinfo.timestep,minu)

	maxu = vdc_mapvox2user(dfd, -1, stateinfo.timestep,stateinfo.maxrange)
	max = vdc_mapuser2vox(dfd,reflevel,stateinfo.timestep,maxu)


	; Create an array large enough to hold the volume subregion
	;
	f = fltarr(max[0]-min[0]+1,max[1]-min[1]+1,max[2]-min[2]+1)

	; Select the variable and time step we want to read
	;
	vdc_openvarread, dfd, stateinfo.timestep, stateinfo.varname, reflevel

	; Read the subregion
	;
	vdc_regread, dfd, min, max, f

	vdc_closevar, dfd

	vdc_regreaddestroy, dfd

	return, f

end
