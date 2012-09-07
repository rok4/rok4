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

################################################################################
=begin nd
Group: variable

variable: $self
    *    id             => undef,
    *    resolution     => undef,
    *    topleftcornerx => undef,
    *    topleftcornery => undef,
    *    tilewidth      => undef, # often 256
    *    tileheight     => undef, # often 256
    *    matrixwidth    => undef,
    *    matrixheight   => undef,
=cut

####################################################################################################
#                                       CONSTRUCTOR METHODS                                        #
####################################################################################################

# Group: constructor

sub new {
  my $this = shift;

  my $class= ref($this) || $this;
  # IMPORTANT : if modification, think to update natural documentation (just above) and pod documentation (bottom)
  my $self = {
    id             => undef,
    resolution     => undef,
    topleftcornerx => undef,
    topleftcornery => undef,
    tilewidth      => undef,
    tileheight     => undef,
    matrixwidth    => undef,
    matrixheight   => undef,
  };

  bless($self, $class);
  
  TRACE;
  
  # init. class
  if (! $self->_init(@_)) {
    ERROR ("One parameter is missing !");
    return undef;
  }
  
  return $self;
}

sub _init {
    my $self   = shift;
    my $params = shift;

    TRACE;
    
    return FALSE if (! defined $params);
    
    # parameters mandatoy !
    return FALSE if (! exists($params->{id})            || ! defined ($params->{id}));
    return FALSE if (! exists($params->{resolution})    || ! defined ($params->{resolution}));
    return FALSE if (! exists($params->{topleftcornerx})|| ! defined ($params->{topleftcornerx}));
    return FALSE if (! exists($params->{topleftcornery})|| ! defined ($params->{topleftcornery}));
    return FALSE if (! exists($params->{tilewidth})     || ! defined ($params->{tilewidth}));
    return FALSE if (! exists($params->{tileheight})    || ! defined ($params->{tileheight}));
    return FALSE if (! exists($params->{matrixwidth})   || ! defined ($params->{matrixwidth}));
    return FALSE if (! exists($params->{matrixheight})  || ! defined ($params->{matrixheight}));
    
    # init. params
    $self->{id} = $params->{id};
    $self->{resolution} = $params->{resolution};
    $self->{topleftcornerx} = $params->{topleftcornerx};
    $self->{topleftcornery} = $params->{topleftcornery};
    $self->{tilewidth} = $params->{tilewidth};
    $self->{tileheight} = $params->{tileheight};
    $self->{matrixwidth} = $params->{matrixwidth};
    $self->{matrixheight} = $params->{matrixheight};

    return TRUE;
}

####################################################################################################
#                                   COORDINATES MANIPULATION                                       #
####################################################################################################

#
=begin nd
method: getImgGroundWidth

Return the ground width of an image, whose number of tile (widthwise) is can be provided.

Parameters:
    tilesPerWidth - Optionnal (1 if undefined) 
=cut
sub getImgGroundWidth {
    my $self  = shift;
    my $tilesPerWidth = shift;
    
    $tilesPerWidth = 1 if (! defined $tilesPerWidth);
    
    my $xRes = Math::BigFloat->new($self->getResolution);
    my $imgGroundWidth = $self->getTileWidth * $tilesPerWidth * $xRes;
    
    return $imgGroundWidth;
}

#
=begin nd
method: getImgGroundHeight

Return the ground height of an image, whose number of tile (heightwise) is can be provided.

Parameters:
    tilesPerHeight - Optionnal (1 if undefined) 
=cut
sub getImgGroundHeight {
    my $self  = shift;
    my $tilesPerHeight = shift;
    
    $tilesPerHeight = 1 if (! defined $tilesPerHeight);
    
    my $yRes = Math::BigFloat->new($self->getResolution);
    my $imgGroundHeight = $self->getTileHeight * $tilesPerHeight * $yRes;
    
    return $imgGroundHeight;
}

#
=begin nd
method: columnToX

Return the X coordinate, in the TMS SRS, of the upper left corner, from the column indice and the number of tiles per width.

