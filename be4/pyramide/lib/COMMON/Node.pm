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

Class: COMMON::Node

Descibe a node of a <COMMON::QTree> or a <COMMON::NNGraph>. Allow different storage (FileSystem, Ceph, Swift).

Using:
    (start code)
    use COMMON::Node

    my $tm = COMMON::TileMatrix->new(...)
    
    my $graph = COMMON::Qtree->new(...)
    #or
    my $graph = COMMON::NNGraph->new(...)
    
    my $node = COMMON::Node->new({
        col => 51,
        row => 756,
        tm => $tm,
        graph => $graph,
        type => 'FILE'
    });
    (end code)

Attributes:
    col - integer - Column, according to the TMS grid.
    row - integer - Row, according to the TMS grid.

    storageType - string - Final storage type for the node : "FILE", "CEPH" or "S3"

    bgImageBasename - string - 
    bgMaskBasename - string - 

    workImageBasename - string - 
    workMaskBasename - string - 
    workExtension - string - extension of the temporary work image, lower case. Default value : tif.

    tm - <COMMON::TileMatrix> - Tile matrix associated to the level which the node belong to.
    graph - <COMMON::NNGraph> or <COMMON::QTree> - Graph which contains the node.
    w - integer - Own node's weight
    W - integer - Accumulated weight (own weight + childs' accumulated weights sum)

    code - string - Commands to execute to generate this node (to write in a script)
    script - <COMMON::Script> - Script in which the node will be generated

    nodeSources - <COMMON::Node> array - Nodes from which this node is generated (working for <COMMON::NNGraph>)
    geoImages - <COMMON::GeoImage> array - Source images from which this node (if it belongs to the tree's bottom level) is generated (working for <COMMON::QTree>)
=cut

################################################################################

package COMMON::Node;

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

# Constant: STORAGETYPES
# Define allowed values for attribute storage type.
my @STORAGETYPES = ("FILE", "CEPH", "S3", "SWIFT");

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

        w => 0,
        W => 0,
        code => '',
        script => undef,

        # Sources pour générer ce noeud
        nodeSources => [],
        geoImages => [],

        bgImageBasename => undef,
        bgMaskBasename => undef,

        workImageBasename => undef,
        workMaskBasename => undef,
        workExtension => "tif", # for images, masks are always tif
        
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

    $this->{storageType} = $params->{type};
    if (! defined $this->{storageType} || ! defined COMMON::Array::isInArray($this->{storageType}, @STORAGETYPES) ) {
        ERROR("Node's storage type is undef or not valid: ".$this->{storageType});
        return undef;
    }
    
    # init. params    
    $this->{col} = $params->{col};
    $this->{row} = $params->{row};
    $this->{tm} = $params->{tm};
    $this->{graph} = $params->{graph};
    $this->{w} = 0;
    $this->{W} = 0;
    $this->{code} = '';

    $this->{workImageBasename} = sprintf "%s_%s_%s_I", $this->getLevel(), $this->{col}, $this->{row};
        
    return $this;
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
    my $this = shift;
    my $x = shift;
    my $y = shift;
    
    my ($xMinNode,$yMinNode,$xMaxNode,$yMaxNode) = $this->getBBox();
    
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
    my $this = shift;
    my ($xMin,$yMin,$xMax,$yMax) = @_;
    my ($xMinNode,$yMinNode,$xMaxNode,$yMaxNode) = $this->getBBox();
    
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
    my $this = shift;
    return $this->{script};
}

=begin nd
Function: writeInScript

Write own code in the associated script.

Parameters (list):
    additionnalText - string - Optionnal, can be undefined, text to add after the own code.
=cut
sub writeInScript {
    my $this = shift;
    my $additionnalText = shift;

    my $text = $this->{code};
    $text .= $additionnalText if (defined $additionnalText);
    
    $this->{script}->write($text, $this->getOwnWeight());
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

########## work files

# Function: setWorkExtension
sub setWorkExtension {
    my $this = shift;
    my $ext = shift;
    
    $this->{workExtension} = lc($ext);
}

# Function: addBgImage
sub addWorkMask {
    my $this = shift;
    $this->{workMaskBasename} = sprintf "%s_%s_%s_M", $this->getLevel, $this->{col}, $this->{row};
}

# Function: getWorkImageName
sub getWorkImageName {
    my $this = shift;
    my $withExtension = shift;
    
    return $this->{workImageBasename}.".".$this->{workExtension} if ($withExtension);
    return $this->{workImageBasename};
}

# Function: getWorkMaskName
sub getWorkMaskName {
    my $this = shift;
    my $withExtension = shift;
    
    return $this->{workMaskBasename}.".tif" if ($withExtension);
    return $this->{workMaskBasename};
}

=begin nd
Function: getWorkBaseName

Returns the work image base name (no extension) : "level_col_row"
=cut
sub getWorkBaseName {
    my $this = shift;
    return (sprintf "%s_%s_%s", $this->getLevel, $this->{col}, $this->{row});
}

########## background files

# Function: addBgImage
sub addBgImage {
    my $this = shift;

    $this->{bgImageBasename} = sprintf "%s_%s_%s_BgI", $this->getLevel, $this->{col}, $this->{row};
}

# Function: getBgImageName
sub getBgImageName {
    my $this = shift;
    my $withExtension = shift;

    return undef if (! defined $this->{bgImageBasename});
    
    return $this->{bgImageBasename}.".tif" if ($withExtension);
    return $this->{bgImageBasename};
}

# Function: addBgMask
sub addBgMask {
    my $this = shift;

    $this->{bgMaskBasename} = sprintf "%s_%s_%s_BgM", $this->getLevel, $this->{col}, $this->{row};
}

# Function: getBgMaskName
sub getBgMaskName {
    my $this = shift;
    my $withExtension = shift;

    return undef if (! defined $this->{bgMaskBasename});
    
    return $this->{bgMaskBasename}.".tif" if ($withExtension);
    return $this->{bgMaskBasename};
}

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

# Function: getNodeSources
sub getNodeSources {
    my $this = shift;
    return $this->{nodeSources};
}

# Function: getGeoImages
sub getGeoImages {
    my $this = shift;
    return $this->{geoImages};
}

=begin nd
Function: addNodeSources

Parameters (list):
    nodes - <COMMON::Node> array - Source nodes to add
=cut
sub addNodeSources {
    my $this = shift;
    my @nodes = shift;
    
    push(@{$this->getNodeSources()},@nodes);
    
    return TRUE;
}

=begin nd
Function: addGeoImages

Parameters (list):
    images - <GeoImage> array - Source images to add
=cut
sub addGeoImages {
    my $this = shift;
    my @images = shift;
    
    push(@{$this->getGeoImages()},@images);
    
    return TRUE;
}

=begin nd
Function: setCode

Parameters (list):
    code - string - Code to set.
=cut
sub setCode {
    my $this = shift;
    my $code = shift;
    $this->{code} = $code;
}

# Function: getBBox
sub getBBox {
    my $this = shift;
    
    my @Bbox = $this->{tm}->indicesToBbox(
        $this->{col},
        $this->{row},
        $this->{graph}->getPyramid->getTilesPerWidth,
        $this->{graph}->getPyramid->getTilesPerHeight
    );
    
    return @Bbox;
}

# Function: getOwnWeight
sub getOwnWeight {
    my $this = shift;
    return $this->{w};
}

# Function: getAccumulatedWeight
sub getAccumulatedWeight {
    my $this = shift;
    return $this->{W};
}

=begin nd
Function: setOwnWeight

Parameters (list):
    weight - integer - Own weight to set
=cut
sub setOwnWeight {
    my $this = shift;
    my $weight = shift;
    $this->{w} = $weight;
}

# Function: getScriptID
sub getScriptID {
    my $this = shift;
    return $this->{script}->getID;
}

=begin nd
Function: setAccumulatedWeight

AccumulatedWeight = children's weights sum + own weight = provided weight + already store own weight.
=cut
sub setAccumulatedWeight {
    my $this = shift;
    my $childrenWeight = shift;
    $this->{W} = $childrenWeight + $this->getOwnWeight;
}

=begin nd
Function: getPossibleChildren

Returns a <COMMON::Node> array, containing children (length is always 4, with undefined value for children which don't exist), an empty array if the node is a leaf.

Warning:
    Do not mistake with <getChildren>
=cut
sub getPossibleChildren {
    my $this = shift;
    return $this->{graph}->getPossibleChildren($this);
}

=begin nd
Function: getChildren

Returns a <COMMON::Node> array, containing real children (max length = 4), an empty array if the node is a leaf.

Warning:
    Do not mistake with <getPossibleChildren>
=cut
sub getChildren {
    my $this = shift;
    return $this->{graph}->getChildren($this);
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
    my $this = shift;
    my $exportBg = shift;
    my $prefix = shift;
    
    $prefix = "" if (! defined $prefix);


    my @Bbox = $this->getBBox;
    my $output = "";

    $output = sprintf "IMG %s%s.%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n",
        $prefix, $this->{workImageBasename}, $this->{workExtension},
        $this->{tm}->getSRS(),
        $Bbox[0], $Bbox[3], $Bbox[2], $Bbox[1],
        $this->getTM()->getResolution(), $this->getTM()->getResolution();

    if (defined $this->{workMaskBasename}) {
        $output .= sprintf "MSK %s%s.tif\n", $prefix,  $this->{workMaskBasename};
    }

    if ($exportBg) {
        if (defined $this->{bgImageBasename}) {
            $output .= sprintf "IMG %s%s.tif\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n",
                $prefix, $this->{bgImageBasename},
                $this->{tm}->getSRS(),
                $Bbox[0], $Bbox[3], $Bbox[2], $Bbox[1],
                $this->getTM()->getResolution(), $this->getTM()->getResolution();
                
            if (defined $this->{bgMaskBasename}) {
                $output .= sprintf "MSK %s%s.tif\n", $prefix, $this->{bgMaskBasename};
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
    my $this = shift;
    my $exportBg = shift;
    my $prefix = shift;
    
    $prefix = "" if (! defined $prefix);


    my @Bbox = $this->getBBox();
    my $output = "";

    $output = sprintf "IMG %s%s.%s\t%s\t%s\t%s\t%s\t%s\t%s\n",
        $prefix, $this->{workImageBasename}, $this->{workExtension},
        $Bbox[0], $Bbox[3], $Bbox[2], $Bbox[1],
        $this->getTM()->getResolution(), $this->getTM()->getResolution();

    if (defined $this->{workMaskBasename}) {
        $output .= sprintf "MSK %s%s.tif\n", $prefix,  $this->{workMaskBasename};
    }
    
    if ($exportBg) {
        if (defined $this->{bgImageBasename}) {
            $output .= sprintf "IMG %s%s.tif\t%s\t%s\t%s\t%s\t%s\t%s\n",
                $prefix, $this->{bgImageBasename},
                $Bbox[0], $Bbox[3], $Bbox[2], $Bbox[1],
                $this->getTM()->getResolution(), $this->getTM()->getResolution();
                
            if (defined $this->{bgMaskBasename}) {
                $output .= sprintf "MSK %s%s.tif\n", $prefix, $this->{bgMaskBasename};
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
    my $this = shift;
    my $exportBg = shift;

    my $output = sprintf " %s.%s", $this->{workImageBasename}, $this->{workExtension};

    if (defined $this->{workMaskBasename}) {
        $output .= sprintf " %s.tif", $this->{workMaskBasename};
    } else {
        $output .= " 0"
    }
    
    if ($exportBg) {
    
        if (defined $this->{bgImageBasename}) {
            $output .= sprintf " %s.tif", $this->{bgImageBasename};
            
            if (defined $this->{bgMaskBasename}) {
                $output .= sprintf " %s.tif", $this->{bgMaskBasename};
            } else {
                $output .= " 0"
            }
        } else {
            $output .= " 0 0"
        }
        
    }

    return $output;
}

1;
__END__