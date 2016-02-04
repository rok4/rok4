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

Class: BE4::ProxyGDAL

Proxy to use different versions of GDAL transparently

Using:
    (start code)
    use BE4::ProxyGDAL;
    my $geom = BE4::ProxyGDAL::geometryFromWKT("POLYGON(0 0,1 0,1 1,0 1,0 0)") ;
    my $sr = BE4::ProxyGDAL::spatialReferenceFromSRS("EPSG:4326") ;
    (end code)
=cut

################################################################################

package BE4::ProxyGDAL;

use strict;
use warnings;

use Data::Dumper;

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

####################################################################################################
#                               Group: Geometry functions                                          #
####################################################################################################

=begin nd
Function: geometryFromWKT

Return an OGR geometry from a WKT string.

Parameters (list):
    wkt - string - WKT geometry

Return :
    a <Geo::OGR::Geometry> object, undef if failure
=cut
sub geometryFromWKT {
    my $wkt = shift;
    
    my $geom;

    if ($gdalVersion =~ /^2/) {
        #version 2.x
        eval { $geom = Geo::OGR::Geometry->new(WKT=>$wkt); };
    } elsif ($gdalVersion =~ /^1/) {
        #version 1.x
        eval { $geom = Geo::OGR::Geometry->create(WKT=>$wkt); };
    }

    if ($@) {
        ERROR(sprintf "WKT geometry is not valid : %s \n", $@ );
        return undef;
    }

    return $geom
}


=begin nd
Function: getBbox

Return the bbox from a OGR geometry object

Parameters (list):
    geom - <Geo::OGR::Geometry> - OGR geometry object

Return (list):
    ($xmin,$xmax,$ymin,$ymax), xmin is undefined if failure
=cut
sub getBbox {
    my $geom = shift;

    my $bboxref = $geom->GetEnvelope();

    return ($bboxref->[0],$bboxref->[1],$bboxref->[2],$bboxref->[3]);
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
            INFO("top");
            eval { $sr = Geo::OSR::SpatialReference->new(Proj4 => '+init='.lc($srs).' +wktext'); };
            if ($@) {
                ERROR("$@");
                ERROR (sprintf "Impossible to initialize the final spatial coordinate system (%s) to know if coordinates have to be reversed !\n",$srs);
                return undef;
            }
        }
    } elsif ($gdalVersion =~ /^1/) {
        #version 1.x
        # Have coodinates to be reversed ?
        $sr= new Geo::OSR::SpatialReference;
        eval { $sr->ImportFromProj4('+init='.$srs.' +wktext'); };
        if ($@) {
            INFO("top");
            eval { $sr->ImportFromProj4('+init='.lc($srs).' +wktext'); };
            if ($@) {
                ERROR("$@");
                ERROR (sprintf "Impossible to initialize the final spatial coordinate system (%s) to know if coordinates have to be reversed !\n",$srs);
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
    
    my $srcSR = BE4::ProxyGDAL::spatialReferenceFromSRS($src);
    if (! defined $srcSR) {
        ERROR(sprintf "Impossible to initialize the initial spatial coordinate system (%s) !", $src);
        return undef;
    }

    my $dstSR = BE4::ProxyGDAL::spatialReferenceFromSRS($dst);
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



1;
__END__
