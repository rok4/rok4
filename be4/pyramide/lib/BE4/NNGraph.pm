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

Class: BE4::NNGraph

Representation of a "nearest neighbour" pyramid : pyramid's image = <Node>.

(see NNGraphTMS.png)

To generate this kind of graph, we use :
    - *jobNumber* scripts by level : to generate image to work format.
    - *jobNumber* scripts : to move image in the final pyramid, with the wanted format (compression...)

=> *jobNumber x (levelNumber + 1)* scripts

Organization in the <Forest> scripts' array :

(see script_NNGraph.png)

Link between a node and his children or his father is not trivial. It is calculated and store in the <Node> object.

Using:
    (start code)
    use BE4::NNGraph;

    # NNGraph object creation
    my $objNNGraph = BE4::QTree->new($objForest, $objDataSource, $objPyramid, $objCommands);

    ...

    # Fill each node with computing code
    $objNNGraph->computeYourself();
    (end code)

Attributes:
    forest - <Forest> - Forest which this tree belong to.
    pyramid - <Pyramid> - Pyramid linked to this tree.
    commands - <Commands> - Command to use to generate images.
    datasource - <DataSource> - Data source to use to define bottom level nodes and generate them.

    bbox - double array - Datasource bbox, [xmin,ymin,xmax,ymax], in TMS' SRS
    nodes - <Node> hash - Structure is:
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
        nX : BE4::Node
        (end code)
        
    bottomID - string - Bottom level identifiant
    topID - string - Top level identifiant
=cut

################################################################################

package BE4::NNGraph;

use Geo::OSR;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
use File::Basename;
use File::Path;
use Data::Dumper;

# My Module
use BE4::DataSource;
use BE4::Node;

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
    objPyr - <Pyramid> - Pyramid linked to this tree
    objCommands - <Commands> - Commands to use to generate pyramid's images

See also:
    <_init>, <_load>
=cut
sub new {
    my $this = shift;

    my $class= ref($this) || $this;
    # IMPORTANT : if modification, think to update natural documentation (just above) and pod documentation (bottom)
    my $self = {
        # in
        forest    => undef,
        pyramid    => undef,
        commands    => undef,
        datasource => undef,
        # out
        bbox => [],
        nodes => {},
        bottomID => undef,
        topID    => undef,
    };

    bless($self, $class);

    TRACE;

    # init. class
    return undef if (! $self->_init(@_));
    # load 
    return undef if (! $self->_load());

    return $self;
}

=begin nd
Function: _init

Checks and stores informations.

Parameters (list):
    objForest - <Forest> - Forest which this tree belong to
    objSrc - <DataSource> - Data source which determine bottom level nodes
    objPyr - <Pyramid> - Pyramid linked to this tree
    objCommands - <Commands> - Commands to use to generate pyramid's images
=cut
sub _init {
    my $self = shift;
    my $objForest  = shift;
    my $objSrc  = shift;
    my $objPyr  = shift;
    my $objCommands  = shift;

    TRACE;

    # mandatory parameters !
    if (! defined $objForest || ref ($objForest) ne "BE4::Forest") {
        ERROR("Can not load Forest !");
        return FALSE;
    }
    if (! defined $objSrc || ref ($objSrc) ne "BE4::DataSource") {
        ERROR("Can not load DataSource !");
        return FALSE;
    }
    if (! defined $objPyr || ref ($objPyr) ne "BE4::Pyramid") {
        ERROR("Can not load Pyramid !");
        return FALSE;
    }
    if (! defined $objCommands || ref ($objCommands) ne "BE4::Commands") {
        ERROR("Can not load Commands !");
        return FALSE;
    }

    # init. params    
    $self->{forest} = $objForest; 
    $self->{pyramid} = $objPyr;
    $self->{datasource} = $objSrc; 
    $self->{commands} = $objCommands;    

    return TRUE;
}

=begin nd
Function: _load

