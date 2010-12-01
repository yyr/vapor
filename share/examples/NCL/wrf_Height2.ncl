
;   Example script to produce plots for a WRF real-data run,
;   with the ARW coordinate dynamics option.
;   Interpolating to specified height levels
;   In this example we obtain all the fields at once and interpolate to all height levels at once

load "$NCARG_ROOT/lib/ncarg/nclscripts/csm/gsn_code.ncl"
load "$NCARG_ROOT/lib/ncarg/nclscripts/wrf/WRFUserARW.ncl"

begin
;
; The WRF ARW input file.  
; This needs to have a ".nc" appended, so just do it.
  a = addfile("./wrfout_d01_2000-01-24_12:00:00.nc","r")

; We generate plots, but what kind do we prefer?
  type = "x11"
; type = "pdf"
; type = "ps"
; type = "ncgm"
  wks = gsn_open_wks(type,"plt_HeightLevel2")


; Set some basic resources
  res = True
  res@MainTitle = "REAL-TIME WRF"
  res@Footer = False

  pltres = True
  mpres = True
  mpres@mpGeophysicalLineColor = "Black"
  mpres@mpNationalLineColor    = "Black"
  mpres@mpUSStateLineColor     = "Black"
  mpres@mpGridLineColor        = "Black"
  mpres@mpLimbLineColor        = "Black"
  mpres@mpPerimLineColor       = "Black"


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; What times and how many time steps are in the data set?
  times  = wrf_user_list_times(a)  ; get times in the file
  ntimes = dimsizes(times)         ; number of times in the file

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; First get the variables we will need        

    tc = wrf_user_getvar(a,"tc",-1)        ; T in C
    u  = wrf_user_getvar(a,"ua",-1)        ; u averaged to mass points
    v  = wrf_user_getvar(a,"va",-1)        ; v averaged to mass points
    p  = wrf_user_getvar(a, "pressure",-1) ; pressure is our vertical coordinate
    z  = wrf_user_getvar(a, "z",-1)        ; grid point height
    rh = wrf_user_getvar(a,"rh",-1)        ; relative humidity

; The specific height levels that we want the data interpolated to.
; And interpolate to these levels
  height_levels = (/ 250., 2000./)            ; height levels to plot - in meter
  nlevels       = dimsizes(height_levels)     ; number of height levels

  p_plane  = wrf_user_intrp3d( p,z,"h",height_levels,0.,False)
  tc_plane = wrf_user_intrp3d(tc,z,"h",height_levels,0.,False)
  rh_plane = wrf_user_intrp3d(rh,z,"h",height_levels,0.,False)
  u_plane  = wrf_user_intrp3d( u,z,"h",height_levels,0.,False)
  v_plane  = wrf_user_intrp3d( v,z,"h",height_levels,0.,False)

  u_plane = u_plane*1.94386     ; kts
  v_plane = v_plane*1.94386     ; kts
    u_plane@units = "kts"
    v_plane@units = "kts"

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

  do it = 0,ntimes-1,2             ; TIME LOOP

    print("Working on time: " + times(it) )
    res@TimeLabel = times(it)   ; Set Valid time to use on plots


    do level = 0,nlevels-1                 ; LOOP OVER LEVELS

      height = height_levels(level) 

    ; Add some level into to the plot
      res@PlotLevelID = .001*height + " km"


    ; Plotting options for T                
      opts = res                          
      opts@cnLineColor = "Red"
      opts@ContourParameters = (/ 5.0 /)
      opts@cnInfoLabelOrthogonalPosF = 0.07  ; offset second label information
      opts@gsnContourLineThicknessesScale = 2.0
      contour_tc = wrf_contour(a,wks,tc_plane(it,level,:,:),opts)
      delete(opts)


    ; Plotting options for Pressure
      opts = res                          
      opts@cnLineColor = "Blue"
      opts@gsnContourLineThicknessesScale = 3.0
      contour_p = wrf_contour(a,wks,p_plane(it,level,:,:),opts)
      delete(opts)


    ; Plotting options for RH                
      opts = res                          
      opts@cnFillOn = True  
      opts@ContourParameters = (/ 10., 90., 10./)
      opts@cnFillColors = (/"White","White","White", \
                            "White","Chartreuse","Green",\
                            "Green3","Green4", \
                            "ForestGreen","PaleGreen4"/)
      contour_rh = wrf_contour(a,wks,rh_plane(it,level,:,:),opts)
      delete(opts)


    ; Plotting options for Wind Vectors                 
      opts = res          
      opts@FieldTitle = "Wind"   ; overwrite Field Title
      opts@NumVectors = 47       ; wind barb density
      vector = wrf_vector(a,wks,u_plane(it,level,:,:),v_plane(it,level,:,:),opts)
      delete(opts)


    ; MAKE PLOTS                                       
      plot = wrf_map_overlays(a,wks,(/contour_rh,contour_tc,contour_p, \
                                vector/),pltres,mpres)


    end do      ; END OF LEVEL LOOP


  end do        ; END OF TIME LOOP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

end
