# Copyright © (2011) Institut national de l'information
#                    géographique et forestière 
# 
# Géoportail SAV <geop_services@geoportail.fr>
# 
# This software is a computer program whose purpose is to publish geographic
# data using OGC WMS and WMTS protocol.
# 
# This software is governed by the CeCILL-C license under French law and
# abiding by the rules of distribution of free software.  You can  use, 
# modify and/ or redistribute the software under the terms of the CeCILL-C
# license as circulated by CEA, CNRS and INRIA at the following URL
# "http://www.cecill.info". 
# 
# As a counterpart to the access to the source code and  rights to copy,
# modify and redistribute granted by the license, users are provided only
# with a limited warranty  and the software's author,  the holder of the
# economic rights,  and the successive licensors  have only  limited
# liability. 
# 
# In this respect, the user's attention is drawn to the risks associated
# with loading,  using,  modifying and/or developing or reproducing the
# software by the user in light of its specific status of free software,
# that may mean  that it is complicated to manipulate,  and  that  also
# therefore means  that it is reserved for developers  and  experienced
# professionals having in-depth computer knowledge. Users are therefore
# encouraged to load and test the software's suitability as regards their
# requirements in conditions enabling the security of their systems and/or 
# data to be ensured and,  more generally, to use and operate it in the 
# same conditions as regards security. 
# 
# The fact that you are presently reading this means that you have had
# 
# knowledge of the CeCILL-C license and that you accept its terms.

################################################################################

=begin nd
File: ProxyGDAL.pm

Class: COMMON::ProxyGDAL

(see COMMON_ProxyGDAL.png)

Proxy to use different versions of GDAL transparently

Using:
    (start code)
    use COMMON::ProxyGDAL;
    my $geom = COMMON::ProxyGDAL::geometryFromString("WKT", "POLYGON(0 0,1 0,1 1,0 1,0 0)") ;
    my $sr = COMMON::ProxyGDAL::spatialReferenceFromSRS("EPSG:4326") ;
    (end code)
=cut

################################################################################

package COMMON::ProxyGDAL;

use strict;
use warnings;

use Data::Dumper;

use COMMON::Array;

require Exporter;
use AutoLoader qw(AUTOLOAD);

our @ISA = qw(Exporter);

our %EXPORT_TAGS = ( 'all' => [ qw() ] );
our @EXPORT_OK   = ( @{$EXPORT_TAGS{'all'}} );
our @EXPORT      = qw();

use Geo::GDAL;
use Geo::OSR;
use Geo::OGR;

use Log::Log4perl qw(:easy);

################################################################################
# Globale

my $gdalVersion = Geo::GDAL::VersionInfo();

################################################################################
# Constantes
use constant TRUE  => 1;
use constant FALSE => 0;

my @FORMATS = ("WKT", "BBOX", "GML", "GeoJSON");

####################################################################################################
#                              Group: Geometry constructors                                        #
####################################################################################################

=begin nd
Function: geometryFromString

Return an OGR geometry from a WKT string.

Parameters (list):
    format - string - Geomtry format
    string - string - String geometry according format

Return :
    a <Geo::OGR::Geometry> object, undef if failure
=cut
sub geometryFromString {
    my $format = shift;
    my $string = shift;

    if (! defined COMMON::Array::isInArray($format, @FORMATS)) {
        ERROR("Geometry string format unknown $format");
        return undef;
    }

    if ($format eq "BBOX") {
        my ($xmin,$ymin,$xmax,$ymax) = split(/,/, $string);
        return geometryFromBbox($xmin,$ymin,$xmax,$ymax);
    }
    
    my $geom;

    if ($gdalVersion =~ /^2/) {
        #version 2.x
        eval { $geom = Geo::OGR::Geometry->new($format => $string); };
    } elsif ($gdalVersion =~ /^1/) {
        #version 1.x
        eval { $geom = Geo::OGR::Geometry->create($format => $string); };
    }

    if ($@) {
        ERROR(sprintf "String geometry is not valid : %s \n", $@ );
        return undef;
    }

    return $geom
}

=begin nd
Function: geometryFromFile

Return an OGR geometry from a file.

Parameters (list):
    file - string - File containing the geometry description

Return :
    a <Geo::OGR::Geometry> object, undef if failure
