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
File: TileMatrix.pm

Class: BE4::TileMatrix

A Tile Matrix defines a grid for a level. Informations are extracted from a XML file.

Using:
    (start code)
    use BE4::TileMatrix;

    my $params = {
        id             => "18",
        resolution     => 0.5,
        topLeftCornerX => 0,
        topLeftCornerY => 12000000,
        tileWidth      => 256,
        tileHeight     => 256,
        matrixWidth    => 10080,
        matrixHeight   => 84081,
    };

    my $objTM = BE4::TileMatrix->new($params);                # ie '/home/ign/tms/'
    (end code)

Attributes:
    id - string - TM identifiant.
    resolution - double - Ground size of a pixel, using unity of the SRS.
    topLeftCornerX - double - X coordinate of the upper left corner for the level, the grid's origin.
    topLeftCornerY - double - Y coordinate of the upper left corner for the level, the grid's origin.
    tileWidth - integer - Pixel width of a tile.
    tileHeight - integer -  Pixel height of a tile.
    matrixWidth - integer - Number of tile in the grid, widthwise.
    matrixHeight - integer -  Number of tile in the grid, heightwise.
    targetsTm - <TileMatrix> array - Determine other levels which use this one to be generated. Empty if this level belong to a quad tree <TileMatrixSet>.

Limits:
    Resolution have to be the same  X and Y wise.
=cut

################################################################################

package BE4::TileMatrix;

use strict;
use warnings;

use Math::BigFloat;
use Log::Log4perl qw(:easy);

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

TileMatrix constructor. Bless an instance.

Parameters (hash):
    id - string - Level identifiant
    resolution - double - X and Y wise resolution
    topLeftCornerX - double - Origin easting
    topLeftCornerY - double - Origin northing
    tileWidth - integer -  Tile width, in pixel
    tileHeight - integer - Tile height, in pixel
    matrixWidth - integer - Grid width, in tile
    matrixHeight - integer - Grid height, in tile

See also:
    <_init>
=cut
sub new {
    my $this = shift;
    my $params = shift;

    my $class= ref($this) || $this;
    # IMPORTANT : if modification, think to update natural documentation (just above)
    my $self = {
        id             => undef,
        resolution     => undef,
        topLeftCornerX => undef,
        topLeftCornerY => undef,
        tileWidth      => undef,
        tileHeight     => undef,
        matrixWidth    => undef,
        matrixHeight   => undef,
        targetsTm   => [],
    };

    bless($self, $class);

    TRACE;

    # init. class
    if (! $self->_init($params)) {
        ERROR ("One parameter is missing !");
        return undef;
    }

    return $self;
}

=begin nd
Function: _init

Check and store TileMatrix's informations.

Parameters (hash):
    id - string - Level identifiant
    resolution - double - X and Y wise resolution
    topLeftCornerX - double - Origin easting
    topLeftCornerY - double - Origin northing
    tileWidth - integer -  Tile width, in pixel
    tileHeight - integer - Tile height, in pixel
    matrixWidth - integer - Grid width, in tile
    matrixHeight - integer - Grid height, in tile
=cut
sub _init {
    my $self   = shift;
    my $params = shift;

    TRACE;
    
    return FALSE if (! defined $params);
    
    # parameters mandatory !
    return FALSE if (! exists($params->{id}) || ! defined ($params->{id}));
    return FALSE if (! exists($params->{resolution}) || ! defined ($params->{resolution}));
    return FALSE if (! exists($params->{topLeftCornerX}) || ! defined ($params->{topLeftCornerX}));
    return FALSE if (! exists($params->{topLeftCornerY}) || ! defined ($params->{topLeftCornerY}));
    return FALSE if (! exists($params->{tileWidth}) || ! defined ($params->{tileWidth}));
    return FALSE if (! exists($params->{tileHeight}) || ! defined ($params->{tileHeight}));
    return FALSE if (! exists($params->{matrixWidth}) || ! defined ($params->{matrixWidth}));
    return FALSE if (! exists($params->{matrixHeight}) || ! defined ($params->{matrixHeight}));
    
    # init. params
    $self->{id} = $params->{id};
    $self->{resolution} = $params->{resolution};
    $self->{topLeftCornerX} = $params->{topLeftCornerX};
    $self->{topLeftCornerY} = $params->{topLeftCornerY};
    $self->{tileWidth} = $params->{tileWidth};
    $self->{tileHeight} = $params->{tileHeight};
    $self->{matrixWidth} = $params->{matrixWidth};
    $self->{matrixHeight} = $params->{matrixHeight};

    return TRUE;
}

