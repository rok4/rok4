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

package BE4::QTree;

use strict;
use warnings;

use Math::BigFloat;
use Geo::OSR;
use Geo::OGR;
use Data::Dumper;

use BE4::DataSource;
use BE4::Node;
use BE4::Array;

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

################################################################################
=begin nd
Group: variable

variable: $self
    * forest : BE4::Forest
    * pyramid : BE4::Pyramid
    * commands : BE4::Commands
    * datasource : BE4::DataSource
    
    * bbox - datasource bbox, [xmin,ymin,xmax,ymax], in TMS' SRS
    * nodes : hash
|   level1 => {
|      x1_y2 => n1,
|      x2_y2 => n2,
|      x3_y2 => n3, ...}
|   level2 => { 
|      x1_y2 => n4,
|      x2_y2 => n5, ...}
|
|   nX : BE4::Node

    * cutLevelID : string
    * bottomID : string
    * topID : string
=cut

####################################################################################################
#                                       CONSTRUCTOR METHODS                                        #
####################################################################################################

# Group: constructor

#
=begin nd
method: new

Parameters:
    objForest - BE4::Forest in which this tree is.
    objSrc - BE4::DataSource, used to defined nodes
    objPyr - BE4::Pyramid
    objCommands - BE4::Commands, used to compute tree
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
        # levels
        cutLevelID    => undef,
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

#
=begin nd
method: _init

Parameters:
    objForest - BE4::Forest in which this tree is.
    objSrc - BE4::DataSource, used to defined nodes
    objPyr - BE4::Pyramid
    objCommands - BE4::Commands, used to compute tree
=cut
sub _init {
    my $self = shift;
    my $objForest = shift;
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

#
=begin nd
method: _load

Determine all nodes from the bottom level to the top level, thanks to the dta source.
=cut
sub _load {
    my $self = shift;

    TRACE;

    # initialisation pratique:
    my $tms = $self->{pyramid}->getTileMatrixSet;
    my $src = $self->{datasource};
    
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
        $src->getExtent->AssignSpatialReference($srsini);
        
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
#                                 NODES DETERMINATION METHODS                                      #
####################################################################################################

# Group: nodes determination methods

#
=begin nd
method: identifyBottomNodes

Calculate all nodes in bottom level concerned by the datasource (tiles which touch the data source extent).

Parameters:
    ct - a Geo::OSR::CoordinateTransformation object, to convert data extent or images' bbox.
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
    } else {
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

                my $GMLtile = sprintf "<gml:Polygon><gml:outerBoundaryIs><gml:LinearRing><gml:coordinates>%s,%s %s,%s %s,%s %s,%s %s,%s</gml:coordinates></gml:LinearRing></gml:outerBoundaryIs></gml:Polygon>",
                    $xmin,$ymin,
                    $xmin,$ymax,
                    $xmax,$ymax,
                    $xmax,$ymin,
                    $xmin,$ymin;
                
                my $OGRtile = Geo::OGR::Geometry->create(GML=>$GMLtile);
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
                        ERROR(sprintf "Cannot create Node for level %s, indices %s,%s.",
                              $self->{bottomID}, $i, $j);
                        return FALSE;
                    }
                    $self->{nodes}->{$bottomID}->{$nodeKey} = $node;
                }
            }
        }
    }
  
    return TRUE;  
}

#
=begin nd
method: identifyAboveNodes

Calculate all nodes in above levels concerned by the datasource (tiles which touch the data source extent).
=cut
sub identifyAboveNodes {
    my $self = shift;
    
    
    # initialisation pratique:
    my $tms = $self->{pyramid}->getTileMatrixSet;
    my $src = $self->{datasource};
    
    # Calcul des branches à partir des feuilles
    for (my $i = $src->getBottomOrder; $i <= $src->getTopOrder; $i++){
        my $levelID = $tms->getIDfromOrder($i);

        # pyramid's limits update : we store data's limits in the pyramid's levels
        $self->{pyramid}->updateTMLimits($levelID,@{$self->{bbox}});

        foreach my $node ($self->getNodesOfLevel($levelID)) {
            
            if ($i != $src->getTopOrder) {
                my $aboveLevelID = $tms->getIDfromOrder($i+1);
                my $parentNodeKey = int($node->getCol/2)."_".int($node->getRow/2);
                if (exists $self->{nodes}->{$aboveLevelID}->{$parentNodeKey}) {
                    # This Node already exists
                    next;
                }
                # Create a new Node
                my $node = BE4::Node->new({
                    i => int($node->getCol/2),
                    j => int($node->getRow/2),
                    tm => $tms->getTileMatrix($aboveLevelID),
                    graph => $self,
                });
                if (! defined $node) { 
                    ERROR(sprintf "Cannot create Node for level %s, indices %s,%s.",
                          $aboveLevelID, int($node->getRow/2), int($node->getRow/2));
                    return FALSE;
                }
                $self->{nodes}->{$aboveLevelID}->{$parentNodeKey} = $node;
            }
        }

        DEBUG(sprintf "Number of cache images by level (%s) : %d",
              $levelID, scalar keys(%{$self->{nodes}{$levelID}}));
    }
    
    return TRUE;  
}

