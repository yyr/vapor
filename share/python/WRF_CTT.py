#Python program to calculate Cloud-top temperature
#3D Inputs:  P, PB, T, QCLOUD, QICE
#2D Output: WRF_CTT
#Copied (and adapted) from RIP4 fortran code cttcalc.f
#constants:
#iice should be 1 if QSNOW and QICE is present
iice = (vapor.VariableExists(__TIMESTEP__,"QSNOW") and vapor.VariableExists(__TIMESTEP__,"QICE"))
grav=9.81
celkel=273.15 #celsius to kelvin difference
c = 2.0/7.0
abscoefi = 0.272
abscoef = 0.145
#calculate TK:
pf = P+PB
tmk = (T+300.0)*power(pf*.00001,c)
s = shape(P)	#size of the input array
ss = [s[1],s[2]] # shape of 2d arrays
#initialize with value at bottom:
WRF_CTT = tmk[0,:,:]-celkel
opdepthd = zeros(ss,float32) #next value
opdepthu = zeros(ss,float32) #previous value
#Sweep array from top to bottom, accumulating optical depth
if (iice == 1):
	for k in range(s[0]-2,-1,-1): #start at top-1, end at bottom (k=0) accumulate opacity
		dp =100.*(pf[k,:,:]-pf[k+1,:,:]) 
		opdepthd=opdepthu+(abscoef*QCLOUD[k,:,:]+abscoefi*QICE[k,:,:])*dp/grav
		logArray= logical_and(opdepthd > 1.0,opdepthu <= 1.0)
		#identify level when opac=1 is reached
		WRF_CTT = where(logArray,tmk[k,:,:]-celkel,WRF_CTT[:,:])
		opdepthu = opdepthd
else:
	for k in range(s[0]-2,-1,-1): #start at top-1, end at bottom (k=0) accumulate opacity
		dp =100.*(pf[k,:,:]-pf[k+1,:,:]) 
		logArray1 = tmk[k,:,:]<celkel
		opdepthd= where(logArray1,opdepthu+abscoefi*QCLOUD[k,:,:]*dp/grav,opdepthu+abscoef*QCLOUD[k,:,:]*dp/grav)
		logArray= logical_and(opdepthd > 1.0,opdepthu <= 1.0)
		#identify level when opac=1 is reached
		WRF_CTT = where(logArray,tmk[k,:,:]-celkel,WRF_CTT[:,:])
		opdepthu = opdepthd