#!/bin/bash
#

# First make sure we have the utilities we'll need...
fetchProg=""
nosave=`which curl 2>/dev/null`
if  [ $? -eq 0 ]; then
    fetchProg="curl -o"
fi
nosave=`which wget 2>/dev/null`
if  [ $? -eq 0 ]; then
    fetchProg="wget -O"
fi
if [ "$fetchProg" = "" ]; then
    echo "Could not find \"wget\" or \"curl\"; an image can't be retrieved."
    exit 1
fi

# need GDAL's gdal_translate program to convert to geotiff...
#nosave=`which gdal_translate 2>/dev/null`
#if ! [ $? -eq 0 ]; then
#    echo "Could not find \"gdal_translate\"; needed to convert image into a geotiff."
#    exit 1
#fi


# defaults
minLon=""
maxLon=""
minLat=""
maxLat=""
xres=1024
yres=1024
imageFile=""
imageFormat="image/tiff"
tempFile="tmpImage.tiff"
layer="bmng200406"
host="http://www.nasa.network.com/wms"
amp="&"
decLLRegEx=^[\-+]?[0-9]+[\.]?[0-9]*$

# parse command-line options...
while [ $# -gt 0 ]
do
  case $1 in

  -e) for i in $2 $3 $4 $5
      do
          if ! [[ $i =~ ${decLLRegEx} ]] ; then
              echo $i " is not a valid decimal lon/lat string"
              exit 1
          fi
      done
      minLon=$2
      minLat=$3
      maxLon=$4
      maxLat=$5
      shift; shift; shift; shift
      ;;

  -r) if ! ( [[ $2 =~ ^[0-9]+$ ]] && [[ $3 =~ ^[0-9]+$ ]]) ; then
          echo "bad resolution values given: " $2 " x " $3
          exit 1
      fi
      xres=$2
      yres=$3
      shift; shift
      ;;

  -h) host=$2
      shift
      ;;

  -o) imageFile=$2
      shift
      ;;

  -l) layer=$2
      shift
      ;;

  -f) imageFormat=$2
      shift
      ;;

  --help)
      echo "Usage: " $0 " {optional parameters}"
      echo "Optional parameters:"
      echo "    -r xres yres"
      echo "          resolution of requested image; default is " ${xres}"x"${yres}
      echo "    -e minLon minlat maxLon maxLat"
      echo "          bounding box of image;"
      echo "          user will be interactively prompted for these values if not specified on the commandline" 
      echo "    -o imageFilename"
      echo "          name for the requested image file; default is named after requested image layer"
      echo "    -h URL"
      echo "          expert usage: URL for WMS server"
      echo "    -l layername"
      echo "          with default WMS server, specify \"BMNG\" (default) or \"Landsat\";"
      echo "          expert usage: image-layer name to fetch"
      echo "    -f format"
      echo "          expert usage: image format; default is \"tiff\""
      echo "    --help"
      echo "          this text"
      exit 0
  esac
  shift
done

if [ "${minLon}" = "" ] || [ "${minLat}" = "" ] || [ "${maxLon}" = "" ] || [ "${maxLat}" = "" ] ; then
    minLon=""; maxLon=""; minLat=""; maxLat=""
    echo "Enter bounds for image:"
    until [ "${minLon}" != "" ]
    do
        echo -n "  minimum longitude: "
        read minLon
        if ! [[ ${minLon} =~ ${decLLRegEx} ]] ; then
            echo "invalid...enter as (signed) decimal value"
            minLon=""
        fi
    done
    until [ "${maxLon}" != "" ]
    do
        echo -n "  maximum longitude: "
        read maxLon
        if ! [[ ${maxLon} =~ ${decLLRegEx} ]] ; then
            echo "invalid...enter as (signed) decimal value"  
            maxLon="" 
        fi
    done
    until [ "${minLat}" != "" ]
    do
        echo -n "  minimum latitude: "
        read minLat
        if ! [[ ${minLat} =~ ${decLLRegEx} ]] ; then
            echo "invalid...enter as (signed) decimal value"
            minLat=""
        fi
    done
    until [ "${maxLat}" != "" ]
    do
        echo -n "  maximum latitude: "
        read maxLat
        if ! [[ ${maxLat} =~ ${decLLRegEx} ]] ; then
            echo "invalid...enter as (signed) decimal value"
            maxLat=""
        fi
    done
fi

# We recognize two special layer names from the NASA WMS server...
wmsLayer=${layer}
if [ "${layer}" = "BMNG" ] ; then
    wmsLayer="bmng200406"
elif [ "${layer}" = "Landsat" ] ; then
    wmsLayer="esat"
fi

# If no image filename specified, name it after the layer...
if [ "${imageFile}" = "" ] ; then
    imageFile=${layer}.tiff
fi          

echo "Extent:           " $minLon","$minLat " (LL)  " $maxLon","$maxLat "(UR)"
echo "Image resolution: " $xres "X" $yres
echo "Image filename:   " $imageFile
echo "Image layer:      " $layer
echo "WMS URL:          " $host

# compose the URL
url1=${host}"?""request=GetMap"${amp}"service=wms"${amp}"version=1.3"${amp}"layers="${wmsLayer}
url2="styles=default"${amp}"bbox="${minLon}","${minLat}","${maxLon}","${maxLat}
url3="format="${imageFormat}${amp}"height="${yres}${amp}"width="${xres}${amp}"srs=epsg:4326"
url=${url1}${amp}${url2}${amp}${url3}

#echo $url
#echo ${fetchProg}
${fetchProg} ${tempFile} ${url} 

if  ( grep -s "ServiceException" ${tempFile} ); then
    echo "Received WMS ServiceException:"
    cat ${tempFile}
    exit 1
fi

if [ "${imageFormat}" = "image/tiff" ] ; then
    tiff2geotiff -4 "+proj=longlat" -n "${minLon} ${maxLat} ${maxLon} ${minLat}" ${tempFile} ${imageFile}
else
    echo "gdal_translate -of Gtiff -a_srs EPSG:4326 -a_ullr " ${minLon} ${maxLat} ${maxLon} ${minLat} ${tempFile} ${imageFile}
    gdal_translate -of Gtiff -a_srs EPSG:4326 -a_ullr ${minLon} ${maxLat} ${maxLon} ${minLat} ${tempFile} ${imageFile}
fi

if [ -f ${imageFile} ] ; then
    echo "Image filename is: " ${imageFile}
else
    echo "Image fetch seems to have failed."
fi

rm -f ${tempFile}
