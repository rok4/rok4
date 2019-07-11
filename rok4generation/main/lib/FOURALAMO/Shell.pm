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

Class: FOURALAMO::Shell

(see ROK4GENERATION/libperlauto/FOURALAMO_Shell.png)

Configure and assemble commands used to generate vector pyramid's slabs.
=cut

################################################################################

package FOURALAMO::Shell;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
use File::Basename;
use File::Path;
use Data::Dumper;

use COMMON::Node;

require Exporter;
use AutoLoader qw(AUTOLOAD);

our @ISA = qw(Exporter);

our %EXPORT_TAGS = ( 'all' => [ qw() ] );
our @EXPORT_OK   = ( @{$EXPORT_TAGS{'all'}} );
our @EXPORT      = qw();

################################################################################

use constant TRUE  => 1;
use constant FALSE => 0;


####################################################################################################
#                                     Group: GLOBAL VARIABLES                                      #
####################################################################################################

my $COMMONTEMPDIR;

sub setGlobals {
    $COMMONTEMPDIR = shift;

    $COMMONTEMPDIR = File::Spec->catdir($COMMONTEMPDIR,"COMMON");

    # Common directory
    if (! -d $COMMONTEMPDIR) {
        DEBUG (sprintf "Create the common temporary directory '%s' !", $COMMONTEMPDIR);
        eval { mkpath([$COMMONTEMPDIR]); };
        if ($@) {
            ERROR(sprintf "Can not create the common temporary directory '%s' : %s !", $COMMONTEMPDIR, $@);
            return FALSE;
        }
    }
    
    return TRUE;
}

####################################################################################################
#                                        Group: MAKE JSONS                                         #
####################################################################################################

# Constant: MAKEJSON_W
use constant MAKEJSON_W => 15;

my $MAKEJSON = <<'FUNCTION';

mkdir -p ${TMP_DIR}/jsons/
MakeJson () {
    local srcsrs=$1
    local bbox=$2
    local bbox_ext=$3
    local dburl=$4
    local sql=$5
    local output=$6

    ogr2ogr -s_srs $srcsrs -f "GeoJSON" ${OGR2OGR_OPTIONS} -clipsrc $bbox_ext -spat $bbox -sql "$sql" ${TMP_DIR}/jsons/${output}.json PG:"$dburl"
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi     
}
FUNCTION

=begin nd
Function: makeJsons

Parameters (list):
    node - <COMMON::Node> - Top node for which extract data to GeoJson.
    databaseSource - <COMMON::DatabaseSource> - To use to extract vector data.

Code exmaple:
    (start code)
    MakeJson "273950.309374068154368 6203017.719398627074048 293518.188615073275904 6222585.598639632195584" "host=postgis.ign.fr dbname=bdtopo user=ign password=PWD port=5432" "SELECT geometry FROM bdtopo_2018.roads WHERE type='highway'" roads
    (end code)

Returns:
    An array (code, weight), ("",-1) if error.
=cut
sub makeJsons {
    my $node = shift;
    my $databaseSource = shift;

    my ($dburl, $srcSrs) = $databaseSource->getDatabaseInfos();

    my @tables = $databaseSource->getSqlExports();


    my @bbox = COMMON::ProxyGDAL::convertBBox( $node->getGraph()->getCoordTransPyramidDatasource(), $node->getBBox());

    # On va agrandir la bbox de 5% pour être sur de tout avoir
    my @bbox_extended = @bbox;
    my $w = ($bbox[2] - $bbox[0])*0.05;
    my $h = ($bbox[3] - $bbox[1])*0.05;
    $bbox_extended[0] -= $w;
    $bbox_extended[2] += $w;
    $bbox_extended[1] -= $h;
    $bbox_extended[3] += $h;

    my $bbox_ext_string = join(" ", @bbox_extended);
    my $bbox_string = join(" ", @bbox);

    my $code = "";
    for (my $i = 0; $i < scalar @tables; $i += 2) {
        my $sql = $tables[$i];
        my $dstTableName = $tables[$i+1];
        $code .= sprintf "MakeJson \"$srcSrs\" \"$bbox_string\" \"$bbox_ext_string\" \"$dburl\" \"$sql\" $dstTableName\n";
    }

    return ($code, MAKEJSON_W * scalar @tables / 2);
}

####################################################################################################
#                                        Group: MAKE TILES                                         #
####################################################################################################

# Constant: MAKETILES_W
use constant MAKETILES_W => 100;

my $MAKETILES = <<'FUNCTION';

