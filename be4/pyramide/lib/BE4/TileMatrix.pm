package BE4::TileMatrix;

# use strict;
use warnings;

use Log::Log4perl qw(:easy);

require Exporter;
use AutoLoader qw(AUTOLOAD);

our @ISA = qw(Exporter);

our %EXPORT_TAGS = ( 'all' => [ qw() ] );
our @EXPORT_OK   = ( @{$EXPORT_TAGS{'all'}} );
our @EXPORT      = qw();

################################################################################
# version
our $VERSION = '0.0.1';

################################################################################
# constantes
use constant TRUE  => 1;
use constant FALSE => 0;

################################################################################
# Preloaded methods go here.

#
# Group: Preloaded methods
#

#
# function: get/set in init()
#
#   get/set with field, ID Resolution TileWidth TileHeight MatrixWidth MatrixHeight
#

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
# sample :
# 
#    <id>0</id>
#    <resolution>0.703125</resolution>
#    <topLeftCornerX>-180</topLeftCornerX>
#    <topLeftCornerY>90</topLeftCornerY>
#    <tileWidth>256</tileWidth>
#    <tileHeight>256</tileHeight>
#    <matrixWidth>2</matrixWidth>
#    <matrixHeight>1</matrixHeight>
# 

#
# Group: variable
#

#
# variable: $self
#
#    *    id             => undef,
#    *    resolution     => undef,
#    *    topleftcornerx => undef,
#    *    topleftcornery => undef,
#    *    tilewidth      => undef, # ie 256 by default ?
#    *    tileheight     => undef, # ie 256 by default ?
#    *    matrixwidth    => undef,
#    *    matrixheight   => undef,

#
# Group: constructor
#

################################################################################
# constructor
sub new {
  my $this = shift;

  my $class= ref($this) || $this;
  my $self = {
    id             => undef,
    resolution     => undef,
    topleftcornerx => undef,
    topleftcornery => undef,
    tilewidth      => undef, # ie 256 by default ?
    tileheight     => undef, # ie 256 by default ?
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

################################################################################
# privates init.
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
################################################################################
# get/set
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
################################################################################
# to_string method
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

# Below is stub documentation for your module. You'd better edit it!

=head1 NAME

  BE4::TileMatrix - one level of a TileMatrixSet.

=head1 SYNOPSIS

  use BE4::TileMatrix;
  
  my $params = {
    id             => 0,
    resolution     => 1,
    topleftcornerx => 0.0000000000000002,
    topleftcornery => 100,
    tilewidth      => 256,
    tileheight     => 256,
    matrixwidth    => 1,
    matrixheight   => 1,
  };

  my $objH = BE4::TileMatrix->new($params);
  
  $objH->setResolution(2);  # 2
  $objH->setTopLeftCornerX(100); # 100
  $objH->setTileWidth() # not enough arguments to setTileWidth, stopped !
  
  $objH->to_string();
  ...

=head1 DESCRIPTION

=head1 LIMITATIONS AND BUGS

 No test on the type value !
 Limit of precision to X and Y : 10 decimal !

=head2 EXPORT

None by default.

=head1 SEE ALSO

  eg package module following :
 
  BE4::TileMatrix

=head1 AUTHOR

Bazonnais Jean Philippe, E<lt>jpbazonnais@E<gt>

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2011 by Bazonnais Jean Philippe

This library is free software; you can redistribute it and/or modify
it under the same terms as Perl itself, either Perl version 5.10.1 or,
at your option, any later version of Perl 5 you may have available.

=cut

