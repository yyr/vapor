;	$Id$
;
;	This example shows how to read a single data volume
;	from a VDF data collection using a "Buffered Read" object. The entire
;	spatial domain of the volume is retrieved (as opposed to fetching 
;	a spatial subregion). However, the volume is extracted at 1/8th 
;	its native spatial resolution (1/2 resolution along each dimension).
;
;	Note: in order to run this example you must have first created
;	a VDF data collection by running either the WriteVDF.pro
;	or WriteTimeVaryVDF.pro example programs



;
;	Number of forward wavelet transforms.
;	A value of 0 indicates that the data should be read at full  (native)
;	resolution. A value of 1 implies a single transform - 1/8th 
;	resolution.
;
num_xforms = 1

;
;	Create a VDF metadata object from an existing metadata file. The
;	metadata file must already exist on disk, having been created from
;	one of the example programs that generates a .vdf file.
;
vdffile = '/tmp/test.vdf'
mfd = vdf_mcreate(vdffile)


;
;	Create a "Buffered Read" object to read the data, passing the 
;	metadata object handle created by vdf_mcreate() as an argument
;
dfd = vdf_bufreadcreate(mfd)

;
;	Determine the dimensions of the volume at the given transformation
;	level. 
;
;	Note. vdf_getdim() correctly handles dimension calucation for 
;	volumes with non-power-of-two dimensions. 
;
dim = vdf_getdim(dfd, num_xforms)

;
;	Create an appropriately sized array to hold the volume
;
f = fltarr(dim)
slice = fltarr(dim[0], dim[1])


;
;	Prepare to read the indicated time step and variable
;
varnames = ['ml']
vdf_openvarread, dfd, 0, varnames[0], num_xforms

;
;	Read the volume one slice at a time
;
for z = 0, dim[2]-1 do begin
	vdf_bufreadslice, dfd, slice
	
	; IDL won't let us read directly into a subscripted array - need 
	; to read into a 2D array and then copy to 3D :-(
	;
	f[*,*,z] = slice
endfor

;
; Close the currently opened variable/time-step. 
;
vdf_closevar, dfd


;
;	Destroy the "buffered read" data transformation object. 
;	We're done with it.
;
vdf_bufreaddestroy, dfd

end
