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

Class: COMMON::GraphNode

Descibe a node of a <QTree> or a <Graph>. Allow different storage (FileSystem, Ceph, Swift)

Using:
    (start code)
    use COMMON::GraphNode

    my $tm = COMMON::TileMatrix->new(...)
    
    my $graph = COMMON::Qtree->new(...)
    #or
    my $graph = COMMON::NNGraph->new(...)
    
    my $node = COMMON::GraphNode->new({
        i => 51,
        j => 756,
        tm => $tm,
        graph => $graph,
        type => 'FS'
    });
    (end code)

Attributes:
    i - integer - Column, according to the TMS grid.
    j - integer - Row, according to the TMS grid.
    
    filestorage - string hash - informations for file system final storage
|       pyramidName - string - relative path of this node in the pyramid (generated from i,j). Example : level16/00/12/L5.tif
|       bgImageBasename - string - 
|       bgMaskBasename - string -

    cephstorage - string hash - informations for ceph final storage
|       pyramidName - string - object suffix name in the ceph pool. Example level16_6545_981
|       poolName - string - ceph pool name.

    s3storage - string hash - informations for s3 final storage
|       pyramidName - string - object suffix name in the s3 bucket. Example level16_6545_981
|       bucketName - string - s3 bucket name.

    swiftstorage - string hash - informations for swift final storage
