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
mfd = vdf_mcreate(vdffile)


print, "Block size = ", vdf_mgetblocksize(mfd)

print, "Dimension = ", vdf_mgetdimension(mfd)

print, "Num Filter Coefficients = ", vdf_mgetfiltercoef(mfd)

print, "Num Lifting Coefficients = ", vdf_mgetliftingcoef(mfd)

print, "Num Wavelet Transforms = ", vdf_mgetnumtransforms(mfd)

print, "Grid Type = ", vdf_mgetgridtype(mfd)

print, "Coordinate Type = ", vdf_mgetcoordtype(mfd)

print, "Volume extents = ", vdf_mgetextents(mfd)

print, "Num Time Steps = ", vdf_mgetnumtimesteps(mfd)

print, "Variable Names = ", vdf_mgetvarnames(mfd)

print, "Global comment = ", vdf_mgetcomment(mfd)


timesteps = vdf_mgetnumtimesteps(mfd)
varnames = vdf_mgetvarnames(mfd)
nvarnames = n_elements(varnames)

for ts = 0, timesteps[0]-1 do begin
	print, "Time step ", ts, " metadata"
	print, "	User Time = ", vdf_mgettusertime(mfd, ts)
	print, "	XCoords = ", vdf_mgettxcoords(mfd, ts)
	print, "	YCoords = ", vdf_mgettycoords(mfd, ts)
	print, "	ZCoords = ", vdf_mgettzcoords(mfd, ts)
	print, "	Comment = ", vdf_mgettcomment(mfd, ts)

	for v = 0, nvarnames-1 do begin
		print, "	Variable ", varnames[v], " metadata"
		print, "		Data Range = ", vdf_mgetvdrange(mfd, ts, varnames[v])
		print, "		Comment = ", vdf_mgetvcomment(mfd, ts, varnames[v])

	endfor
endfor


vdf_mdestroy, mfd

end
