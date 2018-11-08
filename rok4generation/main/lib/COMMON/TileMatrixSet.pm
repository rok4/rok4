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

Class: COMMON::TileMatrixSet

Load and store all information about a Tile Matrix Set. A Tile Matrix Set is a XML file which describe a grid for several levels.

(see TileMatrixSet.png)

We tell the difference between :
    - quad tree TMS : resolutions go by tows and borders are aligned. To generate a pyramid which is based on this kind of TMS, we use <QTree>
    (see QTreeTMS.png)
    - "nearest neighbour" TMS : centers are aligned (used for DTM generations, with a "nearest neighbour" interpolation). To generate a pyramid which is based on this kind of TMS, we use <Graph>
    (see NNGraphTMS.png)

Using:
    (start code)
    use COMMON::TileMatrixSet;

    my $filepath = "/home/ign/tms/LAMB93_50cm.tms";
    my $objTMS = COMMON::TileMatrixSet->new($filepath);

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

    levelsBind - hash - Link between Tile matrix identifiants (string, the key) and order in ascending resolutions (integer, the value).
    topID - string - Higher level ID.
    topResolution - double - Higher level resolution.
    bottomID - string - Lower level ID.
    bottomResolution - double - Lower level resolution.

    srs - string - Spatial Reference System, casted in uppercase (EPSG:4326).
    coordinatesInversion - boolean - Precise if we have to reverse coordinates to harvest in this SRS. For some SRS, we have to reverse coordinates when we compose WMS request (1.3.0). Used test to determine this SRSs is : if the SRS is geographic and an EPSG one.
    tileMatrix - <COMMON::TileMatrix> hash - Keys are Tile Matrix identifiant, values are <TileMatrix> objects.
    type - string - Type of TMS, QTREE or NNGRAPH

Limitations:
    File name of tms must be with extension : tms or TMS.

    All levels must be continuous (QuadTree) and unique.

=cut

################################################################################

package COMMON::TileMatrixSet;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
use XML::LibXML;

use Data::Dumper;

use COMMON::TileMatrix;
use COMMON::ProxyGDAL;

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
    acceptUntypedTMS - boolean - optional : Do we accept a TMS that is neither a quad tree nor a neares neighbour graph (default : refused)

See also:
    <_load>
=cut
sub new {
    my $class = shift;
    my $pathfile = shift;
    my $acceptUntypedTMS = shift;

    $class = ref($class) || $class;
    # IMPORTANT : if modification, think to update natural documentation (just above)
    my $this = {
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
        type => undef
    };

    bless($this, $class);


    # init. class
    return undef if (! defined $pathfile);

    if (! -f $pathfile) {
        ERROR ("File TMS doesn't exist ($pathfile)!");
        return undef;
    }

    # init. params
    $this->{PATHFILENAME} = $pathfile;
    $this->{filepath} = File::Basename::dirname($pathfile);
    $this->{filename} = File::Basename::basename($pathfile);
    $this->{name} = File::Basename::basename($pathfile);
    $this->{name} =~ s/\.(tms|TMS)$//;
    
    # load
    return undef if (! $this->_load($acceptUntypedTMS));

    return $this;
}

=begin nd
Function: _load

Read and parse the Tile Matrix Set XML file to create a TileMatrix object for each level.

It determines if the TMS match with a quad tree:
    - resolutions go by twos between two contigues levels
    - top left corner coordinates and pixel dimensions are same for all levels

If TMS is a nearest neighbour graph, we have to determine the lower source level for each level (used for the generation).

Parameters (list):
    acceptUntypedTMS - boolean - optional : Do we accept a TMS that is neither a quad tree nor a neares neighbour graph (default : refused)

See also:
    <computeTmSource>
