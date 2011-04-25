#Python program to calculate radar reflectivity using WRF variables
#Copied from NCL/Fortran source code wrf_user_dbz.f
#Based on work by Mark Stoellinga, U. of Washington
#Inputs:  P, PB, QRAIN, QGRAUP, QSNOW, T, QVAPOR
#Outputs: WRF_DBZ (3d) and WRF_DBZ_MAX (2d)
#WRF_DBZ_MAX is a 2D variable, the maximum over vertical columns of WRF_DBZ
#parameters ivarint and iliqskin defined as in original code, either 0 or 1.
#If iliqskin=1, frozen particles above freezing are assumed to scatter as a liquid particle
#If ivarint=0, the intercept parameters are assumed constant with values of 
# 8*10^6, 2*10^7, and 4*10^6, for rain, snow, and graupel respectively.
#If ivarint=1, intercept parameters are used as calculated in Thompson, Rasmussen, and Manning,
#2004 Monthly Weather Review, Vol 132, No. 2, pp.519-542
# By default they are given value 0, but they can be changed by editing the following 2 lines:
iliqskin = 0
ivarint = 0
c = 2.0/7.0
R1=1.e-15
RON = 8.e6
RON2 = 1.e10
SON = 2.e7
GON = 5.37
RON_MIN = 8.e6
RON_QR0 = 0.0001
RON_DELQR0 = 0.25*RON_QR0
RON_CONST1R = (RON2-RON_MIN)*0.5
RON_CONST2R = (RON2+RON_MIN)*0.5
RN0_R = 8.e6
RN0_S = 2.e7
RN0_G = 4.e6
GAMMA_SEVEN =720.
RHOWAT = 1000.
RHO_R = RHOWAT
RHO_S = 100.
RHO_G = 400.
ALPHA = 0.224
CELKEL = 273.15
PI = 3.141592653589793
RD = 287.04
FACTOR_R = GAMMA_SEVEN*1.e18*power((1./(pi*RHO_R)),1.75)
FACTOR_S = GAMMA_SEVEN*1.e18*power((1./(pi*RHO_S)),1.75)*power((RHO_S/RHOWAT),2.*ALPHA)
FACTOR_G = GAMMA_SEVEN*1.e18*power((1./(pi*RHO_G)),1.75)*power((RHO_G/RHOWAT),2.*ALPHA)

#calculate Temp. in Kelvin
PRESS = P+PB
TK = (T+300.)*numpy.power(PRESS*.00001,c)

#Force Q arrays to be nonnegative:
QVAPOR = maximum(QVAPOR,0.0)
QSNOW = maximum(QSNOW,0.0)
QRAIN = maximum(QRAIN,0.0)
QGRAUP = 0.0

TestT = less(TK,CELKEL)
QSNOW = where(TestT,QRAIN, QSNOW)
QRAIN = where(TestT,0.0, QRAIN)

VIRTUAL_T = TK*(0.622 + QVAPOR)/(0.622*(1.+QVAPOR))
RHOAIR = PRESS/(RD*VIRTUAL_T)

FACTORB_S = FACTOR_S
FACTORB_G = FACTOR_G

if (iliqskin == 1):
	FACTORB_S = float32(where(TestT,FACTOR_S,FACTOR_S/ALPHA))
	FACTORB_G = float32(where(TestT,FACTOR_G, FACTOR_G/ALPHA))

if (ivarint == 1):
	TEMP_C = minimum(-.001, TK-CELKEL)
	SONV = minimum(2.e8,2.e6*exp(-.12*TEMP_C))
	TestG = less(R1, QGRAUP)
	GONV = where(TestG,2.38*power(PI*RHO_G/(RHOAIR*QGRAUP),0.92),GON)
	GONV = where(TestG,maximum(1.e4,minimum(GONV,GON)),GONV)
	RONV = where(greater(QRAIN,R1),RON_CONST1R*tanh((RON_QR0-QRAIN)/RON_DELQR0) + RON_CONST2R,RON2)
else:
	GONV = RN0_G
	RONV = RN0_R
	SONV = RN0_S

Z_E = FACTOR_R*power(RHOAIR*QRAIN,1.75)/power(RONV,0.75) 
Z_E = Z_E+FACTORB_S*power(RHOAIR*QSNOW,1.75)/power(SONV,0.75)
Z_E = Z_E+FACTORB_G*power(RHOAIR*QGRAUP,1.75)/power(GONV,0.75)
Z_E = maximum(Z_E, 0.001)
QGROUP=0
WRF_DBZ = 10.0*log10(Z_E)
WRF_DBZ_MAX = amax(WRF_DBZ,axis=0)