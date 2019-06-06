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
File: NNGraph.pm

Class: COMMON::NNGraph

(see ROK4GENERATION/libperlauto/COMMON_NNGraph.png)

Representation of a "nearest neighbour" pyramid : pyramid's image = <COMMON::Node>.

(see ROK4GENERATION/NNGraphTMS.png)

To generate this kind of graph, we use :
    - *jobNumber* scripts by level : to generate image to work format.
    - *jobNumber* scripts : to move image in the final pyramid, with the wanted format (compression...)

=> *jobNumber x (levelNumber + 1)* scripts

Organization in the <Forest> scripts' array :

(see ROK4GENERATION/script_NNGraph.png)

Link between a node and his children or his father is not trivial. It is calculated and store in the <COMMON::Node> object.

Using:
    (start code)
    use COMMON::NNGraph;

    # NNGraph object creation
    my $objNNGraph = COMMON::QTree->new($objForest, $objDataSource);

    ...

    # Fill each node with computing code
    $objNNGraph->computeYourself();
    (end code)

Attributes:
    forest - <COMMON::Forest> - Forest which this tree belong to.
    pyramid - <COMMON::PyramidRaster> - Pyramid linked to this tree.
    commands - <COMMON::ShellCommandsRaster> - Command to use to generate images.
    datasource - <COMMON::DataSource> - Data source to use to define bottom level nodes and generate them.

    ct_source_pyramid - <Geo::OSR::CoordinateTransformation> - Coordinate transformation from datasource srs to pyramid srs
    ct_pyramid_source - <Geo::OSR::CoordinateTransformation> - Coordinate transformation from pyramid srs to datasource srs

    bbox - double array - Datasource bbox, [xmin,ymin,xmax,ymax], in TMS' SRS
    nodes - <COMMON::Node> hash - Structure is:
        (start code)
        level1 => {
           c1_r2 => n1,
           c2_r2 => n2,
           c3_r2 => n3, ...}
        level2 => {
           c1_r2 => n4,
           c2_r2 => n5, ...}

        cX : node's column
        rX : node's row
        nX : <COMMON::Node>
        (end code)
        
    bottomID - string - Bottom level identifiant
    topID - string - Top level identifiant
=cut

################################################################################

package COMMON::NNGraph;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
use File::Basename;
use File::Path;
use Data::Dumper;

# My Module
use COMMON::DataSource;
use COMMON::Node;
use COMMON::ProxyGDAL;
use COMMON::Array;
use COMMON::ShellCommandsRaster;

use Log::Log4perl qw(:easy);

require Exporter;
use AutoLoader qw(AUTOLOAD);

our @ISA = qw(Exporter);

our %EXPORT_TAGS = ( 'all' => [ qw() ] );
our @EXPORT_OK   = ( @{$EXPORT_TAGS{'all'}} );
our @EXPORT      = qw();

################################################################################
# Constantes
# Booleans
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

NNGraph constructor. Bless an instance.

Parameters (list):
    objForest - <Forest> - Forest which this tree belong to
    objSrc - <DataSource> - Datasource which determine bottom level nodes

See also:
    <_init>, <_load>
=cut
sub new {
    my $class = shift;
    my $objForest = shift;
    my $objSrc = shift;

    $class = ref($class) || $class;
    # IMPORTANT : if modification, think to update natural documentation (just above) and pod documentation (bottom)
    my $this = {
        # in
        forest    => undef,
        pyramid    => undef,
        commands    => undef,
        datasource => undef,
        # ct
        ct_source_pyramid => undef,
        ct_pyramid_source => undef,
        # out
        bbox => [],
        nodes => {},
        bottomID => undef,
        topID    => undef,
    };

    bless($this, $class);

    # mandatory parameters !
    if (! defined $objForest || ref ($objForest) ne "COMMON::Forest") {
        ERROR("Can not load Forest !");
        return FALSE;
    }
    if (! defined $objSrc || ref ($objSrc) ne "COMMON::DataSource") {
        ERROR("Can not load DataSource !");
        return FALSE;
    }

    # init. params    
    $this->{forest} = $objForest; 
    $this->{pyramid} = $objForest->getPyramid();
    $this->{datasource} = $objSrc; 
    $this->{commands} = $objForest->getCommands(); 

    # load 
    return undef if (! $this->_load());

    return $this;
}

=begin nd
Function: _load

