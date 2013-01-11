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
File: TileMatrixSet.pm

Class: BE4::TileMatrixSet

Load and store all information about a Tile Matrix Set. A Tile Matrix Set is a XML file which describe a grid for several levels.

(see TileMatrixSet.png)

We tell the difference between :
    - quad tree TMS : resolutions go by tows and borders are aligned. To generate a pyramid which is based on this kind of TMS, we use <QTree>
    (see QTreeTMS.png)
    - "nearest neighbour" TMS : centers are aligned (used for DTM generations, with a "nearest neighbour" interpolation). To generate a pyramid which is based on this kind of TMS, we use <Graph>
    (see NNGraphTMS.png)

Using:
    (start code)
    use BE4::TileMatrixSet;

    my $filepath = "/home/ign/tms/LAMB93_50cm.tms";
    my $objTMS = BE4::TileMatrixSet->new($filepath);

    $objTMS->getTileMatrixCount()};      # ie 19
    $objTMS->getTileMatrix(12);          # object TileMatrix with level id = 12
    $objTMS->getSRS();                   # ie 'IGNF:LAMB93'
    $objTMS->getName();                  # ie 'LAMB93_50cm'
    $objTMS->getFile();                  # ie 'LAMB93_50cm.tms'
    $objTMS->getPath();                  # ie '/home/ign/tms/'
    (end code)

Attributes:
    PATHFILENAME - string - Complete file path : /path/to/SRS_RES.tms
    name - string - Basename part of PATHFILENAME : SRS_RES
    filename - string - Filename part of PATHFILENAME : SRS_RES.tms
    filepath - string - Directory part of PATHFILENAME : /path/to

    levelsBind - hash - Link between Tile matrix identifiants (string) and order in ascending resolutions (integer).
    topID - string - Higher level ID.
    topResolution - double - Higher level resolution.
    bottomID - string - Lower level ID.
    bottomResolution - double - Lower level resolution.

    srs - string - Spatial Reference System, casted in uppercase (EPSG:4326).
    coordinatesInversion - boolean - Precise if we have to reverse coordinates to harvest in this SRS. For some SRS, we have to reverse coordinates when we compose WMS request (1.3.0). Used test to determine this SRSs is : if the SRS is geographic and an EPSG one.
    tileMatrix - <TileMatrix> hash - Keys are Tile Matrix identifiant, values are <TileMatrix> objects.
    isQTree - boolean - Precise if this TMS match with a quad tree. TRUE if this TMS describe a quad tree, FALSE otherwise.

Limitations:
    File name of tms must be with extension : tms or TMS.

    All levels must be continuous (QuadTree) and unique.

=cut

################################################################################

package BE4::TileMatrixSet;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
use XML::LibXML;

use Data::Dumper;
use Geo::OSR;

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

BEGIN {}
INIT {}
END {}

####################################################################################################
#                                        Group: Constructors                                       #
####################################################################################################

=begin nd
Constructor: new

TileMatrixSet constructor. Bless an instance. Fill file's informations.

Parameters (list):
    pathfile - string - Path to the Tile Matrix File (with extension .tms or .TMS)

See also:
    <_load>
=cut
sub new {
    my $this = shift;
    my $pathfile = shift;

    my $class= ref($this) || $this;
    # IMPORTANT : if modification, think to update natural documentation (just above)
    my $self = {
        PATHFILENAME => undef,
        name     => undef,
        filename => undef,
        filepath => undef,
        #
        levelsBind => undef,
        topID => undef,
        topResolution => undef,
        bottomID => undef,
        bottomResolution  => undef,
        #
        srs        => undef,
        coordinatesInversion  => FALSE,
        tileMatrix => {},
        #
        isQTree => undef,
    };

    bless($self, $class);

    TRACE;

    # init. class
    return undef if (! defined $pathfile);

    if (! -f $pathfile) {
      ERROR ("File TMS doesn't exist !");
      return undef;
    }

    # init. params
    $self->{PATHFILENAME} = $pathfile;
    $self->{filepath} = File::Basename::dirname($pathfile);
    $self->{filename} = File::Basename::basename($pathfile);
    $self->{name} = File::Basename::basename($pathfile);
    $self->{name} =~ s/\.(tms|TMS)$//;
    
    # load
    return undef if (! $self->_load());

    return $self;
}

=begin nd
Function: _load

