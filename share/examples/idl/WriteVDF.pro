;	$Id$
;
;	This example shows how to create a data set containing 
;	a single variable and at a single time step
;


; 
;	Dimensions of the data volumes - all volumes in a data set must be 
;	of the same dimension
;
dim = [64,64,64]

;
;	Number of forward wavelet transforms to apply to data stored in
;	the data set. A value of 0 indicates that the data should not 
;	be transformed. A value of 1 implies a single transform. The resulting
;	data will be accessible at the original resolution and 1/8th resolution 
;	(half resolution in each dimension). A value of two implies two 
;	coarsenings, and so on.
;
num_xforms = 1

;
;	Create a new VDF metadata object of the indicated dimension and 
;	transform
;	level. vdf_mcreate() returns a handle for future operations on 
;	the metadata object.
;
mfd = vdf_mcreate(dim,num_xforms)

;
;	Set the maximum number of timesteps in the data set. Note, valid data 
;	set may contain less than the maximum number of time steps, 
;	but not more
;
timesteps = 1
vdf_msetnumtimesteps, mfd,timesteps

;
;	Set the names of the variables the data set will contain. In this case,
;	only a single variable will be present, "ml"
;
varnames = ['ml']
vdf_msetvarnames, mfd, varnames

;
;	Store the metadata object in a file for subsequent use
;
vdffile = '/tmp/test.vdf'
vdf_mwrite, mfd, vdffile

;
;	Destroy the metadata object. We're done with it.
;
vdf_mdestroy, mfd


;
;	At this point we've defined all of the metadata associated with 
;	the test data set. Now we're ready to begin populating the data
;	set with actual data. Our data, in this case, will be a 
;	time-varying Marschner Lobb function sampled on a regular grid
;


;
;	Create a "buffered write" data transformation object. The data
;	transformation object will permit us to write (transform) raw
;	data into the data set. The metadata for the data volumes is
;	obtained from the metadata file we created previously. I.e.
;	'vdffile' must contain the path to a previously created .vdf
;	file. The 'vdf_bufwritecreate' returns a handle, 'dfd', for 
;	subsequent ;	operations.
;
dfd = vdf_bufwritecreate(vdffile)


; Create a synthetic data volume
;
f = marschner_lobb(dim[0], dim[1], dim[2])

; 
; Prepare the data set for writing. We need to identify the time step
; and the name of the variable that we wish to store
;
vdf_openvarwrite, dfd, 0, varnames[0]


;
; Write (transform) the volume to the data set one slice at a time
;
for z = 0, dim[2]-1 do begin
	vdf_bufwriteslice, dfd, f[*,*,z]
endfor

;
; Close the currently opened variable/time-step. We're done writing
; to it
;
vdf_closevar, dfd


;
;	Destroy the "buffered write" data transformation object. 
;	We're done with it.
;
vdf_bufwritedestroy, dfd

end
