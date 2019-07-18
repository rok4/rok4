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
File: Node.pm

Class: FOURALAMO::Node

(see ROK4GENERATION/libperlauto/COMMON_Node.png)

Describe a node of a <COMMON::QTree> or a <COMMON::NNGraph>. Allow different storage (FileSystem, Ceph, Swift).

Using:
    (start code)
    use FOURALAMO::Node

    my $tm = COMMON::TileMatrix->new(...)
    
    my $graph = COMMON::Qtree->new(...)
    #or
    my $graph = COMMON::NNGraph->new(...)
    
    my $node = FOURALAMO::Node->new({
        col => 51,
        row => 756,
        tm => $tm,
        graph => $graph
    });
    (end code)

Attributes:
    col - integer - Column, according to the TMS grid.
    row - integer - Row, according to the TMS grid.

    storageType - string - Final storage type for the node : "FILE", "CEPH" or "S3"

    workImageBasename - string - <LEVEL>_<COL>_<ROW>_I

    tm - <COMMON::TileMatrix> - Tile matrix associated to the level which the node belong to.
    graph - <COMMON::NNGraph> or <COMMON::QTree> - Graph which contains the node.

    script - <COMMON::Script> - Script in which the node will be generated
=cut

################################################################################

package FOURALAMO::Node;

use strict;
use warnings;

use Log::Log4perl qw(:easy);

use File::Spec ;
use Data::Dumper ;
use COMMON::Base36 ;

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
#                                        Group: Constructors                                       #
####################################################################################################

=begin nd
Constructor: new

Node constructor. Bless an instance.

Parameters (hash):
    type - string - Final storage type : 'FILE', 'CEPH', 'S3', 'SWIFT'
    col - integer - Node's column
    row - integer - Node's row
    tm - <COMMON::TileMatrix> - Tile matrix of the level which node belong to
    graph - <COMMON::NNGraph> or <COMMON::QTree> - Graph containing the node.

See also:
    <_init>
=cut
sub new {
    my $class = shift;
    my $params = shift;
    
    $class = ref($class) || $class;
    # IMPORTANT : if modification, think to update natural documentation (just above)
    my $this = {
        col => undef,
        row => undef,
        tm => undef,
        graph => undef,

        script => undef,

        workImageBasename => undef,
        
        # Stockage final de la dalle dans la pyramide pour ce noeud
        storageType => undef
    };
    
    bless($this, $class);
    
    # mandatory parameters !
    if (! defined $params->{col}) {
        ERROR("Node's column is undef !");
        return undef;
    }
    if (! defined $params->{row}) {
        ERROR("Node's row is undef !");
        return undef;
    }
    if (! defined $params->{tm}) {
        ERROR("Node's tile matrix is undef !");
        return undef;
    }
    if (! defined $params->{graph}) {
        ERROR("Node's graph is undef !");
        return undef;
    }

    # init. params    
    $this->{col} = $params->{col};
    $this->{row} = $params->{row};
    $this->{tm} = $params->{tm};
    $this->{graph} = $params->{graph};
    $this->{storageType} = $this->{graph}->getPyramid()->getStorageType();
    $this->{workImageBasename} = sprintf "%s_%s_%s_I", $this->getLevel(), $this->{col}, $this->{row};
        
    return $this;
}

####################################################################################################
#                                Group: Getters - Setters                                          #
####################################################################################################

# Function: getLevel
sub getLevel {
    my $this = shift;
    return $this->{tm}->getID;
}

# Function: getTM
sub getTM {
    my $this = shift;
    return $this->{tm};
}

# Function: getGraph
sub getGraph {
    my $this = shift;
    return $this->{graph};
}

# Function: getSlabPath
sub getSlabPath {
    my $this = shift;
    my $type = shift;
    my $full = shift;

    return $this->{graph}->getPyramid()->getSlabPath($type, $this->getLevel(), $this->getCol(), $this->getRow(), $full);
}


# Function: getSlabSize
sub getSlabSize {
    my $this = shift;

    return $this->{graph}->getPyramid()->getSlabSize($this->getLevel());
}