Read and parse the Tile Matrix Set XML file to create a TileMatrix object for each level.

It determines if the TMS match with a quad tree:
    - resolutions go by twos between two contigues levels
    - top left corner coordinates and pixel dimensions are same for all levels

If TMS is not a quad tree, we have to determine the lower source level for each level (used for the genaration).

See also:
    <computeTmSource>
=cut
sub _load {
    my $self = shift;
    
    TRACE;
    
    # read xml pyramid
    my $parser  = XML::LibXML->new();
    my $xmltree =  eval { $parser->parse_file($self->{PATHFILENAME}); };

    if (! defined ($xmltree) || $@) {
        ERROR (sprintf "Can not read the XML file TMS : %s !", $@);
        return FALSE;
    }

    my $root = $xmltree->getDocumentElement;

    my @TMs = $root->getElementsByTagName('tileMatrix');
  
    # load tileMatrix
    foreach my $tm (@TMs) {
        # we identify level max (with the best resolution, the smallest) and level min (with the 
        # worst resolution, the biggest)
        my $id = $tm->findvalue('id');
        my $res = $tm->findvalue('resolution');
        
        if (! defined $self->{topID} || $res > $self->{topResolution}) {
            $self->{topID} = $id;
            $self->{topResolution} = $res;
        }
        if (! defined $self->{bottomID} || $res < $self->{bottomResolution}) {
            $self->{bottomID} = $id;
            $self->{bottomResolution} = $res;
        }
        
        my $objTM = BE4::TileMatrix->new({
            id => $id,
            resolution     => $res,
            topLeftCornerX => $tm->findvalue('topLeftCornerX'),
            topLeftCornerY => $tm->findvalue('topLeftCornerY'),
            tileWidth      => $tm->findvalue('tileWidth'),
            tileHeight     => $tm->findvalue('tileHeight'),
            matrixWidth    => $tm->findvalue('matrixWidth'),
            matrixHeight   => $tm->findvalue('matrixHeight'),
        });
        
        if (! defined $objTM) {
            ERROR(sprintf "Cannot create the TileMatrix object for the level '%s'",$id);
            return FALSE;
        }
        
        $self->{tileMatrix}->{$id} = $objTM;
        undef $objTM;
    }
    
    if (! $self->getCountTileMatrix()) {
        ERROR (sprintf "No tile matrix loading from XML file TMS !");
        return FALSE;
    }
    
    # srs (== crs)
    my $crs = $root->findnodes('crs');
    if (! defined $crs) {
        ERROR (sprintf "Can not determine parameter 'crs' in the XML file TMS !");
        return FALSE;
    }
    $self->{srs} = uc($crs); # srs is cast in uppercase in order to ease comparisons
    
    # Have coodinates to be reversed ?
    my $sr= new Geo::OSR::SpatialReference;
    eval { $sr->ImportFromProj4('+init='.$self->{srs}.' +wktext'); };
    if ($@) {
        eval { $sr->ImportFromProj4('+init='.lc($self->{srs}).' +wktext'); };
        if ($@) {
            ERROR("$@");
            ERROR (sprintf "Impossible to initialize the final spatial coordinate system (%s) to know if coordinates have to be reversed !\n",$self->{srs});
            return FALSE;
        }
    }

    my $authority = (split(":",$self->{srs}))[0];
    if ($sr->IsGeographic() && uc($authority) eq "EPSG") {
        INFO(sprintf "Coordinates will be reversed in requests (SRS : %s)",$self->{srs});
        $self->{coordinatesInversion} = TRUE;
    } else {
        INFO(sprintf "Coordinates order will be kept in requests (SRS : %s)",$self->{srs});
        $self->{coordinatesInversion} = FALSE;
    }
    
    # clean
    $xmltree = undef;
    
    # tileMatrix list sort by resolution
    my @tmList = $self->getTileMatrixByArray();
  
    # Is TMS a QuadTree ? If not, we use a graph (less efficient for calculs)
    $self->{isQTree} = TRUE; # default value
    if (scalar(@tmList) != 1) {
        my $epsilon = $tmList[0]->getResolution / 100 ;
        for (my $i = 0; $i < scalar(@tmList) - 1;$i++) {
            if ( abs($tmList[$i]->getResolution*2 - $tmList[$i+1]->getResolution) > $epsilon ) {
                $self->{isQTree} = FALSE;
                INFO(sprintf "Not a QTree : resolutions don't go by twos : level '%s' (%s) and level '%s' (%s).",
                    $tmList[$i]->{id},$tmList[$i]->getResolution,
                    $tmList[$i+1]->{id},$tmList[$i+1]->getResolution);
                last;
            }
            elsif ( abs($tmList[$i]->getTopLeftCornerX - $tmList[$i+1]->getTopLeftCornerX) > $epsilon ) {
                $self->{isQTree} = FALSE;
                ERROR(sprintf "Not a QTree : 'topleftcornerx' is not the same for all levels : level '%s' (%s) and level '%s' (%s).",
                    $tmList[$i]->{id},$tmList[$i]->getTopLeftCornerX,
                    $tmList[$i+1]->{id},$tmList[$i+1]->getTopLeftCornerX);
                last;
            }
            elsif ( abs($tmList[$i]->getTopLeftCornerY - $tmList[$i+1]->getTopLeftCornerY) > $epsilon ) {
                $self->{isQTree} = FALSE;
                ERROR(sprintf "Not a QTree : 'topleftcornery' is not the same for all levels : level '%s' (%s) and level '%s' (%s).",
                    $tmList[$i]->{id},$tmList[$i]->getTopLeftCornerY,
                    $tmList[$i+1]->{id},$tmList[$i+1]->getTopLeftCornerY);
                last;
            }
            elsif ( $tmList[$i]->getTileWidth != $tmList[$i+1]->getTileWidth) {
                $self->{isQTree} = FALSE;
                ERROR(sprintf "Not a QTree : 'tilewidth' is not the same for all levels : level '%s' (%s) and level '%s' (%s).",
                    $tmList[$i]->{id},$tmList[$i]->getTileWidth,
                    $tmList[$i+1]->{id},$tmList[$i+1]->getTileWidth);
                last;
            }
            elsif ( $tmList[$i]->getTileHeight != $tmList[$i+1]->getTileHeight) {
                $self->{isQTree} = FALSE;
                INFO(sprintf "Not a QTree : 'tileheight' is not the same for all levels : level '%s' (%s) and level '%s' (%s).",
                    $tmList[$i]->{id},$tmList[$i]->getTileHeight,
                    $tmList[$i+1]->{id},$tmList[$i+1]->getTileHeight);
                last;
            }
        };
    };
  
    # on fait un hash pour retrouver l'ordre d'un niveau a partir de son id.
    for (my $i=0; $i < scalar @tmList; $i++){
        $self->{levelsBind}{$tmList[$i]->getID()} = $i;
    }
    
    if ($self->isQTree) { return TRUE;}
    
    ## Adding informations about child/parent in TM objects
    for (my $i = 0; $i < scalar(@tmList) ;$i++) {
        if (! $self->computeTmSource($tmList[$i])) {
            ERROR(sprintf "Nor a QTree neither a Graph made for nearest neighbour generation. No source for level %s.",$tmList[$i]->getID());
            return FALSE;
        }
    }
    
    return TRUE;
}

