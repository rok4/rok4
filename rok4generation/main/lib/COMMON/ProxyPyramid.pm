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
File: ProxyPyramid.pm

Class: COMMON::ProxyPyramid

(see ROK4GENERATION/libperlauto/COMMON_ProxyPyramid.png)

Proxy to load a pyramid, whatever the type (raster, raster on demand or vector)

Using:
    (start code)
    use COMMON::ProxyPyramid;
    my $pyramid = COMMON::ProxyPyramid::load("/home/ign/pyramid.pyr") ;
    (end code)
=cut

################################################################################

package COMMON::ProxyPyramid;

use strict;
use warnings;

use Data::Dumper;

use COMMON::PyramidRaster;
use COMMON::PyramidRasterOD;
use COMMON::PyramidVector;

require Exporter;
use AutoLoader qw(AUTOLOAD);

our @ISA = qw(Exporter);

our %EXPORT_TAGS = ( 'all' => [ qw() ] );
our @EXPORT_OK   = ( @{$EXPORT_TAGS{'all'}} );
our @EXPORT      = qw();

use Log::Log4perl qw(:easy);

################################################################################
# Constantes
use constant TRUE  => 1;
use constant FALSE => 0;

####################################################################################################
#                                     Group: Loader                                                #
####################################################################################################

=begin nd
Function: load

Return a typed pyramid. To avoid error trace from constructors, we turn off logs. We try to load in this order :
* <COMMON::PyramidRasterOD>
* <COMMON::PyramidVector>
* <COMMON::Pyramidraster>

Parameters (list):
    path - string - Path to the pyramid's descriptor

Return :
    a <COMMON::PyramidRasterOD> or <COMMON::PyramidVector> or <COMMON::Pyramidraster> object, undef if none
=cut
sub load {
    my $path = shift;

    if (! -f $path) {
        ERROR("Descriptor file does not exists");
        return undef;
    }


    my $logPyr = get_logger("COMMON::PyramidRasterOD");
    my $logLev = get_logger("COMMON::LevelRasterOD");

    my $LOGLEVEL = $logPyr->level();

    $logPyr->level($OFF);
    $logLev->level($OFF);
    my $pyramid = COMMON::PyramidRasterOD->new("DESCRIPTOR", $path);
    $logPyr->level($LOGLEVEL);
    $logLev->level($LOGLEVEL);
    if (defined $pyramid) { return $pyramid; }

    $logPyr = get_logger("COMMON::PyramidRaster");
    $logLev = get_logger("COMMON::LevelRaster");
    $logPyr->level($OFF);
    $logLev->level($OFF);
    $pyramid = COMMON::PyramidRaster->new("DESCRIPTOR", $path);
    $logPyr->level($LOGLEVEL);
    $logLev->level($LOGLEVEL);
    if (defined $pyramid) { return $pyramid; }


    $logPyr = get_logger("COMMON::PyramidVector");
    $logLev = get_logger("COMMON::LevelVector");
    $logPyr->level($OFF);
    $logLev->level($OFF);
    $pyramid = COMMON::PyramidVector->new("DESCRIPTOR", $path);
    $logPyr->level($LOGLEVEL);
    $logLev->level($LOGLEVEL);
    if (defined $pyramid) { return $pyramid; }

    return undef;
}

1;
__END__