####################################################################################################
#                                          COMPUTE METHODS                                         #
####################################################################################################

# Group: compute methods

#
=begin nd
method: computeYourself

Determine codes and weights for each node of the current QTree, and share work on scripts, so as to optimize execution time.

Three steps:
    - browse QTree : add weight and code to the nodes.
    - determine the cut level, to distribute fairly work.
    - browse the QTree once again: we write commands in different scripts.

Parameter:
    NEWLIST - stream to the cache's list, to add new images.
    
See Also:
    <computeBranch>, <shareNodesOnJobs>, <writeBranchCode>, <writeTopCode>
=cut
sub computeYourself {
    my $self = shift;

    TRACE;
    
    my @topLevelNodeList = $self->getNodesOfTopLevel;
    
    # --------------------------- WEIGHT --------------------------------
    # Pondération de l'arbre en fonction des opérations à réaliser.
    foreach my $topNode (@topLevelNodeList) {
        if (! $self->computeBranch($topNode,TRUE)) {
            ERROR(sprintf "Can not weight the node of the top level '%s'!", $topNode->getWorkBaseName);
            return FALSE;
        }
    }
    
    # -------------------------- SHARING --------------------------------
    # Détermination du cutLevel optimal et répartition des noeuds sur les jobs,
    # en tenant compte du fait qu'ils peuvent déjà contenir du travail, du fait
    # de la pluralité des arbres à traiter.
    
    $self->shareNodesOnJobs;

    if (! defined $self->{cutLevelID}) {
        ERROR("Impssible to determine the cut level !");
        return FALSE;
    }
    INFO (sprintf "CutLevel : %s", $self->{cutLevelID});
    
    # ----------------------- CODE GENERATION ---------------------------
    # Création du code script (ajouté aux noeuds de l'arbre).
    foreach my $topNode (@topLevelNodeList) {
        # Top level nodes will be generated by the script finisher, if it is not the cut level too.
        # Cut level nodes' scripts have been determined by shareJobsOnNodes.
        if ($self->{cutLevelID} ne $self->{topID}) {
            $topNode->setScript($self->getScriptFinisher);
        }

        if (! $self->computeBranch($topNode,FALSE)) {
            ERROR(sprintf "Can not compute code for the node of the top level '%s'!", $topNode->getWorkBaseName);
            return FALSE;
        }
    }
    
    # ---------------------------- WRITTING -----------------------------
    # Split scripts
    my @cutLevelNodes = $self->getNodesOfCutLevel();
    foreach my $node (@cutLevelNodes) {
        $node->getScript->print(
            sprintf("echo \"NODE : LEVEL:%s X:%s Y:%s\"\n", $node->getLevel, $node->getCol, $node->getRow));
        $self->writeBranchCode($node);
    }
    
    # Final script    
    if ($self->getTopID eq $self->getCutLevelID){
        INFO("Final script will be empty");
        $self->printInFinisher("echo \"Final script have nothing to do.\" \n");
    } else {
        my @nodeList = $self->getNodesOfTopLevel;
        foreach my $node (@nodeList){
            $self->writeTopCode($node);
        }
    }
    
    return TRUE;
}

#
=begin nd
method: computeBranch

Recursive method, which allow to browse tree downward.

2 cases.
    - the node belong to the bottom level -> computeBottomImage
    - the node does not belong to the bottom level -> computeBranch on each child, then computeAboveImage

Parameter:
    node - BE4::Node, to treat.
    justWeight - boolean, if TRUE, we want to weight node, if FALSE, we want to compute code for the node.
    
See Also:
    <computeBottomImage>, <computeAboveImage>
