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

use Log::Log4perl qw(:easy);

require Exporter;
use AutoLoader qw(AUTOLOAD);

our @ISA = qw(Exporter);

our %EXPORT_TAGS = ( 'all' => [ qw() ] );
our @EXPORT_OK   = ( @{$EXPORT_TAGS{'all'}} );
our @EXPORT      = qw();

################################################################################
# constantes
use constant TRUE  => 1;
use constant FALSE => 0;

################################################################################

BEGIN {}
INIT {
  # getter/setter except "topleftcornerx topleftcornery" because of the precision !
  foreach my $i (qw(ID Resolution TileWidth TileHeight MatrixWidth MatrixHeight)) {
    my $field = $i;
    
    *{"get$field"} = sub {
      my $self = shift;
      TRACE ("$i");
      return $self->{lc $field};
    };
    
    *{"set$field"} = sub {
      my $self = shift;
      TRACE ("$i");
      @_ or die "not enough arguments to set$field, stopped !";
      $self->{lc $field} = shift;
      return 1;
    };
  }
}
END {}

################################################################################
=begin nd
Group: variable

variable: $self
    *    id             => undef,
    *    resolution     => undef,
    *    topleftcornerx => undef,
    *    topleftcornery => undef,
    *    tilewidth      => undef, # ie 256 by default ?
    *    tileheight     => undef, # ie 256 by default ?
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
  my $self = {
    id             => undef,
    resolution     => undef,
    topleftcornerx => undef,
    topleftcornery => undef,
    tilewidth      => undef, # often 256
    tileheight     => undef, # often 256
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
    $self->setID($params->{id});
    $self->setResolution($params->{resolution});
    $self->setTopLeftCornerX($params->{topleftcornerx});
    $self->setTopLeftCornerY($params->{topleftcornery});
    $self->setTileWidth($params->{tilewidth});
    $self->setTileHeight($params->{tileheight});
    $self->setMatrixWidth($params->{matrixwidth});
    $self->setMatrixHeight($params->{matrixheight});

    
    return TRUE;
}

####################################################################################################
#                                       GETTERS / SETTERS                                          #
####################################################################################################

# Group: getters - setters

sub getTopLeftCornerX {
    my $self = shift;
    TRACE ("getTopLeftCornerX");
    return $self->{topleftcornerx}; 
}
sub setTopLeftCornerX {
    my $self = shift;
    TRACE ("setTopLeftCornerX");
    if (@_) {
        my $v = shift;
        $self->{topleftcornerx} = sprintf ("%.10f", $v);
    }
}
sub getTopLeftCornerY {
    my $self = shift;
    TRACE ("getTopLeftCornerY");
    return $self->{topleftcornery}; 
}
sub setTopLeftCornerY {
    my $self = shift;
    TRACE ("setTopLeftCornerY");
    if (@_) {
        my $v = shift;
        $self->{topleftcornery} = sprintf ("%.10f", $v);
    }
}

####################################################################################################
#                                           OTHERS                                                 #
####################################################################################################

# Group: others

sub to_string {
  
  my $self = shift;
  
  TRACE;
  
  printf "%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n", 
    $self->getID(),
    $self->getResolution(),
    $self->getTopLeftCornerX(),
    $self->getTopLeftCornerY(),
    $self->getTileWidth(),
    $self->getTileHeight(),
    $self->getMatrixWidth(),
    $self->getMatrixHeight();
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