####################################################################################################
#                                   Group: Coordinates manimulators                                #
####################################################################################################

=begin nd
Function: getImgGroundWidth

Returns the ground width of an image, whose number of tile (widthwise) can be provided.

Parameters (list):
    tilesPerWidth - integer - Optionnal (1 if undefined)
=cut
sub getImgGroundWidth {
    my $self  = shift;
    my $tilesPerWidth = shift;
    
    $tilesPerWidth = 1 if (! defined $tilesPerWidth);
    
    my $xRes = Math::BigFloat->new($self->getResolution);
    my $imgGroundWidth = $xRes * $self->getTileWidth * $tilesPerWidth;
    
    return $imgGroundWidth;
}

=begin nd
Function: getImgGroundHeight

Returns the ground height of an image, whose number of tile (heightwise) can be provided.

Parameters (list):
    tilesPerHeight - integer - Optionnal (1 if undefined) 
=cut
sub getImgGroundHeight {
    my $self  = shift;
    my $tilesPerHeight = shift;
    
    $tilesPerHeight = 1 if (! defined $tilesPerHeight);
    
    my $yRes = Math::BigFloat->new($self->getResolution);
    my $imgGroundHeight = $yRes * $self->getTileHeight * $tilesPerHeight;
    
    return $imgGroundHeight;
}

=begin nd
Function: columnToX

Returns the X coordinate, in the TMS SRS, of the upper left corner, from the column indice and the number of tiles per width.

Parameters (list):
    col - integer - Column indice
    tilesPerWidth - integer - Optionnal (1 if undefined) 
=cut
sub columnToX {
    my $self  = shift;
    my $col   = shift;
    my $tilesPerWidth = shift;
    
    $tilesPerWidth = 1 if (! defined $tilesPerWidth);
    
    my $xo  = $self->getTopLeftCornerX;
    my $rx  = Math::BigFloat->new($self->getResolution);
    my $width = $self->getTileWidth;
    
    my $x = $xo + $col * $rx * $width * $tilesPerWidth;
    
    return $x;
}

=begin nd
Function: rowToY

Returns the Y coordinate, in the TMS SRS, of the upper left corner, from the row indice and the number of tiles per height.

Parameters (list):
    row - integer - Row indice
    tilesPerHeight - integer - Optionnal (1 if undefined)
=cut
sub rowToY {
    my $self  = shift;
    my $row   = shift;
    my $tilesPerHeight = shift;
    
    $tilesPerHeight = 1 if (! defined $tilesPerHeight);
    
    my $yo = $self->getTopLeftCornerY;
    my $ry = Math::BigFloat->new($self->getResolution);
    my $height = $self->getTileHeight;
    
    my $y = $yo - ($row * $ry * $height * $tilesPerHeight);
    
    return $y;
}

=begin nd
Function: xToColumn

Returns the column indice for the given X coordinate and the number of tiles per width.

Parameters (list):
    x - double - x-axis coordinate
    tilesPerWidth - integer - Optionnal (1 if undefined) 
=cut
sub xToColumn {
    my $self  = shift;
    my $x     = shift;
    my $tilesPerWidth = shift;
    
    $tilesPerWidth = 1 if (! defined $tilesPerWidth);
    
    my $xo  = $self->getTopLeftCornerX;
    my $rx  = Math::BigFloat->new($self->getResolution);
    my $width = $self->getTileWidth;
    
    my $col = int(($x - $xo) / ($rx * $width * $tilesPerWidth)) ;
    
    return $col;
}

#
=begin nd
Function: yToRow

Returns the row indice for the given Y coordinate and the number of tiles per height.

Parameters (list):
    y - double - y-axis coordinate
    tilesPerHeight - integer - Optionnal (1 if undefined) 
=cut
sub yToRow {
    my $self  = shift;
    my $y     = shift;
    my $tilesPerHeight = shift;
    
    $tilesPerHeight = 1 if (! defined $tilesPerHeight);
    
    my $yo  = $self->getTopLeftCornerY;
    my $ry  = Math::BigFloat->new($self->getResolution);
    my $height = $self->getTileHeight;
    
    my $row = int(($yo - $y) / ($ry * $height * $tilesPerHeight)) ;
    
    return $row;
}

#
=begin nd
Function: indicesToBBox

Returns the BBox from image's indices in a list : (xMin,yMin,xMax,yMax).

Parameters (list):
    i - integer - Image's column
    j - integer - Image's row
    tilesPerWidth - integer - Number of tile in the image, widthwise
    tilesPerHeight - integer - Number of tile in the image, heightwise
