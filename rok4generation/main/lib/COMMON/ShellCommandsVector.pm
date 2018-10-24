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
File: Commands.pm

Class: COMMON::ShellCommandsVector

Configure and assemble commands used to generate pyramid's images.

All schemes in this page respect this legend :

(see formats.png)

Using:
    (start code)
    use COMMON::ShellCommandsVector;

    # Commands object creation
    my $objCommands = COMMON::ShellCommandsVector->new(
        $objPyramid,
    );
    (end code)

Attributes:
    pyramid - <COMMON::PyramidVector> - Output pyramid to generate.
=cut

################################################################################

package COMMON::ShellCommandsVector;

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
#                                        Group: Constructors                                       #
####################################################################################################

=begin nd
Constructor: new

ShellCommands constructor. Bless an instance.

Parameters (list):
    pyr - <COMMON::PyramidVector> - Image pyramid to generate
=cut
sub new {
    my $class = shift;
    my $pyramid = shift;
    
    $class = ref($class) || $class;
    # IMPORTANT : if modification, think to update natural documentation (just above)
    my $this = {
        pyramid => undef,
    };

    bless($this, $class);

    if (! defined $pyramid || ref ($pyramid) ne "COMMON::PyramidVector") {
        ERROR("Can not load Pyramid !");
        return undef;
    }
    $this->{pyramid} = $pyramid;

    return $this;
}

####################################################################################################
#                                        Group: MAKE JSONS                                         #
####################################################################################################

# Constant: MAKEJSON_W
use constant MAKEJSON_W => 1;

my $MAKEJSON = <<'FUNCTION';

mkdir -p ${TMP_DIR}/jsons/
MakeJson () {
    local bbox=$1
    local dburl=$2
    local sql=$3
    local output=$4 

    ogr2ogr -f "GeoJSON" -spat $bbox -sql "$sql" ${TMP_DIR}/jsons/${output}.json PG:"$dburl"
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi     
}
FUNCTION

=begin nd
Function: makeJsons

Parameters (list):
    node - <Node> - Top node to generate thanks to a 'tippecanoe' command.
    databaseSource - <COMMON::DatabaseSource> - To use to extract vector data.

Returns:
    An array (code, weight), ("",-1) if error.
=cut
sub makeJsons {
    my $this = shift;
    my $node = shift;
    my $databaseSource = shift;
    
    my ($code, $weight) = ("",MAKEJSON_W);

    $code = $databaseSource->getCommandMakeJsons($node->getBBox());

    return ($code,$weight);
}

####################################################################################################
#                                        Group: MAKE TILES                                         #
####################################################################################################

# Constant: MAKETILES_W
use constant MAKETILES_W => 1;

my $MAKETILES = <<'FUNCTION';

