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
# Constantes
use constant TRUE  => 1;
use constant FALSE => 0;

################################################################################
=begin nd
Group: variable

variable: $self
    * PATHFILENAME => undef,
    * name     => undef,
    * filename => undef,
    * filepath => undef,
    * levelIdx => undef, # hash binding Tile matrix identifiants (string) and order (integer) in ascending resolutions
    * leveltop => undef,
    * resworst => undef,
    * levelbottom => undef,
    * resbest  => undef,
    * srs => undef, # srs is casted in uppercase
    * tilematrix => {}, # an hash of TileMatrix objects
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
        
        my $objTM = BE4::TileMatrix->new({
            id => $k,
            resolution     => $v->{resolution},
            topleftcornerx => $v->{topLeftCornerX},
            topleftcornery => $v->{topLeftCornerY},
            tilewidth      => $v->{tileWidth},
            tileheight     => $v->{tileHeight},
            matrixwidth    => $v->{matrixWidth},
            matrixheight   => $v->{matrixHeight},
        });
        
        if (! defined $objTM) {
            ERROR(sprintf "Cannot create the TileMatrix object for thye level '%s'",$k);
            return FALSE;
        }
        
        $self->{tilematrix}->{$k} = $objTM;
        undef $objTM;
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
  
    # Is TMS a QuadTree ? If not, we don't know how to do -> ERROR
    
    if (scalar(@tmList) != 1) {
        my $epsilon = $tmList[0]->{resolution} / 100 ;
        for (my $i = 0; $i < scalar(@tmList) - 1;$i++) {
            if ( abs($tmList[$i]->{resolution}*2 - $tmList[$i+1]->{resolution}) > $epsilon ) {
                ERROR(sprintf "Resolutions have to go by twos : level '%s' (%s) and level '%s' (%s) are not valid",
                    $tmList[$i]->{id},$tmList[$i+1]->{resolution},
                    $tmList[$i]->{id},$tmList[$i+1]->{resolution});
                return FALSE;
            }
            if ( abs($tmList[$i]->{topLeftCornerX} - $tmList[$i+1]->{topLeftCornerX}) > $epsilon ) {
                ERROR(sprintf "'topLeftCornerX' have to be the same for all levels : level '%s' (%s) and level '%s' (%s) are not valid",
                    $tmList[$i]->{id},$tmList[$i+1]->{topLeftCornerX},
                    $tmList[$i]->{id},$tmList[$i+1]->{topLeftCornerX});
                return FALSE;
            }
            if ( abs($tmList[$i]->{topLeftCornerY} - $tmList[$i+1]->{topLeftCornerY}) > $epsilon ) {
                ERROR(sprintf "'topLeftCornerY' have to be the same for all levels : level '%s' (%s) and level '%s' (%s) are not valid",
                    $tmList[$i]->{id},$tmList[$i+1]->{topLeftCornerY},
                    $tmList[$i]->{id},$tmList[$i+1]->{topLeftCornerY});
                return FALSE;
            }
            if ( $tmList[$i]->{tileWidth} != $tmList[$i+1]->{tileWidth}) {
                ERROR(sprintf "'tileWidth' have to be the same for all levels : level '%s' (%s) and level '%s' (%s) are not valid",
                    $tmList[$i]->{id},$tmList[$i+1]->{tileWidth},
                    $tmList[$i]->{id},$tmList[$i+1]->{tileWidth});
                return FALSE;
            }
            if ( $tmList[$i]->{tileHeight} != $tmList[$i+1]->{tileHeight}) {
                ERROR(sprintf "'tileHeight' have to be the same for all levels : level '%s' (%s) and level '%s' (%s) are not valid",
                    $tmList[$i]->{id},$tmList[$i+1]->{tileHeight},
                    $tmList[$i]->{id},$tmList[$i+1]->{tileHeight});
                return FALSE;
            }
        };
    };
  
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
# TileWidth TileHeight
sub getTileWidth {
  my $self = shift;
  my $levelID = shift;
  
  $levelID = $self->{levelbottom} if (! defined $levelID);
  
  # size of tile in pixel !
  return $self->{tilematrix}->{$levelID}->{tilewidth};
}
sub getTileHeight {
  my $self = shift;
  my $levelID = shift;
  
  $levelID = $self->{levelbottom} if (! defined $levelID);
  
  # size of tile in pixel !
  return $self->{tilematrix}->{$self->{levelbottom}}->{tileheight};
}

####################################################################################################
#                                    TILE MATRIX MANAGER                                           #
####################################################################################################

# Group: tile matrix manager

#
=begin nd
method: getTileMatrixByArray

Returns:
    The tile matrix array in the ascending resolution order.
=cut
sub getTileMatrixByArray {
    my $self = shift;

    TRACE("sort by Resolution...");
    
    my @levels;

    foreach my $k (sort {$a->getResolution() <=> $b->getResolution()} (values %{$self->{tilematrix}})) {
        push @levels, $k;
    }

    return @levels;
}

#
=begin nd
method: getTileMatrix

Returns:
    The tile matrix from the supplied ID. This ID is the TMS ID (string) and not the ascending resolution order (integer).