=cut
sub computeBranch {
    
    my $self = shift;
    my $node = shift;
    my $justWeight = shift;

    my $weight = 0;

    TRACE;
    
    my $res = '';
    my @childList = $self->getChildren($node);
    if (scalar @childList == 0){
        if (! $self->computeBottomImage($node,$justWeight)) {
            ERROR(sprintf "Cannot compute the bottom image : %s",$node->getWorkName);
            return FALSE;
        }
        return TRUE;
    }
    foreach my $n (@childList) {
        # Children will be generated by the same script as their parent, except for cut level nodes.
        # Cut level nodes' scripts have been determined by shareJobsOnNodes.
        if (! $justWeight && ($n->getLevel ne $self->{cutLevelID})) {
            $n->setScript($node->getScript);
        }
        
        if (! $self->computeBranch($n,$justWeight)) {
            ERROR(sprintf "Cannot compute the branch from node %s", $node->getWorkBaseName);
            return FALSE;
        }
        $weight += $n->getAccumulatedWeight;
    }

    if (! $self->computeAboveImage($node,$justWeight)) {
        ERROR(sprintf "Cannot compute the above image : %s", $node->getWorkName);
        return FALSE;
    }

    $node->setAccumulatedWeight($weight) if $justWeight;

    return TRUE;
}

#
=begin nd
method: computeBottomImage

Treat a bottom node : determine code or weight.

2 cases.
    - native projection, lossless compression and images as data -> mergeNtiff
    - reprojection or lossy compression or just a WMS service as data -> wget

Parameter:
    node - bottom level's BE4::Node, to treat.
    justWeight - boolean, if TRUE, we want to weight node, if FALSE, we want to compute code for the node.
    
=cut
sub computeBottomImage {
    
    my $self = shift;
    my $node = shift;
    my $justWeight = shift;

    TRACE;
    
    # Temporary weight and code
    my ($c,$w);
    # Final weight and code
    my $weight  = 0;
    my $code  = "\n";
    
    if ($self->getDataSource->hasHarvesting) {
        # Datasource has a WMS service : we have to use it
        ($c,$w) = $self->{commands}->wms2work($node,$self->getDataSource->getHarvesting,$justWeight,"I");
        if (! defined $c) {
            ERROR(sprintf "Cannot harvest image for node %s",$node->getWorkBaseName("I"));
            return FALSE;
        }
        
        $code .= $c;
        $weight += $w;
    } else {    
        ($c,$w) = $self->{commands}->mergeNtiff($node,$justWeight);
        if ($w == -1) {
            ERROR(sprintf "Cannot compose mergeNtiff command for the node %s.",$node->getWorkBaseName);
            return FALSE;
        }
        $code .= $c;
        $weight += $w;
    }

    # copie de l'image de travail créée dans le rep temp vers l'image de cache dans la pyramide.
    my $after = "none";
    if (! $justWeight && $node->getLevel eq $self->getTopID) {
        $after = "remove";
    } elsif (! $justWeight && $node->getLevel eq $self->getCutLevelID) {
        # Work images have to be put in the final script temporary directory : the root
        $after = "move";
    }
    ($c,$w) = $self->{commands}->work2cache($node,"\${TMP_DIR}",$after);
    $code .= $c;
    $weight += $w;

    if ($justWeight) {
        $node->setOwnWeight($weight);
        $node->setAccumulatedWeight(0);
    } else {
        $node->setCode($code);
    }

    return TRUE;
}

#
=begin nd
method: computeAboveImage

Treat an above node (different to the bottom level) : determine code or weight.

To generate an above node, we use children (merge4tiff). If we have not 4 children or if children contain nodata, we have to supply a background, a color or an image if exists.

Parameter:
    node - above level's BE4::Node, to treat.
    justWeight - boolean, if TRUE, we want to weight node, if FALSE, we want to compute code for the node.
=cut
sub computeAboveImage {
    
    my $self = shift;
    my $node = shift;
    my $justWeight = shift;
    $justWeight = FALSE if (! defined $justWeight);

    TRACE;

    # Temporary weight and code
    my ($c,$w);
    # Final weight and code
    my $weight  = 0;
    my $code  = "\n";
    
    # Maintenant on constitue la liste des images à passer à merge4tiff.
    ($c,$w) = $self->{commands}->merge4tiff($node,$self->getDataSource->getHarvesting,$justWeight);
    if ($w == -1) {
        ERROR(sprintf "Cannot compose merge4tiff command for the node %s.",$node->getWorkBaseName);
        return FALSE;
    }
    $code .= $c;
    $weight += $w;

    # Copie de l'image de travail crée dans le rep temp vers l'image de cache dans la pyramide.
    my $after = "none";
    if (! $justWeight && $node->getLevel eq $self->getTopID) {
        $after = "remove";
    } elsif (! $justWeight && $node->getLevel eq $self->getCutLevelID) {
        # Work images have to be put in the final script temporary directory : the root
        $after = "move";
    }
    ($c,$w) = $self->{commands}->work2cache($node,"\${TMP_DIR}",$after);
    $code .= $c;
    $weight += $w;
    
    if ($justWeight) {
        $node->setOwnWeight($weight);
    } else {
        $node->setCode($code);
    }

    return TRUE;
}

