from numpy import *

def mag3d(a1,a2,a3): 
	return sqrt(a1*a1 + a2*a2 + a3*a3)

def mag2d(a1,a2): 
	return sqrt(a1*a1 + a2*a2)

# NAME:
#       DERIV_FINDIFF
#
# PURPOSE:
#       Computes first order derivatives using sixth order finite
#       differences in regular Cartesian grids
#
# CALLING SEQUENCE:
#       APRIME = DERIV_FINDIFF,IN,DIR,DX
#
# PARAMETERS:
#       A[in]:   3D array with the field component
#       DIR[in]:  direction (1,2, or 3) of the the derivative
#       DX[in]:   spatial step of the grid in the direction DIR
#
# RETURNS:       
#       APRIME[out]: 3D array with the spatial derivative
#
#
def deriv_findiff(a,dir,dx):

	s = shape(a)	#size of the input array
	aprime = array(a)	#output has the same size than input

	#
	# derivative for dir=1
	#
	if dir == 3: 

		#forward differences near the first boundary
		for i in range(3):
			aprime[:,:,i] = (-11*a[:,:,i]+18*a[:,:,i+1]-9*a[:,:,i+2]+2*a[:,:,i+3]) / (6*dx)     

		#centered differences
		for i in range(3,s[2]-3):
			aprime[:,:,i] = (-a[:,:,i-3]+9*a[:,:,i-2]-45*a[:,:,i-1]+45*a[:,:,i+1] -9*a[:,:,i+2]+a[:,:,i+3])/(60*dx) 

		#backward differences near the second boundary
		for i in range(s[2]-3,s[2]):
			aprime[:,:,i] = (-2*a[:,:,i-3]+9*a[:,:,i-2]-18*a[:,:,i-1]+11*a[:,:,i]) /(6*dx)     

	#
	# derivative for dir=2
	#
	if dir == 2: 
		for i in range(3):
			aprime[:,i,:] = (-11*a[:,i,:]+18*a[:,i+1,:]-9*a[:,i+2,:]+2*a[:,i+3,:]) /(6*dx)     #forward differences near the first boundary

		for i in range(3,s[1]-3):
			aprime[:,i,:] = (-a[:,i-3,:]+9*a[:,i-2,:]-45*a[:,i-1,:]+45*a[:,i+1,:] -9*a[:,i+2,:]+a[:,i+3,:])/(60*dx) #centered differences

		for i in range(s[1]-3,s[1]):
			aprime[:,i,:] = (-2*a[:,i-3,:]+9*a[:,i-2,:]-18*a[:,i-1,:]+11*a[:,i,:]) /(6*dx)     #backward differences near the second boundary

	#
	# derivative for dir=3
	#
	if dir == 1:
		for i in range(3):
			aprime[i,:,:] = (-11*a[i,:,:]+18*a[i+1,:,:]-9*a[i+2,:,:]+2*a[i+3,:,:]) /(6*dx)     #forward differences near the first boundary

		for i in range(3,s[0]-3):
			aprime[i,:,:] = (-a[i-3,:,:]+9*a[i-2,:,:]-45*a[i-1,:,:]+45*a[i+1,:,:] -9*a[i+2,:,:]+a[i+3,:,:])/(60*dx) #centered differences

		for i in range(s[0]-3,s[0]):
			aprime[i,:,:] = (-2*a[i-3,:,:]+9*a[i-2,:,:]-18*a[i-1,:,:]+11*a[i,:,:]) /(6*dx)     #backward differences near the second boundary

	return aprime

def curl_findiff(A,B,C):

	ext = (__BOUNDS__[3]-__BOUNDS__[0], __BOUNDS__[4]-__BOUNDS__[1],__BOUNDS__[5]-__BOUNDS__[2])
	print ext
	usrmax = vapor.MapVoxToUser([__BOUNDS__[3],__BOUNDS__[4],__BOUNDS__[5]],__REFINEMENT__)
	usrmin = vapor.MapVoxToUser([__BOUNDS__[0],__BOUNDS__[1],__BOUNDS__[2]],__REFINEMENT__)
	print usrmax, usrmin
	dz = (usrmax[2]-usrmin[2])/ext[2]	# grid delta in Python coord system
	dy =  (usrmax[1]-usrmin[1])/ext[1]
	dx =  (usrmax[0]-usrmin[0])/ext[0]
	
#  in Python coordinates, C,B,A are X,Y,Z components of vector field
	aux1 = deriv_findiff(A,2,dy)       #x component of the curl (in python system)
	aux2 = deriv_findiff(B,3,dz)
	outx = aux2-aux1						#is -z component in user coordinates...

	aux1 = deriv_findiff(C,3,dz)       #y component of the curl
	aux2 = deriv_findiff(A,1,dx)
	outy = aux2-aux1

	aux1 = deriv_findiff(B,1,dx)       #z component of the curl
	aux2 = deriv_findiff(C,2,dy)
	outz = aux2-aux1
# now reverse order of results to go back to user coordinates
	return outz, outy, outx

