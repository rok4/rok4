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

package BE4::Tree;

use strict;
use warnings;

use Math::BigFloat;
use Geo::OSR;
use Geo::OGR;
use Data::Dumper;

use BE4::DataSource;
use BE4::ImageDesc;

use Log::Log4perl qw(:easy);

require Exporter;
use AutoLoader qw(AUTOLOAD);

our @ISA = qw(Exporter);

our %EXPORT_TAGS = ( 'all' => [ qw() ] );
our @EXPORT_OK   = ( @{$EXPORT_TAGS{'all'}} );
our @EXPORT      = qw();

################################################################################
# Constantes
# booleans
use constant TRUE  => 1;
use constant FALSE => 0;
# commands' weights
use constant MERGE4TIFF_W => 1;
use constant MERGENTIFF_W => 4;
use constant CACHE2WORK_PNG_W => 3;
use constant WGET_W => 35;
use constant TIFF2TILE_W => 0;
use constant TIFFCP_W => 0;

################################################################################

BEGIN {}
INIT {}
END {}

################################################################################
=begin nd
Group: variable

variable: $self
    * pyramid    => undef, # object Pyramid !
    * datasource => undef, # object DataSource !
    * job_number => undef, # param value !
    * levels => {},
|   level1 => {
|      x1_y2 => [[objimage1] or harvesting,w1,c1],
|      x2_y2 => [[objimage2],w2,c2],
|      x3_y2 => [[objimage3],w3,c3], ...}
|   level2 => { 
|      x1_y2 => [w,W,c],
|      x2_y2 => [w',W',c'], ...}
|objimage = ImageSource object
|w = own node's weight  
|W = accumulated weight (childs' weights sum)
|c = commands to generate this node (to write in a script)

    * cutLevelID    => undef, # top level for the parallele processing
    * bottomLevelID => undef, # first level under the source images resolution
    * topLevelID    => undef, # top level of the pyramid (ie of its tileMatrix)
    * tms      => undef, # TileMatrixSet object
=cut

####################################################################################################
#                                       CONSTRUCTOR METHODS                                        #
####################################################################################################

# Group: constructor

sub new {
    my $this = shift;

    my $class= ref($this) || $this;
    my $self = {
        # in
        pyramid    => undef,
        datasource => undef,
        job_number => undef,
        # out
        levels => {},
        cutLevelID    => undef,
        bottomLevelID => undef,
        topLevelID    => undef,
        tms      => undef
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
method: _init
Define the number of level, get source images.
=cut
sub _init {
    my $self = shift;
    my $objSrc  = shift;
    my $objPyr  = shift;
    my $job_number = shift;

    TRACE;

    # mandatory parameters !
    if (! defined $objSrc) {
        ERROR("Data source is undef !");
        return FALSE;
    }
    if (! defined $objPyr) {
        ERROR("Pyramid is undef !");
        return FALSE;
    }
    if (! defined $job_number) {
        ERROR("The number of job is undef !");
        return FALSE;
    }

    # init. params    
    $self->{pyramid} = $objPyr;
    $self->{tms} = $objPyr->getTileMatrixSet();
    $self->{datasource} = $objSrc; 
    $self->{job_number} = $job_number;    

    return TRUE;
}

=begin nd
method: _load
Build Tree by intersecting src images with le lower level images of the pyramid.

Getting parents upward to the top level of the pyramid.
=cut
sub _load {
    my $self = shift;

    TRACE;

    # initialisation pratique:
    my $tms    = $self->{tms};
    my $src    = $self->{datasource};
    
    # récupération d'information dans la source de données
    $self->{topLevelID} = $self->{datasource}->{topLevelID};
    $self->{bottomLevelID} = $self->{datasource}->{bottomLevelID};

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
        $self->{datasource}->{extent}->AssignSpatialReference($srsini);
        
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

    # identifier les dalles du niveau de base à mettre à jour et les associer aux images sources:
    if (! $self->identifyBottomTiles($ct)) {
        ERROR(sprintf "Cannot determine bottom tiles for the level %s",$src->{bottomLevelID});
        return FALSE;
    }

    INFO(sprintf "N. Tile Cache to the bottom level : %d",scalar keys(%{$self->{levels}{$self->{bottomLevelID}}}));

    # Calcul des branches à partir des feuilles et de leur poids:
    for (my $i = $src->{bottomLevelOrder}; $i <= $src->{topLevelOrder}; $i++){
        my $levelID = $tms->getTileMatrixID($i);
        
        foreach my $refnode ($self->getNodesOfLevel($levelID)){
            # pyramid's limits update : we store data's limits in the object Pyramid
            $self->{pyramid}->updateLimits($levelID,$refnode->{x},$refnode->{y});
            
            if ($i != $src->{topLevelOrder}) {
                my $aboveLevelID = $tms->getTileMatrixID($i+1);
                my $parentNodeID = int($refnode->{x}/2)."_".int($refnode->{y}/2);
                if (! defined($self->{levels}{$aboveLevelID}{$parentNodeID})){
                    # A new node for this level
                    $self->{levels}{$aboveLevelID}{$parentNodeID} = [0,0];
                }
            }
        }

        DEBUG(sprintf "N. Tile Cache by level (%s) : %d",$levelID, scalar keys(%{$self->{levels}{$levelID}}));
    }

    return TRUE;
}



####################################################################################################
#                                     BOTTOM LEVEL METHODS                                         #
####################################################################################################

# Group: bottom level methods

=begin nd
method: identifyBottomTiles
Calculate terrain size (in SRS's units) of a tile, for the supplied level.
=cut
sub identifyBottomTiles {
    my $self = shift;
    my $ct = shift;
    
    TRACE();
    
    my ($ImgGroundWidth, $ImgGroundHeight) = $self->imgGroundSizeOfLevel($self->{bottomLevelID});
    my $tm = $self->{tms}->getTileMatrix($self->{bottomLevelID});
    my ($TLCX,$TLCY) = ($tm->getTopLeftCornerX(),$tm->getTopLeftCornerY());
    my $datasource = $self->{datasource};
    
    if ($datasource->hasImages()) {
        # We have real data as source. Images determine bottom tiles
        my @images = $datasource->getImages();
        foreach my $objImg (@images){
            # On reprojette l'emprise si nécessaire
            my %bbox = $objImg->computeBBox($ct);
            if ($bbox{xMin} == 0 && $bbox{xMax} == 0) {
                ERROR(sprintf "Impossible to compute BBOX for the image '%s'. Probably limits are reached !", $objImg->getName());
                return FALSE;
            }
            
            # On divise les coord par la taille des dalles de cache pour avoir les indices min et max en x et y
            my $iMin=int(($bbox{xMin} - $TLCX) / $ImgGroundWidth);
            my $iMax=int(($bbox{xMax} - $TLCX) / $ImgGroundWidth);
            my $jMin=int(($TLCY - $bbox{yMax}) / $ImgGroundHeight);
            my $jMax=int(($TLCY - $bbox{yMin}) / $ImgGroundHeight);
            
            for (my $i = $iMin; $i<= $iMax; $i++){
                for (my $j = $jMin; $j<= $jMax; $j++){
                    my $idxkey = sprintf "%s_%s", $i, $j;
                    if ($datasource->hasHarvesting()) {
                        # we use WMS service to generate this leaf
                        $self->{levels}{$self->{bottomLevelID}}{$idxkey}[0] = 0;
                        # { level1 => { x1_y2 => [0,w1],
                        #               x2_y2 => [0,w2],
                        #               x3_y2 => [0,w3], ...} }
                    } else {
                        # we use images to generate this leaf
                        push (@{$self->{levels}{$self->{bottomLevelID}}{$idxkey}[0]},$objImg);
                        # { level1 => { x1_y2 => [[list objimage1],w1],
                        #               x2_y2 => [[list objimage2],w2],
                        #               x3_y2 => [[list objimage3],w3], ...} }
                    }
                    $self->{levels}{$self->{bottomLevelID}}{$idxkey}[1] = 0;
                }
            }
        }
    } else {
        # We have just a WMS service as source. We use extent to determine bottom tiles
        my $convertExtent = $datasource->{extent}->Clone();
        eval { $convertExtent->Transform($ct); };
        if ($@) { 
            ERROR(sprintf "Cannot convert extent for the datasource : %s",$@);
            return FALSE;
        }
        
        my $bboxref = $convertExtent->GetEnvelope(); #bboxref = [xmin,xmax,ymin,ymax]
        
        my $iMin=int(($bboxref->[0] - $TLCX) / $ImgGroundWidth);
        my $iMax=int(($bboxref->[1] - $TLCX) / $ImgGroundWidth);
        my $jMin=int(($TLCY - $bboxref->[3]) / $ImgGroundHeight);
        my $jMax=int(($TLCY - $bboxref->[2]) / $ImgGroundHeight);
        
        for (my $i = $iMin; $i <= $iMax; $i++) {
            for (my $j = $jMin; $j <= $jMax; $j++) {
                my $xmin = $ImgGroundWidth*$i + $TLCX;
                my $xmax = $ImgGroundWidth*($i+1) + $TLCX;
                my $ymin = $TLCY - $ImgGroundHeight*($j+1);
                my $ymax = $TLCY - $ImgGroundHeight*$j;
                
                my $GMLtile = sprintf "<gml:Polygon><gml:outerBoundaryIs><gml:LinearRing><gml:coordinates>%s,%s %s,%s %s,%s %s,%s %s,%s</gml:coordinates></gml:LinearRing></gml:outerBoundaryIs></gml:Polygon>",
                    $xmin,$ymin,
                    $xmin,$ymax,
                    $xmax,$ymax,
                    $xmax,$ymin,
                    $xmin,$ymin;
                
                my $OGRtile = Geo::OGR::Geometry->create(GML=>$GMLtile);
                if ($OGRtile->Intersect($convertExtent)){
                    my $idxkey = sprintf "%s_%s", $i, $j;
                    $self->{levels}{$self->{bottomLevelID}}{$idxkey}[0] = 0;
                    $self->{levels}{$self->{bottomLevelID}}{$idxkey}[1] = 0;
                    
                    # { level1 => { x1_y2 => [0,w1],
                    #               x2_y2 => [0,w2],
                    #               x3_y2 => [0,w3], ...} }
                }
            }
        }
    }
  
    return TRUE;  
}

=begin nd
method: imgGroundSizeOfLevel
Calculate terrain size (in SRS's units) of a tile, for the supplied level.
=cut
sub imgGroundSizeOfLevel(){
    my $self = shift;
    my $levelID = shift;
    
    TRACE();
    
    my $tm = $self->{tms}->getTileMatrix($levelID);
    my $xRes = Math::BigFloat->new($tm->getResolution()); 
    my $yRes = Math::BigFloat->new($tm->getResolution());
    my $imgGroundWidth = $tm->getTileWidth()  * $self->{pyramid}->getTilePerWidth() * $xRes;
    my $imgGroundHeight = $tm->getTileHeight() * $self->{pyramid}->getTilePerHeight() * $yRes;
    
    DEBUG (sprintf "Size ground (level %s): %s * %s SRS unit", $levelID, $imgGroundWidth, $imgGroundHeight);
    
    return ($imgGroundWidth,$imgGroundHeight);
}

####################################################################################################
#                                         CUT LEVEL METHODS                                        #
####################################################################################################

# Group: cut level methods

=begin nd
method: shareNodesOnJobs
Determine the cutLevel to optimize sharing into scripts and execution time.

Return the distribution array.
=cut
sub shareNodesOnJobs {
    my $self = shift;
    my ($nodeRack,$weights) = @_;

    TRACE;

    my $tms = $self->{tms};
    my $optimalWeight = undef;
    my $cutLevelID = undef;
    my @jobsSharing = undef;
    my @jobsWeights = undef;

    # calcul du poids total de l'arbre : c'est la somme des poids cumulé des noeuds du topLevel
    my $wholeTreeWeight = 0;
    my @topLevelNodeList = $self->getNodesOfTopLevel();
    foreach my $node (@topLevelNodeList) {
        $wholeTreeWeight += $self->getAccumulatedWeightOfNode($node);
    }

    for (my $i = $self->getTopLevelOrder(); $i >= $self->getBottomLevelOrder(); $i--){
        my $levelID = $tms->getTileMatrixID($i);
        my @levelNodeList = $self->getNodesOfLevel($levelID);
        
        if ($levelID ne $self->{bottomLevelID} && scalar @levelNodeList < $self->{job_number}) {
            next;
        }
        
        @levelNodeList =
            sort {$self->getAccumulatedWeightOfNode($b) <=> $self->getAccumulatedWeightOfNode($a)} @levelNodeList;

            
        my @TMP_WEIGHTS;
        
        for (my $j = 0; $j <= $self->{job_number}; $j++) {
            # On initialise les poids avec ceux des jobs
            $TMP_WEIGHTS[$j] = $weights->[$j];
        }
        
        my $finisherWeight = $wholeTreeWeight;
        my @TMP_JOBS;
        
        for (my $j = 0; $j < scalar @levelNodeList; $j++) {
            my $indexMin = $self->minArrayIndex(1,@TMP_WEIGHTS);
            my $nodeWeight = $self->getAccumulatedWeightOfNode($levelNodeList[$j]);
            $TMP_WEIGHTS[$indexMin] += $nodeWeight;
            $finisherWeight -= $nodeWeight;
            push @{$TMP_JOBS[$indexMin-1]}, $levelNodeList[$j];
        }
        
        
        # on additionne le poids du job le plus "lourd" et le poids du finisher pour quantifier le
        # pire temps d'exécution
        $TMP_WEIGHTS[0] += $finisherWeight;
        my $worstWeight = $self->maxArrayValue(1,@TMP_WEIGHTS) + $finisherWeight;

        # on compare ce pire des cas avec celui obtenu jusqu'ici. S'il est plus petit, on garde ce niveau comme
        # cutLevel (a priori celui qui optimise le temps total de la génération de la pyramide).
        if (! defined $optimalWeight || $worstWeight < $optimalWeight) {
            $optimalWeight = $worstWeight;
            $cutLevelID = $levelID;
            @jobsSharing = @TMP_JOBS;            
            @jobsWeights = @TMP_WEIGHTS;
            DEBUG (sprintf "New cutLevel found : %s (worstWeight : %s)",$levelID,$optimalWeight);
        }

    }

    $self->{cutLevelID} = $cutLevelID;
    
    # We store results in array references
    for (my $i = 0; $i < $self->{job_number}; $i++) {
        $nodeRack->[$i] = $jobsSharing[$i];
        $weights->[$i] = $jobsWeights[$i];
    }
    # Weights' array is longer
    $weights->[$self->{job_number}] = $jobsWeights[$self->{job_number}];

}

####################################################################################################
#                                            ARRAY TOOLS                                           #
####################################################################################################

# Group: array tools

=begin nd
method: minArrayIndex
Return index of the smaller element in a array, begining with the element 'first'
=cut
sub minArrayIndex {
    my $self = shift;
    my $first = shift;
    my @array = @_;
    
    TRACE;

    my $min = undef;
    my $minIndex = undef;

    for (my $i = $first; $i < scalar @array; $i++){
        if (! defined $minIndex || $min > $array[$i]) {
            $min = $array[$i];
            $minIndex = $i;
        }
    }

    return $minIndex;
}

# method: maxArrayValue
#  Return the greater value in a array, begining with the element 'first'
#-------------------------------------------------------------------------------
sub maxArrayValue {
    my $self = shift;
    my $first = shift;
    my @array = @_;

    TRACE;

    my $max = undef;

    for (my $i = $first; $i < scalar @array; $i++){
        if (! defined $max || $max < $array[$i]) {
            $max = $array[$i];
        }
    }

    return $max;
}

####################################################################################################
#                                         GETTERS / SETTERS                                        #
####################################################################################################

# Group: getters - setters


sub getCutLevelID {
  my $self = shift;
  return $self->{cutLevelID};
}

sub getTopLevelID {
  my $self = shift;
  return $self->{topLevelID};
}

# method: getTopLevelOrder
sub getTopLevelOrder {
  my $self = shift;
  return $self->{tms}->getTileMatrixOrder($self->{topLevelID});
}
# method: getBottomLevelOrder
sub getBottomLevelOrder {
  my $self = shift;
  return $self->{tms}->getTileMatrixOrder($self->{bottomLevelID});
}

# method: getComputingCode
sub getComputingCode {
    my $self = shift;
    my $node = shift;
    my $keyidx = sprintf "%s_%s", $node->{x}, $node->{y};
    return $self->{levels}{$node->{level}}{$keyidx}[2];
}
# method: getAccumulatedWeightOfNode
sub getAccumulatedWeightOfNode {
    my $self = shift;
    my $node = shift;
    my $keyidx = sprintf "%s_%s", $node->{x}, $node->{y};
    return $self->{levels}{$node->{level}}{$keyidx}[1];
}

# method: getImgDescOfNode
#  Return image's description from a node (level + i + j)
#------------------------------------------------------------------------------
sub getImgDescOfNode {
  my $self = shift;
  my $node = shift;
  
  my %params = ();
  my ($ImgGroundWith, $ImgGroundHeight) = $self->imgGroundSizeOfLevel($node->{level});
  
  my $tms = $self->{pyramid}->getTileMatrixSet();
  my $tm  = $tms->getTileMatrix($node->{level});

  $params{filePath} = $self->{pyramid}->getCachePathOfImage($node->{level}, $node->{x}, $node->{y}, 'data');
  $params{xMin} = $tm->getTopLeftCornerX() + $node->{x} * $ImgGroundWith;   
  $params{yMax} = $tm->getTopLeftCornerY() - $node->{y} * $ImgGroundHeight; 
  $params{xMax} = $params{xMin} + $ImgGroundWith;                         
  $params{yMin} = $params{yMax} - $ImgGroundHeight;
  $params{xRes} = $tm->getResolution();
  $params{yRes} = $tm->getResolution();
  
  my $desc = BE4::ImageDesc->new(%params);

  return $desc
}

# method: getImgDescOfBottomNode
#  Return input images' descriptions used to generate the supplied bottom level node.
#------------------------------------------------------------------------------
sub getImgDescOfBottomNode(){
  my $self = shift;
  my $node = shift;
  
  my $keyidx = sprintf "%s_%s", $node->{x}, $node->{y};
  return undef if ($node->{level} ne $self->{bottomLevelID});
  return $self->{levels}{$node->{level}}{$keyidx}[0];
}

# method: getWeightOfBottomNode
#  Return weight of the supplied bottom level node
#------------------------------------------------------------------------------
sub getWeightOfBottomNode(){
  my $self = shift;
  my $node = shift;
  
  my $keyidx = sprintf "%s_%s", $node->{x}, $node->{y};
  return undef if ($node->{level} ne $self->{bottomLevelID});
  return $self->{levels}{$node->{level}}{$keyidx}[1];
}

# method: isInTree
#  Return the node's content in the tree, undef if the node does not exist.
#------------------------------------------------------------------------------
sub isInTree(){
  my $self = shift;
  my $node = shift;
  my $keyidx = sprintf "%s_%s", $node->{x}, $node->{y};
  return $self->{levels}{$node->{level}}{$keyidx};
}

# method: getPossibleChilds
#  Return an array of the four possible childs from a node, an empty array if the node is a leaf
#------------------------------------------------------------------------------
sub getPossibleChilds(){
    my $self = shift;
    my $node = shift;
    
    my @res;
    if ($node->{level} eq $self->{bottomLevelID}) {
        return @res;
    }
    
    my $lowerLevelID = $self->{tms}->getBelowTileMatrixID($node->{level});
    for (my $i=0; $i<=1; $i++){
        for (my $j=0; $j<=1; $j++){
            my $childNode = {level => $lowerLevelID, x => $node->{x}*2+$j, y => $node->{y}*2+$i};
            push(@res, $childNode);
            DEBUG(sprintf "Possible Child for %s_%s_%s (level_idx) : %s_%s_%s",
            $node->{level}, $node->{x},  $node->{y},
            $childNode->{level}, $childNode->{x},  $childNode->{y});
        }
    }
    return @res;
}

# method: getChilds
#  Return an array of the real (in the tree) childs from a node, an empty array if the node is a leaf
#------------------------------------------------------------------------------
sub getChilds(){
  my $self = shift;
  my $node = shift;

  my @res;
  foreach my $childNode ($self->getPossibleChilds($node)){
    if (defined $self->isInTree($childNode)){
      push(@res, $childNode);
    }
  }
  return @res
}

# method: getNodesOfLevel
#  Return all nodes of the supplied level
#------------------------------------------------------------------------------
sub getNodesOfLevel(){
  my $self = shift;
  my $levelID= shift;

  if (! defined $levelID) {
    ERROR("Level undef ?");
    return undef;
  }
  
  my @nodes;
  my $lvl=$self->{levels}->{$levelID};

  foreach my $k (keys(%$lvl)){
    my ($x,$y) = split(/_/, $k);
    push(@nodes, {level => $levelID, x => $x, y => $y});
  }

  return @nodes;
}

# method: getNodesOfTopLevel
#  Return all nodes of the top level
#------------------------------------------------------------------------------
sub getNodesOfTopLevel(){
  my $self = shift;
  return $self->getNodesOfLevel($self->{topLevelID});
}

# method: getNodesOfCutLevel
#  Return all nodes of the cut level
#------------------------------------------------------------------------------
sub getNodesOfCutLevel(){
    my $self = shift;
    return $self->getNodesOfLevel($self->{cutLevelID});
}


# method: setComputingCode
#  Add to the node the code to generate it
#------------------------------------------------------------------------------
sub setComputingCode(){
    my $self = shift;
    my $node = shift;
    my $code = shift;

    my $keyidx = sprintf "%s_%s", $node->{x}, $node->{y};

    $self->{levels}{$node->{level}}{$keyidx}[2] = $code;

}

# method: updateWeightOfNode
#  Add to the node's weight the supplied weight
#------------------------------------------------------------------------------
sub updateWeightOfNode(){
    my $self = shift;
    my $node = shift;
    my $weight = shift;

    my $keyidx = sprintf "%s_%s", $node->{x}, $node->{y};

    if ($node->{level} eq $self->{bottomLevelID}) {
        $self->{levels}{$node->{level}}{$keyidx}[1] += $weight;
    } else {
        $self->{levels}{$node->{level}}{$keyidx}[0] += $weight;
    }
}

# method: setAccumulatedWeightOfNode
#  Calculate the accumulated weight. It's the node's weight added to the supplied weight (childs' weights' sum).
#------------------------------------------------------------------------------
sub setAccumulatedWeightOfNode(){
    my $self = shift;
    my $node = shift;
    my $weight = shift;

    my $keyidx = sprintf "%s_%s", $node->{x}, $node->{y};

    return if ($node->{level} eq $self->{bottomLevelID});

    $self->{levels}{$node->{level}}{$keyidx}[1] = $weight + $self->{levels}{$node->{level}}{$keyidx}[0];
    
}

####################################################################################################
#                                           OTHERS                                                 #
####################################################################################################

# Group: others

# method: exportTree
#  Export dans un fichier texte de l'arbre complet.
#  (Orienté maintenance !)
#------------------------------------------------------------------------------
sub exportTree {
  my $self = shift;
  my $file = shift; # filepath !
  
  # sur le bottomlevel, on a :
  # { level1 => { x1_y2 => [[objimage1, objimage2, ...],w1,c1],
  #               x2_y2 => [[objimage2],w2,c2],
  #               x3_y2 => [[objimage3],w3,c3], ...} }
  
  # on exporte dans un fichier la liste des indexes par images sources en projection :
  #  imagesource
  #  - x1_y2 => /OO/IF/ZX.tif => xmin, ymin, xmax, ymax
  #  - x2_y2 => /OO/IF/YX.tif => xmin, ymin, xmax, ymax
  #  ...
  TRACE;

  my $idLevel = $self->{bottomLevelID};
  my $lstIdx  = $self->{levels}->{$idLevel};
  
  if (! open (FILE, ">", $file)) {
    ERROR ("Can not create file ('$file') !");
    return FALSE;
  }
  
  my $refpyr  = $self->{pyramid};
  my $refdata = $self->{datasource};
  
  my $srsini   = $refdata->getSRS();
  my $resini   = $refdata->getResolution();
  my @bboxini  = $refdata->computeBbox(); # (Upper Left, Lower Right) !
  
  my $srsfinal  = $refpyr->getTileMatrixSet()->getSRS();
  my $resfinal  = $refpyr->getTileMatrixSet()->getTileMatrix($idLevel)->getResolution();
  my @bboxfinal;
  
  my $idxmin = 0;
  my $idxmax = 0;
  my $idymin = 0;
  my $idymax = 0;
  
  foreach my $idx (keys %$lstIdx) {
    
    my ($idxXmin, $idxYmax) = split(/_/, $idx);
    my ($idxXmax, $idxYmin) = ($idxXmin+1, $idxYmax-1);
    
    if (!$idxmin && !$idxmax && !$idymin && !$idymax) {
      $idxmin = $idxXmin;
      $idxmax = $idxXmax;
      $idymin = $idxYmin;
      $idymax = $idxYmax;
    }
    
    $idxmin = $idxXmin if ($idxmin > $idxXmin);
    $idxmax = $idxXmax if ($idxmax < $idxXmax);
    $idymin = $idxYmin if ($idymin > $idxYmin);
    $idymax = $idxYmax if ($idymax < $idxYmax);
  }
  
  # (xmin, ymin, xmax, ymax) !
  push @bboxfinal, ($refpyr->_IDXtoX($idLevel,$idxmin),
                    $refpyr->_IDXtoY($idLevel,$idymin),
                    $refpyr->_IDXtoX($idLevel,$idxmax),
                    $refpyr->_IDXtoY($idLevel,$idymax));
  
  printf FILE "----------------------------------------------------\n";
  printf FILE "=> Data Source :\n";
  printf FILE "   - resolution [%s]\n", $resini;
  printf FILE "   - srs        [%s]\n", $srsini;
  printf FILE "   - bbox       [%s, %s, %s, %s]\n", $bboxini[0], $bboxini[3], $bboxini[2], $bboxini[1];
  printf FILE "----------------------------------------------------\n";
  printf FILE "=> Index (level n° %s):\n", $idLevel;
  printf FILE "   - resolution [%s]\n", $resfinal;
  printf FILE "   - srs        [%s]\n", $srsfinal;
  printf FILE "   - bbox       [%s, %s, %s, %s]\n", $bboxfinal[0], $bboxfinal[1], $bboxfinal[2], $bboxfinal[3];
  printf FILE "----------------------------------------------------\n";
 
  while( my ($k, $v) = each(%$lstIdx)) {
    
    my ($idxXmin, $idxYmax) = split(/_/, $k);
    my ($idxXmax, $idxYmin) = ($idxXmin+1, $idxYmax-1);
    my $cachename = $refpyr->getCacheNameOfImage($idLevel, $idxXmin, $idxYmax, "data");
    
    # image xmin ymin xmax ymax
    printf FILE "\n- idx %s (%s) => [%s,%s,%s,%s]\n", $k, $cachename,
      $refpyr->_IDXtoX($idLevel,$idxXmin),
      $refpyr->_IDXtoY($idLevel,$idxYmax),
      $refpyr->_IDXtoX($idLevel,$idxXmax),
      $refpyr->_IDXtoY($idLevel,$idxYmin);
    
    next if (ref $v ne 'ARRAY');
    
    foreach my $objImage (@$v) {
      
      next if (ref $objImage ne 'BE4::ImageSource');
      
      printf FILE "\t%s\n", $objImage->{filename};
    }
  }
  
  close FILE;
  
  return TRUE;
}

1;
__END__

# Below is stub documentation for your module. You'd better edit it!

=head1 NAME

    BE4::Tree - reprentation of the final pyramid : tile = node

=head1 SYNOPSIS

    use BE4::Tree;
  
    # Tree object creation
    my $objTree = = BE4::Tree->new(
        $objDataSource,
        $objPyramid,
        $job_number
    );

=head1 DESCRIPTION

    A Tree object

        * pyramid (Pyramid object)
        * datasource (Datasource object)
        * job_number

        * levels :  {   level1 => { x1_y2 => [[objimage1],w1,c1],
                                    x2_y2 => [[objimage2],w2,c2],
                                    x3_y2 => [[objimage3],w3,c3], ...}
                        level2 => { x1_y2 => [w,W,c], x2_y2 => [w',W',c'], ...}
                        with objimage = ImageSource object
                        with w = own node's weight  
                        with W = accumulated weight (own weight add to childs' weights sum)
                        with c = commands to generate this node (to write in a script)
                    }
        * cutLevelID
        * bottomLevelID
        * topLevelID
        * tms

=head2 EXPORT

    None by default.

=head1 SEE ALSO

    BE4::Pyramid
    BE4::Datasource
    BE4::TileMatrix

=head1 AUTHOR

    Satabin Théo, E<lt>tsatabin@E<gt>

=head1 COPYRIGHT AND LICENSE

    Copyright (C) 2011 by Satabin Théo

    This library is free software; you can redistribute it and/or modify
    it under the same terms as Perl itself, either Perl version 5.10.1 or,
    at your option, any later version of Perl 5 you may have available.

=cut
