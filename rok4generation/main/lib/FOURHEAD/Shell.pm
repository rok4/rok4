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
File: Shell.pm

Class: FOURHEAD::Shell

(see ROK4GENERATION/libperlauto/FOURHEAD_Shell.png)

Provides functions to generate slab for 4head tool.

Using:
    (start code)
    use FOURHEAD::Shell;

    my $scriptInit = FOURHEAD::Shell::getScriptInitialization($pyramid);
    (end code)
=cut

################################################################################

package FOURHEAD::Shell;

use strict;
use warnings;

use Data::Dumper;

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
#                                  Group: Shell functions                                          #
####################################################################################################

my $M4TFUNCTION = <<'M4TFUNCTION';
Merge4tiff () {
    local imgOut=$1
    local mskOut=$2
    shift 2
    local imgIn=( 0 $1 $3 $5 $7 )
    local mskIn=( 0 $2 $4 $6 $8 )
    shift 8

    local forRM=''

    # Entrées
    
    local inM4T=''
    
    for i in `seq 1 4`;
    do
        if [ ${imgIn[$i]} != '0' ] ; then
            forRM="$forRM ${TMP_DIR}/${imgIn[$i]}"
            inM4T=`printf "$inM4T -i%.1d ${TMP_DIR}/${imgIn[$i]}" $i`
            
            if [ ${mskIn[$i]} != '0' ] ; then
                inM4T=`printf "$inM4T -m%.1d ${TMP_DIR}/${mskIn[$i]}" $i`
                forRM="$forRM ${TMP_DIR}/${mskIn[$i]}"
            fi
        fi
    done
    
    # Sorties
    local outM4T=''
    
    if [ ${mskOut} != '0' ] ; then
        outM4T="-mo ${TMP_DIR}/${mskOut}"
    fi
    
    outM4T="$outM4T -io ${TMP_DIR}/${imgOut}"
    
    # Appel à la commande merge4tiff
    merge4tiff -c zip $inM4T $outM4T ${MERGE4TIFF_OPTIONS}
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
    
    # Suppressions
    rm $forRM
}
M4TFUNCTION

my $FILE_SLABFUNCTIONS = <<'SLABFUNCTIONS';
PullSlab () {
    local input=$1
    local output=$2

    cache2work -c zip ${PYR_DIR}/$input ${TMP_DIR}/$output
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
}

PushSlab () {
    local input=$1
    local output=$2
    local options=$3
        
    local dir=`dirname ${PYR_DIR}/$output`
    
    if [ ! -d $dir ] ; then mkdir -p $dir ; fi
        
    work2cache ${TMP_DIR}/$input ${PYR_DIR}/$output ${options}
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
}
SLABFUNCTIONS

my $S3_SLABFUNCTIONS = <<'SLABFUNCTIONS';
PullSlab () {
    local input=$1
    local output=$2

    cache2work -c zip -bucket ${PYR_BUCKET} $input ${TMP_DIR}/$output
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
}

PushSlab () {
    local input=$1
    local output=$2
    local options=$3
        
    work2cache ${TMP_DIR}/$input -bucket ${PYR_BUCKET} $output ${options}
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
}
SLABFUNCTIONS

my $SWIFT_SLABFUNCTIONS = <<'SLABFUNCTIONS';
PullSlab () {
    local input=$1
    local output=$2

    cache2work -c zip -container ${PYR_CONTAINER} ${KEYSTONE_OPTION} $input ${TMP_DIR}/$output
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
}

PushSlab () {
    local input=$1
    local output=$2
    local options=$3
        
    work2cache ${TMP_DIR}/$input -container ${PYR_CONTAINER} ${KEYSTONE_OPTION} $output ${options}
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi   
}
SLABFUNCTIONS

my $CEPH_SLABFUNCTIONS = <<'SLABFUNCTIONS';
PullSlab () {
    local input=$1
    local output=$2

    cache2work -c zip -pool ${PYR_POOL} $input ${TMP_DIR}/$output
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
}

PushSlab () {
    local input=$1
    local output=$2
    local options=$3
        
    work2cache ${TMP_DIR}/$input -pool ${PYR_POOL} $output ${options}
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi    
}
SLABFUNCTIONS

####################################################################################################
#                                   Group: Export function                                         #
####################################################################################################

=begin nd
Function: getScriptInitialization

Parameters (list):
    pyramid - <COMMON::PyramidRaster> - Pyramid to generate
    temp - string - Temporary directory

Returns:
    Global variables and functions to print into script
=cut
sub getScriptInitialization {
    my $pyramid = shift;
    my $temp = shift;

    my $string = sprintf "WORK2CACHE_IMAGE_OPTIONS=\"-t %s %s -c %s\"\n",
        $pyramid->getTileMatrixSet()->getTileWidth(), $pyramid->getTileMatrixSet()->getTileWidth(),
        $pyramid->getImageSpec()->getCompression();

    $string .= sprintf "WORK2CACHE_MASK_OPTIONS=\"-t %s %s -c zip\"\n",
        $pyramid->getTileMatrixSet()->getTileWidth(), $pyramid->getTileMatrixSet()->getTileWidth();
    
    $string .= sprintf "MERGE4TIFF_OPTIONS=\"-g %s -n %s\"\n",
        $pyramid->getImageSpec()->getGamma(),
        $pyramid->getNodata()->getValue();
    
    $string .= "TMP_DIR=$temp\n";

    if ($pyramid->getStorageType() eq "FILE") {
        $string .= sprintf "PYR_DIR=%s\n", $pyramid->getDataDir();
        $string .= $FILE_SLABFUNCTIONS;
    }
    elsif ($pyramid->getStorageType() eq "CEPH") {
        $string .= sprintf "PYR_POOL=%s\n", $pyramid->getDataPool();
        $string .= $CEPH_SLABFUNCTIONS;
    }
    elsif ($pyramid->getStorageType() eq "S3") {
        $string .= sprintf "PYR_BUCKET=%s\n", $pyramid->getDataBucket();
        $string .= $S3_SLABFUNCTIONS;
    }
    elsif ($pyramid->getStorageType() eq "SWIFT") {
        $string .= sprintf "PYR_CONTAINER=%s\n", $pyramid->getDataContainer();
        if ($pyramid->keystoneConnection()) {
            $string .= "KEYSTONE_OPTION=\"-ks\"\n";
        } else {
            $string .= "KEYSTONE_OPTION=\"\"\n";
        }
        $string .= $SWIFT_SLABFUNCTIONS;
    }

    $string .= $M4TFUNCTION;

    return $string;
}

1;
__END__
