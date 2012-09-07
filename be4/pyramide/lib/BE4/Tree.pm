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
    * pyramid    => undef, # object Pyramid !
    * datasource => undef, # object DataSource !
    * bbox => [], # datasource bbox, [xmin,ymin,xmax,ymax], in TMS' SRS
    * job_number => undef, # param value !
    * nodes => {},
|   level1 => {
|      x1_y2 => [[objimage1],w1,c1],
|      x2_y2 => [[objimage2],w2,c2],
|      x3_y2 => [[objimage3],w3,c3], ...}
|   level2 => { 
|      x1_y2 => [w,W,c],
|      x2_y2 => [w',W',c'], ...}
|
|   objimage = ImageSource object
|   w = own node's weight
|   W = accumulated weight (childs' weights sum)
|   c = commands to generate this node (to write in a script)

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
    # IMPORTANT : if modification, think to update natural documentation (just above) and pod documentation (bottom)
    my $self = {
        # in
        pyramid    => undef,
        datasource => undef,
        bbox => [],
        job_number => undef,
        # out
        nodes => {},
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

#
=begin nd
method: _load

Determine all nodes from the bottom level to the top level, thanks to the dta source.
=cut
sub _load {
    my $self = shift;

    TRACE;

    # initialisation pratique:
    my $tms = $self->{tms};
    my $src = $self->{datasource};
    
    # récupération d'information dans la source de données
    $self->{topLevelID} = $self->{datasource}->getTopID;
    $self->{bottomLevelID} = $self->{datasource}->getBottomID;

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
        $self->{datasource}->getExtent->AssignSpatialReference($srsini);
        
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
        ERROR(sprintf "Cannot determine bottom tiles for the level %s",$src->getBottomID);
        return FALSE;
    }

    INFO(sprintf "Number of cache images to the bottom level (%s) : %d",$self->{bottomLevelID},scalar keys(%{$self->{nodes}{$self->{bottomLevelID}}}));

    # Calcul des branches à partir des feuilles et de leur poids:
    for (my $i = $src->getBottomOrder; $i <= $src->getTopOrder; $i++){
        my $levelID = $tms->getIDfromOrder($i);
        
        foreach my $refnode ($self->getNodesOfLevel($levelID)) {
            # pyramid's limits update : we store data's limits in the pyramid's nodes
            $self->{pyramid}->updateTMLimits($levelID,@{$self->{bbox}});
            
            if ($i != $src->getTopOrder) {
                my $aboveLevelID = $tms->getIDfromOrder($i+1);
                my $parentNodeID = int($refnode->{x}/2)."_".int($refnode->{y}/2);
                if (! defined($self->{nodes}{$aboveLevelID}{$parentNodeID})){
                    # A new node for this level
                    $self->{nodes}{$aboveLevelID}{$parentNodeID} = [0,0];
                }
            }
        }

        DEBUG(sprintf "Number of cache images by level (%s) : %d",$levelID, scalar keys(%{$self->{nodes}{$levelID}}));
    }

    return TRUE;
}

####################################################################################################
#                                     BOTTOM LEVEL METHODS                                         #
####################################################################################################

# Group: bottom level methods

#
=begin nd
method: identifyBottomTiles

Calculate all nodes concerned by the datasource (tiles which touch the data source extent).

Parameters:
    ct - a Geo::OSR::CoordinateTransformation object, to convert data extent or images' bbox.
=cut
sub identifyBottomTiles {
    my $self = shift;
    my $ct = shift;
    
    TRACE();
    
    my $tm = $self->{tms}->getTileMatrix($self->{bottomLevelID});
    my $datasource = $self->{datasource};
    my ($TPW,$TPH) = ($self->{pyramid}->getTilesPerWidth,$self->{pyramid}->getTilesPerHeight);
    
    if ($datasource->hasImages()) {
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
            my $iMin = $tm->xToColumn($bbox[0],$TPW);
            my $iMax = $tm->xToColumn($bbox[2],$TPW);
            my $jMin = $tm->yToRow($bbox[3],$TPH);
            my $jMax = $tm->yToRow($bbox[1],$TPH);
            
            for (my $i = $iMin; $i<= $iMax; $i++){
                for (my $j = $jMin; $j<= $jMax; $j++){
                    my $idxkey = sprintf "%s_%s", $i, $j;
                    if ($datasource->hasHarvesting()) {
                        # we use WMS service to generate this leaf
                        $self->{nodes}{$self->{bottomLevelID}}{$idxkey}[0] = 0;
                        # { level1 => { x1_y2 => [0,w1],
                        #               x2_y2 => [0,w2],
                        #               x3_y2 => [0,w3], ...} }
                    } else {
                        # we use images to generate this leaf
                        push (@{$self->{nodes}{$self->{bottomLevelID}}{$idxkey}[0]},$objImg);
                        # { level1 => { x1_y2 => [[list objimage1],w1],
                        #               x2_y2 => [[list objimage2],w2],
                        #               x3_y2 => [[list objimage3],w3], ...} }
                    }
                    $self->{nodes}{$self->{bottomLevelID}}{$idxkey}[1] = 0;
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
        
        my $iMin = $tm->xToColumn($bboxref->[0],$TPW);
        my $iMax = $tm->xToColumn($bboxref->[1],$TPW);
        my $jMin = $tm->yToRow($bboxref->[3],$TPH);
        my $jMax = $tm->yToRow($bboxref->[2],$TPH);
        
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
                    my $idxkey = sprintf "%s_%s", $i, $j;
                    $self->{nodes}{$self->{bottomLevelID}}{$idxkey}[0] = 0;
                    $self->{nodes}{$self->{bottomLevelID}}{$idxkey}[1] = 0;
                    
                    # { level1 => { x1_y2 => [0,w1],
                    #               x2_y2 => [0,w2],
                    #               x3_y2 => [0,w3], ...} }
                }
            }
        }
    }
  
    return TRUE;  
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
method: imgGroundSizeOfLevel

Calculate terrain size (in SRS's units) of a cache image (do not mistake for a tile), for the supplied level.
=cut
sub imgGroundSizeOfLevel {
    my $self = shift;
    my $levelID = shift;
    
    TRACE;
    
    my $tm = $self->{tms}->getTileMatrix($levelID);
    
    my $imgGroundWidth = $tm->getImgGroundWidth($self->{pyramid}->getTilesPerWidth);
    my $imgGroundHeight = $tm->getImgGroundHeight($self->{pyramid}->getTilesPerHeight);
    
    return ($imgGroundWidth,$imgGroundHeight);
}

####################################################################################################
#                                         CUT LEVEL METHODS                                        #
####################################################################################################

# Group: cut level methods

#
=begin nd
method: shareNodesOnJobs

Determine the cutLevel to optimize sharing into scripts and execution time.

Parameters:
    nodeRack - reference to array, to return nodes' sharing (length = number of jobs).
    weights - reference to array, contains current weights (jobs are already filled if several data sources) and to return new weights (length = number of jobs + 1, the script finisher).
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
        my $levelID = $tms->getIDfromOrder($i);
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

#
=begin nd
method: minArrayIndex

Parameters:
    first - integer, indice from which minimum is looked for.
    array - numbers (floats or integers) array

Returns:
    Index of the smaller element in a array, begining with the element 'first'
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

#
=begin nd
method: maxArrayValue

Parameters:
    first - integer, indice from which maximum is looked for.
    array - numbers (floats or integers) array

Returns:
    The greater value in a array, begining with the element 'first'
=cut
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

sub getDataSource{
    my $self = shift;
    return $self->{datasource};
}

sub getCutLevelID {
    my $self = shift;
    return $self->{cutLevelID};
}

sub getTopLevelID {
    my $self = shift;
    return $self->{topLevelID};
}

sub getTopLevelOrder {
    my $self = shift;
    return $self->{tms}->getOrderfromID($self->{topLevelID});
}

sub getBottomLevelOrder {
    my $self = shift;
    return $self->{tms}->getOrderfromID($self->{bottomLevelID});
}

sub getComputingCode {
    my $self = shift;
    my $node = shift;
    my $keyidx = sprintf "%s_%s", $node->{x}, $node->{y};
    return $self->{nodes}{$node->{level}}{$keyidx}[2];
}

sub getAccumulatedWeightOfNode {
    my $self = shift;
    my $node = shift;
    my $keyidx = sprintf "%s_%s", $node->{x}, $node->{y};
    return $self->{nodes}{$node->{level}}{$keyidx}[1];
}

#
=begin nd
method: getImgDescOfNode

Parameters:
    node - node we want the description

Returns:
    An ImageDesc object, representing the give node.
=cut
sub getImgDescOfNode {
    my $self = shift;
    my $node = shift;
    
    my $params = {};
    
    my $tm  = $self->{tms}->getTileMatrix($node->{level});
    
    $params->{filePath} = $self->{pyramid}->getCachePathOfImage($node, 'data');
    ($params->{xMin},$params->{yMin},$params->{xMax},$params->{yMax}) = $tm->indicesToBBox(
        $node->{x}, $node->{y}, $self->{pyramid}->getTilesPerWidth, $self->{pyramid}->getTilesPerHeight);
    $params->{xRes} = $tm->getResolution;
    $params->{yRes} = $tm->getResolution;
    
    my $desc = BE4::ImageDesc->new($params);
    
    return $desc
}

#
=begin nd
method: getGeoImgOfBottomNode

Parameters:
    node - node we want GeoImage objects.

Returns:
    An array of GeoImage objects, used to generate this node. Undef if level is not bottom level or if node does not exist in the tree
=cut
sub getGeoImgOfBottomNode {
  my $self = shift;
  my $node = shift;
  
  my $keyidx = sprintf "%s_%s", $node->{x}, $node->{y};
  
  return undef if ($node->{level} ne $self->{bottomLevelID});
  return undef if (! exists $self->{nodes}{$node->{level}}{$keyidx});
  
  return $self->{nodes}{$node->{level}}{$keyidx}[0];
}

#
=begin nd
method: isInTree

Parameters:
    node - node we want to know if it is in the tree.

Returns:
    A boolean : TRUE if the node exists, FALSE otherwise.
=cut
sub isInTree(){
  my $self = shift;
  my $node = shift;
  
  return FALSE if (! defined $node);
  
  my $keyidx = sprintf "%s_%s", $node->{x}, $node->{y};
  return (exists $self->{nodes}{$node->{level}}{$keyidx});
}

#
=begin nd
method: getPossibleChildren

Parameters:
    node - node we want to know possible children.

Returns:
    An array of the four possible childs from a node, an empty array if the node is a leaf.
=cut
sub getPossibleChildren(){
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
        }
    }
    return @res;
}

#
=begin nd
method: getChildren

Parameters:
    node - node we want to know children.

Returns:
    An array of the real children from a node (four max.), an empty array if the node is a leaf.
=cut
sub getChildren {
  my $self = shift;
  my $node = shift;

  my @res;
  foreach my $childNode ($self->getPossibleChildren($node)){
    if ($self->isInTree($childNode)){
      push(@res, $childNode);
    }
  }
  return @res
}

sub getNodesOfLevel {
  my $self = shift;
  my $levelID= shift;

  if (! defined $levelID) {
    ERROR("Level undef ?");
    return undef;
  }
  
  my @nodes;
  my $lvl=$self->{nodes}->{$levelID};

  foreach my $k (keys(%$lvl)){
    my ($x,$y) = split(/_/, $k);
    push(@nodes, {level => $levelID, x => $x, y => $y});
  }

  return @nodes;
}

sub getNodesOfTopLevel {
  my $self = shift;
  return $self->getNodesOfLevel($self->{topLevelID});
}

sub getNodesOfCutLevel {
    my $self = shift;
    return $self->getNodesOfLevel($self->{cutLevelID});
}

sub getNodesOfBottomLevel {
    my $self = shift;
    return $self->getNodesOfLevel($self->{bottomLevelID});
}

sub setComputingCode(){
    my $self = shift;
    my $node = shift;
    my $code = shift;

    my $keyidx = sprintf "%s_%s", $node->{x}, $node->{y};

    $self->{nodes}{$node->{level}}{$keyidx}[2] = $code;

}

#
=begin nd
method: updateWeightOfNode

Parameters:
    node - node we want to update weight.
    weight - value to add to the node's weight.
=cut
sub updateWeightOfNode(){
    my $self = shift;
    my $node = shift;
    my $weight = shift;

    my $keyidx = sprintf "%s_%s", $node->{x}, $node->{y};

    if ($node->{level} eq $self->{bottomLevelID}) {
        $self->{nodes}{$node->{level}}{$keyidx}[1] += $weight;
    } else {
        $self->{nodes}{$node->{level}}{$keyidx}[0] += $weight;
    }
}

#
=begin nd
method: setAccumulatedWeightOfNode

Parameters:
    node - node we want to store accumulated weight.
    weight - value to add to the node's own weight to obtain accumulated weight (childs' accumulated weights' sum).
=cut
sub setAccumulatedWeightOfNode(){
    my $self = shift;
    my $node = shift;
    my $weight = shift;

    my $keyidx = sprintf "%s_%s", $node->{x}, $node->{y};

    return if ($node->{level} eq $self->{bottomLevelID});

    $self->{nodes}{$node->{level}}{$keyidx}[1] = $weight + $self->{nodes}{$node->{level}}{$keyidx}[0];
    
}

1;
__END__

=head1 NAME

BE4::Tree - reprentation of the final pyramid : cache image = node

=head1 SYNOPSIS

    use BE4::Tree;
    
    my $job_number = 4; # 4 split scripts + one finisher = 5 scripts
  
    # Tree object creation
    my $objTree = = BE4::Tree->new($objDataSource, $objPyramid, $job_number);
    
    ...
    
    # Determine cut level, after having weighted the tree
    my @nodeRack;
    my @weights;
    $objTree->shareNodesOnJobs(\@nodeRack,\@weights);

=head1 DESCRIPTION

=head2 ATTRIBUTES

=over 4

=item pyramid

A Pyramid object.

=item datasource

A Datasource object.

=item bbox

Array [xmin,ymin,xmax,ymax], bbox of datasource in the TMS' SRS.

=item job_number

=item nodes

An hash, composition of each node in the tree (code to generate the node, own weight, accumulated weight):

    {
        bottomLevelID => {
            if images as source
            x1_y2 => [[objGeoImage1],w1,c1],
            x2_y2 => [[objGeoImage2],w2,c2],
            x3_y2 => [[objGeoImage3],w3,c3],...

            or, if just a WMS service as source
            x1_y2 => [0,w1,c1],
            x2_y2 => [0,w2,c2],
            x3_y2 => [0,w3,c3],...
        }
        aboveLevelID => {
            x1_y2 => [w,W,c],
            x2_y2 => [w',W',c'], ...
        }
    }
    
    with objGeoImage = GeoImage object
    with w = own node's weight
    with W = accumulated weight (own weight added to children's weights sum)
    with c = commands to generate this node (to write in a script)

=item cutLevelID

Split scripts will generate cache to this level. Script finisher will be generate above.

=item bottomLevelID, topLevelID

Extrem levels identifiants of the tree.

=item tms

A TileMatrixSet object, used by the tree.

=back

=head1 SEE ALSO

=head2 POD documentation

=begin html

<ul>
<li><A HREF="./lib-BE4-DataSource.html">BE4::DataSource</A></li>
<li><A HREF="./lib-BE4-Pyramid.html">BE4::Pyramid</A></li>
<li><A HREF="./lib-BE4-TileMatrixSet.html">BE4::TileMatrixSet</A></li>
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
