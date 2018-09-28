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
#                                        Group: OGR 2 OGR                                          #
####################################################################################################

# Constant: OGR2OGR_W
use constant OGR2OGR_W => 1;

my $MAKETILES = <<'FUNCTION';
Ogr2ogrs () {
    local bbox=$1
    local dburl=$2
    shift 2

    for i in `seq 1 $#`;
    do
        tablename=$1
        jsonname="${tablename}.json"
        ogr2ogr -f "GeoJSON" -spat $bbox "$jsonname" $dburl $tablename
        shift
    done
    
}
FUNCTION

=begin nd
Function: makeTiles

Use the 'MergeNtiff' bash function. Write a configuration file, with sources.

(see mergeNtiff.png)

Parameters (list):
    node - <Node> - Node to generate thanks to a 'mergeNtiff' command.
    
Example:
|    MergeNtiff 19_397_3134.txt

Returns:
    An array (code, weight), ("",-1) if error.
=cut
sub makeTiles {
    my $this = shift;
    my $node = shift;
    
    my ($c, $w);
    my ($code, $weight) = ("",MERGENTIFF_W);

    return ($code,$weight);
}

####################################################################################################
#                                        Group: MakeTiles                                          #
####################################################################################################

# Constant: MAKETILES_W
use constant MAKETILES_W => 1;

my $MAKETILES = <<'FUNCTION';
MakeTiles () {
    tippecanoe __tpc__
    rm dossier
}
FUNCTION

=begin nd
Function: makeTiles

Use the 'MergeNtiff' bash function. Write a configuration file, with sources.

(see mergeNtiff.png)

Parameters (list):
    node - <Node> - Node to generate thanks to a 'mergeNtiff' command.
    
Example:
|    MergeNtiff 19_397_3134.txt

Returns:
    An array (code, weight), ("",-1) if error.
=cut
sub makeTiles {
    my $this = shift;
    my $node = shift;
    
    my ($c, $w);
    my ($code, $weight) = ("",MERGENTIFF_W);

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
    pbf2cache __p2c__
}
P2CFUNCTION

my $SWIFT_P2CFUNCTION = <<'P2CFUNCTION';
BackupListFile () {
    echo "List file back up to do"
}


StoreSlab () {
    pbf2cache __p2c__
}
P2CFUNCTION


my $CEPH_P2CFUNCTION = <<'P2CFUNCTION';
BackupListFile () {
    local objectName=`basename ${LIST_FILE}`
    rados -p ${PYR_POOL} put ${objectName} ${LIST_FILE}
}


StoreSlab () {
    pbf2cache __p2c__
}
P2CFUNCTION


my $FILE_P2CFUNCTION = <<'P2CFUNCTION';
BackupListFile () {
    cp ${LIST_FILE} ${PYR_DIR}/
}


StoreSlab () {
    pbf2cache __p2c__
}
P2CFUNCTION

=begin nd
Function: pbf2cache

Copy image from work directory to cache and transform it (tiled and compressed) thanks to the 'PBF2CACHE' bash function (PBF2CACHE).

(see PBF2CACHE.png)

Example:
|    PBF2CACHE ${TMP_DIR}/19_395_3137.tif IMAGE/19/02/AF/Z5.tif

Parameter:
    node - <Node> - Node whose image have to be transfered in the cache.
    workDir - string - Work image directory, can be an environment variable.

Returns:
    An array (code, weight), ("",-1) if error.
=cut
sub pbf2cache {
    my $this = shift;
    my $node = shift;
    
    my $cmd = "";
    my $weight = 0;
    

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

    ######## OGR2OGRS #########

    my $conf_o2o = ""

    $functions .= $OGR2OGRS;

    $functions =~ s/__o2o__/$conf_o2o/g;

    ######## MAKETILES ########

    my $conf_p2c = ""

    $functions .= $MAKETILES;

    $functions =~ s/__tpc__/$conf_p2c/g;

    ######## PBF2CACHE ########

    # pour les images    
    my $conf_p2c = sprintf "-t %s %s",
        $this->{pyramid}->getTileMatrixSet()->getTileWidth(), $this->{pyramid}->getTileMatrixSet()->getTileHeight();

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