|       pyramidName - string - object suffix name in the swift container. Example level16_6545_981
|       containerName - string - swift container name.

    workImageBasename - string - 
    workMaskBasename - string - 
    workExtension - string - extension of the temporary work image, lower case. Default value : tif.

    tm - <TileMatrix> - Tile matrix associated to the level which the node belong to.
    graph - <Graph> or <QTree> - Graph which contains the node.
    w - integer - Own node's weight
    W - integer - Accumulated weight (own weight + childs' accumulated weights sum)

    code - string - Commands to execute to generate this node (to write in a script)
    script - <Script> - Script in which the node will be generated

    nodeSources - <Node> array - Nodes from which this node is generated (working for <NNGraph>)
    geoImages - <GeoImage> array - Source images from which this node (if it belongs to the tree's bottom level) is generated (working for <QTree>)
=cut

################################################################################

package COMMON::GraphNode;

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

################################################################################

BEGIN {}
INIT {}
END {}

####################################################################################################
#                                        Group: Constructors                                       #
####################################################################################################

=begin nd
Constructor: new

Node constructor. Bless an instance.

Parameters (hash):
    type - string - Final storage type : 'FS', 'CEPH', 'S3' or 'SWIFT'
    i - integer - Node's column
    j - integer - Node's row
    tm - <TileMatrix> - Tile matrix of the level which node belong to
    graph - <Graph> or <QTree> - Graph containing the node.

See also:
    <_init>
=cut
sub new {
    my $this = shift;
    my $params = shift;
    
    my $class= ref($this) || $this;
    # IMPORTANT : if modification, think to update natural documentation (just above)
    my $self = {
        i => undef,
        j => undef,
        
        tm => undef,
        graph => undef,
        w => 0,
        W => 0,
        code => '',
        script => undef,
        nodeSources => [],
        geoImages => [],

        workImageBasename => undef,
        workMaskBasename => undef,
        workExtension => "tif", # for images, masks are always tif
        
        filestorage => undef,
        cephstorage => undef,
        s3storage => undef,
        swiftstorage => undef
    };
    
    bless($self, $class);
    
    TRACE;
    
    # init. class
    return undef if (! $self->_init($params));
    
    return $self;
}

=begin nd
Function: _init

Check and store node's attributes values. Initialize weights to 0. Calculate the pyramid's relative path, from indices, thanks to <Base36::indicesToB36Path>.

Parameters (hash):
    type - string - Final storage type : 'FS', 'CEPH', 'S3' or 'SWIFT'
    i - integer - Node's column
    j - integer - Node's row
    tm - <TileMatrix> - Tile matrix of the level which node belong to
    graph - <Graph> or <QTree> - Graph containing the node.
=cut
sub _init {
    my $self = shift;
    my $params = shift;
    
    TRACE;
    
    # mandatory parameters !
    if (! defined $params->{i}) {
        ERROR("Node's column is undef !");
        return FALSE;
    }
    if (! defined $params->{j}) {
        ERROR("Node's row is undef !");
        return FALSE;
    }
    if (! defined $params->{tm}) {
        ERROR("Node's tile matrix is undef !");
        return FALSE;
    }
    if (! defined $params->{graph}) {
        ERROR("Node's graph is undef !");
        return FALSE;
    }

    my $type = $params->{type};
    if (! defined $type || "|FS|SWIFT|S3|CEPH|" !~ m/\|$type\|/) {
        ERROR("Node's storage type is undef or not valid !");
        return FALSE;
    }
    
    # init. params    
    $self->{i} = $params->{i};
    $self->{j} = $params->{j};
    $self->{tm} = $params->{tm};
    $self->{graph} = $params->{graph};
    $self->{w} = 0;
    $self->{W} = 0;
    $self->{code} = '';

    if ($type eq "FS") {
        my $base36path = COMMON::Base36::indicesToB36Path($params->{i}, $params->{j}, $self->getGraph->getPyramid->getDirDepth()+1);
        $self->{filestorage}->{pyramidName} = File::Spec->catfile($self->getLevel, $base36path.".tif");
    }
    elsif ($type eq "CEPH") {
        $self->{cephstorage}->{pyramidName} = sprintf "%s_%s_%s", $self->getLevel, $params->{i}, $params->{j};
    }
    elsif ($type eq "S3") {
        $self->{s3storage}->{pyramidName} = sprintf "%s_%s_%s", $self->getLevel, $params->{i}, $params->{j};
    }
    elsif ($type eq "SWIFT") {
        ERROR("Swift storage not yet implemented");
        return FALSE;
    }

    $self->{workImageBasename} = sprintf "%s_%s_%s_I", $self->getLevel, $params->{i}, $params->{j};
    
    return TRUE;
}


####################################################################################################
#                                Group: Geographic tools                                           #
####################################################################################################

=begin nd
Function: isPointInNodeBbox

Returns a boolean indicating if the point is inside the bbox of the node.
  
Parameters:
    x - double - X coordinate of the point you want to know if it is in
    y - double - Y coordinate of the point you want to know if it is in
=cut
sub isPointInNodeBbox {
    my $self = shift;
    my $x = shift;
    my $y = shift;
    
    my ($xMinNode,$yMinNode,$xMaxNode,$yMaxNode) = $self->getBBox();
    
    if ( $xMinNode <= $x && $x <= $xMaxNode && $yMinNode <= $y && $y <= $yMaxNode ) {
        return TRUE;
    } else {
        return FALSE;
    }
}

=begin nd
Function: isBboxIntersectingNodeBbox

Tests if the provided bbox intersects the bbox of the node.
      
Parameters:
    Bbox - double list - (xmin,ymin,xmax,ymax) : coordinates of the bbox
=cut
sub isBboxIntersectingNodeBbox {
    my $self = shift;
    my ($xMin,$yMin,$xMax,$yMax) = @_;
    my ($xMinNode,$yMinNode,$xMaxNode,$yMaxNode) = $self->getBBox();
    
    if ($xMax > $xMinNode && $xMin < $xMaxNode && $yMax > $yMinNode && $yMin < $yMaxNode) {
        return TRUE;
    } else {
        return FALSE;
    }
    
}

####################################################################################################
#                                Group: Getters - Setters                                          #
####################################################################################################

# Function: getScript
sub getScript {
    my $self = shift;
    return $self->{script};
}

=begin nd
Function: writeInScript

Write own code in the associated script.

Parameters (list):
    additionnalText - string - Optionnal, can be undefined, text to add after the own code.
=cut
sub writeInScript {
    my $self = shift;
    my $additionnalText = shift;

    my $text = $self->{code};
    $text .= $additionnalText if (defined $additionnalText);
    
    $self->{script}->write($text, $self->getOwnWeight());
}

=begin nd
Function: setScript

Parameters (list):
    script - <COMMON::GraphScript> - Script to set.
=cut
sub setScript {
    my $self = shift;
    my $script = shift;
    
    $self->{script} = $script; 
}

# Function: getCol
sub getCol {
    my $self = shift;
    return $self->{i};
}

# Function: getRow
sub getRow {
    my $self = shift;
    return $self->{j};
}

# Function: getPyramidName
sub getPyramidName {
    my $self = shift;

    if (defined $self->{filestorage}) {
        return $self->{filestorage}->{pyramidName};
    }
    elsif (defined $self->{cephstorage}) {
        return $self->{cephstorage}->{pyramidName};
    }
    elsif (defined $self->{s3storage}) {
        return $self->{s3storage}->{pyramidName};
    }
    elsif (defined $self->{swiftstorage}) {
        return $self->{swiftstorage}->{pyramidName};
    }

    return undef;    
}

########## work files

# Function: setWorkExtension
sub setWorkExtension {
    my $self = shift;
    my $ext = shift;
    
    $self->{workExtension} = lc($ext);
}

# Function: addBgImage
sub addWorkMask {
    my $self = shift;
    $self->{workMaskBasename} = sprintf "%s_%s_%s_M", $self->getLevel, $self->{i}, $self->{j};
}

# Function: getWorkImageName
sub getWorkImageName {
    my $self = shift;
    my $withExtension = shift;
    
    return $self->{workImageBasename}.".".$self->{workExtension} if ($withExtension);
    return $self->{workImageBasename};
}

# Function: getWorkMaskName
sub getWorkMaskName {
    my $self = shift;
    my $withExtension = shift;
    
    return $self->{workMaskBasename}.".tif" if ($withExtension);
    return $self->{workMaskBasename};
}

=begin nd
Function: getWorkBaseName

Returns the work image base name (no extension) : "level_col_row"
=cut
sub getWorkBaseName {
    my $self = shift;
    return (sprintf "%s_%s_%s", $self->getLevel, $self->{i}, $self->{j});
}

########## background files

# Function: addBgImage
sub addBgImage {
    my $self = shift;

    if (! defined $self->{filestorage}) {
        ERROR("Background is not handled for no file system storage");
    }

    $self->{bgImageBasename} = sprintf "%s_%s_%s_BgI", $self->getLevel, $self->{i}, $self->{j};
}

# Function: getBgImageName
sub getBgImageName {
    my $self = shift;
    my $withExtension = shift;

    if (! defined $self->{filestorage}) {
        ERROR("Background is not handled for no file system storage");
    }
    
    return $self->{bgImageBasename}.".tif" if ($withExtension);
    return $self->{bgImageBasename};
}

# Function: addBgMask
sub addBgMask {
    my $self = shift;

    if (! defined $self->{filestorage}) {
        ERROR("Background is not handled for no file system storage");
    }

    $self->{bgMaskBasename} = sprintf "%s_%s_%s_BgM", $self->getLevel, $self->{i}, $self->{j};
}

# Function: getBgMaskName
sub getBgMaskName {
    my $self = shift;
    my $withExtension = shift;

    if (! defined $self->{filestorage}) {
        ERROR("Background is not handled for no file system storage");
    }
    
    return $self->{bgMaskBasename}.".tif" if ($withExtension);
    return $self->{bgMaskBasename};
}





# Function: getLevel
sub getLevel {
    my $self = shift;
    return $self->{tm}->getID;
}

# Function: getTM
sub getTM {
    my $self = shift;
    return $self->{tm};
}

# Function: getGraph
sub getGraph {
    my $self = shift;
    return $self->{graph};
}

# Function: getNodeSources
sub getNodeSources {
    my $self = shift;
    return $self->{nodeSources};
}

# Function: getGeoImages
sub getGeoImages {
    my $self = shift;
    return $self->{geoImages};
}

=begin nd
Function: addNodeSources

Parameters (list):
    nodes - <Node> array - Source nodes to add
=cut
sub addNodeSources {
    my $self = shift;
    my @nodes = shift;
    
    push(@{$self->getNodeSources()},@nodes);
    
    return TRUE;
}

=begin nd
Function: addGeoImages

Parameters (list):
    images - <GeoImage> array - Source images to add
=cut
sub addGeoImages {
    my $self = shift;
    my @images = shift;
    
    push(@{$self->getGeoImages()},@images);
    
    return TRUE;
}

=begin nd
Function: setCode

Parameters (list):
    code - string - Code to set.
=cut
sub setCode {
    my $self = shift;
    my $code = shift;
    $self->{code} = $code;
}

# Function: getBBox
sub getBBox {
    my $self = shift;
    
    my @Bbox = $self->{tm}->indicesToBBox(
        $self->{i},
        $self->{j},
        $self->{graph}->getPyramid->getTilesPerWidth,
        $self->{graph}->getPyramid->getTilesPerHeight
    );
    
    return @Bbox;
}

# Function: getOwnWeight
sub getOwnWeight {
    my $self = shift;
    return $self->{w};
}

# Function: getAccumulatedWeight
sub getAccumulatedWeight {
    my $self = shift;
    return $self->{W};
}

=begin nd
Function: setOwnWeight

Parameters (list):
    weight - integer - Own weight to set
=cut
sub setOwnWeight {
    my $self = shift;
    my $weight = shift;
    $self->{w} = $weight;
}

# Function: getScriptID
sub getScriptID {
    my $self = shift;
    return $self->{script}->getID;
}

=begin nd
Function: setAccumulatedWeight

AccumulatedWeight = children's weights sum + own weight = provided weight + already store own weight.
=cut
sub setAccumulatedWeight {
    my $self = shift;
    my $childrenWeight = shift;
    $self->{W} = $childrenWeight + $self->getOwnWeight;
}

=begin nd
Function: getPossibleChildren

Returns a <Node> array, containing children (length is always 4, with undefined value for children which don't exist), an empty array if the node is a leaf.

Warning:
    Do not mistake with <getChildren>
=cut
sub getPossibleChildren {
    my $self = shift;
    return $self->{graph}->getPossibleChildren($self);
}

=begin nd
Function: getChildren

Returns a <Node> array, containing real children (max length = 4), an empty array if the node is a leaf.

Warning:
    Do not mistake with <getPossibleChildren>
=cut
sub getChildren {
    my $self = shift;
    return $self->{graph}->getChildren($self);
}

####################################################################################################
#                                Group: Export methods                                             #
####################################################################################################

=begin nd
Function: exportForMntConf

Export attributes of the Node for mergeNtiff configuration file. Provided paths will be written as is, so can be relative or absolute (or use environment variables).

Masks and backgrounds are always TIFF images.

Parameters (list):
    exportBg - boolean - Export background files (image + mask) if presents. Considered only for file storage
    prefix - string - String to add before paths, can be undefined.
=cut
sub exportForMntConf {
    my $self = shift;
    my $exportBg = shift;
    my $prefix = shift;
    
    $prefix = "" if (! defined $prefix);

    TRACE;

    my @Bbox = $self->getBBox;
    my $output = "";

    $output = sprintf "IMG %s%s.%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n",
        $prefix, $self->{workImageBasename}, $self->{workExtension},
        $self->{tm}->getSRS(),
        $Bbox[0], $Bbox[3], $Bbox[2], $Bbox[1],
        $self->getTM()->getResolution(), $self->getTM()->getResolution();

    if (defined $self->{workMaskBasename}) {
        $output .= sprintf "MSK %s%s.tif\n", $prefix,  $self->{workMaskBasename};
    }
    
    if ($exportBg && defined $self->{filestorage}) {
        if (defined $self->{filestorage}->{bgImageBasename}) {
            $output .= sprintf "IMG %s%s.tif\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n",
                $prefix, $self->{filestorage}->{bgImageBasename},
                $self->{tm}->getSRS(),
                $Bbox[0], $Bbox[3], $Bbox[2], $Bbox[1],
                $self->getTM()->getResolution(), $self->getTM()->getResolution();
                
            if (defined $self->{filestorage}->{bgMaskBasename}) {
                $output .= sprintf "MSK %s%s.tif\n", $prefix, $self->{filestorage}->{bgMaskBasename};
            }        
        }
    }

    return $output;
}

=begin nd
Function: exportForDntConf

Export attributes of the Node for decimateNtiff configuration file. Provided paths will be written as is, so can be relative or absolute.

Masks and backgrounds are always TIFF images.

Parameters (list):
    exportBg - boolean - Export background files (image + mask) if presents. Considered only for file storage
    prefix - string - String to add before paths, can be undefined.
=cut
sub exportForDntConf {
    my $self = shift;
    my $exportBg = shift;
    my $prefix = shift;
    
    $prefix = "" if (! defined $prefix);

    my @Bbox = $self->getBBox();
    my $output = "";

    $output = sprintf "IMG %s%s.%s\t%s\t%s\t%s\t%s\t%s\t%s\n",
        $prefix, $self->{workImageBasename}, $self->{workExtension},
        $Bbox[0], $Bbox[3], $Bbox[2], $Bbox[1],
        $self->getTM()->getResolution(), $self->getTM()->getResolution();

    if (defined $self->{workMaskBasename}) {
        $output .= sprintf "MSK %s%s.tif\n", $prefix,  $self->{workMaskBasename};
    }
    
    if ($exportBg && defined $self->{filestorage}) {
        if (defined $self->{filestorage}->{bgImageBasename}) {
            $output .= sprintf "IMG %s%s.tif\t%s\t%s\t%s\t%s\t%s\t%s\n",
                $prefix, $self->{filestorage}->{bgImageBasename},
                $Bbox[0], $Bbox[3], $Bbox[2], $Bbox[1],
                $self->getTM()->getResolution(), $self->getTM()->getResolution();
                
            if (defined $self->{filestorage}->{bgMaskBasename}) {
                $output .= sprintf "MSK %s%s.tif\n", $prefix, $self->{filestorage}->{bgMaskBasename};
            }        
        }
    }

    return $output;
}

=begin nd
Function: exportForM4tConf

Export work files (output) and eventually background (input) of the Node for the Merge4tiff call line.

Parameters (list):
    exportBg - boolean - Export background files (image + mask) if presents. Considered only for file storage
=cut
sub exportForM4tConf {
    my $self = shift;
    my $exportBg = shift;

    my $output = sprintf " %s.%s", $self->{workImageBasename}, $self->{workExtension};

    if (defined $self->{workMaskBasename}) {
        $output .= sprintf " %s.tif", $self->{workMaskBasename};
    } else {
        $output .= " 0"
    }
    
    if ($exportBg && defined $self->{filestorage}) {
    
        if (defined $self->{filestorage}->{bgImageBasename}) {
            $output .= sprintf " %s.tif", $self->{filestorage}->{bgImageBasename};
            
            if (defined $self->{filestorage}->{bgMaskBasename}) {
                $output .= sprintf " %s.tif", $self->{filestorage}->{bgMaskBasename};
            } else {
                $output .= " 0"
            }
        } else {
            $output .= " 0 0"
        }
        
    }

    return $output;
}

=begin nd
Function: exportForDebug

Returns all image's components. Useful for debug.

Example:
    (start code)
    (end code)
=cut
sub exportForDebug {
    my $self = shift ;
    
    my $output = "";
    
    $output .= sprintf "Object COMMON::GraphNode :\n";

    if (defined $self->{filestorage}) {
        $output .= "\tFile storage\n";
    }
    elsif (defined $self->{cephstorage}) {
        $output .= "\tCeph storage\n";
    }
    elsif (defined $self->{s3storage}) {
        $output .= "\tS3 storage\n";
    }
    elsif (defined $self->{swiftstorage}) {
        $output .= "\tSwift storage\n";
    }
    else {
        $output .= "\tNo storage !!!\n";
    }

    $output .= sprintf "\tLevel : %s\n",$self->getLevel();
    $output .= sprintf "\tTM Resolution : %s\n",$self->getTM()->getResolution();
    $output .= sprintf "\tColonne : %s\n",$self->getCol();
    $output .= sprintf "\tLigne : %s\n",$self->getRow();
    if (defined $self->getScript()) {
        $output .= sprintf "\tScript ID : %\n",$self->getScriptID();
    } else {
        $output .= sprintf "\tScript undefined.\n";
    }
    $output .= sprintf "\tNoeud Source :\n";
    foreach my $node_sup ( @{$self->getNodeSources()} ) {
        $output .= sprintf "\t\tResolution : %s, Colonne ; %s, Ligne : %s\n",$node_sup->getTM()->getResolution(),$node_sup->getCol(),$node_sup->getRow();
    }
    $output .= sprintf "\tGeoimage Source :\n";
    
    foreach my $img ( @{$self->getGeoImages()} ) {
        $output .= sprintf "\t\tNom : %s\n",$img->getName();
    }

    $output .= sprintf "\tworkImageBasename : %s\n",$self->{workImageBasename} if (defined $self->{workImageBasename});
    $output .= sprintf "\tworkMaskBasename : %s\n",$self->{workMaskBasename} if (defined $self->{workMaskBasename});
    $output .= sprintf "\tworkExtension : %s\n",$self->{workExtension} if (defined $self->{workExtension});
    
    return $output;
}

1;
__END__