=cut
sub geometryFromFile {
    my $file = shift;

    if (! -f $file) {
        ERROR("Geometry file $file does not exist");
        return undef;
    }

    my $format;
    if ($file =~ m/\.json$/i) {
        $format = "GeoJSON";
    } elsif ($file =~ m/\.gml$/i) {
        $format = "GML";
    } else {
        $format = "WKT";
    }

    if (! open FILE, "<", $file ){
        ERROR(sprintf "Cannot open the file %s.",$file);
        return FALSE;
    }

    my $string = '';
    while( defined( my $line = <FILE> ) ) {
        $string .= $line;
    }
    close(FILE);
    
    return geometryFromString($format, $string);
}

=begin nd
Function: geometryFromBbox

Return the OGR Geometry from a bbox

Parameters (list):
    xmin - float
    ymin - float
    xmax - float
    ymax - float

Return (list):
    a <Geo::OGR::Geometry> object, undef if failure
=cut
sub geometryFromBbox {
    my ($xmin,$ymin,$xmax,$ymax) = @_;

    if ($xmax <= $xmin || $ymax <= $ymin) {
        ERROR("Coordinates are not logical for a bbox (max < min) : $xmin,$ymin $xmax,$ymax");
        return undef ;
    }

    return geometryFromString("WKT", sprintf "POLYGON((%s %s,%s %s,%s %s,%s %s,%s %s))",
        $xmin,$ymin,
        $xmin,$ymax,
        $xmax,$ymax,
        $xmax,$ymin,
        $xmin,$ymin
    );
}


####################################################################################################
#                               Group: Geometry tools                                              #
####################################################################################################


=begin nd
Function: getBbox

Return the bbox from a OGR geometry object

Parameters (list):
    geom - <Geo::OGR::Geometry> - OGR geometry object

Return (list):
    (xmin,ymin,xmax,ymax), xmin is undefined if failure
=cut
sub getBbox {
    my $geom = shift;

    my $bboxref = $geom->GetEnvelope();

    return ($bboxref->[0],$bboxref->[2],$bboxref->[1],$bboxref->[3]);
}

=begin nd
Function: getBboxes

Return the bboxes from a OGR geometry object. If geometry is a collection, we have one bbox per geometry in the collection.

Parameters (list):
    geom - <Geo::OGR::Geometry> - OGR geometry object

Return (Array reference of array references):
    Array reference of N (xmin,ymin,xmax,ymax), undef if failure
=cut
sub getBboxes {
    my $geom = shift;

    my $bboxes = [];

    my $count = $geom->GetGeometryCount();

    for (my $i = 0; $i < $count ; $i++) {
        my $part = $geom->GetGeometryRef($i);
        my $bboxpart = $part->GetEnvelope();

        push @{$bboxes}, [$bboxpart->[0],$bboxpart->[2],$bboxpart->[1],$bboxpart->[3]];
    }
    return $bboxes;
}

=begin nd
Function: getConvertedGeometry

Parameters (list):
    geom - <Geo::OGR::Geometry> - OGR geometry object
    ct - <Geo::OSR::CoordinateTransformation> - OGR geometry object

Return (list):
    a <Geo::OGR::Geometry> object, undefined if failure
=cut
sub getConvertedGeometry {
    my $geom = shift;
    my $ct = shift;

    my $convertExtent = $geom->Clone();
    if (defined $ct) {
        eval { $convertExtent->Transform($ct); };
        if ($@) { 
            ERROR(sprintf "Cannot convert geometry : %s", $@);
            return undef;
        }
    }

    return $convertExtent;
}

=begin nd
Function: isIntersected

Return TRUE if 2 geomtries intersect. They have to own the same spatial reference

Parameters (list):
    geom1 - <Geo::OGR::Geometry> - geometry 1
    geom2 - <Geo::OGR::Geometry> - geometry 2

Return (list):
    TRUE if intersect, FALSE otherwise
=cut
sub isIntersected {
    my $geom1 = shift;
    my $geom2 = shift;

    return $geom1->Intersect($geom2);
}


=begin nd
Function: getUnion

Return union of two geometries. They have to own the same spatial reference

Parameters (list):
    geom1 - <Geo::OGR::Geometry> - geometry 1
    geom2 - <Geo::OGR::Geometry> - geometry 2

Returns geometry union
=cut
sub getUnion {
    my $geom1 = shift;
    my $geom2 = shift;

    return $geom1->Union($geom2);
}

####################################################################################################
#                               Group: Export tools                                                #
####################################################################################################

=begin nd
Function: getWkt

Return the geometry as WKT

Parameters (list):
    geom - <Geo::OGR::Geometry> - OGR geometry object

Return (list):
    the WKT string
=cut
sub getWkt {
    my $geom = shift;

    return $geom->ExportToWkt();
}