mkdir -p ${TMP_DIR}/pbfs/
MakeTiles () {

    rm -r ${TMP_DIR}/pbfs/*

    local ndetail=12
    let sum=${BOTTOM_LEVEL}+$ndetail

    if [[ "$sum" -gt 32 ]] ; then
        let ndetail=32-${BOTTOM_LEVEL}
    fi

    tippecanoe ${TIPPECANOE_OPTIONS} --no-tile-compression --base-zoom ${TOP_LEVEL} --full-detail $ndetail -Z ${TOP_LEVEL} -z ${BOTTOM_LEVEL} -e ${TMP_DIR}/pbfs/  ${TMP_DIR}/jsons/*.json
    if [ $? != 0 ] ; then echo $0; fi

    rm ${TMP_DIR}/jsons/*.json
}
FUNCTION

=begin nd
Function: makeTiles

Returns:
    An array (code, weight), ("",-1) if error.
=cut
sub makeTiles {
    
    my ($c, $w);
    my ($code, $weight) = ("",MAKETILES_W);

    $code = "MakeTiles\n";

    return ($code,$weight);
}

####################################################################################################
#                                        Group: PBF TO CACHE                                       #
####################################################################################################

# Constant: PBF2CACHE_W
use constant PBF2CACHE_W => 1;


my $CEPH_P2CFUNCTION = <<'P2CFUNCTION';
BackupListFile () {
    local objectName=`basename ${LIST_FILE}`
    rados -p ${PYR_POOL} put ${objectName} ${LIST_FILE}
}


PushSlab () {
    local level=$1
    local ulcol=$2
    local ulrow=$3
    local imgName=$4

    pbf2cache ${PBF2CACHE_OPTIONS} -r ${TMP_DIR}/pbfs/${level} -ultile $ulcol $ulrow -pool ${PYR_POOL} $imgName
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
    echo "0/$imgName" >> ${TMP_LIST_FILE}
}
P2CFUNCTION


my $FILE_P2CFUNCTION = <<'P2CFUNCTION';
BackupListFile () {
    cp ${LIST_FILE} ${PYR_DIR}/
}

PushSlab () {
    local level=$1
    local ulcol=$2
    local ulrow=$3
    local imgName=$4

    local dir=`dirname ${PYR_DIR}/$imgName`
    if [ ! -d $dir ] ; then mkdir -p $dir ; fi

    pbf2cache ${PBF2CACHE_OPTIONS} -r ${TMP_DIR}/pbfs/${level} -ultile $ulcol $ulrow ${PYR_DIR}/$imgName
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
    echo "0/$imgName" >> ${TMP_LIST_FILE}
}
P2CFUNCTION

=begin nd
Function: pbf2cache

Example:
|    PushSlab 10 6534 9086 IMAGE/10/AB/CD.tif

Parameter:
    node - <COMMON::Node> - Node whose image have to be transfered in the cache from PBF tiles (generated by tippecanoe).

Returns:
    An array (code, weight), ("",-1) if error.
=cut
sub pbf2cache {
    my $node = shift;
    
    my $cmd = "";
    my $weight = PBF2CACHE_W;

    my $pyrName = $node->getSlabPath("IMAGE", FALSE);
    $cmd = sprintf "PushSlab %s %s %s %s\n", $node->getLevel(), $node->getUpperLeftTile(), $pyrName;
    
    return ($cmd,$weight);
}

####################################################################################################
#                                   Group: Export function                                         #
####################################################################################################

=begin nd
Function: getScriptInitialization

Parameters (list):
    pyramid - <COMMON::PyramidVector> - Pyramid to generate

Returns:
    Global variables and functions to print into script
=cut
sub getScriptInitialization {
    my $pyramid = shift;


    my $string = sprintf "LIST_FILE=\"%s\"\n", $pyramid->getListFile();
    $string .= "COMMON_TMP_DIR=\"$COMMONTEMPDIR\"\n";

    $string .= sprintf "OGR2OGR_OPTIONS=\"-a_srs %s -t_srs %s\"\n", $pyramid->getTileMatrixSet()->getSRS(), $pyramid->getTileMatrixSet()->getSRS();

    $string .= sprintf "TIPPECANOE_OPTIONS=\"-s %s -al -ap\"\n", $pyramid->getTileMatrixSet()->getSRS();

    $string .= sprintf "PBF2CACHE_OPTIONS=\"-t %s %s\"\n", $pyramid->getTilesPerWidth(), $pyramid->getTilesPerHeight();

    if ($pyramid->getStorageType() eq "FILE") {
        $string .= sprintf "PYR_DIR=%s\n", $pyramid->getDataDir();
        $string .= $FILE_P2CFUNCTION;
    }
    elsif ($pyramid->getStorageType() eq "CEPH") {
        $string .= sprintf "PYR_POOL=%s\n", $pyramid->getDataPool();
        $string .= $CEPH_P2CFUNCTION;
    }

    $string .= $MAKETILES;
    $string .= $MAKEJSON;

    return $string;
}
  
1;
__END__