Determines all nodes from the bottom level to the top level, thanks to the data source.
=cut
sub _load {
    my $self = shift;

    TRACE;

    # initialisation pratique:
    my $tms = $self->{pyramid}->getTileMatrixSet;
    my $src = $self->{datasource};
    my $tilesPerWidth = $self->{pyramid}->getTilesPerWidth();
    my $tilesPerHeight = $self->{pyramid}->getTilesPerHeight();
    
    # récupération d'information dans la source de données
    $self->{topID} = $self->{datasource}->getTopID;
    $self->{bottomID} = $self->{datasource}->getBottomID;

    # initialisation de la transfo de coord du srs des données initiales vers
    # le srs de la pyramide. Si les srs sont identiques on laisse undef.
    my $ct = undef;
    
    my $srsini= new Geo::OSR::SpatialReference;
    if ($tms->getSRS() ne $src->getSRS()){
        eval { $srsini->ImportFromProj4('+init='.$src->getSRS().' +wktext'); };
        if ($@) { 
            eval { $srsini->ImportFromProj4('+init='.lc($src->getSRS()).' +wktext'); };
            if ($@) { 
                ERROR($@);
                ERROR(sprintf "Impossible to initialize the initial spatial coordinate system (%s) !",
                      $src->getSRS());
                return FALSE;
            }
        }
        
        my $srsfin= new Geo::OSR::SpatialReference;
        eval { $srsfin->ImportFromProj4('+init='.$tms->getSRS().' +wktext'); };
        if ($@) {
            eval { $srsfin->ImportFromProj4('+init='.lc($tms->getSRS()).' +wktext'); };
            if ($@) {
                ERROR($@);
                ERROR(sprintf "Impossible to initialize the destination spatial coordinate system (%s) !",
                      $tms->getSRS());
                return FALSE;
            }
        }
        $ct = new Geo::OSR::CoordinateTransformation($srsini, $srsfin);
    }

    # identifier les noeuds du niveau de base à mettre à jour et les associer aux images sources:
    if (! $self->identifyBottomNodes($ct)) {
        ERROR(sprintf "Cannot determine bottom tiles for the level %s",$src->getBottomID);
        return FALSE;
    }

    INFO(sprintf "Number of cache images to the bottom level (%s) : %d",
         $self->{bottomID},scalar keys(%{$self->{nodes}{$self->{bottomID}}}));
    
    # identifier les noeuds des niveaux supérieurs
    if (! $self->identifyAboveNodes) {
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

Parameters (list):
    ct - <Geo::OSR::CoordinateTransformation> - To convert data extent or images' bbox.
=cut
sub identifyBottomNodes {
    my $self = shift;
    my $ct = shift;
    
    TRACE();
    
    my $bottomID = $self->{bottomID};
    my $tm = $self->{pyramid}->getTileMatrixSet->getTileMatrix($bottomID);
    if (! defined $tm) {
        ERROR(sprintf "Impossible de récupérer le TM à partir de %s (bottomID) et du TMS : %s.",$bottomID,$self->getPyramid()->getTileMatrixSet()->exportForDebug());
        return FALSE;
    };
    my $datasource = $self->{datasource};
    my ($TPW,$TPH) = ($self->{pyramid}->getTilesPerWidth,$self->{pyramid}->getTilesPerHeight);
    
    if ($datasource->hasImages) {
        # We have real data as source. Images determine bottom tiles
        my @images = $datasource->getImages();
        foreach my $objImg (@images){
            # On reprojette l'emprise si nécessaire
            my @bbox = $objImg->convertBBox($ct); # [xMin, yMin, xMax, yMax]
            if ($bbox[0] == 0 && $bbox[2] == 0) {
                ERROR(sprintf "Impossible to compute BBOX for the image '%s'. Probably limits are reached !", $objImg->getName());
                return FALSE;
            }
            
            $self->updateBBox($bbox[0], $bbox[1], $bbox[2], $bbox[3]);
            
            # On divise les coord par la taille des dalles de cache pour avoir les indices min et max en x et y
            my ($iMin, $jMin, $iMax, $jMax) = $tm->bboxToIndices($bbox[0],$bbox[1],$bbox[2],$bbox[3],$TPW,$TPH);
            
            for (my $i = $iMin; $i<= $iMax; $i++){
                for (my $j = $jMin; $j<= $jMax; $j++){
                    my $nodeKey = sprintf "%s_%s", $i, $j;

                    if ($datasource->hasHarvesting) {
                        # we use WMS service to generate this leaf
                        if (exists $self->{nodes}->{$bottomID}->{$nodeKey}) {
                            # This Node already exists
                            next;
                        }
                        # Create a new Node
                        my $node = BE4::Node->new({
                            i => $i,
                            j => $j,
                            tm => $tm,
                            graph => $self,
                        });
                        if (! defined $node) { 
                            ERROR(sprintf "Cannot create Node for level %s, indices %s,%s.",
                                  $self->{bottomID}, $i, $j);
                            return FALSE;
                        }
                        $self->{nodes}->{$bottomID}->{$nodeKey} = $node;
                    } else {
                        # we use images to generate this leaf
                        if (exists $self->{nodes}->{$bottomID}->{$nodeKey}) {
                            # This Node already exists
                            # We add this GeoImage to this node
                            $self->{nodes}->{$bottomID}->{$nodeKey}->addGeoImages($objImg);
                            next;
                        }
                        # Create a new Node
                        my $node = BE4::Node->new({
                            i => $i,
                            j => $j,
                            tm => $tm,
                            graph => $self,
                        });
                        if (! defined $node) { 
                            ERROR(sprintf "Cannot create Node for level %s, indices %s,%s.",
                                  $self->{bottomID}, $i, $j);
                            return FALSE;
                        }
                        $node->addGeoImages($objImg);
                        $self->{nodes}->{$bottomID}->{$nodeKey} = $node;
                    }
                }
            }
        }
    } elsif (defined $datasource->getExtent) {
        # We have just a WMS service as source. We use extent to determine bottom tiles
        my $convertExtent = $datasource->getExtent->Clone();
        if (defined $ct) {
            eval { $convertExtent->Transform($ct); };
            if ($@) { 
                ERROR(sprintf "Cannot convert extent for the datasource : %s",$@);
                return FALSE;
            }
        }
        
        my $bboxref = $convertExtent->GetEnvelope(); #bboxref = [xmin,xmax,ymin,ymax]
        
        $self->updateBBox($bboxref->[0],$bboxref->[2],$bboxref->[1],$bboxref->[3]);
        
        my ($iMin, $jMin, $iMax, $jMax) = $tm->bboxToIndices(
            $bboxref->[0],$bboxref->[2],$bboxref->[1],$bboxref->[3],$TPW,$TPH);
        
        for (my $i = $iMin; $i <= $iMax; $i++) {
            for (my $j = $jMin; $j <= $jMax; $j++) {
                my ($xmin,$ymin,$xmax,$ymax) = $tm->indicesToBBox($i,$j,$TPW,$TPH);

                my $WKTtile = sprintf "POLYGON((%s %s,%s %s,%s %s,%s %s,%s %s))",
                    $xmin,$ymin,
                    $xmin,$ymax,
                    $xmax,$ymax,
                    $xmax,$ymin,
                    $xmin,$ymin;

                my $OGRtile = Geo::OGR::Geometry->create(WKT=>$WKTtile);
                if ($OGRtile->Intersect($convertExtent)){
                    my $nodeKey = sprintf "%s_%s", $i, $j;
                    # Create a new Node
                    my $node = BE4::Node->new({
                        i => $i,
                        j => $j,
                        tm => $tm,
                        graph => $self,
                    });
                    if (! defined $node) { 
                        ERROR(sprintf "Cannot create Node for level %s, indices %s,%s.", $self->{bottomID}, $i, $j);
                        return FALSE;
                    }
                    $self->{nodes}->{$bottomID}->{$nodeKey} = $node;
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
            
            my ($i, $j) = split(/,/, $line);
            
            my $nodeKey = sprintf "%s_%s", $i, $j;
            
            if (exists $self->{nodes}->{$bottomID}->{$nodeKey}) {
                # This Node already exists
                next;
            }
            
            my ($xmin,$ymin,$xmax,$ymax) = $tm->indicesToBBox($i,$j,$TPW,$TPH);

            $self->updateBBox($xmin,$ymin,$xmax,$ymax);
            
            # Create a new Node
            my $node = BE4::Node->new({
                i => $i,
                j => $j,
                tm => $tm,
                graph => $self,
            });
            if (! defined $node) { 
                ERROR(sprintf "Cannot create Node for level %s, indices %s,%s.", $self->{bottomID}, $i, $j);
                return FALSE;
            }
            $self->{nodes}->{$bottomID}->{$nodeKey} = $node;
        }
        
        close(LISTIN);        
    }
  
    return TRUE;  
}

=begin nd
Function: identifyAboveNodes

Calculate all nodes in above levels. We generate a above level node if one or more children are generated.

We have to use "nearest neighbour" interpolation with this kinf of graph. So (beacause pixel's center are aligned), we keep the value from the below level. Goal is to always have values from source data, no average.

(see aboveNodes_NNGraph_2.png)

When we load the TMS, we precise links between different levels (source and targets). For each level, we identify above nodes (thanks to bounding boxes) which will be generated from the node. We store all this parent-child relations.

(see aboveNodes_NNGraph.png)
=cut
sub identifyAboveNodes {
    my $self = shift;
    
    # initialisation pratique:
    my $tms = $self->{pyramid}->getTileMatrixSet;
    my $src = $self->{datasource};
    my $tilesPerWidth = $self->{pyramid}->getTilesPerWidth();
    my $tilesPerHeight = $self->{pyramid}->getTilesPerHeight();
    
    # Calcul des branches à partir des feuilles
    for (my $k = $src->getBottomOrder; $k <= $src->getTopOrder; $k++){

        my $levelID = $tms->getIDfromOrder($k);
        # pyramid's limits update : we store data's limits in the pyramid's levels
        $self->{pyramid}->updateTMLimits($levelID,@{$self->{bbox}});
        # si un niveau est vide on a une erreur
        if ($self->isLevelEmpty($levelID)) {
            ERROR (sprintf "The level %s has no nodes. Invalid use of TMS for nearest neighbour interpolation.",$levelID);
            return FALSE;
        }
        
        my $sourceTm = $tms->getTileMatrix($levelID);
        
        my @targetsTm = @{$sourceTm->getTargetsTm()};
        next if (scalar(@targetsTm) == 0);
               
        # on n'a plus rien à calculer, on sort
        last if ($k == $src->getTopOrder );

        foreach my $node ( $self->getNodesOfLevel($levelID) ) {
            
            # On récupère la BBOX du noeud pour calculer les noeuds cibles
            my ($xMin,$yMin,$xMax,$yMax) = $node->getBBox();
            
            foreach my $targetTm (@targetsTm) {
                next if ($tms->getOrderfromID($targetTm->getID()) > $src->getTopOrder());
                my ($iMin, $jMin, $iMax, $jMax) = $targetTm->bboxToIndices(
                    $xMin,$yMin,$xMax,$yMax,$tilesPerWidth,$tilesPerHeight);
                
                for (my $i = $iMin; $i < $iMax + 1; $i++){
                    for (my $j = $jMin ; $j < $jMax +1 ; $j++) {

                        my $idxkey = sprintf "%s_%s",$i,$j;
                        my $newnode = undef;
                        if (! defined $self->{nodes}->{$targetTm->getID}->{$idxkey}) {
                            $newnode = new BE4::Node({
                                i => $i,
                                j => $j,
                                tm => $targetTm,
                                graph => $self,
                            });
                            ## intersection avec la bbox des données initiales
                            if ( $newnode->isBboxIntersectingNodeBbox($self->getBbox())) {
                                $self->{nodes}->{$targetTm->getID()}->{$idxkey} = $newnode ;
                                $newnode->addNodeSources($node); 
                            }
                        } else {
                            $newnode = $self->{nodes}->{$targetTm->getID()}->{$idxkey};
                            $newnode->addNodeSources($node); 
                        }             
                    }
                }

            }
        }

        DEBUG(sprintf "Number of cache images by level (%s) : %d",
              $levelID, scalar keys(%{$self->{nodes}->{$levelID}}));
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
    my $self = shift;
    
    my $src = $self->{datasource};
    my $tms = $self->getPyramid()->getTileMatrixSet();
  
   #Initialisation
   my $Finisher_Index = 0;
   # boucle sur tous les niveaux en partant de ceux du bas
   for(my $i = $src->getBottomOrder; $i <= $src->getTopOrder; $i++) {
       # boucle sur tous les noeuds du niveau
       my $levelID = $tms->getIDfromOrder($i);
       foreach my $node ($self->getNodesOfLevel($levelID)) {
           # on détermine dans quel script on l'écrit en se basant sur les poids
           my @ScriptsOfLevel = $self->getScriptsOfLevel($levelID);
           my @WeightsOfLevel = map {$_->getWeight();} @ScriptsOfLevel ;
           my $script_index = BE4::Array::minArrayIndex(0,@WeightsOfLevel);
           my $script = $ScriptsOfLevel[$script_index];
           # on stocke l'information dans l'objet node
           $node->setScript($script);
           # on détermine le script à ecrire
           my ($c,$w) ;
           if ($self->getDataSource->hasHarvesting) {
                # Datasource has a WMS service : we have to use it
                ($c,$w) = $self->{commands}->wms2work($node,$self->getDataSource->getHarvesting);
                if (! defined $c) {
                    ERROR(sprintf "Cannot harvest image for node %s",$node->getWorkBaseName());
                    return FALSE;
                }
           } else {
                if ($i == $src->getBottomOrder) {
                    # on utilise mergeNtiff pour le niveau du bas (à partir des images sources)
                    ($c,$w) = $self->{commands}->mergeNtiff($node);
                    if ($w == -1) {
                        ERROR(sprintf "Cannot compose mergeNtiff command for the node %s.",$node->getWorkBaseName());
                        return FALSE;
                    }
                } else {
                    # on utilise decimateNtiff pour les niveaux supérieurs, par décimation d'un niveau inférieur
                    ($c,$w) = $self->{commands}->decimateNtiff($node);
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
                   
           # final script with all tiff2tile commands
           # on ecrit dans chacun des scripts de manière tournante
           my $finisher = $self->getForest()->getScript($Finisher_Index);
           ($c,$w) = $self->{commands}->work2cache($node,"\${ROOT_TMP_DIR}/".$node->getScript()->getID());
           # on ecrit la commande dans le fichier
           $finisher->write($c);
           #on met à jour l'index
           if ($Finisher_Index == $self->getForest()->getSplitNumber() - 1) {
               $Finisher_Index = 0;
           } else {
               $Finisher_Index ++;
           }

       }
   }
    
    TRACE;
    
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
    my $self = shift;
    my $level = shift;
    my $i = shift;
    my $j = shift;
  
    return FALSE if (! defined $level || ! defined $i || ! defined $j);
    
    my $nodeKey = $i."_".$j;
    return (exists $self->{nodes}->{$level}->{$nodeKey});
}

####################################################################################################
#                                Group: Getters - Setters                                          #
####################################################################################################

# Function: getPyramid
sub getPyramid {
    my $self = shift;
    return $self->{pyramid};
}

# Function: getForest
sub getForest {
    my $self = shift;
    return $self->{forest};
}

# Function: getDataSource
sub getDataSource {
    my $self = shift;
    return $self->{datasource};
}

# Function: getTopID
sub getTopID {
    my $self = shift;
    return $self->{topID};
}

# Function: getBottomID
sub getBottomID {
    my $self = shift;
    return $self->{bottomID};
}

# Function: getTopOrder
sub getTopOrder {
    my $self = shift;
    return $self->{pyramid}->getTileMatrixSet->getOrderfromID($self->{topID});
}

# Function: getBottomOrder
sub getBottomOrder {
    my $self = shift;
    return $self->{pyramid}->getTileMatrixSet->getOrderfromID($self->{bottomID});
}

=begin nd
Function: getNodesOfLevel

Returns a <Node> array, contaning all nodes of the provided level.

Parameters (list):
    level - string - Level ID whose we want all nodes.
=cut
sub getNodesOfLevel {
    my $self = shift;
    my $levelID= shift;
    
    if (! defined $levelID) {
        ERROR("Undefined Level");
        return undef;
    }
    
    return values (%{$self->{nodes}->{$levelID}});
}

=begin nd
Function: isLevelEmpty

Returns a boolean, precise if level is empty.

Parameters (list):
    level - string - Level ID
=cut
sub isLevelEmpty {
    my $self = shift;
    my $levelID= shift;
    
    if (! defined $levelID) {
        ERROR("Undefined Level");
        return undef;
    }
    
    return FALSE if (scalar(keys(%{$self->{nodes}->{$levelID}})) > 0) ;
    return TRUE;
}

# Function: getNodesOfTopLevel
sub getNodesOfTopLevel {
    my $self = shift;
    return $self->getNodesOfLevel($self->{topID});
}

# Function: getBbox
sub getBbox {
    my $self =shift;
    return ($self->{bbox}[0],$self->{bbox}[1],$self->{bbox}[2],$self->{bbox}[3]);
}

=begin nd
Function: updateBBox

Compare provided and stored extrems coordinates and update values.

Parameters (list):
    xmin, ymin, xmax, ymax - double - New coordinates to compare with current bbox.
=cut
sub updateBBox {
    my $self = shift;
    my ($xmin,$ymin,$xmax,$ymax) = @_;

    TRACE();
    
    if (! defined $self->{bbox}[0] || $xmin < $self->{bbox}[0]) {$self->{bbox}[0] = $xmin;}
    if (! defined $self->{bbox}[1] || $ymin < $self->{bbox}[1]) {$self->{bbox}[1] = $ymin;}
    if (! defined $self->{bbox}[2] || $xmax > $self->{bbox}[2]) {$self->{bbox}[2] = $xmax;}
    if (! defined $self->{bbox}[3] || $ymax > $self->{bbox}[3]) {$self->{bbox}[3] = $ymax;}
}

=begin nd
method: getScriptsOfLevel

Returns a <Script> array, used scripts to generate the supllied level.

Parameters (list):
    level - string - Level identifiant, whose scripts we want.
=cut
sub getScriptsOfLevel {
    my $self = shift;
    my $levelID = shift;
    my $order =  $self->getPyramid()->getOrderfromID($levelID);
    
    my $numberOfScriptByLevel = $self->getForest()->getSplitNumber();
    my $numberOfFinisher = $self->getForest()->getSplitNumber();
    
    my $start_index = $numberOfFinisher + ($order - $self->getBottomOrder) * $numberOfScriptByLevel ;
    my $end_index = $start_index + $numberOfScriptByLevel - 1;

    return @{$self->getForest()->getScripts()}[$start_index .. $end_index];
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
    my $self = shift ;
    
    my $src = $self->getDataSource;
    my $tms = $self->getPyramid->getTileMatrixSet;
    
    my $output = "";
    
   # boucle sur tous les niveaux en partant de ceux du bas
   for (my $i = $src->getBottomOrder; $i <= $src->getTopOrder; $i++) {
       $output .= sprintf "Description du niveau '%s' : \n",$i;
       # boucle sur tous les noeuds du niveau
       foreach my $node ( $self->getNodesOfLevel($tms->getIDfromOrder($i))) {
         $output .= sprintf "\tNoeud : %s_%s ; TM Résolution : %s ; Calculé à partir de : \n",$node->getCol(),$node->getRow(),$node->getTM()->getResolution();
         foreach my $node_sup ( @{$node->getNodeSources()} ) {
             #print Dumper ($node_sup);
             $output .= sprintf "\t\t Noeud :%s_%s , TM Resolution : %s\n",$node_sup->getCol(),$node_sup->getRow(),$node_sup->getTM()->getResolution();
         }
       }
   }
   
   return $output;
}

1;
__END__