####################################################################################################
#                                          WRITER METHODS                                          #
####################################################################################################

# Group: writer methods

#
=begin nd
method: writeBranchCode

Recursive method, which allow to browse tree (downward) and concatenate node's commands.

Parameter:
    node - BE4::Node whose code is written.
=cut
sub writeBranchCode {
    my $self = shift;
    my $node = shift;

    TRACE;

    my $code = '';
    my @childList = $self->getChildren($node);

    # Le noeud est une feuille
    if (scalar @childList == 0){
        $node->writeInScript;
        return TRUE;
    }

    # Le noeud a des enfants
    foreach my $n (@childList){
        $self->writeBranchCode($n);
    }
    
    $node->writeInScript;

    return TRUE;
}

#
=begin nd
method: writeTopCode

Recursive method, which allow to browse downward the tree, from the top, to the cut level and write commands in the script finisher.

Parameter:
    node - BE4::Node whose code is written.
=cut
sub writeTopCode {
    my $self = shift;
    my $node = shift;

    TRACE;

    # Rien à faire, le niveau CutLevel est déjà fait et les images de travail sont déjà là. 
    return TRUE if ($node->getLevel eq $self->getCutLevelID);

    my @childList = $self->getChildren($node);
    foreach my $n (@childList){
        $self->writeTopCode($n);
    }
    
    $node->writeInScript;

    return TRUE;
}

####################################################################################################
#                                         CUT LEVEL METHODS                                        #
####################################################################################################

# Group: cut level methods

#
=begin nd
method: shareNodesOnJobs

Determine the cutLevel to optimize sharing into scripts and execution time.

=cut
sub shareNodesOnJobs {
    my $self = shift;

    TRACE;

    my $tms = $self->{pyramid}->getTileMatrixSet;
    my $splitNumber = $self->{forest}->getSplitNumber;
    
    my $optimalWeight = undef;
    my $cutLevelID = undef;
    
    my @INIT_WEIGHTS = undef;
    my @jobsWeights = undef;

    # calcul du poids total de l'arbre : c'est la somme des poids cumulé des noeuds du topLevel
    my $wholeTreeWeight = 0;
    my @topLevelNodeList = $self->getNodesOfTopLevel;
    foreach my $node (@topLevelNodeList) {
        $wholeTreeWeight += $node->getAccumulatedWeight;
    }
    
    for (my $j = 0; $j <= $splitNumber; $j++) {
        # On initialise les poids avec ceux des jobs
        $INIT_WEIGHTS[$j] = $self->{forest}->getWeightOfScript($j);
    }

    for (my $i = $self->getTopOrder(); $i >= $self->getBottomOrder(); $i--) {
        my $levelID = $tms->getIDfromOrder($i);
        my @levelNodeList = $self->getNodesOfLevel($levelID);
        
        if ($levelID ne $self->{bottomID} && scalar @levelNodeList < $splitNumber) {
            next;
        }
        
        @levelNodeList = sort {$b->getAccumulatedWeight <=> $a->getAccumulatedWeight} @levelNodeList;

        my @TMP_WEIGHTS = @INIT_WEIGHTS;
        
        my $finisherWeight = $wholeTreeWeight;
        
        for (my $j = 0; $j < scalar @levelNodeList; $j++) {
            my $scriptInd = BE4::Array->minArrayIndex(1,@TMP_WEIGHTS);
            my $nodeWeight = $levelNodeList[$j]->getAccumulatedWeight;
            $TMP_WEIGHTS[$scriptInd] += $nodeWeight;
            $finisherWeight -= $nodeWeight;
            $levelNodeList[$j]->setScript($self->{forest}->getScript($scriptInd));
        }
        
        # on additionne le poids du job le plus "lourd" et le poids du finisher pour quantifier le
        # pire temps d'exécution
        $TMP_WEIGHTS[0] += $finisherWeight;
        my $worstWeight = BE4::Array->maxArrayValue(1,@TMP_WEIGHTS) + $finisherWeight;
        
        DEBUG(sprintf "For the level $levelID, the worst weight is $worstWeight.");

        # on compare ce pire des cas avec celui obtenu jusqu'ici. S'il est plus petit, on garde ce niveau comme
        # cutLevel (a priori celui qui optimise le temps total de la génération de la pyramide).
        if (! defined $optimalWeight || $worstWeight < $optimalWeight) {
            $optimalWeight = $worstWeight;
            $cutLevelID = $levelID;
            @jobsWeights = @TMP_WEIGHTS;
            DEBUG (sprintf "New cutLevel found : %s (worstWeight : %s)",$levelID,$optimalWeight);
        }
    }
    
    # We store results in array references
    for (my $i = 0; $i <= $splitNumber; $i++) {
        $self->{forest}->setWeightOfScript($i,$jobsWeights[$i]);
    }

    $self->{cutLevelID} = $cutLevelID;
}