=cut
sub indicesToBBox {
    my $self  = shift;
    my $i     = shift;
    my $j     = shift;
    my $tilesPerWidth = shift;
    my $tilesPerHeight = shift;
    
    my $imgGroundWidth = $self->getImgGroundWidth($tilesPerWidth);
    my $imgGroundHeight = $self->getImgGroundHeight($tilesPerHeight);
    
    my $xMin = $self->getTopLeftCornerX + $imgGroundWidth * $i;
    my $yMax = $self->getTopLeftCornerY - $imgGroundHeight * $j;
    my $xMax = $xMin + $imgGroundWidth;
    my $yMin = $yMax - $imgGroundHeight;
    
    return ($xMin,$yMin,$xMax,$yMax);
}

#
=begin nd
Function: bboxToIndices

Returns the extrem indices from a bbox in a list : (iMin,jMin,iMax,jMax).

Parameters (list):
    xMin,yMin,xMax,yMax - bounding box
    tilesPerWidth - integer - Number of tile in the image, widthwise
    tilesPerHeight - integer - Number of tile in the image, heightwise
=cut
sub bboxToIndices {
    my $self = shift;
    my $xMin = shift;
    my $yMin = shift;
    my $xMax = shift;
    my $yMax = shift;
    my $tilesPerWidth = shift;
    my $tilesPerHeight = shift;
    
    my $iMin = $self->xToColumn($xMin,$tilesPerWidth);
    my $iMax = $self->xToColumn($xMax,$tilesPerWidth);
    my $jMin = $self->yToRow($yMax,$tilesPerHeight);
    my $jMax = $self->yToRow($yMin,$tilesPerHeight);
    
    return ($iMin,$jMin,$iMax,$jMax);
}

####################################################################################################
#                                Group: Getters - Setters                                          #
####################################################################################################

# Function: getID
sub getID {
    my $self = shift;
    return $self->{id}; 
}

# Function: getResolution
sub getResolution {
    my $self = shift;
    return $self->{resolution}; 
}

# Function: getTileWidth
sub getTileWidth {
    my $self = shift;
    return $self->{tileWidth}; 
}

# Function: getTileHeight
sub getTileHeight {
    my $self = shift;
    return $self->{tileHeight}; 
}

# Function: getMatrixWidth
sub getMatrixWidth {
    my $self = shift;
    return $self->{matrixWidth}; 
}

# Function: getMatrixHeight
sub getMatrixHeight {
    my $self = shift;
    return $self->{matrixHeight}; 
}

# Function: getTopLeftCornerX
sub getTopLeftCornerX {
    my $self = shift;
    return Math::BigFloat->new($self->{topLeftCornerX}); 
}

# Function: getTopLeftCornerY
sub getTopLeftCornerY {
    my $self = shift;
    return Math::BigFloat->new($self->{topLeftCornerY}); 
}

# Function: getTargetsTm
sub getTargetsTm {
    my $self = shift;
    return $self->{targetsTm}
}

=begin nd
Function: addTargetTm

Parameters (list):
    tm - <TileMatrix> - Tile Matrix to add to target ones
=cut
sub addTargetTm {
    my $self = shift;
    my $tm = shift;
    push @{$self->{targetsTm}}, $tm;
}

####################################################################################################
#                                Group: Export methods                                             #
####################################################################################################

=begin nd
Function: exportForDebug

Returns all informations about the tile matrix. Useful for debug.

Example:
    (start code)
    (end code)
=cut
sub exportForDebug {
    my $self = shift;
    
    my $export = "";
    
    $export .= sprintf "\nObject BE4::TileMatrix :\n";
    $export .= sprintf "\t ID : %s \n", $self->getID();
    $export .= sprintf "\t Resolution : %s \n", $self->getResolution();
    $export .= sprintf "\t Top left corner : %s, %s \n", $self->getTopLeftCornerX(), $self->getTopLeftCornerY();
    $export .= sprintf "\t Tile width : %s \n", $self->getTileWidth();
    $export .= sprintf "\t Tile height : %s \n", $self->getTileHeight();
    $export .= sprintf "\t Matrix width : %s \n", $self->getMatrixWidth();
    $export .= sprintf "\t Matrix height : %s \n", $self->getMatrixHeight();
    $export .= sprintf "\t Targets tile matrix IDs (size:%s) :\n", scalar(@{$self->getTargetsTm()});
    foreach my $tm (@{$self->getTargetsTm()}) {
        $export .= sprintf "\t\t -> %s \n",$tm->getID();
    };
    
    return $export;  
};


1;
__END__
