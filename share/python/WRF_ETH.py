#Python program to calculate equivalent potential temperature using WRF variables
#Copied from NCL/Fortran source code DEQTHECALC in eqthecalc.f
#3D Inputs:  P, PB, T, QVAPOR
#3D Output: WRF_ETH
c = 2.0/7.0
EPS = 0.622
GAMMA = 287.04/1004.0
GAMMAMD = 0.608 -0.887
TLCLC1 = 2840.0
TLCLC2 = 3.5
TLCLC3 = 4.805
TLCLC4 = 55.0
THTECON1 = 3376.0
THTECON2 = 2.54
THTECON3 = 0.81
TH = T+300.0
#calculate Temp. in Kelvin
PRESS = 0.01*(P+PB)
TK = TH*power(PRESS*.001,c)
Q = maximum(QVAPOR, 1.e-15)
E = Q*PRESS/(EPS+Q)
TLCL = TLCLC4+ TLCLC1/(numpy.log(numpy.power(TK,TLCLC2)/E)-TLCLC3)
EXPNT = (THTECON1/TLCL - THTECON2)*Q*(1.0+THTECON3*Q)
WRF_ETH = TK*power(1000.0/PRESS,GAMMA*(1.0+GAMMAMD*Q))*exp(EXPNT)