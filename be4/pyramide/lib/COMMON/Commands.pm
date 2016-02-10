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
File: Command.pm

Class: COMMON::Commands


Using:
    (start code)
    (end code)
=cut

################################################################################

package COMMON::Commands;

use strict;
use warnings;

use Data::Dumper;

use COMMON::NoData;

require Exporter;
use AutoLoader qw(AUTOLOAD);

our @ISA = qw(Exporter);

our %EXPORT_TAGS = ( 'all' => [ qw() ] );
our @EXPORT_OK   = ( @{$EXPORT_TAGS{'all'}} );
our @EXPORT      = qw();

################################################################################
# Constantes
use constant TRUE  => 1;
use constant FALSE => 0;

use constant CREATE_NODATA => "createNodata";

################################################################################

=begin nd
Function: createNodataCeph

Compose the command to create a nodata tile and execute it.

Returns TRUE if the nodata tile is succefully written on a ceph cluster, FALSE otherwise.

Parameters (list):
=cut
sub createNodataCeph {
    my $poolName = shift;
    my $objName = shift;
    my $nodataObj = shift;
    my $width = shift;
    my $height = shift;
    my $compression = shift; 

    
    my $cmd = sprintf ("%s -n %s",CREATE_NODATA, $nodataObj->getValue());
    $cmd .= sprintf ( " -c %s", $compression);
    $cmd .= sprintf ( " -p %s", $nodataObj->getPhotometric);
    $cmd .= sprintf ( " -t %s %s",$width,$height);
    $cmd .= sprintf ( " -b %s", $nodataObj->getBitsPerSample);
    $cmd .= sprintf ( " -s %s", $nodataObj->getSamplesPerPixel);
    $cmd .= sprintf ( " -a %s", $nodataObj->getSampleFormat);
    $cmd .= sprintf ( " -pool %s", $poolName);
    $cmd .= sprintf ( " %s", $objName);
    
    if (! system($cmd) == 0) {
        ERROR (sprintf "The command to create a nodata tile is incorrect : '%s'",$cmd);
        return FALSE;
    }

    return TRUE; 
}

1;
__END__
