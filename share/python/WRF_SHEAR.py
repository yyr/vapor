#Python program to calculate Horizontal wind shear
#To avoid meaningless values below terrain, set U and V to have value 0 below terrain.
s = shape(U)	#size of the input arrays  
ext = (__BOUNDS__[3]-__BOUNDS__[0], __BOUNDS__[4]-__BOUNDS__[1],__BOUNDS__[5]-__BOUNDS__[2])
usrmax = vapor.MapVoxToUser([__BOUNDS__[3],__BOUNDS__[4],__BOUNDS__[5]],__REFINEMENT__)
usrmin = vapor.MapVoxToUser([__BOUNDS__[0],__BOUNDS__[1],__BOUNDS__[2]],__REFINEMENT__)
dz =  (usrmax[0]-usrmin[0])/ext[0] #vertical delta in grid

WRF_SHEAR = empty(s,float32)
UDIFF = (U[1:s[0]-1,:,:]-U[0:s[0]-2,:,:])*(U[1:s[0]-1,:,:]-U[0:s[0]-2,:,:])
VDIFF = (V[1:s[0]-1,:,:]-V[0:s[0]-2,:,:])*(V[1:s[0]-1,:,:]-V[0:s[0]-2,:,:])
WRF_SHEAR[0:s[0]-2,:,:] = sqrt(UDIFF+VDIFF)/dz
WRF_SHEAR[s[0]-1,:,:] = 0.0 
