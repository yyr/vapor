;
;   AddCurlVDF.pro
;
;   Utility to read three variables from a VDF, calculate their curl
;   and put it back into the VDF.
;   All three variables must be present at full resolution.
;   This is performed one time-step at a time.
;   The vdf file is replaced.  The previous vdf file is saved, with
;   _saved appended to its name, in case of catastrophic failure
;
;   Arguments are:
;   vdffile = file path of the metadata file
;   varx, vary, varz = the 3 variables defining the file
;     whose magnitude is being calculated
;   curlx,curly,curlz are the names for the three components
;   of the curl being calculated
;   timestep specifies the (only) timestep for which the magnitude
;     is calculated.  If multiple timesteps are needed,
;     the calculation must be repeated for each of them.
;

PRO AddCurlVDF,vdffile,varx,vary,varz,curlx,curly,curlz,timestep

;
;   Start with the current metadata:
;

mfd = vdf_create(vdffile)

;
;   save the current vdf file (in case we screw up)
;

savedvdffile = STRING(vdffile,'_saved')
vdf_write,mfd,savedvdffile

;
;   Add the new variable names to the current variable names:
;

varnames = vdf_getvarnames(mfd)
numvarsarray = size(varnames)
numvars = 3 + numvarsarray[1]
newvarnames = strarr(numvars)
;   Need to make sure these are not in the list!
repeatvariables = 0
print, 'numvars = ',numvars
FOR I = 0, numvars-4 DO BEGIN
    print, 'I = ',I,' var = ',varnames[I]
    newvarnames[I] = varnames[I]
    if (varnames[I] EQ curlx) THEN repeatvariables = 1+repeatvariables
    if (varnames[I] EQ curly) THEN repeatvariables = 1+repeatvariables
    if (varnames[I] EQ curlz) THEN repeatvariables = 1+repeatvariables
ENDFOR

IF (repeatvariables GT 0 AND repeatvariables LT 3) THEN BEGIN
    print, 'ERROR:  some but not all curl variable names exist already'
    STOP
ENDIF

IF (repeatvariables EQ 0) THEN BEGIN
    newvarnames[numvars-3] = curlx
    newvarnames[numvars-2] = curly
    newvarnames[numvars-1] = curlz
ENDIF

IF (repeatvariables EQ 3) THEN newvarnames = varnames


;
;   reset the varnames in mfd to the new value:
;
if (repeatvariables EQ 0) THEN vdf_setvarnames,mfd,newvarnames

reflevel = vdf_getnumtransforms(mfd)
numtimesteps = vdf_getnumtimesteps(mfd)

;
;   Create "Buffered Read" objects for each variable to read the data, passing the
;   metadata object handle created by vdf_create() as an argument
;

dfdx = vdc_bufreadcreate(mfd)
dfdy = vdc_bufreadcreate(mfd)
dfdz = vdc_bufreadcreate(mfd)
dfdcurlx = vdc_bufwritecreate(mfd)
dfdcurly = vdc_bufwritecreate(mfd)
dfdcurlz = vdc_bufwritecreate(mfd)


;
;
;   Determine the dimensions of the x-variable at the full transformation
;   level.
;   This is used as the dimension of all the variables
;   Note. vdc_getdim() correctly handles dimension calucation for
;   volumes with non-power-of-two dimensions.
;

dim = vdc_getdim(dfdx, reflevel)

;
;   Create appropriately sized arrays to hold the source and result data
;

srcx = fltarr(dim[0],dim[1],dim[2])
srcy = fltarr(dim[0],dim[1],dim[2])
srcz = fltarr(dim[0],dim[1],dim[2])

dstx = fltarr(dim[0],dim[1],dim[2])
dsty = fltarr(dim[0],dim[1],dim[2])
dstz = fltarr(dim[0],dim[1],dim[2])

;
;   Prepare to read the indicated time step and variables
;

vdc_openvarread, dfdx, timestep, varx, reflevel

;
;   Read the volume one slice at a time
;
slcx = fltarr(dim[0],dim[1])
slcy = fltarr(dim[0],dim[1])
slcz = fltarr(dim[0],dim[1])

FOR z = 0, dim[2]-1 DO BEGIN
    vdc_bufreadslice, dfdx, slcx

    ; copy to 3d array
    srcx[*,*,z] = slcx

    ;  Report every 100 reads:
    IF ((z MOD 100) EQ 0) THEN print,'reading x  slice ',z
ENDFOR
vdc_closevar, dfdx
vdc_bufreaddestroy, dfdx
vdc_openvarread, dfdy, timestep, vary, reflevel
FOR z = 0, dim[2]-1 DO BEGIN
    vdc_bufreadslice, dfdy, slcy
    ; copy to 3d array
    srcy[*,*,z] = slcy
    ;  Report every 100 reads:
    IF ((z MOD 100) EQ 0) THEN print,'reading y  slice ',z
ENDFOR

vdc_closevar, dfdy
vdc_bufreaddestroy, dfdy
vdc_openvarread, dfdz, timestep, varz, reflevel
FOR z = 0, dim[2]-1 DO BEGIN
    vdc_bufreadslice, dfdz, slcz
    ; copy to 3d array
    srcz[*,*,z] = slcz
    ;  Report every 100 reads:
    IF ((z MOD 100) EQ 0) THEN print,'reading z  slice ',z
ENDFOR

vdc_closevar, dfdz
vdc_bufreaddestroy, dfdz
;  Now perform the curl on the data
curl_findiff,srcx,srcy,srcz,dstx,dsty,dstz,.01,.01,.01

print,'performed the curl on ',varx,' ', vary,' ', varz


vdc_openvarwrite, dfdcurlx, timestep, curlx, reflevel

; write the data one slice at a time
FOR z = 0, dim[2]-1 DO BEGIN
    slcx = dstx[*,*,z]
    vdc_bufwriteslice,dfdcurlx, slcx
    ;  Report every 100 writes:
    IF ((z MOD 100) EQ 0) THEN print,'writing x slice ',z
ENDFOR

vdc_closevar, dfdcurlx
vdc_bufwritedestroy, dfdcurlx
vdc_openvarwrite, dfdcurly, timestep, curly, reflevel
FOR z = 0, dim[2]-1 DO BEGIN
    slcy = dsty[*,*,z]
    vdc_bufwriteslice,dfdcurly, slcy
    ;  Report every 100 writes:
    IF ((z MOD 100) EQ 0) THEN print,'writing y slice ',z
ENDFOR

vdc_closevar, dfdcurly
vdc_bufwritedestroy, dfdcurly
vdc_openvarwrite, dfdcurlz, timestep, curlz, reflevel
FOR z = 0, dim[2]-1 DO BEGIN
    slcz = dstz[*,*,z]
    vdc_bufwriteslice,dfdcurlz, slcz
    ;  Report every 100 writes:
    IF ((z MOD 100) EQ 0) THEN print,'writing z slice ',z
ENDFOR

vdc_closevar, dfdcurlz
vdc_bufwritedestroy, dfdcurlz
;
;  Replace the vdf file with the new one
;

vdf_write,mfd,vdffile
vdf_destroy, mfd

end