=begin nd
Function: getJson

Return the geometry as JSON

Parameters (list):
    geom - <Geo::OGR::Geometry> - OGR geometry object

Return (list):
    the JSON string
=cut
sub getJson {
    my $geom = shift;

    return $geom->ExportToJson();
}

=begin nd
Function: getGml

Return the geometry as GML

Parameters (list):
    geom - <Geo::OGR::Geometry> - OGR geometry object

Return (list):
    the GML string
=cut
sub getGml {
    my $geom = shift;

    return $geom->ExportToGML();
}

=begin nd
Function: getKml

Return the geometry as KML

Parameters (list):
    geom - <Geo::OGR::Geometry> - OGR geometry object

Return (list):
    the KML string
=cut
sub getKml {
    my $geom = shift;

    return $geom->ExportToKML();
}

=begin nd
Function: getWkb

Return the geometry as hexadecimal WKB

Parameters (list):
    geom - <Geo::OGR::Geometry> - OGR geometry object

Return (list):
    the WKB string
=cut
sub getWkb {
    my $geom = shift;

    return $geom->AsHEXWKB();
}


=begin nd
Function: exportFile

Write geometry into a file. Detect format from extension.

Parameters (list):
    geom - <Geo::OGR::Geometry> - OGR geometry object
    path - string - Path to file to write

Return (list):
    TRUE if success, FALSE if failure
=cut
sub exportFile {
    my $geom = shift;
    my $path = shift;

    open(OUT, ">$path") or do {
        ERROR("Cannot open $path to write in it");
        return FALSE;
    };

    if ($path =~ m/\.kml$/) {
        print OUT getKml($geom);
    }
    elsif ($path =~ m/\.gml$/) {
        print OUT getGml($geom);
    }
    elsif ($path =~ m/\.json$/) {
        print OUT getJson($geom);
    }
    elsif ($path =~ m/\.wkb$/) {
        print OUT getWkb($geom);
    }
    else {
        print OUT getWkt($geom);
    }
    
    close(OUT);

    return TRUE;
}


####################################################################################################
#                               Group: Georeferenced images functions                              #
####################################################################################################


=begin nd
Function: getGeoreferencement

Return the georeferencement (bbox, resolution) from an image

Parameters (list):
    filepath - string - Path of the referenced image

Return (hash reference):
{
    dimensions => [width, height],
    resolutions => [xres, yres],
    bbox => [xmin, ymin, xmax, ymax]
}

    undefined if failure
=cut
sub getGeoreferencement {
    my $filepath = shift;

    my $dataset;
    eval { $dataset= Geo::GDAL::Open($filepath, 'ReadOnly'); };
    if ($@) {
        ERROR (sprintf "Can not open image ('%s') : '%s' !", $filepath, $@);
        return undef;
    }

    my $refgeo = $dataset->GetGeoTransform();
    if (! defined ($refgeo) || scalar (@$refgeo) != 6) {
        ERROR ("Can not found geometric parameters of image ('$filepath') !");
        return undef;
    }

    # forced formatting string !
    my ($xmin, $dx, $rx, $ymax, $ry, $ndy)= @$refgeo;

    my $res = {
        dimensions => [$dataset->{RasterXSize}, $dataset->{RasterYSize}],
        resolutions => [sprintf("%.12f", $dx), sprintf("%.12f", abs($ndy))],
        bbox => [
            sprintf("%.12f", $xmin),
            sprintf("%.12f", $ymax + $ndy*$dataset->{RasterYSize}),
            sprintf("%.12f", $xmin + $dx*$dataset->{RasterXSize}),
            sprintf("%.12f", $ymax)
        ],
    };
    
    return $res;
}


=begin nd
Function: getPixel

Return the pixel informations from an image

Parameters (list):
    filepath - string - Path of the image

Return (list):
    a <COMMON::Pixel> object, undefined if failure