####################################################################################################
#                                         GETTERS / SETTERS                                        #
####################################################################################################

# Group: getters - setters

sub getDataSource{
    my $self = shift;
    return $self->{datasource};
}

sub getPyramid{
    my $self = shift;
    return $self->{pyramid};
}

sub getCutLevelID {
    my $self = shift;
    return $self->{cutLevelID};
}

sub getTopID {
    my $self = shift;
    return $self->{topID};
}

sub getTopOrder {
    my $self = shift;
    return $self->{pyramid}->getTileMatrixSet->getOrderfromID($self->{topID});
}

sub getBottomOrder {
    my $self = shift;
    return $self->{pyramid}->getTileMatrixSet->getOrderfromID($self->{bottomID});
}

sub getScriptFinisher {
    my $self = shift;
    return $self->{forest}->getScript(0); 
}

sub printInFinisher {
    my $self = shift;
    my $text = shift;
    
    $self->{forest}->getScript(0)->print($text); 
}

#
=begin nd
method: containsNode

Parameters:
    level - level of the node we want to know if it is in the qtree.
    x - x coordinate of the node we want to know if it is in the qtree.
    y - y coordinate of the node we want to know if it is in the qtree.

Returns:
    A boolean : TRUE if the node exists, FALSE otherwise.
=cut
sub containsNode {
    my $self = shift;
    my $level = shift;
    my $x = shift;
    my $y = shift;
  
    return FALSE if (! defined $level);
    
    my $nodeKey = $x."_".$y;
    return (exists $self->{nodes}->{$level}->{$nodeKey});
}

#
=begin nd
method: updateBBox

Compare old extrems coordinates and update values.

Parameters:
    xmin, ymin, xmax, ymax - new coordinates to compare with current bbox.
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

#
=begin nd
method: getPossibleChildren

Parameters:
    node - BE4::Node whose we want to know children.

