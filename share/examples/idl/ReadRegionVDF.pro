;	$Id$
;
;	This example shows how to read a single data volume
;	from a VDF data collection using a "Region Read" object. The data 
;	volume	is not read in its entirety -- a spatial subregion (the first
;	octant) is extracted from the full spatial domain.  The volume
;	resolution, however, is the native data resolution.
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
num_xforms = 0

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
dfd = vdf_regreadcreate(mfd)

;
;	Determine the dimensions of the volume at the given transformation
;	level. 
;
;	Note. vdf_getdim() correctly handles dimension calucation for 
;	volumes with non-power-of-two dimensions. 
;
dim = vdf_getdim(dfd, num_xforms)

;
;	Compute the coordinates for the desired subregion. In this case, the 
;	first octant will be read
min = [0,0,0]
max = (dim / 2) - 1

;
;	Create an appropriately sized array to hold the volume
;
f = fltarr(dim/2)


;
;	Prepare to read the indicated time step and variable
;
varnames = ['ml']
vdf_openvarread, dfd, 0, varnames[0], num_xforms

;
;	Read the volume subregion. Note, unlike the buffered read/write
;	objects, the "Region Reader" object does not read a single slice
;	at a time -- it slurps in the entire in single call.
;
vdf_regread, dfd, min, max, f

;
; Close the currently opened variable/time-step. 
;
vdf_closevar, dfd


;
;	Destroy the "buffered read" data transformation object. 
;	We're done with it.
;
vdf_regreaddestroy, dfd

vdf_mdestroy, mfd

end
