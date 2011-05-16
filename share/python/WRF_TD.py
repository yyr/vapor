#Python program to calculate dewpoint temperature
#3D Inputs:  P, PB, QVAPOR
#3D Output: WRF_TD
#Let PR = 0.1*(P+PB) (pressure in hPa)
#and QV = MAX(QVAPOR,0)
#Where TDC = QV*PR/(0.622+QV)
# TDC = MAX(TDC,0.001)
#Formula is (243.5*log(TDC) - 440.8)/(19.48-log(TDC))
QV = maximum(QVAPOR,0.0)
TDC = 0.01*QV*(P+PB)/(0.622+QV)
TDC = numpy.maximum(TDC,0.001)
WRF_TD =(243.5*log(TDC) - 440.8)/(19.48 - log(TDC))