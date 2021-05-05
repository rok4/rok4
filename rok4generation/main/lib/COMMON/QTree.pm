# Copyright © (2011) Institut national de l'information
#                    géographique et forestière 
# 
# Géoportail SAV <contact.geoservices@ign.fr>
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
File: QTree.pm

Class: COMMON::QTree

(see ROK4GENERATION/libperlauto/COMMON_QTree.png)

Representation of a quad tree image pyramid : pyramid's image = <BE4::Node> or <FOURALAMO::Node>

(see ROK4GENERATION/QTreeTMS.png)

To generate this kind of graph, we use :
    - *jobNumber* scripts : to generate and format image from the bottom level to the cut level.
    - 1 script (finisher) : to generate and format image from the cut level to the top level.

=> *jobNumber + 1* scripts

Organization in the <COMMON::Forest> scripts' array :

(see ROK4GENERATION/script_QTree.png)

As a tree, a node has just one parent. As a QUAD tree, the parent belong to the above level and a node has 4 children at most.

Link between a node and his children or his father is trivial, and needn't to be store :
    - To know parent's indices, we divide own indices by 2 (and keep floor), and the level is the just above one
    - To know 4 possible chlidren's, in the just below level :
|        i*2, j*2
|        i*2, j*2 + 1
|        i*2 + 1, j*2
|        i*2 + 1, j*2 + 1

Using:
    (start code)
    use COMMON::QTree;

    # QTree object creation
    my $objQTree = COMMON::QTree->new($objForest, $objDataSource);

    ...

    $objQTree->computeYourself();
    (end code)

Attributes:
    pyramid - <COMMON::PyramidRaster> or <COMMON::PyramidVector> - Pyramid linked to this tree.
    datasource - <COMMON::DataSource> - Data source to use to define bottom level nodes and generate them.

    ct_source_pyramid - <Geo::OSR::CoordinateTransformation> - Coordinate transformation from datasource srs to pyramid srs
    ct_pyramid_source - <Geo::OSR::CoordinateTransformation> - Coordinate transformation from pyramid srs to datasource srs

    bbox - double array - Datasource bbox, [xmin,ymin,xmax,ymax], in TMS' SRS
    nodes - <BE4::Node> or <FOURALAMO::Node> hash - Structure is:
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
        nX : BE4::Node or FOURLAMO::Node
        (end code)

    scripts - <COMMON::Script> hash - The same for all QTree of the <COMMON::Forest>.

    cutLevelID - string - Cut level identifiant. To parallelize work, split scripts will generate cache from the bottom to this level. Script finisher will be generate from this above, to top.
    bottomID - string - Bottom level identifiant
    topID - string - Top level identifiant
=cut

################################################################################

package COMMON::QTree;

use strict;
use warnings;

use Math::BigFloat;
use Data::Dumper;

use COMMON::DataSource;
use BE4::Node;
use FOURALAMO::Node;
use BE4::Shell;
use FOURALAMO::Shell;
use COMMON::PyramidRaster;
use COMMON::PyramidVector;
use COMMON::Array;

use BE4::Shell;
use FOURALAMO::Shell;

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

####################################################################################################
#                                       Group: Class methods                                       #
####################################################################################################

=begin nd
Constructor: defineScripts

Create all <COMMON::Script>'s to generate the QTree pyramid. They are stored in the <COMMON::Forest> instance.

Parameters (list):
    scriptInit - string - Shell function to write into each script
    pyramid - <COMMON::PyramidRaster> - NNGraph Pyramid to generate   
