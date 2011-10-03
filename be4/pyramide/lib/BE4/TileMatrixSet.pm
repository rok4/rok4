package BE4::TileMatrixSet;

use strict;
use warnings;

use Log::Log4perl qw(:easy);

use XML::Simple;

use BE4::TileMatrix;

require Exporter;
use AutoLoader qw(AUTOLOAD);

our @ISA = qw(Exporter);

our %EXPORT_TAGS = ( 'all' => [ qw() ] );
our @EXPORT_OK   = ( @{$EXPORT_TAGS{'all'}} );
our @EXPORT      = qw();

################################################################################
# version
our $VERSION = '0.0.2';

################################################################################
# constantes
use constant TRUE  => 1;
use constant FALSE => 0;

################################################################################
# Preloaded methods go here.
BEGIN {}
INIT {}
END {}

#
# Group: variable
#

#
# variable: $self
#
#    *     PATHFILENAME => undef,
#    *     name     => undef,
#    *     filename => undef,
#    *     filepath => undef,
#    *     srs        => undef, # ie proj4 !
#    *     tilematrix => {},

#
# Group: constructor
#

################################################################################
# constructor
sub new {
  my $this = shift;

  my $class= ref($this) || $this;
  my $self = {
    PATHFILENAME => undef,
    #
    name     => undef,
    filename => undef,
    filepath => undef,
    #
    levelmin => undef,
    levelmax => undef,
    #
    srs        => undef, # ie proj4 !
    tileheight => undef, # determined by TileMatrix(0) !
    tilewidth  => undef, # determined by TileMatrix(0) !
    tilematrix => {},
  };

  bless($self, $class);
  
  TRACE;
  
  # init. class
  return undef if (! $self->_init(@_));
  # load 
  return undef if (! $self->_load());
  
  return $self;
}

################################################################################
# privates init.
sub _init {
    my $self     = shift;
    my $pathfile = shift;
    my $levelmin = shift;
    my $levelmax = shift;

    TRACE;
    
    return FALSE if (! defined $pathfile);
    
    if (! -f $pathfile) {
      ERROR ("File TMS doesn't exist !");
      return FALSE;
    }
    
    # init. params    
    $self->{PATHFILENAME}=$pathfile;
    
    #
    $self->{filepath} = File::Basename::dirname($pathfile);
    $self->{filename} = File::Basename::basename($pathfile);
    $self->{name}     = File::Basename::basename($pathfile);
    $self->{name}     =~ s/\.(tms|TMS)$//;
    
    TRACE (sprintf "name : %s", $self->{name});
    
    #
    if (defined $levelmin) {
      $self->{levelmin} = $levelmin;
    }
    
    if (defined $levelmax) {
      $self->{levelmax} = $levelmax;
    }
    
    if (defined $levelmin && defined $levelmax && $levelmin > $levelmax) {
      $self->{levelmax} = $levelmin;
      $self->{levelmin} = $levelmax;
    }
    
    return TRUE;
}

################################################################################
# privates method
sub _load {
  my $self = shift;
  
  TRACE;
  
  my $xmltms  = new XML::Simple(KeepRoot => 0, SuppressEmpty => 1, ContentKey => '-content');
  my $xmltree = eval { $xmltms->XMLin($self->{PATHFILENAME}); };
  
  if ($@) {
    ERROR (sprintf "Can not read the XML file TMS : %s !", $@);
    return FALSE;
  }
  
  # load tileMatrix
  while (my ($k,$v) = each %{$xmltree->{tileMatrix}}) {
    
    next if (defined $self->{levelmin} && $k < $self->{levelmin});
    next if (defined $self->{levelmax} && $k > $self->{levelmax});
    
    my $obj = BE4::TileMatrix->new({
                        id => $k,
                        resolution     => $v->{resolution},
                        topleftcornerx => $v->{topLeftCornerX},
                        topleftcornery => $v->{topLeftCornerY},
                        tilewidth      => $v->{tileWidth}, 
                        tileheight     => $v->{tileHeight},
                        matrixwidth    => $v->{matrixWidth},
                        matrixheight   => $v->{matrixHeight},
                          });
    
    return FALSE if (! defined $obj);
    
    $self->{tilematrix}->{$k} = $obj;
    undef $obj;
  }
  
  if (! $self->getCountTileMatrix()) {
    ERROR (sprintf "No tilematrix loading from XML file TMS !");
    return FALSE;
  }
  
  # srs (== crs)
  if (! exists ($xmltree->{crs}) || ! defined ($xmltree->{crs})) {
    ERROR (sprintf "Can not determine parameter 'srs' in the XML file TMS !");
    return FALSE;
  }
  $self->{srs} = $xmltree->{crs};
  
  # clean
  $xmltree = undef;
  $xmltms  = undef;
  
  # tile size
  my $tm = $self->getFirstTileMatrix();
  $self->{tilewidth}  = $tm->getTileWidth();
  $self->{tileheight} = $tm->getTileHeight();
  
  return TRUE;
}
################################################################################
# get
sub getSRS {
  my $self = shift;
  return $self->{srs};
}
sub getName {
  my $self = shift;
  return $self->{name};
}
sub getPath {
  my $self = shift;
  return $self->{filepath};
}
sub getFile {
  my $self = shift;
  return $self->{filename};
}
# TileWidth TileHeight
sub getTileWidth {
  my $self = shift;
  # size of tile in pixel !
  return $self->{tilewidth};
}
sub getTileHeight {
  my $self = shift;
  # size of tile in pixel !
  return $self->{tileheight};
}
################################################################################
# public method to TileMatrix
sub getTileMatrixByArray {
  my $self = shift;
  
  my @levels;
  my $i = ($self->getFirstTileMatrix())->getID();
  while(defined (my $objTm = $self->getNextTileMatrix($i))) {
    push @levels, $objTm;
    $i++;
  }
  
  return sort {$a->getResolution() <=> $b->getResolution()} @levels;
}
sub getTileMatrix {
  my $self = shift;
  my $level= shift; # id !
  
  if (! defined $level) {
    return $self->{tilematrix};
  }
  
  return undef if (! exists($self->{tilematrix}->{$level}));
  
  return $self->{tilematrix}->{$level};
}
sub getCountTileMatrix {
  my $self = shift;
  
  my $count = 0;
  foreach my $l (keys %{$self->{tilematrix}}) {
    $count++;
  }
  return $count;
}
sub getFirstTileMatrix {
  my $self = shift;
  
  my $level = 0;

  TRACE;
  
  # fixme : variable POSIX to put correctly !
  foreach my $k (sort {$a <=> $b} (keys %{$self->{tilematrix}})) {
    $level = $k;
    last;
  }
  
  return $self->{tilematrix}->{$level};
}