Determines all nodes from the bottom level to the top level, thanks to the data source.
=cut
sub _load {
    my $this = shift;

    # initialisation pratique:
    my $tms = $this->{pyramid}->getTileMatrixSet;
    my $src = $this->{datasource};
    my $tilesPerWidth = $this->{pyramid}->getTilesPerWidth();
    my $tilesPerHeight = $this->{pyramid}->getTilesPerHeight();
    
    # récupération d'information dans la source de données
    $this->{topID} = $this->{datasource}->getTopID;
    $this->{bottomID} = $this->{datasource}->getBottomID;

    # initialisation des transformations de coordonnées datasource <-> pyramide
    # Si les srs sont identiques on laisse undef.
    
    if ($tms->getSRS() ne $src->getSRS()){
        $this->{ct_source_pyramid} = COMMON::ProxyGDAL::coordinateTransformationFromSpatialReference($src->getSRS(), $tms->getSRS());
        if (! defined $this->{ct_source_pyramid}) {
            ERROR(sprintf "Cannot instanciate the coordinate transformation object %s -> %s", $src->getSRS(), $tms->getSRS());
            return FALSE;
        }
        $this->{ct_pyramid_source} = COMMON::ProxyGDAL::coordinateTransformationFromSpatialReference($tms->getSRS(), $src->getSRS());
        if (! defined $this->{ct_pyramid_source}) {
            ERROR(sprintf "Cannot instanciate the coordinate transformation object %s -> %s", $tms->getSRS(), $src->getSRS());
            return FALSE;
        }
    }

    # identifier les noeuds du niveau de base à mettre à jour et les associer aux images sources:
    if (! $this->identifyBottomNodes()) {
        ERROR(sprintf "Cannot determine bottom tiles for the level %s",$src->getBottomID);
        return FALSE;
    }

    INFO(sprintf "Number of cache images to the bottom level (%s) : %d",
         $this->{bottomID},scalar keys(%{$this->{nodes}{$this->{bottomID}}}));
    
    # identifier les noeuds des niveaux supérieurs
    if (! $this->identifyAboveNodes) {
        ERROR(sprintf "Cannot determine above levels' tiles.");
        return FALSE;
    }

    return TRUE;
}

####################################################################################################
#                          Group: Nodes determination methods                                      #
####################################################################################################

=begin nd
Function: identifyBottomNodes