####################################################################################################
#                                Group: Getters - Setters                                          #
####################################################################################################

# Function: getPathFilename
sub getPathFilename {
    my $self = shift;
    return $self->{PATHFILENAME};
}

# Function: getSRS
sub getSRS {
  my $self = shift;
  return $self->{srs};
}

# Function: getInversion
sub getInversion {
  my $self = shift;
  return $self->{coordinatesInversion};
}

# Function: getName
sub getName {
  my $self = shift;
  return $self->{name};
}

# Function: getPath
sub getPath {
  my $self = shift;
  return $self->{filepath};
}

# Function: getFile
sub getFile {
  my $self = shift;
  return $self->{filename};
}

# Function: getTopLevel
sub getTopLevel {
  my $self = shift;
  return $self->{topID};
}

# Function: getLevelsBind
sub getLevelsBind {
  my $self = shift;
  return $self->{levelsBind};
}

# Function: getBottomLevel
sub getBottomLevel {
  my $self = shift;
  return $self->{bottomID};
}

# Function: getTopResolution
sub getTopResolution {
  my $self = shift;
  return $self->{topResolution};
}

# Function: getBottomResolution
sub getBottomResolution {
  my $self = shift;
  return $self->{bottomResolution};
}

=begin nd
Function: getTileWidth

Parameters (list):
    ID - string - Level identifiant whose tile pixel width we want.