=cut
sub defineScripts {
    my $scriptInit = shift;
    my $pyramid = shift;

    my $parallelization;
    my $shellClass;
    if (ref ($pyramid) eq "COMMON::PyramidVector") {
        $parallelization = $FOURALAMO::Shell::PARALLELIZATIONLEVEL;
        $shellClass = 'FOURALAMO::Shell';
    } elsif(ref ($pyramid) eq "COMMON::PyramidRaster") {
        $parallelization = $BE4::Shell::PARALLELIZATIONLEVEL;
        $shellClass = 'BE4::Shell';
    }

    my $scripts = {
        splits => [],
        current => 0,
        number => $parallelization,
        finisher => undef
    };

    for (my $i = 1; $i <= $parallelization; $i++) {
        push(
            @{$scripts->{splits}},
            COMMON::Script->new({
                id => "SCRIPT_$i",
                finisher => FALSE,
                shellClass => $shellClass,
                initialisation => $scriptInit
            })
        )
    }

    $scripts->{finisher} = COMMON::Script->new({
        id => "SCRIPT_FINISHER",
        finisher => TRUE,
        shellClass => $shellClass,
        initialisation => $scriptInit
    });

    return $scripts;
}

=begin nd
Constructor: closeScripts

Close all <COMMON::Script>'s stream.

Parameters (list):
    scripts - <COMMON::Script> hash - Scripts pool to close
=cut
sub closeScripts {
    my $scripts = shift;

    $scripts->{finisher}->close();
    foreach my $split (@{$scripts->{splits}}) {
        $split->close();
    }
}

####################################################################################################
#                                        Group: Constructors                                       #
####################################################################################################

=begin nd
Constructor: new

QTree constructor. Bless an instance.

Parameters (list):
    objForest - <COMMON::Forest> - Forest which this tree belong to
    objSrc - <COMMON::DataSource> - Datasource which determine bottom level nodes

See also:
    <_init>, <_load>
=cut
sub new {
    my $class = shift;
    my $objForest = shift;
    my $objSrc = shift;

    $class = ref($class) || $class;
    # IMPORTANT : if modification, think to update natural documentation (just above)
    my $this = {
        # in
        scripts => undef,
        pyramid => undef,
        datasource => undef,
        # ct
        ct_source_pyramid => undef,
        ct_pyramid_source => undef,
        # out
        bbox => [],
        nodes => {},
        # levels
        cutLevelID => undef,
        bottomID => undef,
        topID => undef,
    };

    bless($this, $class);

    # mandatory parameters !
    if (! defined $objForest || ref ($objForest) ne "COMMON::Forest") {
        ERROR("We need a COMMON::Forest to create a QTree");
        return FALSE;
    }
    if (! defined $objSrc || ref ($objSrc) ne "COMMON::DataSource") {
        ERROR("We need a COMMON::DataSource to create a QTree");
        return FALSE;
    }

    # init. params   
    $this->{scripts} = $objForest->getScripts(); 
    $this->{pyramid} = $objForest->getPyramid();
    $this->{datasource} = $objSrc; 

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
    my $tms = $this->{pyramid}->getTileMatrixSet();
    my $src = $this->{datasource};
    
    # récupération d'information dans la source de données
    $this->{topID} = $this->{datasource}->getTopID();
    $this->{bottomID} = $this->{datasource}->getBottomID();

    # initialisation des transformations de coordonnées datasource <-> pyramide
    # Si les srs sont identiques on laisse undef.
    
    if ($tms->getSRS() ne $src->getSRS()){
        $this->{ct_source_pyramid} = COMMON::ProxyGDAL::coordinateTransformationFromSpatialReference($src->getSRS(), $tms->getSRS());
        if (! defined $this->{ct_source_pyramid}) {
            ERROR(sprintf "Cannot instanciate the coordinate transformation object %s->%s", $src->getSRS(), $tms->getSRS());
            return FALSE;
        }
        $this->{ct_pyramid_source} = COMMON::ProxyGDAL::coordinateTransformationFromSpatialReference($tms->getSRS(), $src->getSRS());
        if (! defined $this->{ct_pyramid_source}) {
            ERROR(sprintf "Cannot instanciate the coordinate transformation object %s->%s", $tms->getSRS(), $src->getSRS());
            return FALSE;
        }
    }

    # identifier les noeuds du niveau de base à mettre à jour et les associer aux images sources:
    if ( ! $this->identifyBottomNodes() ) {
        ERROR(sprintf "Cannot determine bottom tiles for the level %s",$src->getBottomID());
        return FALSE;
    }

    # identifier les noeuds des niveaux supérieurs
    if ( ! $this->identifyAboveNodes() ) {
        ERROR(sprintf "Cannot determine above levels' tiles.");
        return FALSE;
    }

    # définir le niveau de coupure
    my $firstOrder = $this->getBottomOrder();
    if (ref ($this->{pyramid}) eq "COMMON::PyramidVector") {
        # En vecteur, on force le choix du cut level au niveau du haut en ne testant que celui là.
        $this->{cutLevelID} = $this->{topID};
    } elsif (ref ($this->{pyramid}) eq "COMMON::PyramidRaster") {

        for (my $i = $this->getBottomOrder(); $i <= $this->getTopOrder(); $i++) {
            my $levelID = $tms->getIDfromOrder($i);
            
            if (! defined $this->{cutLevelID}) {
                $this->{cutLevelID} = $levelID;
                next;
            }

            my $levelNodesCount = scalar(keys(%{$this->{nodes}->{$levelID}}));

            if ($levelNodesCount < 5 * $this->{scripts}->{number}) {
                last;
            }
            $this->{cutLevelID} = $levelID;
        }
    }

    INFO("Cut level ID : ".$this->{cutLevelID});
         
    return TRUE;
}