=cut
sub getPixel {
    my $filepath = shift;

    my $dataset;
    eval { $dataset= Geo::GDAL::Open($filepath, 'ReadOnly'); };
    if ($@) {
        ERROR (sprintf "Can not open image ('%s') : '%s' !", $filepath, $@);
        return undef;
    }

    my $i = 0;

    my $DataType       = undef;
    my $Band           = undef;
    my @Interpretation;

    foreach my $objBand ($dataset->Bands()) {

        push @Interpretation, lc $objBand->ColorInterpretation();

        if (!defined $DataType) {
            $DataType = lc $objBand->DataType();
        } else {
            if (! (lc $objBand->DataType() eq $DataType)) {
                ERROR (sprintf "DataType is not the same (%s and %s) for all band in this image !", lc $objBand->DataType(), $DataType);
                return undef;
            }
        }
        
        $i++;
    }

    $Band = $i;

    my $bitspersample = undef;
    my $photometric = undef;
    my $sampleformat = undef;
    my $samplesperpixel = undef;

    if ($DataType eq "byte") {
        $bitspersample = 8;
        $sampleformat  = "uint";
    }
    else {
        ($sampleformat, $bitspersample) = ($DataType =~ /(\w+)(\d{2})/);
    }

    if ($Band == 3) {
        foreach (@Interpretation) {
            last if ($_ !~ m/(red|green|blue)band/);
        }
        $photometric     = "rgb";
        $samplesperpixel = 3;
    }

    if ($Band == 4) {
        foreach (@Interpretation) {
            last if ($_ !~ m/(red|green|blue|alpha)band/);
        }
        $photometric     = "rgb";
        $samplesperpixel = 4;
    }

    if ($Band == 1) {
        if ($Interpretation[0] eq "grayindex") {
            $photometric     = "gray";
            $samplesperpixel = 1;
        }
        if ($Interpretation[0] eq "paletteindex") {
            $photometric     = "gray";
            $samplesperpixel = 1;
            $bitspersample = 1;
        }
    }

    if (! (defined $bitspersample && defined $photometric && defined $sampleformat && defined $samplesperpixel)) {
        ERROR ("The format of this image ('$filepath') is not handled by be4 !");
        return undef;
    }
    
    return COMMON::Pixel->new({
        bitspersample => $bitspersample,
        photometric => $photometric,
        sampleformat => $sampleformat,
        samplesperpixel => $samplesperpixel
    });
}

####################################################################################################
#                               Group: Spatial Reference functions                                 #
####################################################################################################

=begin nd
Function: spatialReferenceFromSRS

Return an OSR spatial reference from a SRS string.

Parameters (list):
    srs - string - SRS string, whose authority is known by proj4

Return :
    a <Geo::OSR::SpatialReference> object, undef if failure
=cut
sub spatialReferenceFromSRS {
    my $srs = shift;

    my $sr;
    if ($gdalVersion =~ /^2/) {
        #version 2.x    
        eval { $sr = Geo::OSR::SpatialReference->new(Proj4 => '+init='.$srs.' +wktext'); };
        if ($@) {
            eval { $sr = Geo::OSR::SpatialReference->new(Proj4 => '+init='.lc($srs).' +wktext'); };
            if ($@) {
                ERROR("$@");
                ERROR (sprintf "Impossible to initialize the spatial coordinate system (%s) to know if coordinates have to be reversed !\n",$srs);
                return undef;
            }
        }
    } elsif ($gdalVersion =~ /^1/) {
        #version 1.x
        # Have coodinates to be reversed ?
        $sr= new Geo::OSR::SpatialReference;
        eval { $sr->ImportFromProj4('+init='.$srs.' +wktext'); };
        if ($@) {
            eval { $sr->ImportFromProj4('+init='.lc($srs).' +wktext'); };
            if ($@) {
                ERROR("$@");
                ERROR (sprintf "Impossible to initialize the spatial coordinate system (%s) to know if coordinates have to be reversed !\n",$srs);
                return undef;
            }
        }
    }

    return $sr;
}

=begin nd
Function: isGeographic

Parameters (list):
    srs - a <Geo::OSR::SpatialReference> - SRS to test if geographic

=cut
sub isGeographic {
    my $sr = shift;
    return $sr->IsGeographic();
}


=begin nd
Function: coordinateTransformationFromSpatialReference

Return an OSR coordinate transformation from a source and a destination spatial references.

Parameters (list):
    src - string - Source spatial reference, whose authority is known by proj4
    dst - string - Destination spatial reference, whose authority is known by proj4

Return :
    a <Geo::OSR::CoordinateTransformation> object, undef if failure
=cut
sub coordinateTransformationFromSpatialReference {
    my $src = shift;
    my $dst = shift;
    
    my $srcSR = COMMON::ProxyGDAL::spatialReferenceFromSRS($src);
    if (! defined $srcSR) {
        ERROR(sprintf "Impossible to initialize the initial spatial coordinate system (%s) !", $src);
        return undef;
    }

    my $dstSR = COMMON::ProxyGDAL::spatialReferenceFromSRS($dst);
    if (! defined $srcSR) {
        ERROR(sprintf "Impossible to initialize the destination spatial coordinate system (%s) !", $dst);
        return undef;
    }

    my $ct = Geo::OSR::CoordinateTransformation->new($srcSR, $dstSR);

    return $ct;
}

