;	$Id$
;
;	This example shows how to extact various bits of metadata information
;	from a from a VDF data collection.
;
;	Note: in order to run this example you must have first created
;	a VDF data collection by running either the WriteVDF.pro
;	or WriteTimeVaryVDF.pro example programs


;
;	Create a VDF metadata object from an existing metadata file. The
;	metadata file must already exist on disk, having been created from
;	one of the example programs that generates a .vdf file.
;
vdffile = '/tmp/test.vdf'
mfd = vdf_create(vdffile)


print, "Block size = ", vdf_getblocksize(mfd)

print, "Dimension = ", vdf_getdimension(mfd)

print, "Num Filter Coefficients = ", vdf_getfiltercoef(mfd)

print, "Num Lifting Coefficients = ", vdf_getliftingcoef(mfd)

print, "Num Wavelet Transforms = ", vdf_getnumtransforms(mfd)

print, "Grid Type = ", vdf_getgridtype(mfd)

print, "Coordinate Type = ", vdf_getcoordtype(mfd)

print, "Volume extents = ", vdf_getextents(mfd)

print, "Num Time Steps = ", vdf_getnumtimesteps(mfd)

print, "Variable Names = ", vdf_getvarnames(mfd)

print, "Global comment = ", vdf_getcomment(mfd)


timesteps = vdf_getnumtimesteps(mfd)
varnames = vdf_getvarnames(mfd)
nvarnames = n_elements(varnames)

for ts = 0, timesteps[0]-1 do begin
	print, "Time step ", ts, " metadata"
	print, "	User Time = ", vdf_gettusertime(mfd, ts)
	print, "	XCoords = ", vdf_gettxcoords(mfd, ts)
	print, "	YCoords = ", vdf_gettycoords(mfd, ts)
	print, "	ZCoords = ", vdf_gettzcoords(mfd, ts)
	print, "	Comment = ", vdf_gettcomment(mfd, ts)

	for v = 0, nvarnames-1 do begin
		print, "	Variable ", varnames[v], " metadata"
		print, "		Data Range = ", vdf_getvdrange(mfd, ts, varnames[v])
		print, "		Comment = ", vdf_getvcomment(mfd, ts, varnames[v])

	endfor
endfor


vdf_destroy, mfd

end