mkdir -p ${TMP_DIR}/pbfs/
MakeTiles () {

    rm -r ${TMP_DIR}/pbfs/*

    tippecanoe __tpc__ --no-tile-compression -Z ${TOP_LEVEL} -z ${BOTTOM_LEVEL} -e ${TMP_DIR}/pbfs/  ${TMP_DIR}/jsons/*.json
    if [ $? != 0 ] ; then echo $0; fi

    rm ${TMP_DIR}/jsons/*.json
}
FUNCTION

=begin nd
Function: makeTiles

Parameters (list):
    node - <Node> - Top node to generate thanks to a 'tippecanoe' command.
    
Example:
|    MakeTiles

Returns:
    An array (code, weight), ("",-1) if error.
=cut
sub makeTiles {
    my $this = shift;
    my $node = shift;
    
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

my $S3_P2CFUNCTION = <<'P2CFUNCTION';
BackupListFile () {
    echo "List file back up to do"
}

StoreSlab () {
    local level=$1
    local ulcol=$2
    local ulrow=$3
    local imgName=$4

    pbf2cache __p2c__ -r ${TMP_DIR}/pbfs/${level} -ultile $ulcol $ulrow -bucket ${PYR_BUCKET} $imgName
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
}
P2CFUNCTION

my $SWIFT_P2CFUNCTION = <<'P2CFUNCTION';
BackupListFile () {
    echo "List file back up to do"
}


StoreSlab () {
    local level=$1
    local ulcol=$2
    local ulrow=$3
    local imgName=$4

    pbf2cache __p2c__ -r ${TMP_DIR}/pbfs/${level} -ultile $ulcol $ulrow -container ${PYR_CONTAINER} $imgName
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
}
P2CFUNCTION


my $CEPH_P2CFUNCTION = <<'P2CFUNCTION';
BackupListFile () {
    local objectName=`basename ${LIST_FILE}`
    rados -p ${PYR_POOL} put ${objectName} ${LIST_FILE}
}


StoreSlab () {
    local level=$1
    local ulcol=$2
    local ulrow=$3
    local imgName=$4

    pbf2cache __p2c__ -r ${TMP_DIR}/pbfs/${level} -ultile $ulcol $ulrow -pool ${PYR_POOL} $imgName
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
}
P2CFUNCTION


my $FILE_P2CFUNCTION = <<'P2CFUNCTION';
BackupListFile () {
    cp ${LIST_FILE} ${PYR_DIR}/
}


StoreSlab () {
    local level=$1
    local ulcol=$2
    local ulrow=$3
    local imgName=$4

    local dir=`dirname ${PYR_DIR}/$imgName`
    if [ ! -d $dir ] ; then mkdir -p $dir ; fi

    pbf2cache __p2c__ -r ${TMP_DIR}/pbfs/${level} -ultile $ulcol $ulrow ${PYR_DIR}/$imgName
    if [ $? != 0 ] ; then echo $0 : Erreur a la ligne $(( $LINENO - 1)) >&2 ; exit 1; fi
}
P2CFUNCTION

=begin nd
Function: pbf2cache

Example:
|    StoreSlab 10 6534 9086 IMAGE/10/...

Parameter:
    node - <Node> - Node whose image have to be transfered in the cache from PBF tiles (generated by tippecanoe).

Returns:
    An array (code, weight), ("",-1) if error.
=cut
sub pbf2cache {
    my $this = shift;
    my $node = shift;
    
    my $cmd = "";
    my $weight = PBF2CACHE_W;

    my $pyrName = $this->{pyramid}->getSlabPath("IMAGE", $node->getLevel(), $node->getCol(), $node->getRow(), FALSE);
    $cmd = sprintf "StoreSlab %s %s %s %s\n", $node->getLevel(), $node->getUpperLeftTile(), $pyrName;
    
    return ($cmd,$weight);
}

####################################################################################################
#                               Group: Functions configuration                                     #
####################################################################################################


=begin nd
Function: getConfiguredFunctions

Configure bash functions to write in scripts' header thanks to pyramid's components.
=cut
sub getConfiguredFunctions {
    my $this = shift;

    my $functions = "";

    ######## MAKEJSONS ########

    $functions .= $MAKEJSON;

    ######## MAKETILES ########

    my $conf_tpc = sprintf "-s %s", $this->{pyramid}->getTileMatrixSet()->getSRS();

    $functions .= $MAKETILES;

    $functions =~ s/__tpc__/$conf_tpc/g;

    ######## PBF2CACHE ########

    # pour les images    
    my $conf_p2c = sprintf "-t %s %s",
        $this->{pyramid}->getTilesPerWidth(), $this->{pyramid}->getTilesPerHeight();

    # Selon le type de stockage, on a une version différente des fonctions de stockage final
    if ($this->{pyramid}->getStorageType() eq "FILE") {
        $functions .= $FILE_P2CFUNCTION;
    }
    elsif ($this->{pyramid}->getStorageType() eq "S3") {
        $functions .= $S3_P2CFUNCTION;
    }
    elsif ($this->{pyramid}->getStorageType() eq "SWIFT") {
        $functions .= $SWIFT_P2CFUNCTION;
    }
    elsif ($this->{pyramid}->getStorageType() eq "CEPH") {
        $functions .= $CEPH_P2CFUNCTION;
    }

    $functions =~ s/__p2c__/$conf_p2c/g;


    return $functions;
}

####################################################################################################
#                                Group: Export methods                                             #
####################################################################################################

=begin nd
Function: exportForDebug

Returns all commands' informations. Useful for debug.

Example:
    (start code)
    (end code)
=cut
sub exportForDebug {
    my $this = shift ;

    my $export = "";

    $export .= "\nObject COMMON::ShellCommandsVector :\n";
    return $export;
}
  
1;
__END__