=cut
sub getTileWidth {
  my $self = shift;
  my $levelID = shift;
  
  $levelID = $self->{bottomID} if (! defined $levelID);
  
  # size of tile in pixel !
  return $self->{tileMatrix}->{$levelID}->getTileWidth;
}

=begin nd
Function: getTileHeight

Parameters (list):
    ID - string - Level identifiant whose tile pixel height we want.
=cut
sub getTileHeight {
  my $self = shift;
  my $ID = shift;
  
  $ID = $self->{bottomID} if (! defined $ID);
  
  # size of tile in pixel !
  return $self->{tileMatrix}->{$ID}->getTileHeight;
}

# Function: isQTree
sub isQTree {
    my $self = shift;
    return $self->{isQTree};
}

=begin nd
Function: getTileMatrixByArray

Returns the tile matrix array in the ascending resolution order.
=cut
sub getTileMatrixByArray {
    my $self = shift;

    my @levels;

    foreach my $k (sort {$a->getResolution() <=> $b->getResolution()} (values %{$self->{tileMatrix}})) {
        push @levels, $k;
    }

    return @levels;
}

=begin nd
Function: getTileMatrix

Returns the tile matrix from the supplied ID. This ID is the TMS ID (string) and not the ascending resolution order (integer). Returns undef if ID is undefined or if asked level doesn't exist.

Parameters (list):
    ID - string - Wanted level identifiant
=cut
sub getTileMatrix {
    my $self = shift;
    my $ID = shift;

    if (! defined $ID || ! exists($self->{tileMatrix}->{$ID})) {
        return undef;
    }

    return $self->{tileMatrix}->{$ID};
}

=begin nd
Function: getCountTileMatrix

Returns the count of tile matrix in the TMS.
=cut
sub getCountTileMatrix {
    my $self = shift;
    return scalar (keys %{$self->{tileMatrix}});
}

=begin nd
Function: getIDfromOrder

Returns the tile matrix ID from the ascending resolution order (integer).
    - 0 (bottom level, smallest resolution)
    - NumberOfTM-1 (top level, biggest resolution).

Parameters (list):
    order - integer - Level order, whose identifiant we want.
=cut
sub getIDfromOrder {
    my $self = shift;
    my $order= shift;

    TRACE;

    foreach my $k (keys %{$self->{levelsBind}}) {
        if ($self->{levelsBind}->{$k} == $order) {return $k;}
    }

    return undef;
}

=begin nd
Function: getBelowLevelID

Returns the tile matrix ID below the given tile matrix (ID).

Parameters (list):
    ID - string - Level identifiant, whose below level ID we want.
=cut
sub getBelowLevelID {
    my $self = shift;
    my $ID= shift;

    TRACE;

    return undef if (! exists $self->{levelsBind}->{$ID});
    my $order = $self->{levelsBind}->{$ID};
    return undef if ($order == 0);
    return $self->getIDfromOrder($order-1);
}

=begin nd
Function: getOrderfromID

Returns the tile matrix order from the ID.
    - 0 (bottom level, smallest resolution)
    - NumberOfTM-1 (top level, biggest resolution).

Parameters (list):
    ID - string - Level identifiant, whose order we want.
=cut
sub getOrderfromID {
    my $self = shift;
    my $ID= shift;

    TRACE;

    if (exists $self->{levelsBind}->{$ID}) {
        return $self->{levelsBind}->{$ID};
    } else {
        return undef;
    }
}

####################################################################################################
#                             Group: Tile Matrix manager                                           #
####################################################################################################

=begin nd
Function: computeTmSource

Defines the tile matrix for the provided one. This method is only used for "nearest neighbour" TMS (Pixels between different level have the same centre).

Parameters (list):
    tmTarget - <TileMatrix> - Tile matrix whose source tile matrix we want to know.

Returns:
    FALSE if there is no TM source for TM target (unless TM target is BotttomTM) 
    TRUE if there is a TM source (obj) for the TM target (obj) in argument.
