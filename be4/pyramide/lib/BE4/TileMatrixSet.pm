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

package BE4::TileMatrixSet;

use strict;
use warnings;

use Log::Log4perl qw(:easy);

use XML::Simple;
use Data::Dumper;

use BE4::TileMatrix;

require Exporter;
use AutoLoader qw(AUTOLOAD);

our @ISA = qw(Exporter);

our %EXPORT_TAGS = ( 'all' => [ qw() ] );
our @EXPORT_OK   = ( @{$EXPORT_TAGS{'all'}} );
our @EXPORT      = qw();

################################################################################
# version
my $VERSION = "0.0.1";

################################################################################
# constantes
use constant TRUE  => 1;
use constant FALSE => 0;

################################################################################
# Preloaded methods go here.
BEGIN {}
INIT {}
END {}

################################################################################
#
# Group: variable
#

#
# variable: $self
#
#    * PATHFILENAME => undef,
#    * name     => undef,
#    * filename => undef,
#    * filepath => undef,
#    * levelIdx => undef, # hash to associate TM's id with order (in ascending resolution order)
#    * leveltop => undef,
#    * resworst => undef,
#    * levelbottom => undef,
#    * resbest  => undef,
#    * srs        => undef, # srs is casted in uppercase
#    * tilematrix => {},
#

####################################################################################################
#                                       CONSTRUCTOR METHODS                                        #
####################################################################################################

# Group: constructor

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
    levelIdx => undef, # hash associant les id de TM à leur indice dans le tableau du TMS (dans le sens croissant des résolutions)
    leveltop => undef,
    resworst => undef,
    levelbottom => undef,
    resbest  => undef,
    #
    srs        => undef, # srs is casted in uppercase
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

sub _init {
    my $self     = shift;
    my $pathfile = shift;

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
    
    return TRUE;
}

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

    # we identify level max (with the best resolution, the smallest) and level min (with the 
    # worst resolution, the biggest)
    
    if (! defined $self->{leveltop} || ! defined $self->{resworst} || $v->{resolution} > $self->{resworst}) {
        $self->{leveltop} = $k;
        $self->{resworst} = $v->{resolution};
    }
    if (! defined $self->{levelbottom} || ! defined $self->{resbest} || $v->{resolution} < $self->{resbest}) {
        $self->{levelbottom} = $k;
        $self->{resbest} = $v->{resolution};
    }
    
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
  $self->{srs} = uc($xmltree->{crs}); # srs is cast in uppercase in order to ease comparisons
  
  # clean
  $xmltree = undef;
  $xmltms  = undef;

  # tilematrix list sort by resolution
  my @tmList = $self->getTileMatrixByArray();

  # on fait un hash pour retrouver l'ordre d'un niveau a partir de son id.
  TRACE("sort by ID...");
  
  for (my $i=0; $i < scalar @tmList; $i++){
    $self->{levelIdx}{$tmList[$i]->getID()} = $i;
  }

  return TRUE;
}

####################################################################################################
#                                       GETTERS / SETTERS                                          #
####################################################################################################

# Group: getters - setters

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
sub getTileWidth {
  my $self = shift;
  # size of tile in pixel !
  return $self->{tilematrix}->{$self->{levelbottom}}->{tilewidth};
}
sub getTileHeight {
  my $self = shift;
  # size of tile in pixel !
  return $self->{tilematrix}->{$self->{levelbottom}}->{tileheight};
}

# method: getTileMatrixByArray
#  Return the tile matrix array in the ascending resolution order.
#---------------------------------------------------------------------------------------------------------------
sub getTileMatrixByArray {
    my $self = shift;

    TRACE("sort by Resolution...");
    
    my @levels;

    foreach my $k (sort {$a->getResolution() <=> $b->getResolution()} (values %{$self->{tilematrix}})) {
        push @levels, $k;
    }

    return @levels;
}