=cut
sub getTileMatrix {
  my $self = shift;
  my $level= shift; # id !
  
  if (! defined $level) {
    return undef;
  }
  
  return undef if (! exists($self->{tilematrix}->{$level}));
  
  return $self->{tilematrix}->{$level};
}

#
=begin nd
method: getCountTileMatrix

Returns:
    The count of tile matrix in the TMS.
=cut
sub getCountTileMatrix {
  my $self = shift;

  return scalar (keys %{$self->{tilematrix}});
}

#
=begin nd
method: getTileMatrixID

Return the tile matrix ID from the ascending resolution order (integer).  
    - 0 (bottom level, smallest resolution)
    - NumberOfTM (top level, biggest resolution).

=cut
sub getTileMatrixID {
    my $self = shift;
    my $order= shift; 

    TRACE;

    foreach my $k (keys %{$self->{levelIdx}}) {
        if ($self->{levelIdx}->{$k} == $order) {return $k;}
    }

    return undef;
}

#
=begin nd
method: getBelowTileMatrixID

Return:
    The tile matrix ID below the given tile matrix (ID).
=cut
sub getBelowTileMatrixID {
    my $self = shift;
    my $ID= shift; 

    TRACE;
    
    return undef if (! exists $self->{levelIdx}->{$ID});
    my $order = $self->{levelIdx}->{$ID};
    return $self->getTileMatrixOrder($order-1);
}

#
=begin nd
method: getTileMatrixOrder

Return the tile matrix order from the ID.
    - 0 (bottom level, smallest resolution)
    - NumberOfTM (top level, biggest resolution).
=cut
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

=head1 NAME

BE4::TileMatrixSet - load a file tilematrixset.

=head1 SYNOPSIS

    use BE4::TileMatrixSet;
    
    my $filepath = "/home/ign/tms/LAMB93_50cm.tms";
    my $objTMS = BE4::TileMatrixSet->new($filepath);
    
    $objTMS->getTileMatrixCount()};  # ie 19
    $objTMS->getTileMatrix(12);      # object TileMatrix with level id = 12
    $objTMS->getSRS();               # ie 'IGNF:LAMB93'
    $objTMS->getName();              # ie 'LAMB93_50cm'
    $objTMS->getFile();              # ie 'LAMB93_50cm.tms'
    $objTMS->getPath();              # ie '/home/ign/tms/'

=head1 DESCRIPTION

=head2 ATTRIBUTES

=over 4

=item PATHFILENAME

Complete file path : /path/to/SRS_RES.tms

=item name, filename, filepath

Split PATHFILENAME : name : SRS_RES, filename : SRS_RES.tms, filepath : /path/to.

=item levelIdx

Hash binding Tile matrix identifiant (string) and order (integer) in ascending resolutions.

=item leveltop, resworst

Higher level ID and its resolution.

=item levelbottom, resbest

Lower level ID and its resolution.

=item srs

=item tilematrix

Hash of TileMatrix objects : levelID => objTileMatrix

=back

=head1 SAMPLE

Sample TMS file (F<LAMB93_50cm.tms>)

    <tileMatrixSet>
	<crs>IGNF:LAMB93</crs>
	<tileMatrix>
            <id>0</id>
            <resolution>131072</resolution>
            <topLeftCornerX> 0 </topLeftCornerX>
            <topLeftCornerY> 12000000 </topLeftCornerY>
            <tileWidth>256</tileWidth>
            <tileHeight>256</tileHeight>
            <matrixWidth>1</matrixWidth>
            <matrixHeight>1</matrixHeight>
	</tileMatrix>
	<tileMatrix>
            <id>1</id>
            <resolution>65536</resolution>
            <topLeftCornerX> 0 </topLeftCornerX>
            <topLeftCornerY> 12000000 </topLeftCornerY>
            <tileWidth>256</tileWidth>
            <tileHeight>256</tileHeight>
            <matrixWidth>1</matrixWidth>
            <matrixHeight>1</matrixHeight>
	</tileMatrix>
        .
        .
        .
        <tileMatrix>
            <id>17</id>
            <resolution>1</resolution>
            <topLeftCornerX> 0 </topLeftCornerX>
            <topLeftCornerY> 12000000 </topLeftCornerY>
            <tileWidth>256</tileWidth>
            <tileHeight>256</tileHeight>
            <matrixWidth>5040</matrixWidth>
            <matrixHeight>42040</matrixHeight>
        </tileMatrix>
	<tileMatrix>
            <id>18</id>
            <resolution>0.5</resolution>
            <topLeftCornerX> 0 </topLeftCornerX>
            <topLeftCornerY> 12000000 </topLeftCornerY>
            <tileWidth>256</tileWidth>
            <tileHeight>256</tileHeight>
            <matrixWidth>10080</matrixWidth>
            <matrixHeight>84081</matrixHeight>
        </tileMatrix>
    </tileMatrixSet>

=head1 LIMITATIONS AND BUGS

File name of tms must be with extension : tms or TMS.

All levels must be continuous (QuadTree) and unique.

=head1 SEE ALSO

=head2 POD documentation

=begin html

<ul>
<li><A HREF="./lib-BE4-TileMatrix.html">BE4::TileMatrix</A></li>
</ul>

=end html

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
