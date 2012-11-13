#ifndef GeoTileMercator_h_
#define GeoTileMercator_h_
#ifdef _WINDOWS
#pragma warning(disable : 4251)
#endif
#include "GeoTile.h"
#include <vapor/common.h>
#ifdef	DEAD
//! \class GeoTileMercator
//! 
//! This class derives the GetTile base class to provide a tiled world
//! mapping system using the Mercator projection.
//! 
//! \author John Clyne
//! \version $Revision$
//! \date    $Date$
//!
#endif
namespace VAPoR {
class PARAMS_API GeoTileMercator : public GeoTile {
public:

 //! GeoTileMercator class constructor
 //!
 //! \param[in] tile_height Height of an image tile in pixels. The tile
 //! width must the same as the tile height
 //! \param[in] pixelsize The size of a pixel in bytes
 //!
 //! 
 GeoTileMercator(size_t tile_height, size_t pixelsize) : 
	GeoTile(
		tile_height, tile_height, pixelsize, 
		-180.0, -85.05112878, 180.0, 85.05112878
	) {}

 

 //! copydoc GeoTile::LatLongToPixelXY()
 //
 virtual void LatLongToPixelXY(
    double lon, double lat, int lod, size_t &pixelX, size_t &pixelY
 ) const;

 //! copydoc GeoTile::PixelXYToLatLon()
 //
 virtual void PixelXYToLatLon(
    size_t pixelX, size_t pixelY, int lod, double &lon, double &lat
 ) const;

};
};
#endif