=cut
sub _load {
    my $this = shift;
    my $acceptUntypedTMS = shift;
    
    
    # read xml pyramid
    my $parser  = XML::LibXML->new();
    my $xmltree =  eval { $parser->parse_file($this->{PATHFILENAME}); };

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
        
        if (! defined $this->{topID} || $res > $this->{topResolution}) {
            $this->{topID} = $id;
            $this->{topResolution} = $res;
        }
        if (! defined $this->{bottomID} || $res < $this->{bottomResolution}) {
            $this->{bottomID} = $id;
            $this->{bottomResolution} = $res;
        }
        
        my $objTM = COMMON::TileMatrix->new({
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
        
        $this->{tileMatrix}->{$id} = $objTM;
        $objTM->setTMS($this);
        undef $objTM;
    }
    
    if (! $this->getCountTileMatrix()) {
        ERROR (sprintf "No tile matrix loading from XML file TMS !");
        return FALSE;
    }
    
    # srs (== crs)
    my $crs = $root->findnodes('crs');
    if (! defined $crs) {
        ERROR (sprintf "Can not determine parameter 'crs' in the XML file TMS !");
        return FALSE;
    }
    $this->{srs} = uc($crs); # srs is cast in uppercase in order to ease comparisons
    
    # Have coodinates to be reversed ?
    my $sr = COMMON::ProxyGDAL::spatialReferenceFromSRS($this->{srs});
    if (! defined $sr) {
        ERROR (sprintf "Impossible to initialize the final spatial coordinate system (%s) to know if coordinates have to be reversed !\n",$this->{srs});
        return FALSE;
    }

    my $authority = (split(":",$this->{srs}))[0];
    if (COMMON::ProxyGDAL::isGeographic($sr) && uc($authority) eq "EPSG") {
        DEBUG(sprintf "Coordinates will be reversed in requests (SRS : %s)",$this->{srs});
        $this->{coordinatesInversion} = TRUE;
    } else {
        DEBUG(sprintf "Coordinates order will be kept in requests (SRS : %s)",$this->{srs});
        $this->{coordinatesInversion} = FALSE;
    }
    
    # clean
    $xmltree = undef;
    
    # tileMatrix list sort by resolution
    my @tmList = $this->getTileMatrixByArray();
        
    # Is TMS a QuadTree ? If not, we use a graph (less efficient for calculs)
    $this->{type} = "QTREE"; # default value
    if (scalar(@tmList) != 1) {
        my $epsilon = $tmList[0]->getResolution / 100 ;
        for (my $i = 0; $i < scalar(@tmList) - 1;$i++) {
            if ( abs($tmList[$i]->getResolution*2 - $tmList[$i+1]->getResolution) > $epsilon ) {
                $this->{type} = "NONE";
                INFO(sprintf "Not a QTree : resolutions don't go by twos : level '%s' (%s) and level '%s' (%s).",
                    $tmList[$i]->{id},$tmList[$i]->getResolution,
                    $tmList[$i+1]->{id},$tmList[$i+1]->getResolution);
                last;
            }
            elsif ( abs($tmList[$i]->getTopLeftCornerX - $tmList[$i+1]->getTopLeftCornerX) > $epsilon ) {
                $this->{type} = "NONE";
                ERROR(sprintf "Not a QTree : 'topleftcornerx' is not the same for all levels : level '%s' (%s) and level '%s' (%s).",
                    $tmList[$i]->{id},$tmList[$i]->getTopLeftCornerX,
                    $tmList[$i+1]->{id},$tmList[$i+1]->getTopLeftCornerX);
                last;
            }
            elsif ( abs($tmList[$i]->getTopLeftCornerY - $tmList[$i+1]->getTopLeftCornerY) > $epsilon ) {
                $this->{type} = "NONE";
                ERROR(sprintf "Not a QTree : 'topleftcornery' is not the same for all levels : level '%s' (%s) and level '%s' (%s).",
                    $tmList[$i]->{id},$tmList[$i]->getTopLeftCornerY,
                    $tmList[$i+1]->{id},$tmList[$i+1]->getTopLeftCornerY);
                last;
            }
            elsif ( $tmList[$i]->getTileWidth != $tmList[$i+1]->getTileWidth) {
                $this->{type} = "NONE";
                ERROR(sprintf "Not a QTree : 'tilewidth' is not the same for all levels : level '%s' (%s) and level '%s' (%s).",
                    $tmList[$i]->{id},$tmList[$i]->getTileWidth,
                    $tmList[$i+1]->{id},$tmList[$i+1]->getTileWidth);
                last;
            }
            elsif ( $tmList[$i]->getTileHeight != $tmList[$i+1]->getTileHeight) {
                $this->{type} = "NONE";
                INFO(sprintf "Not a QTree : 'tileheight' is not the same for all levels : level '%s' (%s) and level '%s' (%s).",
                    $tmList[$i]->{id},$tmList[$i]->getTileHeight,
                    $tmList[$i+1]->{id},$tmList[$i+1]->getTileHeight);
                last;
            }
        };
    };
  
    # on fait un hash pour retrouver l'ordre d'un niveau a partir de son id.
    for (my $i=0; $i < scalar @tmList; $i++){
        $this->{levelsBind}{$tmList[$i]->getID()} = $i;
    }
    


    if ($this->{type} eq "QTREE") { return TRUE;}
    
    ## Adding informations about child/parent in TM objects
    for (my $i = 0; $i < scalar(@tmList) ;$i++) {
        if (! $this->computeTmSource($tmList[$i])) {
            if(defined $acceptUntypedTMS && $acceptUntypedTMS) {
                return TRUE;
            } else {
                ERROR(sprintf "Nor a QTree neither a Graph made for nearest neighbour generation. No source for level %s.",$tmList[$i]->getID());
                return FALSE;
            }
        }
    }

    $this->{type} = "NNGRAPH";
    
    return TRUE;
}

####################################################################################################
#                                Group: Getters - Setters                                          #
####################################################################################################

# Function: getPathFilename
sub getPathFilename {
    my $this = shift;
    return $this->{PATHFILENAME};
}

# Function: getSRS
sub getSRS {
  my $this = shift;
  return $this->{srs};
}

# Function: getInversion
sub getInversion {
  my $this = shift;
  return $this->{coordinatesInversion};
}

# Function: getName
sub getName {
  my $this = shift;
  return $this->{name};
}

# Function: getPath
sub getPath {
  my $this = shift;
  return $this->{filepath};
}

# Function: getFile
sub getFile {
  my $this = shift;
  return $this->{filename};
}

# Function: getTopLevel
sub getTopLevel {
  my $this = shift;
  return $this->{topID};
}

# Function: getBottomLevel
sub getBottomLevel {
  my $this = shift;
  return $this->{bottomID};
}

# Function: getTopResolution
sub getTopResolution {
  my $this = shift;
  return $this->{topResolution};
}

# Function: getBottomResolution
sub getBottomResolution {
  my $this = shift;
  return $this->{bottomResolution};
}

=begin nd
Function: getTileWidth

Parameters (list):
    ID - string - Level identifiant whose tile pixel width we want.
=cut
sub getTileWidth {
  my $this = shift;
  my $levelID = shift;
  
  $levelID = $this->{bottomID} if (! defined $levelID);
  
  # size of tile in pixel !
  return $this->{tileMatrix}->{$levelID}->getTileWidth;
}

=begin nd
Function: getTileHeight

Parameters (list):
    ID - string - Level identifiant whose tile pixel height we want.
=cut
sub getTileHeight {
  my $this = shift;
  my $ID = shift;
  
  $ID = $this->{bottomID} if (! defined $ID);
  
  # size of tile in pixel !
  return $this->{tileMatrix}->{$ID}->getTileHeight;
}

# Function: isQTree
sub isQTree {
    my $this = shift;
    return ($this->{type} eq "QTREE");
}

=begin nd
Function: getTileMatrixByArray

Returns the tile matrix array in the ascending resolution order.
=cut
sub getTileMatrixByArray {
    my $this = shift;

    my @levels;

    foreach my $k (sort {$a->getResolution() <=> $b->getResolution()} (values %{$this->{tileMatrix}})) {
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
    my $this = shift;
    my $ID = shift;

    if (! defined $ID || ! exists($this->{tileMatrix}->{$ID})) {
        return undef;
    }

    return $this->{tileMatrix}->{$ID};
}

=begin nd
Function: getCountTileMatrix

Returns the count of tile matrix in the TMS.
=cut
sub getCountTileMatrix {
    my $this = shift;
    return scalar (keys %{$this->{tileMatrix}});
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
    my $this = shift;
    my $order= shift;


    foreach my $k (keys %{$this->{levelsBind}}) {
        if ($this->{levelsBind}->{$k} == $order) {return $k;}
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
    my $this = shift;
    my $ID= shift;


    return undef if (! exists $this->{levelsBind}->{$ID});
    my $order = $this->{levelsBind}->{$ID};
    return undef if ($order == 0);
    return $this->getIDfromOrder($order-1);
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
    my $this = shift;
    my $ID= shift;


    if (exists $this->{levelsBind}->{$ID}) {
        return $this->{levelsBind}->{$ID};
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
  my $this = shift;
  my $tmTarget = shift;
  
  if ($tmTarget->{id} eq $this->{bottomID}) {
    return TRUE;
  }

  # The TM to be used to compute images in TM Parent
  my $tmSource = undef;
  
  # position du pixel en haut à gauche
  my $xTopLeftCorner_CenterPixel = $tmTarget->getTopLeftCornerX() + 0.5 * $tmTarget->getResolution();
  my $yTopLeftCorner_CenterPixel = $tmTarget->getTopLeftCornerY() - 0.5 * $tmTarget->getResolution();

  for (my $i = $this->getOrderfromID($tmTarget->getID()) - 1; $i >= $this->getOrderfromID($this->getBottomLevel) ;$i--) {
      my $potentialTmSource = $this->getTileMatrix($this->getIDfromOrder($i));
      # la précision vaut 1/100 de la plus petit résolution du TMS
      my $epsilon = $this->getTileMatrix($this->getBottomLevel())->getResolution() / 100;
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
    Object COMMON::TileMatrixSet :
         TMS file complete path : /home/ign/TMS/LAMB93_10cm.tms
         Top level identifiant : 0
         Top level resolution : 209715.2
         Bottom level identifiant : 21
         Bottom level resolution : 0.1
         Spatial Reference System : IGNF:LAMB93
         Coordinates have not to be inversed to harvest with WMS 1.3.0
         This TMS is a quad tree
         TileMatrix Array :
                     ID    | Order |  Resolution
                -----------+-------+------------------
                        21 |  0    | 0.1
                        20 |  1    | 0.2
                        19 |  2    | 0.4
                        18 |  3    | 0.8
                        17 |  4    | 1.6
                        16 |  5    | 3.2
                        15 |  6    | 6.4
                        14 |  7    | 12.8
                        13 |  8    | 25.6
                        12 |  9    | 51.2
                        11 |  10   | 102.4
                        10 |  11   | 204.8
                         9 |  12   | 409.6
                         8 |  13   | 819.2
                         7 |  14   | 1638.4
                         6 |  15   | 3276.8
                         5 |  16   | 6553.6
                         4 |  17   | 13107.2
                         3 |  18   | 26214.4
                         2 |  19   | 52428.8
                         1 |  20   | 104857.6
                         0 |  21   | 209715.2
    (end code)
=cut
sub exportForDebug {
    my $this = shift ;

    my $export = "\nObject COMMON::TileMatrixSet :\n";
    $export .= sprintf "\t TMS file complete path : %s\n", $this->getPathFilename;
    $export .= sprintf "\t Top level identifiant : %s\n", $this->getTopLevel;
    $export .= sprintf "\t Top level resolution : %s\n", $this->getTopResolution;
    $export .= sprintf "\t Bottom level identifiant : %s\n", $this->getBottomLevel;
    $export .= sprintf "\t Bottom level resolution : %s\n", $this->getBottomResolution;
    
    $export .= sprintf "\t Spatial Reference System : %s\n", $this->getSRS;
    if ( $this->getInversion() ) {
        $export .= sprintf "\t Coordinates have to be inversed to harvest with WMS 1.3.0\n";
    } else {
        $export .= sprintf "\t Coordinates have not to be inversed to harvest with WMS 1.3.0\n";
    }

    if ( $this->isQTree() ) {
        $export .= sprintf "\t This TMS is a quad tree\n";
    } else {
        $export .= sprintf "\t This TMS is not a quad tree\n";
    }

    $export .= sprintf "\t TileMatrix Array :\n";
    $export .= sprintf "\t\t     ID    | Order |  Resolution\n";
    $export .= sprintf "\t\t-----------+-------+------------------\n";
    foreach my $tm ( $this->getTileMatrixByArray ) {
        my $id = $tm->getID();
        $export .= sprintf "\t\t %9s |  %-4s | %-14s \n", $id, $this->{levelsBind}->{$id}, $tm->getResolution();
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