# method: getTileMatrix
#  Return the tile matrix from the supplied ID. This ID is the TMS ID (string) and not the ascending resolution 
#  order (integer).
#---------------------------------------------------------------------------------------------------------------
sub getTileMatrix {
  my $self = shift;
  my $level= shift; # id !
  
  if (! defined $level) {
    return undef;
    #return $self->{tilematrix};
  }
  
  return undef if (! exists($self->{tilematrix}->{$level}));
  
  return $self->{tilematrix}->{$level};
}

# method: getCountTileMatrix
#  Return the count of tile matrix in the TMS.
#---------------------------------------------------------------------------------------------------------------
sub getCountTileMatrix {
  my $self = shift;
  
  my $count = 0;
  foreach my $l (keys %{$self->{tilematrix}}) {
    $count++;
  }
  return $count;
}

# method: getBottomTileMatrix
#  Return the bottom tile matrix ID, with the smallest resolution and the order '0'.
#---------------------------------------------------------------------------------
sub getBottomTileMatrix {
    my $self = shift;

    TRACE;

    # FIXME : variable POSIX to put correctly !
    return $self->{tilematrix}->{$self->{levelbottom}};
}

# method: getTopTileMatrix
#  Return the top tile matrix ID, with the biggest resolution and the order 'NumberOfTM'.
#---------------------------------------------------------------------------------
sub getTopTileMatrix {
    my $self = shift;

    TRACE;

    return $self->{tilematrix}->{$self->{leveltop}};
}

# method: getTileMatrixID
#  Return the tile matrix ID from the ascending resolution order (integer) :  
#   - 0 (bottom level, smallest resolution).
#   - NumberOfTM (top level, biggest resolution).
#  Hash levelIdx is used.
#---------------------------------------------------------------------------------
sub getTileMatrixID {
    my $self = shift;
    my $order= shift; 

    TRACE;

    foreach my $k (keys %{$self->{levelIdx}}) {
        if ($self->{levelIdx}->{$k} == $order) {return $k;}
    }

    return undef;
}

# method: getTileMatrixOrder
#  Return the tile matrix order from the ID :  
#   - 0 (bottom level, smallest resolution).
#   - NumberOfTM (top level, biggest resolution).
#  Hash levelIdx is used.
#---------------------------------------------------------------------------------
sub getTileMatrixOrder {
    my $self = shift;
    my $ID= shift; 

    TRACE;

    if (exists $self->{levelIdx}->{$ID}) {
        return $self->{levelIdx}->{$ID};
    } else {
        return undef;
    }
}

####################################################################################################
#                                           OTHERS                                                 #
####################################################################################################

# Group: others

sub to_string {
    my $self = shift;

    TRACE;

    printf "%s\n", $self->{srs};

    my $i = 0;

    while(defined (my $tm = $self->getTileMatrix($self->getTileMatrixID($i)))) {
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

=head1 SYNOPSIS

  use BE4::TileMatrixSet;
  
  my $filepath = "./t/data/tms/LAMB93_50cm_TEST.tms";
  my $objTMS = BE4::TileMatrixSet->new($filepath);


=head1 DESCRIPTION

    A TileMatrixSet object

        * PATHFILENAME
        * name
        * filename
        * filepath
        * srs
        * tilematrix (an hash of TileMatrix objects)

    A TileMatrixSet in XML format, in a file (.tms)

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

=head2 EXPORT

    None by default.

=head1 LIMITATIONS AND BUGS

    File name of tms must be with extension : tms or TMS !
    All levels must be continuous and unique !
    All levels are sorted by id !

=head1 SEE ALSO

    BE4::TileMatrix

=head1 AUTHOR

    Bazonnais Jean Philippe, E<lt>jpbazonnais@E<gt>

=head1 COPYRIGHT AND LICENSE

    Copyright (C) 2011 by Bazonnais Jean Philippe

    This library is free software; you can redistribute it and/or modify
    it under the same terms as Perl itself, either Perl version 5.10.1 or,
    at your option, any later version of Perl 5 you may have available.

=cut
