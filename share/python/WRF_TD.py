#Python program to calculate dewpoint temperature
#Inputs:  P, PB, QVAPOR
#Output: WRF_TD
#Let PR = P+PB
#and QV = MAX(QVAPOR,0)
#Where TDC = QV*PR/(0.622+QV)
# TDC = MAX(TDC,0.001)
#Formula is (243.5*log(TDC) - 440.8)/(19.48-log(TDC))
QV = numpy.maximum(QVAPOR,0.0)
TDC = QV*(P+PB)/(0.622+QV)
TDC = numpy.maximum(TDC,0.001)
WRF_TD = (243.5*log(TDC) - 440.8)/(19.48 - log(TDC))