=cut
sub computeTmSource {
  my $self = shift;
  my $tmTarget = shift;
  
  if ($tmTarget->{id} eq $self->{bottomID}) {
    return TRUE;
  }

  # The TM to be used to compute images in TM Parent
  my $tmSource = undef;
  
  # position du pixel en haut à gauche
  my $xTopLeftCorner_CenterPixel = $tmTarget->getTopLeftCornerX() + 0.5 * $tmTarget->getResolution();
  my $yTopLeftCorner_CenterPixel = $tmTarget->getTopLeftCornerY() - 0.5 * $tmTarget->getResolution();

  for (my $i = $self->getOrderfromID($tmTarget->getID()) - 1; $i >= $self->getOrderfromID($self->getBottomLevel) ;$i--) {
      my $potentialTmSource = $self->getTileMatrix($self->getIDfromOrder($i));
      # la précision vaut 1/100 de la plus petit résolution du TMS
      my $epsilon = $self->getTileMatrix($self->getBottomLevel())->getResolution() / 100;
      my $rapport = $tmTarget->getResolution() / $potentialTmSource->getResolution() ;
      #on veut que le rapport soit (proche d') un entier
      next if ( abs( int( $rapport + 0.5) - $rapport) > $epsilon );
      # on veut que les pixels soient superposables (pour faire une interpolation nn)
      # on regarde le pixel en haut à gauche de tmtarget
      # on verfie qu'il y a bien un pixel correspondant dans tmpotentialsource
      my $potentialTm_xTopLeftCorner_CenterPixel = $potentialTmSource->getTopLeftCornerX() + 0.5 * $potentialTmSource->getResolution() ;
      next if (abs($xTopLeftCorner_CenterPixel - $potentialTm_xTopLeftCorner_CenterPixel) > $epsilon );
      my $potentialTm_yTopLeftCorner_CenterPixel = $potentialTmSource->getTopLeftCornerY() - 0.5 * $potentialTmSource->getResolution() ;
      next if (abs($yTopLeftCorner_CenterPixel - $potentialTm_yTopLeftCorner_CenterPixel) > $epsilon );
      $tmSource = $potentialTmSource;
      last;
  }
  
  # si on n'a rien trouvé, on sort en erreur
  if (!  defined $tmSource) {
     return FALSE;
  }
  
  $tmSource->addTargetTm($tmTarget);
  
  return TRUE;
}

####################################################################################################
#                                Group: Export methods                                             #
####################################################################################################

=begin nd
Function: exportForDebug

Returns all informations about the tile matrix set. Useful for debug.

Example:
    (start code)
    (end code)
=cut
sub exportForDebug {
    my $self = shift ;

    my $export = "";

    $export .= "\nObject BE4::TileMatrixSet :\n";
    $export .= sprintf "\t TMS file complete path : %s\n", $self->getPathFilename;
    $export .= sprintf "\t Top level identifiant : %s\n", $self->getTopLevel;
    $export .= sprintf "\t Top level resolution : %s\n", $self->getTopResolution;
    $export .= sprintf "\t Bottom level identifiant : %s\n", $self->getBottomLevel;
    $export .= sprintf "\t Bottom level resolution : %s\n", $self->getBottomResolution;
    
    $export .= sprintf "\t Spatial Reference System : %s\n", $self->getSRS;
    if ( $self->getInversion() ) {
        $export .= sprintf "\t Coordinates have to be inversed to harvest with WMS 1.3.0\n";
    } else {
        $export .= sprintf "\t Coordinates have not to be inversed to harvest with WMS 1.3.0\n";
    }

    if ( $self->isQTree() ) {
        $export .= sprintf "\t This TMS is a quad tree";
    } else {
        $export .= sprintf "\t This TMS is not a quad tree";
    }


    $export .= sprintf "\t levelsBind hash :\n";
    $export .= sprintf "\t\t   ID   |  Order\n";
    $export .= sprintf "\t\t--------+--------\n";
    my %levelsBind = %{$self->{levelsBind}};
    foreach my $key (keys %levelsBind ) {
        $export .= sprintf "\t\t %6s | %-6s .\n",$key,$levelsBind{$key};
    }
    $export .= sprintf "\t TileMatrix Array :\n";
    $export .= sprintf "\t\t   ID   |  Resolution\n";
    $export .= sprintf "\t\t--------+------------------\n";
    foreach my $tm ( $self->getTileMatrixByArray ) {
        $export .= sprintf "\t\t %6s | %-.14d .\n",$tm->getID(),$tm->getResolution();
    }
    
    return $export;
}

1;
__END__

=begin nd

Group: Details

Details about TMS file.

Sample TMS file (LAMB93_50cm.tms):
    (start code)
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
    (end code)

=cut