####################################################################################################
#                          Group: Nodes determination methods                                      #
####################################################################################################

=begin nd
Function: identifyBottomNodes

Calculate all nodes in bottom level concerned by the datasource (tiles which touch the data source extent or provided in a file).

=cut
sub identifyBottomNodes {
    my $this = shift;

    my $nodeClass;
    if (ref ($this->{pyramid}) eq "COMMON::PyramidVector") {
        $nodeClass = 'FOURALAMO::Node';
    } elsif(ref ($this->{pyramid}) eq "COMMON::PyramidRaster") {
        $nodeClass = 'BE4::Node';
    }
    
    my $bottomID = $this->{bottomID};
    my $tm = $this->{pyramid}->getTileMatrixSet->getTileMatrix($bottomID);
    if (! defined $tm) {
        ERROR(sprintf "Impossible de récupérer le TM à partir de %s (bottomID) et du TMS : %s.",$bottomID,$this->getPyramid()->getTileMatrixSet()->exportForDebug());
        return FALSE;
    };
    my $datasource = $this->{datasource};
    my ($TPW,$TPH) = ($this->{pyramid}->getTilesPerWidth,$this->{pyramid}->getTilesPerHeight);
    
    if ($datasource->hasImages() ) {
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

                        my $node = $nodeClass->new({
                            col => $col,
                            row => $row,
                            tm => $tm,
                            graph => $this
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
                            my $node = $nodeClass->new({
                                col => $col,
                                row => $row,
                                tm => $tm,
                                graph => $this
                            });
                            if (! defined $node) { 
                                ERROR(sprintf "Cannot create Node for level %s, indices %s,%s.", $this->{bottomID}, $col, $row);
                                return FALSE;
                            }
                            
                            $this->{nodes}->{$bottomID}->{$nodeKey} = $node;
                        }

                        $this->{nodes}->{$bottomID}->{$nodeKey}->addGeoImage($objImg);
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
                        my $node = $nodeClass->new({
                            col => $col,
                            row => $row,
                            tm => $tm,
                            graph => $this
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
            my $node = $nodeClass->new({
                col => $col,
                row => $row,
                tm => $tm,
                graph => $this
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

(see ROK4GENERATION/aboveNodes_QTree.png)
=cut
sub identifyAboveNodes {
    my $this = shift;

    my $nodeClass;
    if (ref ($this->{pyramid}) eq "COMMON::PyramidVector") {
        $nodeClass = 'FOURALAMO::Node';
    } elsif(ref ($this->{pyramid}) eq "COMMON::PyramidRaster") {
        $nodeClass = 'BE4::Node';
    }
    
    # initialisation pratique:
    my $tms = $this->{pyramid}->getTileMatrixSet();
    my $src = $this->{datasource};
    
    # Calcul des branches à partir des feuilles
    for (my $i = $src->getBottomOrder(); $i <= $src->getTopOrder(); $i++){
        my $levelID = $tms->getIDfromOrder($i);

        # pyramid's limits update : we store data's limits in the pyramid's levels
        $this->{pyramid}->updateTMLimits($levelID, @{$this->{bbox}});
        DEBUG(sprintf "Number of cache images by level (%s) : %d", $levelID, scalar keys(%{$this->{nodes}{$levelID}}));

        if ($i == $src->getTopOrder()) { last; }

        # On va calculer les noeuds du niveau du dessus
        my $aboveLevelID = $tms->getIDfromOrder($i+1);

        foreach my $node (values (%{$this->{nodes}->{$levelID}})) {
                        
            my $parentNodeKey = int($node->getCol/2)."_".int($node->getRow/2);
            if (exists $this->{nodes}->{$aboveLevelID}->{$parentNodeKey}) {
                # This Node already exists
                next;
            }
            # Create a new Node
            my $node = $nodeClass->new({
                col => int($node->getCol/2),
                row => int($node->getRow/2),
                tm => $tms->getTileMatrix($aboveLevelID),
                graph => $this
            });
            if (! defined $node) { 
                ERROR(sprintf "Cannot create Node for level %s, indices %s,%s.", $aboveLevelID, int($node->getRow/2), int($node->getRow/2));
                return FALSE;
            }
            $this->{nodes}->{$aboveLevelID}->{$parentNodeKey} = $node;
        }
    }
    
    return TRUE;
}

####################################################################################################
#                                   Group: Compute methods                                         #
####################################################################################################

=begin nd
Function: computeYourself

Browse graph and write scripts.
=cut
sub computeYourself {
    my $this = shift;
    
    foreach my $topNode (values (%{$this->{nodes}->{$this->{topID}}})) {
        if ($this->{topID} eq $this->{cutLevelID}) {
            $topNode->setScript($this->{scripts}->{splits}->[$this->{scripts}->{current}]);
            $this->{scripts}->{current} = ($this->{scripts}->{current} + 1) % $this->{scripts}->{number};
        } else {
            $topNode->setScript($this->{scripts}->{finisher});
        }

        if (! $this->computeBranch($topNode)) {
            ERROR(sprintf "Can not compute the node of the top level '%s'!", $topNode->getWorkBaseName());
            return FALSE;
        }
    }

    return TRUE;
}

=begin nd
Function: computeBranch

Recursive method, which allow to browse tree downward.

Raster case:
    - the node belong to the bottom level -> <computeBottomImage>
    - the node does not belong to the bottom level -> <computeBranch> on each child, then <computeAboveImage>

Vector case:
    - the node belong to the top level -> <computeTopImage> then <computeBranch> on each child
    - the node does not belong to the top level -> <computeBelowImage> then <computeBranch> on each child

Parameters (list):
    node - <BE4::Node> or <FOURALAMO::Node> - Node to compute.
=cut
sub computeBranch {
    my $this = shift;
    my $node = shift;

    my @childList = $this->getChildren($node);

    if (ref ($this->{pyramid}) eq "COMMON::PyramidVector") {
        # En vecteur, un noeud du niveau du haut lance un tippecanoe qui va générer 
        # toutes les tuiles jusqu'au niveau du bas. On parcours ensuite tous les noeuds de la branche
        # pour lancer la mise en dalle ROK4

        if ($node->getLevel() eq $this->getTopID()) {
            # ogr2ogr + tippecanoe + pbf2cache
            if (! $this->computeTopImage($node)) {
                ERROR(sprintf "Cannot compute the top image : %s",$node->getWorkBaseName());
                return FALSE;
            }
        } else {
            # pbf2cache
            if (! $this->computeBelowImage($node)) {
                ERROR(sprintf "Cannot compute the bellow image : %s",$node->getWorkBaseName());
                return FALSE;
            }
        }

        foreach my $n (@childList) {
            if ($n->getLevel() eq $this->{cutLevelID}) {
                $n->setScript($this->{scripts}->{splits}->[$this->{scripts}->{current}]);
                $this->{scripts}->{current} = ($this->{scripts}->{current} + 1) % $this->{scripts}->{number};
            } else {
                $n->setScript($node->getScript());
            }
            if (! $this->computeBranch($n)) {
                ERROR(sprintf "Cannot compute the branch from node %s", $node->getWorkBaseName());
                return FALSE;
            }
        }

    }
    elsif (ref ($this->{pyramid}) eq "COMMON::PyramidRaster") {
        if (scalar @childList == 0){
            if (! $this->computeBottomImage($node)) {
                ERROR(sprintf "Cannot compute the bottom image : %s",$node->getWorkBaseName());
                return FALSE;
            }
            return TRUE;
        }
        foreach my $n (@childList) {
            if ($n->getLevel() eq $this->{cutLevelID}) {
                $n->setScript($this->{scripts}->{splits}->[$this->{scripts}->{current}]);
                $this->{scripts}->{current} = ($this->{scripts}->{current} + 1) % $this->{scripts}->{number};
            } else {
                $n->setScript($node->getScript());
            }
            if (! $this->computeBranch($n)) {
                ERROR(sprintf "Cannot compute the branch from node %s", $node->getWorkBaseName());
                return FALSE;
            }
        }

        if (! $this->computeAboveImage($node)) {
            ERROR(sprintf "Cannot compute the above image : %s", $node->getWorkBaseName());
            return FALSE;
        }
    }

    return TRUE;
}

#### Fonctionnement RASTER

=begin nd
Function: computeBottomImage

Treats a bottom node for a raster pyramid : write code.

2 cases:
    - images as data -> <BE4::Node::mergeNtiff>
    - WMS service as data -> <BE4::Node::wms2work>

Then the work image is formatted and move to the final place thanks to <BE4::Node::work2cache>.

Parameters (list):
    node - <BE4::Node> - Bottom level's node, to treat.
    
=cut
sub computeBottomImage {
    my $this = shift;
    my $node = shift;
        
    if ($this->getDataSource()->hasHarvesting()) {
        # Datasource has a WMS service : we have to use it
        if (! $node->wms2work($this->getDataSource()->getHarvesting())) {
            ERROR(sprintf "Cannot harvest image for node %s", $node->getWorkBaseName());
            return FALSE;
        }
    } else {    
        if (! $node->mergeNtiff()) {
            ERROR(sprintf "Cannot compose mergeNtiff command for the node %s.",$node->getWorkBaseName());
            return FALSE;
        }
    }

    $node->work2cache();

    return TRUE;
}

=begin nd
Function: computeAboveImage

Treats an above node for a raster pyramid (different to the bottom level) : write code.

To generate an above node, we use <BE4::Node::merge4tiff> with children.

Then the work image is formatted and move to the final place thanks to <BE4::Node::work2cache>.

Parameters (list):
    node - <BE4::Node> - Above level's node, to treat.
=cut
sub computeAboveImage {
    
    my $this = shift;
    my $node = shift;
    
    # Maintenant on constitue la liste des images à passer à merge4tiff.
    if (! $node->merge4tiff()) {
        ERROR(sprintf "Cannot compose merge4tiff command for the node %s.",$node->getWorkBaseName());
        return FALSE;
    }

    $node->work2cache();

    return TRUE;
}

#### Fonctionnement VECTEUR

=begin nd
Function: computeTopImage

Treats a top node for a vector pyramid : write code.

The call to ogr2ogr extract all data contained in the node. The call to tippecanoe generate all tiles below this node

Finally the call to pbf2cache generate the vector slab.

Parameters (list):
    node - <FOURALAMO::Node> - Top level's node, to treat.
    
=cut
sub computeTopImage {
    
    my $this = shift;
    my $node = shift;
     
    if (! $node->makeJsons($this->getDataSource()->getDatabaseSource())) {
        ERROR(sprintf "Cannot compose ogr2ogrs command for the node %s.",$node->getWorkBaseName());
        return FALSE;
    }

    if (! $node->makeTiles()) {
        ERROR(sprintf "Cannot compose tippecanoe command for the node %s.",$node->getWorkBaseName());
        return FALSE;
    }

    $node->pbf2cache();

    return TRUE;
}

=begin nd
Function: computeBelowImage

Treats a below node for a vector pyramid : write code. Tiles have been generated by tippecanoe, we juste have to call pbf2cache to generater the vector slab.

Parameters (list):
    node - <FOURALAMO::Node> - Below level's node, to treat.
=cut
sub computeBelowImage {
    
    my $this = shift;
    my $node = shift;
    
    # Maintenant on constitue la liste des images à passer à pbf2cache.
    if (! $node->pbf2cache()) {
        ERROR(sprintf "Cannot compose pbf2cache command for the node %s.",$node->getWorkBaseName());
        return FALSE;
    }

    return TRUE;
}

####################################################################################################
#                                Group: Getters - Setters                                          #
####################################################################################################

# Function: getDataSource
sub getDataSource {
    my $this = shift;
    return $this->{datasource};
}

# Function: getPyramid
sub getPyramid {
    my $this = shift;
    return $this->{pyramid};
}

# Function: getCoordTransPyramidDatasource
sub getCoordTransPyramidDatasource {
    my $this = shift;
    return $this->{ct_pyramid_source};
}

# Function: getCutLevelID
sub getCutLevelID {
    my $this = shift;
    return $this->{cutLevelID};
}

# Function: getTopID
sub getTopID {
    my $this = shift;
    return $this->{topID};
}

# Function: getTopOrder
sub getTopOrder {
    my $this = shift;
    return $this->{pyramid}->getTileMatrixSet()->getOrderfromID($this->{topID});
}

# Function: getBottomID
sub getBottomID {
    my $this = shift;
    return $this->{bottomID};
}

# Function: getBottomOrder
sub getBottomOrder {
    my $this = shift;
    return $this->{pyramid}->getTileMatrixSet()->getOrderfromID($this->{bottomID});
}

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
Function: getPossibleChildren

Returns a <BE4::Node> or <FOURALAMO::Node> array, containing children (length is always 4, with undefined value for children which don't exist), an empty array if the node is a leaf.

Warning:
    Do not mistake with <getChildren>

Parameters (list):
    node - <BE4::Node> or <FOURALAMO::Node> - Node whose we want to know possible children.
=cut
sub getPossibleChildren {
    my $this = shift;
    my $node = shift;
    
    my @res;
    if ($node->getLevel eq $this->{bottomID}) {
        return @res;
    }
    
    my $lowerLevelID = $this->{pyramid}->getTileMatrixSet->getBelowLevelID($node->getLevel);
    
    for (my $j=0; $j<=1; $j++){
        for (my $i=0; $i<=1; $i++){
            my $nodeKey = sprintf "%s_%s",$node->getCol*2+$i, $node->getRow*2+$j;
            if (exists $this->{nodes}->{$lowerLevelID}->{$nodeKey}) {
                push @res, $this->{nodes}->{$lowerLevelID}->{$nodeKey};
            } else {
                push @res, undef;
            }
        }
    }
    
    return @res;
}

=begin nd
Function: getChildren

Returns a <BE4::Node> or <FOURALAMO::Node> array, containing real children (max length = 4), an empty array if the node is a leaf.

Warning:
    Do not mistake with <getPossibleChildren>

Parameters (list):
    node - <BE4::Node> or <FOURALAMO::Node> - Node whose we want to know children.
=cut
sub getChildren {
    my $this = shift;
    my $node = shift;
    
    my @res;
    if ($node->getLevel eq $this->{bottomID}) {
        return @res;
    }
    
    my $lowerLevelID = $this->{pyramid}->getTileMatrixSet->getBelowLevelID($node->getLevel);
    
    for (my $j=0; $j<=1; $j++){
        for (my $i=0; $i<=1; $i++){
            my $nodeKey = sprintf "%s_%s",$node->getCol*2+$i, $node->getRow*2+$j;
            if (exists $this->{nodes}->{$lowerLevelID}->{$nodeKey}) {
                push @res, $this->{nodes}->{$lowerLevelID}->{$nodeKey};
            }
        }
    }
    
    return @res;
}

1;
__END__