sub getLastTileMatrix {
  my $self = shift;
  
  my $level = 0;
  
  TRACE;
  
  # fixme : variable POSIX to put correctly !
  foreach my $k (sort {$a <=> $b} (keys %{$self->{tilematrix}})) {
    $level = $k;
  }
  
  return $self->{tilematrix}->{$level};
}
sub getNextTileMatrix {
  my $self = shift;
  my $level= shift; # id !
  
  TRACE;
  
  return $self->getTileMatrix($level);
}
################################################################################
# to_string method
sub to_string {
  my $self = shift;
  
  TRACE;
  
  printf "%s\n", $self->{srs};
  
  my $i = ($self->getFirstTileMatrix())->getID();
  while(defined (my $tm = $self->getNextTileMatrix($i))) {
    printf "tilematrix:\n";
    printf "%s\n", $tm->to_string();
    $i++;
  }
}

1;
__END__

# Below is stub documentation for your module. You'd better edit it!

=head1 NAME

  BE4::TileMatrixSet - load a file tilematrixset.
  You can fix a min or/an a max level to extract tilematrixset.

=head1 SYNOPSIS

  use BE4::TileMatrixSet;
  
  my $filepath = "./t/data/tms/LAMB93_50cm_TEST.tms";
  my $objT = BE4::TileMatrixSet->new($filepath);

  scalar (@{$objT->getTileMatrix()};  # ie 19
  $objT->getTileMatrix(12);           # object TileMatrix with level id = 12
  $objT->getSRS();                    # ie 'IGNF:LAMB93'
  $objT->getName();                   # ie 'LAMB93_50cm_TEST'
  $objT->getFile();                   # ie 'LAMB93_50cm_TEST.tms'
  $objT->getPath();                   # ie './t/data/tms/'
  
  my $i = ($objT->getFirstTileMatrix())->getID();
  while(defined (my $objTm = $objT->getNextTileMatrix($i))) {
    printf "%s\n", $objTm->to_string();
    $i++;
  }
  ...
  
  $objT = BE4::TileMatrixSet->new($filepath, 8, 12);
  $objT->getFirstTileMatrix()->getID(); # 8
  $objT->getLastTileMatrix()->getID();  # 12

=head1 DESCRIPTION

=head2 EXPORT

None by default.

=head1 SAMPLE

* Sample Pyramid file (.pyr) :

  eg SEE ASLO

* Sample TMS file (.tms) :

  [LAMB93_50cm_TEST]
  
  <tileMatrixSet>
	<crs>IGNF:LAMB93</crs>
	<tileMatrix>
		<id>0</id>
		(...)
	</tileMatrix>
	(...)
	<tileMatrix>
                <id>17</id>
                <resolution>1</resolution>
                <topLeftCornerX> 0 </topLeftCornerX>
                <topLeftCornerY> 16777216 </topLeftCornerY>
                <tileWidth>256</tileWidth>
                <tileHeight>256</tileHeight>
                <matrixWidth>5040</matrixWidth>
                <matrixHeight>42040</matrixHeight>
        </tileMatrix>
	<tileMatrix>
                <id>18</id>
                <resolution>0.5</resolution>
                <topLeftCornerX> 0 </topLeftCornerX>
                <topLeftCornerY> 16777216 </topLeftCornerY>
                <tileWidth>256</tileWidth>
                <tileHeight>256</tileHeight>
                <matrixWidth>10080</matrixWidth>
                <matrixHeight>84081</matrixHeight>
        </tileMatrix>
  </tileMatrixSet>

* Sample LAYER file (.lay) :

  eg SEE ASLO

=head1 LIMITATIONS AND BUGS

 File name of tms must be with extension : tms or TMS !
 All levels must be continuous and unique !
 All levels are sorted by id !
 id level must be a numeric !

=head1 SEE ALSO

  eg package module following :
 
  BE4::Layer (?)
  BE4::Pyramid and BE4::Level
  BE4::TileMatrix

=head1 AUTHOR

Bazonnais Jean Philippe, E<lt>jpbazonnais@E<gt>

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2011 by Bazonnais Jean Philippe

This library is free software; you can redistribute it and/or modify
it under the same terms as Perl itself, either Perl version 5.10.1 or,
at your option, any later version of Perl 5 you may have available.

=cut