=begin nd
Function: transformPoint

Convert a XY point, thanks to provided coordinate tranformation

Parameters (list):
    x - float - x of point to converted
    y - float - y of point to converted
    ct - <Geo::OSR::CoordinateTransformation> - Coordinate transformation to use to reproject the point

Return (list) :
    (reprojX, reprojY), x is undef if failure
=cut
sub transformPoint {
    my $x = shift;
    my $y = shift;
    my $ct = shift;

    my $p = 0;
    eval { $p = $ct->TransformPoint($x,$y); };
    if ($@) {
        ERROR($@);
        ERROR("Cannot tranform point %s, %s", $x, $y);
        return (undef, undef);
    }

    return ($p->[0], $p->[1]);
}

=begin nd
Function: convertBBox

Not just convert corners, but 7 points on each side, to determine reprojected bbox. Use OSR library.

Parameters (list):
    ct - <Geo::OSR::CoordinateTransformation> - To convert bbox. Can be undefined (no reprojection).

Returns the converted (according to the given CoordinateTransformation) bbox as a double array (xMin, yMin, xMax, yMax), (0,0,0,0) if error.
=cut
sub convertBBox {
    my $ct = shift;
    my @bbox = @_;

    if (! defined($ct)){
        $bbox[0] = Math::BigFloat->new($bbox[0]);
        $bbox[1] = Math::BigFloat->new($bbox[1]);
        $bbox[2] = Math::BigFloat->new($bbox[2]);
        $bbox[3] = Math::BigFloat->new($bbox[3]);
        return @bbox;
    }
    
    # my ($xmin,$ymin,$xmax,$ymax);
    my $step = 10;
    my $dx = ($bbox[2] - $bbox[0]) / (1.0 * $step);
    my $dy = ($bbox[3] - $bbox[1]) / (1.0 * $step);
    my @polygon = ();
    for my $i (@{[0..$step-1]}) { # bas du polygone, de gauche à droite
        push @polygon, [$bbox[0]+$i*$dx, $bbox[1]]; 
    }
    for my $i (@{[0..$step-1]}) { # côté droit du polygone de haut en bas
        push @polygon, [$bbox[2], $bbox[1]+$i*$dy]; 
    }
    for my $i (@{[0..$step-1]}) { # haut du polygone, de droite à gauche
        push @polygon, [$bbox[2]-$i*$dx, $bbox[3]];
    }
    for my $i (@{[0..$step-1]}) { # côté gauche du polygon de haut en bas
        push @polygon, [$bbox[0], $bbox[3]-$i*$dy];
    }

    my ($xmin_reproj, $ymin_reproj, $xmax_reproj, $ymax_reproj);
    for my $i (@{[0..$#polygon]}) {
        # FIXME: il faut absoluement tester les erreurs ici:
        #        les transformations WGS84G vers PM ne sont pas possible au dela de 85.05°.

        my ($tx, $ty) = COMMON::ProxyGDAL::transformPoint($polygon[$i][0], $polygon[$i][1], $ct);
        if (! defined $tx) {
            ERROR(sprintf "Impossible to transform point (%s,%s). Probably limits are reached !",$polygon[$i][0],$polygon[$i][1]);
            return (0,0,0,0);
        }

        if ($i == 0) {
            $xmin_reproj = $xmax_reproj = $tx;
            $ymin_reproj = $ymax_reproj = $ty;
        } else {
            $xmin_reproj = $tx if $tx < $xmin_reproj;
            $ymin_reproj = $ty if $ty < $ymin_reproj;
            $xmax_reproj = $tx if $tx > $xmax_reproj;
            $ymax_reproj = $ty if $ty > $ymax_reproj;
        }
    }

    my $margeX = ($xmax_reproj - $xmin_reproj) * 0.02; # FIXME: la taille de la marge est arbitraire!!
    my $margeY = ($ymax_reproj - $ymax_reproj) * 0.02; # FIXME: la taille de la marge est arbitraire!!

    $bbox[0] = Math::BigFloat->new($xmin_reproj - $margeX);
    $bbox[1] = Math::BigFloat->new($ymin_reproj - $margeY);
    $bbox[2] = Math::BigFloat->new($xmax_reproj + $margeX);
    $bbox[3] = Math::BigFloat->new($ymax_reproj + $margeY);

    return @bbox;
}

1;
__END__