Returns:
    An array of the real children from a node (length is always 4, with undefined value for children which don't exist), an empty array if the node is a leaf.
=cut
sub getPossibleChildren {
    my $self = shift;
    my $node = shift;
    
    my @res;
    if ($node->getLevel eq $self->{bottomID}) {
        return @res;
    }
    
    my $lowerLevelID = $self->{pyramid}->getTileMatrixSet->getBelowLevelID($node->getLevel);
    
    for (my $j=0; $j<=1; $j++){
        for (my $i=0; $i<=1; $i++){
            my $nodeKey = sprintf "%s_%s",$node->getCol*2+$i, $node->getRow*2+$j;
            if (exists $self->{nodes}->{$lowerLevelID}->{$nodeKey}) {
                push @res, $self->{nodes}->{$lowerLevelID}->{$nodeKey};
            } else {
                push @res, undef;
            }
        }
    }
    
    return @res;
}

#
=begin nd
method: getChildren

Parameters:
    node - BE4::Node whose we want to know children.

Returns:
    An array of the real children from a node (max length = 4), an empty array if the node is a leaf.
=cut
sub getChildren {
    my $self = shift;
    my $node = shift;
    
    my @res;
    if ($node->getLevel eq $self->{bottomID}) {
        return @res;
    }
    
    my $lowerLevelID = $self->{pyramid}->getTileMatrixSet->getBelowLevelID($node->getLevel);
    
    for (my $j=0; $j<=1; $j++){
        for (my $i=0; $i<=1; $i++){
            my $nodeKey = sprintf "%s_%s",$node->getCol*2+$i, $node->getRow*2+$j;
            if (exists $self->{nodes}->{$lowerLevelID}->{$nodeKey}) {
                push @res, $self->{nodes}->{$lowerLevelID}->{$nodeKey};
            }
        }
    }
    
    return @res;
}

sub getNodesOfLevel {
    my $self = shift;
    my $levelID= shift;
    
    if (! defined $levelID) {
        ERROR("Undefined Level");
        return undef;
    }
    
    return values (%{$self->{nodes}->{$levelID}});
}

sub getNodesOfTopLevel {
    my $self = shift;
    return $self->getNodesOfLevel($self->{topID});
}

sub getNodesOfCutLevel {
    my $self = shift;
    return $self->getNodesOfLevel($self->{cutLevelID});
}

sub getNodesOfBottomLevel {
    my $self = shift;
    return $self->getNodesOfLevel($self->{bottomID});
}

####################################################################################################
#                                          EXPORT METHODS                                          #
####################################################################################################

# Group: export methods

sub exportForDebug {
    my $self = shift ;
    
    my $export = "";
    
    $export .= sprintf "\nObject BE4::QTree :\n";
    $export .= sprintf "\t Levels ID:\n";
    $export .= sprintf "\t\t- bottom : %s\n",$self->{bottomID};
    $export .= sprintf "\t\t- cut : %s\n",$self->{cutLevelID};
    $export .= sprintf "\t\t- top : %s\n",$self->{topID};

    $export .= sprintf "\t Number of nodes per level :\n";
    foreach my $level ( keys %{$self->{nodes}} ) {
        $export .= sprintf "\t\tLevel %s : %s node(s)\n",$level,scalar (keys %{$self->{nodes}->{$level}});
    }
    
    $export .= sprintf "\t Bbox (SRS : %s) :\n",$self->{pyramid}->getTileMatrixSet->getSRS;
    $export .= sprintf "\t\t- xmin : %s\n",$self->{bbox}[0];
    $export .= sprintf "\t\t- ymin : %s\n",$self->{bbox}[1];
    $export .= sprintf "\t\t- xmax : %s\n",$self->{bbox}[2];
    $export .= sprintf "\t\t- ymax : %s\n",$self->{bbox}[3];
    
    return $export;
}

1;
__END__

=head1 NAME

BE4::QTree - Representation of a quad tree cache : cache image = node

=head1 SYNOPSIS

    use BE4::QTree;

    # QTree object creation
    my $objQTree = BE4::QTree->new($objForest, $objDataSource, $objPyramid, $objCommands);
    
    ...
    
    # Fill each node with computing code, weight, share job on scripts
    $objQTree->computeYourself();

=head1 DESCRIPTION

=head2 ATTRIBUTES

=over 4

=item forest

A BE4::Forest object.

=item pyramid

A BE4::Pyramid object.

=item commands

A BE4::Commands object.

=item datasource

A BE4::Datasource object.

=item bbox

Array [xmin,ymin,xmax,ymax], bbox of datasource in the TMS' SRS.

=item nodes

An hash, composition of each node in the tree (code to generate the node, own weight, accumulated weight):

    level1 => {
        x1_y2 => n1,
        x2_y2 => n2,
        x3_y2 => n3, ...}
    level2 => { 
        x1_y2 => n4,
        x2_y2 => n5, ...}
        
    nX : BE4::Node

=item cutLevelID

Split scripts will generate cache to this level. Script finisher will be generate above.

=item bottomID, topID

Extrem levels identifiants of the tree.

=back

=head1 SEE ALSO

=head2 POD documentation

=begin html

<ul>
<li><A HREF="./lib-BE4-Forest.html">BE4::Forest</A></li>
<li><A HREF="./lib-BE4-DataSource.html">BE4::DataSource</A></li>
<li><A HREF="./lib-BE4-Pyramid.html">BE4::Pyramid</A></li>
<li><A HREF="./lib-BE4-Commands.html">BE4::Commands</A></li>
<li><A HREF="./lib-BE4-Node.html">BE4::Node</A></li>
</ul>

=end html

=head2 NaturalDocs

=begin html

<A HREF="../Natural/Html/index.html">Index</A>

=end html

=head1 AUTHOR

Satabin Théo, E<lt>theo.satabin@ign.frE<gt>

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2011 by Satabin Théo

This library is free software; you can redistribute it and/or modify it under the same terms as Perl itself, either Perl version 5.10.1 or, at your option, any later version of Perl 5 you may have available.

=cut