Parameters:
    col - Column indice
    tilesPerWidth - Optionnal (1 if undefined) 
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

#
=begin nd
method: rowToY

Return the Y coordinate, in the TMS SRS, of the upper left corner, from the row indice and the number of tiles per height.

Parameters:
    row - Row indice
    tilesPerHeight - Optionnal (1 if undefined)
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

#
=begin nd
method: xToColumn

Return the column indice for the given X coordinate and the number of tiles per width.

Parameters:
    x - x-axis coordinate
    tilesPerWidth - Optionnal (1 if undefined) 
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
method: yToRow

Return the row indice for the given Y coordinate and the number of tiles per height.

Parameters:
    y - y-axis coordinate
    tilesPerHeight - Optionnal (1 if undefined) 
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
method: indicesToBBox

Return the BBox from image's indices in an array : [xMin,yMin,xMax,yMax].

Parameters:
    i,j - Image indices.
    tilesPerWidth,tilesPerHeight - Number of tile in the image, width wise and heightwise.
=cut
sub indicesToBBox {
    my $self  = shift;
    my $i     = shift;
    my $j     = shift;
    my $tilesPerWidth = shift;
    my $tilesPerHeight = shift;
    
    my $imgGroundWidth = $self->getImgGroundWidth($tilesPerWidth);
    my $imgGroundHeight = $self->getImgGroundHeight($tilesPerHeight);
    
    my $xMin = $self->getTopLeftCornerX + $i * $imgGroundWidth;
    my $yMax = $self->getTopLeftCornerY - $j * $imgGroundHeight;
    my $xMax = $xMin + $imgGroundWidth;
    my $yMin = $yMax - $imgGroundHeight;
    
    return ($xMin,$yMin,$xMax,$yMax);
}

####################################################################################################
#                                       GETTERS / SETTERS                                          #
####################################################################################################

# Group: getters - setters

sub getID {
    my $self = shift;
    return $self->{id}; 
}
sub getResolution {
    my $self = shift;
    return $self->{resolution}; 
}
sub getTileWidth {
    my $self = shift;
    return $self->{tilewidth}; 
}
sub getTileHeight {
    my $self = shift;
    return $self->{tileheight}; 
}
sub getMatrixWidth {
    my $self = shift;
    return $self->{matrixwidth}; 
}
sub getMatrixHeight {
    my $self = shift;
    return $self->{matrixheight}; 
}

sub getTopLeftCornerX {
    my $self = shift;
    return $self->{topleftcornerx}; 
}

sub getTopLeftCornerY {
    my $self = shift;
    TRACE ("getTopLeftCornerY");
    return $self->{topleftcornery}; 
}


1;
__END__

=head1 NAME

BE4::TileMatrix - one level of a TileMatrixSet.

=head1 SYNOPSIS

    use BE4::TileMatrix;
    
    my $params = {
        id             => "18",
        resolution     => 0.5,
        topleftcornerx => 0,
        topleftcornery => 12000000,
        tilewidth      => 256,
        tileheight     => 256,
        matrixwidth    => 10080,
        matrixheight   => 84081,
    };
    
    my $objTM = BE4::TileMatrix->new($params);

=head1 DESCRIPTION

=head2 ATTRIBUTES

=over 4

=item id

Identifiant of the level, a string.

=item resolution

Ground size of a pixel, using unity of the SRS.

=item topleftcornerx, topleftcornery

Coordinates of the upper left corner for the level, the grid's origin.

=item tilewidth, tileheight

Pixel size of a tile (256 * 256).

=item matrixwidth, matrixheight

Number of tile in the level, widthwise and heightwise.

=back

=head1 SEE ALSO

=head2 NaturalDocs

=begin html

<A HREF="../Natural/Html/index.html">Index</A>

=end html

=head1 AUTHOR

Bazonnais Jean Philippe, E<lt>jean-philippe.bazonnais@ign.frE<gt>

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2011 by Bazonnais Jean Philippe

This library is free software; you can redistribute it and/or modify it under the same terms as Perl itself, either Perl version 5.10.1 or, at your option, any later version of Perl 5 you may have available.

=cut