# Function: getCol
sub getCol {
    my $this = shift;
    return $this->{col};
}

# Function: getRow
sub getRow {
    my $this = shift;
    return $this->{row};
}

# Function: getStorageType
sub getStorageType {
    my $this = shift;
    return $this->{storageType};
}

########## scripts 

# Function: getScript
sub getScript {
    my $this = shift;
    return $this->{script};
}

=begin nd
Function: setScript

Parameters (list):
    script - <COMMON::Script> - Script to set.
=cut
sub setScript {
    my $this = shift;
    my $script = shift;
    
    if (! defined $script || ref ($script) ne "COMMON::Script") {
        ERROR("We expect to a COMMON::Script object.");
    }
    
    $this->{script} = $script; 
}

########## work

=begin nd
Function: getWorkBaseName

Returns the work image base name (no extension) : "level_col_row"
=cut
sub getWorkBaseName {
    my $this = shift;
    return (sprintf "%s_%s_%s", $this->getLevel, $this->{col}, $this->{row});
}

# Function: getUpperLeftTile
sub getUpperLeftTile {
    my $this = shift;

    return (
        $this->{col} * $this->{graph}->getPyramid()->getTilesPerWidth(),
        $this->{row} * $this->{graph}->getPyramid()->getTilesPerHeight()
    );
}

# Function: getBBox
sub getBBox {
    my $this = shift;
    
    my @Bbox = $this->{tm}->indicesToBbox(
        $this->{col},
        $this->{row},
        $this->{graph}->getPyramid()->getTilesPerWidth(),
        $this->{graph}->getPyramid()->getTilesPerHeight()
    );
    
    return @Bbox;
}

=begin nd
Function: getChildren

Returns a <FOURALAMO::Node> array, containing real children (max length = 4), an empty array if the node is a leaf.
=cut
sub getChildren {
    my $this = shift;
    return $this->{graph}->getChildren($this);
}

####################################################################################################
#                              Group: Processing functions                                         #
####################################################################################################

=begin nd
Function: makeJsons

Parameters (list):
    databaseSource - <COMMON::DatabaseSource> - To use to extract vector data.

Code exmaple:
    (start code)
    MakeJson "273950.309374068154368 6203017.719398627074048 293518.188615073275904 6222585.598639632195584" "host=postgis.ign.fr dbname=bdtopo user=ign password=PWD port=5432" "SELECT geometry FROM bdtopo_2018.roads WHERE type='highway'" roads
    (end code)

Returns:
    TRUE if success, FALSE if failure
=cut
sub makeJsons {
    my $this = shift;
    my $databaseSource = shift;

    my ($dburl, $srcSrs) = $databaseSource->getDatabaseInfos();

    my @tables = $databaseSource->getSqlExports();


    my @bbox = COMMON::ProxyGDAL::convertBBox( $this->getGraph()->getCoordTransPyramidDatasource(), $this->getBBox());

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

    for (my $i = 0; $i < scalar @tables; $i += 2) {
        my $sql = $tables[$i];
        my $dstTableName = $tables[$i+1];
        $this->{script}->write(sprintf "MakeJson \"$srcSrs\" \"$bbox_string\" \"$bbox_ext_string\" \"$dburl\" \"$sql\" $dstTableName\n");
    }

    return TRUE;
}

=begin nd
Function: pbf2cache

Example:
|    PushSlab 10 6534 9086 IMAGE/10/AB/CD.tif

Returns:
    TRUE if success, FALSE if failure
=cut
sub pbf2cache {
    my $this = shift;

    my $pyrName = $this->getSlabPath("IMAGE", FALSE);
    $this->{script}->write(sprintf "PushSlab %s %s %s %s\n", $this->getLevel(), $this->getUpperLeftTile(), $pyrName);
    
    return TRUE;
}

=begin nd
Function: makeTiles

Returns:
    TRUE if success, FALSE if failure
=cut
sub makeTiles {
    my $this = shift;
    
    $this->{script}->write("MakeTiles\n");

    return TRUE;
}

1;
__END__