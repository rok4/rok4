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
use Log::Log4perl qw(:easy);

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

####################################################################################################
#                                Group: Create nodata tile                                         #
####################################################################################################

use constant CREATE_NODATA => "createNodata";

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
    $cmd .= sprintf ( " -p %s", $nodataObj->getPixel->getPhotometric);
    $cmd .= sprintf ( " -t %s %s",$width,$height);
    $cmd .= sprintf ( " -b %s", $nodataObj->getPixel->getBitsPerSample);
    $cmd .= sprintf ( " -s %s", $nodataObj->getPixel->getSamplesPerPixel);
    $cmd .= sprintf ( " -a %s", $nodataObj->getPixel->getSampleFormat);
    $cmd .= sprintf ( " -pool %s", $poolName);
    $cmd .= sprintf ( " %s", $objName);
    
    if (! system($cmd) == 0) {
        ERROR (sprintf "The command to create a nodata tile is incorrect : '%s'",$cmd);
        return FALSE;
    }


    return TRUE; 
}

####################################################################################################
#                                Group: Create nodata tile                                         #
####################################################################################################

# Constant: WGET_W
use constant WGET_W => 35;

my $BASH_DOWNLOAD   = <<'BASH_DOWNLOAD';

Wms2work () {
    local dir=$1
    local harvest_ext=$2
    local final_ext=$3
    local nbTiles=$4
    local min_size=$5
    local url=$6
    shift 6

    local size=0

    mkdir $dir

    for i in `seq 1 $#`;
    do
        nameImg=`printf "$dir/img%.5d.$harvest_ext" $i`
        local count=0; local wait_delay=1
        while :
        do
            let count=count+1
            wget --no-verbose -O $nameImg "$url&BBOX=$1"

            if [ $? == 0 ] ; then
                if [ "$harvest_ext" == "png" ] ; then
                    if pngcheck $nameImg 1>/dev/null ; then break ; fi
                else
                    if tiffck $nameImg 1>/dev/null ; then break ; fi
                fi
            fi
            
            echo "Failure $count : wait for $wait_delay s"
            sleep $wait_delay
            let wait_delay=wait_delay*2
            if [ 3600 -lt $wait_delay ] ; then 
                let wait_delay=3600
            fi
        done
        let size=`stat -c "%s" $nameImg`+$size

        shift
    done
    
    if [ "$size" -le "$min_size" ] ; then
        RM_IMGS["$dir.$final_ext"]="1"

        rm -rf $dir
        return
    fi

    if [ "$nbTiles" != "1 1" ] ; then
        composeNtiff -g $nbTiles -s $dir/ -c zip $dir.$final_ext
        if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
    else
        mv $dir/img00001.$harvest_ext $dir.$final_ext
    fi

    rm -rf $dir
}

BASH_DOWNLOAD

=begin nd
Function: wms2work

Fetch image corresponding to the node thanks to 'wget', in one or more steps at a time. WMS service is described in the current graph's datasource. Use the 'Wms2work' bash function.

Example:
    (start code)
    BBOXES="10018754.17139461632,-626172.13571215872,10644926.30710678016,0.00000000512
    10644926.30710678016,-626172.13571215872,11271098.442818944,0.00000000512
    11271098.442818944,-626172.13571215872,11897270.57853110784,0.00000000512
    11897270.57853110784,-626172.13571215872,12523442.71424327168,0.00000000512
    10018754.17139461632,-1252344.27142432256,10644926.30710678016,-626172.13571215872
    10644926.30710678016,-1252344.27142432256,11271098.442818944,-626172.13571215872
    11271098.442818944,-1252344.27142432256,11897270.57853110784,-626172.13571215872
    11897270.57853110784,-1252344.27142432256,12523442.71424327168,-626172.13571215872
    10018754.17139461632,-1878516.4071364864,10644926.30710678016,-1252344.27142432256
    10644926.30710678016,-1878516.4071364864,11271098.442818944,-1252344.27142432256
    11271098.442818944,-1878516.4071364864,11897270.57853110784,-1252344.27142432256
    11897270.57853110784,-1878516.4071364864,12523442.71424327168,-1252344.27142432256
    10018754.17139461632,-2504688.54284865024,10644926.30710678016,-1878516.4071364864
    10644926.30710678016,-2504688.54284865024,11271098.442818944,-1878516.4071364864
    11271098.442818944,-2504688.54284865024,11897270.57853110784,-1878516.4071364864
    11897270.57853110784,-2504688.54284865024,12523442.71424327168,-1878516.4071364864"
    #
    Wms2work "path/image_several_requests" "png" "tif" "4 4" "250000" "http://localhost/wms-vector?LAYERS=BDD_WLD_WM&SERVICE=WMS&VERSION=1.3.0&REQUEST=getMap&FORMAT=image/png&CRS=EPSG:3857&WIDTH=1024&HEIGHT=1024&STYLES=line&BGCOLOR=0x80BBDA&TRANSPARENT=0X80BBDA" $BBOXES
    (end code)

Parameters (list):
    node - <COMMON::GraphNode> - Node whose image have to be harvested.
    harvesting - <Harvesting> - To use to harvest image.

Returns:
    An array (code, weight), (undef,WGET_W) if error.
=cut
sub wms2work {
    my $node = shift;
    my $harvesting = shift;

    my $pyramid = $node->getGraph->getPyramid;
    
    my @imgSize = $pyramid->getCacheImageSize($node->getLevel); # ie size tile image in pixel !
    my $tms     = $pyramid->getTileMatrixSet;
    
    my $nodeName = $node->getWorkImageName();
    
    my ($xMin, $yMin, $xMax, $yMax) = $node->getBBox();
    
    my ($cmd, $finalExtension) = $harvesting->getCommandWms2work({
        inversion => $tms->getInversion,
        dir => "\${TMP_DIR}/".$nodeName,
        srs => $tms->getSRS,
        bbox => [$xMin, $yMin, $xMax, $yMax],
        width => $imgSize[0],
        height => $imgSize[1]
    });
    
    if (! defined $cmd) {
        return (undef, WGET_W);
    }

    $node->setWorkExtension($finalExtension);
    
    return ($cmd, WGET_W);
}

sub configureFunctions {
    my $configuredFunc = $BASH_DOWNLOAD;
    return $configuredFunc;
}

1;
__END__