Calculate all nodes in bottom level concerned by the datasource (tiles which touch the data source extent).
=cut
sub identifyBottomNodes {
    my $this = shift;
    
    my $bottomID = $this->{bottomID};
    my $tm = $this->{pyramid}->getTileMatrixSet->getTileMatrix($bottomID);
    if (! defined $tm) {
        ERROR(sprintf "Impossible de récupérer le TM à partir de %s (bottomID) et du TMS : %s.",$bottomID,$this->getPyramid()->getTileMatrixSet()->exportForDebug());
        return FALSE;
    };
    my $datasource = $this->{datasource};
    my ($TPW,$TPH) = ($this->{pyramid}->getTilesPerWidth,$this->{pyramid}->getTilesPerHeight);
    
    if ($datasource->hasImages) {
        # We have real data as source. Images determine bottom tiles
        my @images = $datasource->getImages();
        foreach my $objImg (@images){
            # On reprojette l'emprise si nécessaire
            my @bbox = COMMON::ProxyGDAL::convertBBox($this->{ct_source_pyramid}, $objImg->getBBox()); # (xMin, yMin, xMax, yMax)
            if ($bbox[0] == 0 && $bbox[2] == 0) {
                ERROR(sprintf "Impossible to compute BBOX for the image '%s'. Probably limits are reached !", $objImg->getName());
                return FALSE;
            }
            
            $this->updateBBox(@bbox);
            
            # On divise les coord par la taille des dalles de cache pour avoir les indices min et max en x et y
            my ($rowMin, $rowMax, $colMin, $colMax) = $tm->bboxToIndices(@bbox,$TPW,$TPH);
            
            for (my $col = $colMin; $col<= $colMax; $col++){
                for (my $row = $rowMin; $row<= $rowMax; $row++){
                    my $nodeKey = sprintf "%s_%s", $col, $row;

                    if ( $datasource->hasHarvesting() ) {
                        # we use WMS service to generate this leaf
                        if (exists $this->{nodes}->{$bottomID}->{$nodeKey}) {
                            # This Node already exists
                            next;
                        }
                        # Create a new Node
                        my $node = COMMON::Node->new({
                            col => $col,
                            row => $row,
                            tm => $tm,
                            graph => $this,
                            type => $this->{forest}->getStorageType()
                        });
                        if (! defined $node) { 
                            ERROR(sprintf "Cannot create Node for level %s, indices %s,%s.", $this->{bottomID}, $col, $row);
                            return FALSE;
                        }
                        $this->{nodes}->{$bottomID}->{$nodeKey} = $node;
                    } else {
                        # we use images to generate this leaf
                        if (! exists $this->{nodes}->{$bottomID}->{$nodeKey}) {

                            # Create a new Node
                            my $node = COMMON::Node->new({
                                col => $col,
                                row => $row,
                                tm => $tm,
                                graph => $this,
                                type => $this->{forest}->getStorageType()
                            });
                            if (! defined $node) { 
                                ERROR(sprintf "Cannot create Node for level %s, indices %s,%s.", $this->{bottomID}, $col, $row);
                                return FALSE;
                            }
                            
                            $this->{nodes}->{$bottomID}->{$nodeKey} = $node;
                        }

                        $this->{nodes}->{$bottomID}->{$nodeKey}->addGeoImages($objImg);
                    }
                }
            }
        }
    } elsif (defined $datasource->getExtent() ) {
        # We have just a WMS service as source. We use extent to determine bottom tiles
        my $convertExtent = COMMON::ProxyGDAL::getConvertedGeometry($datasource->getExtent(), $this->{ct_source_pyramid});
        if (! defined $convertExtent) {
            ERROR(sprintf "Cannot convert extent for the datasource");
            return FALSE;
        }

        # Pour éviter de balayer une bbox trop grande, on récupère la bbox de chaque partie de la - potentiellement multi - géométrie
        my $bboxes = COMMON::ProxyGDAL::getBboxes($convertExtent);

        foreach my $bb (@{$bboxes}) {
        
            $this->updateBBox(@{$bb});

            my ($rowMin, $rowMax, $colMin, $colMax) = $tm->bboxToIndices(@{$bb},$TPW,$TPH);
            
            for (my $col = $colMin; $col<= $colMax; $col++){
                for (my $row = $rowMin; $row<= $rowMax; $row++){
            
                    my $OGRslab = $tm->indicesToGeom($col, $row, $TPW, $TPH);

                    if (COMMON::ProxyGDAL::isIntersected($OGRslab, $convertExtent)) {
                        my $nodeKey = sprintf "%s_%s", $col, $row;
                        # Create a new Node
                        my $node = COMMON::Node->new({
                            col => $col,
                            row => $row,
                            tm => $tm,
                            graph => $this,
                            type => $this->{forest}->getStorageType()
                        });
                        if (! defined $node) { 
                            ERROR(sprintf "Cannot create Node for level %s, indices %s,%s.", $this->{bottomID}, $col, $row);
                            return FALSE;
                        }
                        $this->{nodes}->{$bottomID}->{$nodeKey} = $node;
                    }
                }
            }
        }
    } else {
        # On a un fichier qui liste les indices des dalles à générer
        my $listfile = $datasource->getList();
        
        open(LISTIN, "<$listfile") or do {
            ERROR(sprintf "Cannot open the file containing the list of image for the bottom level ($listfile)");
            return FALSE;            
        };
        
        while (my $line = <LISTIN>) {
            chomp($line);
            
            my ($col, $row) = split(/,/, $line);
            
            my $nodeKey = sprintf "%s_%s", $col, $row;
            
            if (exists $this->{nodes}->{$bottomID}->{$nodeKey}) {
                # This Node already exists
                next;
            }
            
            my ($xmin,$ymin,$xmax,$ymax) = $tm->indicesToBbox($col,$row,$TPW,$TPH);

            $this->updateBBox($xmin,$ymin,$xmax,$ymax);
            
            # Create a new Node
            my $node = COMMON::Node->new({
                col => $col,
                row => $row,
                tm => $tm,
                graph => $this,
                type => $this->{forest}->getStorageType()
            });
            if (! defined $node) { 
                ERROR(sprintf "Cannot create Node for level %s, indices %s,%s.", $this->{bottomID}, $col, $row);
                return FALSE;
            }
            $this->{nodes}->{$bottomID}->{$nodeKey} = $node;
        }
        
        close(LISTIN);
    }
  
    return TRUE;  
}

