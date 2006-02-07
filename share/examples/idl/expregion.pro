; $Id$
;
; NAME:
;       EXPREGION
;
; PURPOSE:
;		Export a volume subregion to VAPoR. This procedure is a convenience
;		function for exporting a subvolume to VAPOR. In essense, it simply
;		creates a mini VDC with the same basic attributes (dimension, 
;		user extents, num transforms, etc) as an existing "parent" VDC. 
;		The new, mini VDC may be merged with the parent VDC by the
;		vapor gui.
;
;		N.B. This procedure assumes that the region to be exported is sampled
;		at the native (highest) resolution of the grid.
;
; CALLING SEQUENCE:
;       EXPREGION, VDF, ARRAY, STATEINFO
;
; KEYWORD PARAMETERS:
;       VARNAME:	The name of the variable that is being exported. The
;					default is to use the VARNAME structure member specified
;					by the STATEINFO parameter
;
;       COORD:		A three-element array containing the starting XYZ 
;					coordinates (in voxels) of the region
;					to be exported relative to the parent VDC. The
;					default is to use the MINRANGE structure member of 
;					the STATEINFO paramater
;
;
; INPUTS:
;
;		VDF:		Path name of the vdf file that will describe
;					the region
;
;		ARRAY:		A 3D array containig the region to export
;
;		STATEINFO:	A stateinfo structure returned by vaporimport() (the
;					parent VDC)
;
; OUTPUTS:
;
; COMMON BLOCKS:
;       None.
;
;-
pro expregion, vdf, array, stateinfo, VARNAME=varname, COORD=coord


on_error,2                      ;Return to caller if an error occurs
if keyword_set(varname) eq 0 then varname = stateinfo.varname
if keyword_set(timestep) eq 0 then timestep = 0
if keyword_set(coord) eq 0 then coord = stateinfo.minrange


	;
	; Open the .vdf file describing the parent VDC
	;
	mfd = vdf_create(stateinfo.vdfpath)

	;
	; Extract the relevant attributes from the parent VDC
	;
	nfiltercoef = vdf_getfiltercoef(mfd)
	nliftingcoef = vdf_getliftingcoef(mfd)
	bs = vdf_getblocksize(mfd)
	nxforms = vdf_getnumtransforms(mfd)
	dim = vdf_getdimension(mfd)

	vdf_destroy, mfd

	;
	; Create a new, "mini" VDC with the same basic attributes of the
	; parente VDC
	;
	mfd = vdf_create(dim, nxforms, BS=bs, NFILTERCOEF=nfiltercoef, NLIFTINGCOEF=nliftingcoef)

	;
	; Add the new variable to the mini VDC
	;
	varnames = [varname]
	vdf_setvarnames, mfd, varnames

	vdf_write, mfd, vdf
	vdf_destroy, mfd


	;
	; Now write the array to the new VDC
	;
	dfd = vdc_regwritecreate(vdf)

	vdc_openvarwrite, dfd, 0, varnames, -1

	vdc_regwrite, dfd, array, coord

	vdc_closevar, dfd

	vdc_regwritedestroy, dfd

end
