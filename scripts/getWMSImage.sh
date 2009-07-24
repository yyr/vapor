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

printUsage() {
      echo "Usage: " $0 "{optional parameters} minLon minLat maxLon maxLat"
      echo "Optional parameters:"
      echo "    -r xres yres"
      echo "          resolution of requested image; default is " ${xres}"x"${yres}
      echo "    -o imageFilename"
      echo "          name for the requested image file; default is named after requested image layer"
      echo "    -l layername"
      echo "          \"BMNG\" (BlueMarble, the default) or \"Landsat\";"
      echo ""
      echo "Expert-only parameters (see documentation):"
      echo "    -s URL"
      echo "          URL for WMS server"
      echo "    -l layerName"
      echo "          arbitrary image-layer name to fetch"
      echo "    -f format"
      echo "          image format; default is \"image/tiff\""
}

realValGT() {
    a=`bc <<EOF
      $1 > $2
EOF
`
    echo $a
}


if [ $# -lt 4 ]
then
    printUsage
    exit 1
fi

# parse command-line options...
while [ $# -gt 4 ]
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

  -s) host=$2
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

  *)
      printUsage
      exit 0
  esac
  shift
done

# remaining parameters form a possible bounding box?
for i in $1 $2 $3 $4
do
    if ! [[ $i =~ ${decLLRegEx} ]] ; then
        echo $i " is not a valid decimal lon/lat string"
        exit 1
    fi
done
minLon=$1
minLat=$2
maxLon=$3
maxLat=$4

# further test bounds for sanity...
if [ `realValGT -180 ${minLon}` -eq 1 ] || [ `realValGT ${maxLon} 180` -eq 1 ] || \
   [ `realValGT ${minLon} ${maxLon}` -eq 1 ]
then
    echo "Invalid bounding box:"
    echo "  longitudes must range from -180 to 180, with minLon < maxLon"
    exit 1
fi

if [ `realValGT -90 ${minLat}` -eq 1 ] || [ `realValGT ${maxLat} 90` -eq 1 ] || \
   [ `realValGT ${minLat} ${maxLat}` -eq 1 ]
then
    echo "Invalid bounding box:"
    echo "  latitudes must range from -90 to 90, with minLat < maxLat"
    exit 1
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

if [ "${imageFormat}" != "image/tiff" ] ; then
    echo "convert ${tempFile} ${tempFile}2"
    convert ${tempFile} ${tempFile}2.tiff
    rm ${tempFile}
    mv ${tempFile}2.tiff ${tempFile}
fi
echo "tiff2geotiff -4 +proj=longlat -n " ${minLon} ${minLat} ${maxLon} ${maxLat} ${tempFile} ${imageFile}
tiff2geotiff -4 "+proj=longlat" -n "${minLon} ${minLat} ${maxLon} ${maxLat}" ${tempFile} ${imageFile}

if [ -f ${imageFile} ] ; then
    echo "Image filename is: " ${imageFile}
else
    echo "Image fetch seems to have failed."
fi

rm -f ${tempFile}