=begin nd
Function: identifyAboveNodes

Calculate all nodes in above levels. We generate a above level node if one or more children are generated.

We have to use "nearest neighbour" interpolation with this kinf of graph. So (beacause pixel's center are aligned), we keep the value from the below level. Goal is to always have values from source data, no average.

(see ROK4GENERATION/aboveNodes_NNGraph_2.png)

When we load the TMS, we precise links between different levels (source and targets). For each level, we identify above nodes (thanks to bounding boxes) which will be generated from the node. We store all this parent-child relations.

(see ROK4GENERATION/aboveNodes_NNGraph.png)
=cut
sub identifyAboveNodes {
    my $this = shift;
    
    # initialisation pratique:
    my $tms = $this->{pyramid}->getTileMatrixSet;
    my $src = $this->{datasource};
    my $tilesPerWidth = $this->{pyramid}->getTilesPerWidth();
    my $tilesPerHeight = $this->{pyramid}->getTilesPerHeight();
    
    # Calcul des branches à partir des feuilles
    for (my $k = $src->getBottomOrder; $k <= $src->getTopOrder; $k++){

        my $levelID = $tms->getIDfromOrder($k);
        # pyramid's limits update : we store data's limits in the pyramid's levels
        $this->{pyramid}->updateTMLimits($levelID,@{$this->{bbox}});
        # si un niveau est vide on a une erreur
        if ($this->isLevelEmpty($levelID)) {
            ERROR (sprintf "The level %s has no nodes. Invalid use of TMS for nearest neighbour interpolation.",$levelID);
            return FALSE;
        }
        
        my $sourceTm = $tms->getTileMatrix($levelID);
        
        my @targetsTm = @{$sourceTm->getTargetsTm()};
        next if (scalar(@targetsTm) == 0);
               
        # on n'a plus rien à calculer, on sort
        last if ($k == $src->getTopOrder );

        foreach my $node ( $this->getNodesOfLevel($levelID) ) {
            
            # On récupère la BBOX du noeud pour calculer les noeuds cibles
            my ($xMin,$yMin,$xMax,$yMax) = $node->getBBox();
            
            foreach my $targetTm (@targetsTm) {
                next if ($tms->getOrderfromID($targetTm->getID()) > $src->getTopOrder());
                
                my ($rowMin, $rowMax, $colMin, $colMax) = $targetTm->bboxToIndices(
                    $xMin,$yMin,$xMax,$yMax,
                    $tilesPerWidth,$tilesPerHeight
                );
                
                for (my $col = $colMin; $col<= $colMax; $col++){
                    for (my $row = $rowMin; $row<= $rowMax; $row++){

                        my $idxkey = sprintf "%s_%s",$col,$row;
                        my $newnode = undef;
                        if (! defined $this->{nodes}->{$targetTm->getID}->{$idxkey}) {
                            $newnode = new COMMON::Node({
                                col => $col,
                                row => $row,
                                tm => $targetTm,
                                graph => $this,
                                type => $this->{forest}->getStorageType()
                            });
                            ## intersection avec la bbox des données initiales
                            if ( $newnode->isBboxIntersectingNodeBbox($this->getBbox())) {
                                $this->{nodes}->{$targetTm->getID()}->{$idxkey} = $newnode ;
                                $newnode->addSourceNodes($node); 
                            }
                        } else {
                            $newnode = $this->{nodes}->{$targetTm->getID()}->{$idxkey};
                            $newnode->addSourceNodes($node); 
                        }             
                    }
                }

            }
        }

        DEBUG(sprintf "Number of cache images by level (%s) : %d",
              $levelID, scalar keys(%{$this->{nodes}->{$levelID}}));
    }

    return TRUE;  
}

####################################################################################################
#                                   Group: Compute methods                                         #
####################################################################################################

=begin nd
Function: computeYourself

Only one step:
    - browse graph and write commands in different scripts.
=cut
sub computeYourself {
    my $this = shift;
    
    my $src = $this->{datasource};
    my $tms = $this->getPyramid()->getTileMatrixSet();
  
   #Initialisation
   my $Finisher_Index = 0;
   # boucle sur tous les niveaux en partant de ceux du bas
   for(my $i = $src->getBottomOrder; $i <= $src->getTopOrder; $i++) {
       # boucle sur tous les noeuds du niveau
       my $levelID = $tms->getIDfromOrder($i);
       foreach my $node ($this->getNodesOfLevel($levelID)) {
           # on détermine dans quel script on l'écrit en se basant sur les poids
           my @ScriptsOfLevel = $this->getScriptsOfLevel($levelID);
           my @WeightsOfLevel = map {$_->getWeight();} @ScriptsOfLevel ;
           my $script_index = COMMON::Array::minArrayIndex(0,@WeightsOfLevel);
           my $script = $ScriptsOfLevel[$script_index];
           # on stocke l'information dans l'objet node
           $node->setScript($script);
           # on détermine le script à ecrire
           my ($c,$w) ;
           if ($this->getDataSource->hasHarvesting) {
                # Datasource has a WMS service : we have to use it
                ($c,$w) = $this->{commands}->wms2work($node,$this->getDataSource->getHarvesting);
                if (! defined $c) {
                    ERROR(sprintf "Cannot harvest image for node %s",$node->getWorkBaseName());
                    return FALSE;
                }
           } else {
                if ($i == $src->getBottomOrder) {
                    # on utilise mergeNtiff pour le niveau du bas (à partir des images sources)
                    ($c,$w) = $this->{commands}->mergeNtiff($node);
                    if ($w == -1) {
                        ERROR(sprintf "Cannot compose mergeNtiff command for the node %s.",$node->getWorkBaseName());
                        return FALSE;
                    }
                } else {
                    # on utilise decimateNtiff pour les niveaux supérieurs, par décimation d'un niveau inférieur
                    ($c,$w) = $this->{commands}->decimateNtiff($node);
                    if ($w == -1) {
                        ERROR(sprintf "Cannot compose decimateNtiff command for the node %s.",$node->getWorkBaseName());
                        return FALSE;
                    }                    
                }
           }
           # on met à jour les poids
           $script->addWeight($w);
           # on ecrit la commande dans le fichier
           $script->write($c);
                   
           # final script with all work2cache commands
           # on ecrit dans chacun des scripts de manière tournante
           my $finisher = $this->getForest()->getScript($Finisher_Index);
           ($c,$w) = $this->{commands}->work2cache($node,"\${ROOT_TMP_DIR}/".$node->getScript()->getID());
           # on ecrit la commande dans le fichier
           $finisher->write($c);
           #on met à jour l'index
           if ($Finisher_Index == $this->getForest()->getSplitNumber() - 1) {
               $Finisher_Index = 0;
           } else {
               $Finisher_Index ++;
           }

       }
   }
    
    return TRUE;
};

=begin nd
Function: containsNode

Returns a boolean : TRUE if the node belong to this tree, FALSE otherwise (if a parameter is not defined too).

Parameters (list):
    level - string - Level ID of the node we want to know if it is in the quad tree.
    i - integer - Column of the node we want to know if it is in the quad tree.
    j - integer - Row of the node we want to know if it is in the quad tree.
=cut
sub containsNode {
    my $this = shift;
    my $level = shift;
    my $i = shift;
    my $j = shift;
  
    return FALSE if (! defined $level || ! defined $i || ! defined $j);
    
    my $nodeKey = $i."_".$j;
    return (exists $this->{nodes}->{$level}->{$nodeKey});
}

####################################################################################################
#                                Group: Getters - Setters                                          #
####################################################################################################

# Function: getPyramid
sub getPyramid {
    my $this = shift;
    return $this->{pyramid};
}

# Function: getForest
sub getForest {
    my $this = shift;
    return $this->{forest};
}

# Function: getDataSource
sub getDataSource {
    my $this = shift;
    return $this->{datasource};
}

# Function: getCoordTransPyramidDatasource
sub getCoordTransPyramidDatasource {
    my $this = shift;
    return $this->{ct_pyramid_source};
}

# Function: getTopID
sub getTopID {
    my $this = shift;
    return $this->{topID};
}

# Function: getBottomID
sub getBottomID {
    my $this = shift;
    return $this->{bottomID};
}

# Function: getTopOrder
sub getTopOrder {
    my $this = shift;
    return $this->{pyramid}->getTileMatrixSet()->getOrderfromID($this->{topID});
}

# Function: getBottomOrder
sub getBottomOrder {
    my $this = shift;
    return $this->{pyramid}->getTileMatrixSet()->getOrderfromID($this->{bottomID});
}

=begin nd
Function: getNodesOfLevel

Returns a <COMMON::Node> array, contaning all nodes of the provided level.

Parameters (list):
    level - string - Level ID whose we want all nodes.
=cut
sub getNodesOfLevel {
    my $this = shift;
    my $levelID= shift;
    
    if (! defined $levelID) {
        ERROR("Undefined Level");
        return undef;
    }
    
    return values (%{$this->{nodes}->{$levelID}});
}

=begin nd
Function: isLevelEmpty

Returns a boolean, precise if level is empty.

Parameters (list):
    level - string - Level ID
=cut
sub isLevelEmpty {
    my $this = shift;
    my $levelID= shift;
    
    if (! defined $levelID) {
        ERROR("Undefined Level");
        return undef;
    }
    
    return FALSE if (scalar(keys(%{$this->{nodes}->{$levelID}})) > 0) ;
    return TRUE;
}

# Function: getNodesOfTopLevel
sub getNodesOfTopLevel {
    my $this = shift;
    return $this->getNodesOfLevel($this->{topID});
}

# Function: getBbox
sub getBbox {
    my $this =shift;
    return ($this->{bbox}[0],$this->{bbox}[1],$this->{bbox}[2],$this->{bbox}[3]);
}

=begin nd
Function: updateBBox

Compare provided and stored extrems coordinates and update values.

Parameters (list):
    xmin, ymin, xmax, ymax - double - New coordinates to compare with current bbox.
=cut
sub updateBBox {
    my $this = shift;
    my ($xmin,$ymin,$xmax,$ymax) = @_;
    
    if (! defined $this->{bbox}[0] || $xmin < $this->{bbox}[0]) {$this->{bbox}[0] = $xmin;}
    if (! defined $this->{bbox}[1] || $ymin < $this->{bbox}[1]) {$this->{bbox}[1] = $ymin;}
    if (! defined $this->{bbox}[2] || $xmax > $this->{bbox}[2]) {$this->{bbox}[2] = $xmax;}
    if (! defined $this->{bbox}[3] || $ymax > $this->{bbox}[3]) {$this->{bbox}[3] = $ymax;}
}

=begin nd
method: getScriptsOfLevel

Returns a <Script> array, used scripts to generate the supllied level.

Parameters (list):
    level - string - Level identifiant, whose scripts we want.
=cut
sub getScriptsOfLevel {
    my $this = shift;
    my $levelID = shift;
    my $order =  $this->getPyramid()->getTileMatrixSet()->getOrderfromID($levelID);
    
    my $numberOfScriptByLevel = $this->getForest()->getSplitNumber();
    my $numberOfFinisher = $this->getForest()->getSplitNumber();
    
    my $start_index = $numberOfFinisher + ($order - $this->getBottomOrder()) * $numberOfScriptByLevel ;
    my $end_index = $start_index + $numberOfScriptByLevel - 1;

    return @{$this->getForest()->getScripts()}[$start_index .. $end_index];
};

####################################################################################################
#                                Group: Export methods                                             #
####################################################################################################

=begin nd
Function: exportForDebug

Returns all informations about the "nearest neighbour" graph. Useful for debug.

Example:
    (start code)
    (end code)
=cut
sub exportForDebug {
    my $this = shift ;
    
    my $src = $this->getDataSource();
    my $tms = $this->getPyramid->getTileMatrixSet();
    
    my $output = "";
    
   # boucle sur tous les niveaux en partant de ceux du bas
   for (my $i = $src->getBottomOrder; $i <= $src->getTopOrder; $i++) {
       $output .= sprintf "Description du niveau '%s' : \n",$i;
       # boucle sur tous les noeuds du niveau
       foreach my $node ( $this->getNodesOfLevel($tms->getIDfromOrder($i))) {
         $output .= sprintf "\tNoeud : %s_%s ; TM Résolution : %s ; Calculé à partir de : \n",$node->getCol(),$node->getRow(),$node->getTM()->getResolution();
         foreach my $node_sup ( @{$node->getSourceNodes()} ) {
             #print Dumper ($node_sup);
             $output .= sprintf "\t\t Noeud :%s_%s , TM Resolution : %s\n",$node_sup->getCol(),$node_sup->getRow(),$node_sup->getTM()->getResolution();
         }
       }
   }
   
   return $output;
}

1;
__END__
