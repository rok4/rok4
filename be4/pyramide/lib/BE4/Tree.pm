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

use Math::BigFloat;
use Geo::OSR;

use strict;
use warnings;

use Data::Dumper;

# My Module
use BE4::DataSource;
use BE4::ImageDesc;

use Log::Log4perl qw(:easy);

require Exporter;
use AutoLoader qw(AUTOLOAD);

our @ISA = qw(Exporter);

our %EXPORT_TAGS = ( 'all' => [ qw() ] );
our @EXPORT_OK   = ( @{$EXPORT_TAGS{'all'}} );
our @EXPORT      = qw();

#-------------------------------------------------------------------------------
# version
my $VERSION = "0.0.1";

#-------------------------------------------------------------------------------
# booleans
use constant TRUE  => 1;
use constant FALSE => 0;
# commands' weights
use constant MERGE4TIFF_W => 1;
use constant MERGENTIFF_W => 4;
use constant WGET_W => 35;
use constant TIFF2TILE_W => 0;
use constant TIFFCP_W => 0;

#-------------------------------------------------------------------------------
# Global

#-------------------------------------------------------------------------------
# Preloaded methods go here.
BEGIN {}
INIT {}
END {}

####################################################################################################
#                                       CONSTRUCTOR METHODS                                        #
####################################################################################################

#-------------------------------------------------------------------------------
# constructor
sub new {
  my $this = shift;

  my $class= ref($this) || $this;
  my $self = {
    # in
    pyramid    => undef, # object Pyramid !
    datasource => undef, # object DataSource !
    job_number => undef, # param value !
    # out
    levels => {}, # level1 => { x1_y2 => [[objimage1],w1], x2_y2 => [[objimage2],w2], x3_y2 => [[objimage3],w3], ...}
                  # level2 => { x1_y2 => [w,W], x2_y2 => [w',W'], ...}
                  # with objimage = Class ImageSource 
                  # with w = own node's weight  
                  # with W = accumulated weight (childs' weights sum)
    cutLevelId    => undef, # top level for the parallele processing
    bottomLevelId => undef, # first level under the source images resolution
    topLevelId    => undef, # top level of the pyramid (ie of its tileMatrixSet)
    levelIdx      => undef, # hash associant les id de level à leur indice dans le tableau du TMS
    tmList        => [],    # tableau des tm de la pyramide dans l'ordre croissant de taille de pixel   
  };

  bless($self, $class);
  
  TRACE;
  
  # init. class
  return undef if (! $self->_init(@_));
  # load 
  return undef if (! $self->_load());
  
  # DEBUG(Dumper($self));
  
  return $self;
}

# method: _init.
#  Define the number of level
#  get source images.
#-------------------------------------------------------------------------------
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
  $self->{pyramid}   =$objPyr;
  $self->{datasource}=$objSrc; 
  $self->{job_number}=$job_number;    

  return TRUE;
}


# method: _load
#  Build Tree by intersecting src images with le lower level images of the pyramid
#  getting parents upward to the top level of the pyramid.
#---------------------------------------------------------------------------------------------------
sub _load {
    my $self = shift;

    TRACE;

    # initialisation pratique:
    my $tms    = $self->{pyramid}->getTileMatrixSet();
    my $src    = $self->{datasource};
    
    # récupération d'information dans la pyramide
    $self->{topLevelId} = $self->{pyramid}->getTopLevel();
    $self->{bottomLevelId} = $self->{pyramid}->getBottomLevel();

    # tilematrix list sort by resolution
    my @tmList = $tms->getTileMatrixByArray();
    @{$self->{tmList}} = @tmList;

    # on fait un hash pour retrouver l'ordre d'un niveau a partir de son id.
    for (my $i=0; $i < scalar @tmList; $i++){
        $self->{levelIdx}{$tmList[$i]->getID()} = $i;
    }

    # initialisation de la transfo de coord du srs des données initiales vers
    # le srs de la pyramide. Si les srs sont identiques on laisse undef.
    my $ct = undef;  
    if ($tms->getSRS() ne $src->getSRS()){
        my $srsini= new Geo::OSR::SpatialReference;
        eval { $srsini->ImportFromProj4('+init='.$src->getSRS().' +wktext'); };
        if ($@) { 
            eval { $srsini->ImportFromProj4('+init='.lc($src->getSRS()).' +wktext'); };
            if ($@) { 
                ERROR($@);
                ERROR(sprintf "Impossible to initialize the initial spatial coordinate system (%s) !",$src->getSRS());
                return FALSE;
            }
        }
        my $srsfin= new Geo::OSR::SpatialReference;
        eval { $srsfin->ImportFromProj4('+init='.$tms->getSRS().' +wktext'); };
        if ($@) {
            eval { $srsfin->ImportFromProj4('+init='.lc($tms->getSRS()).' +wktext'); };
            if ($@) {
                ERROR($@);
                ERROR(sprintf "Impossible to initialize the destination spatial coordinate system (%s) !",$tms->getSRS());
                return FALSE;
            }
        }
        $ct = new Geo::OSR::CoordinateTransformation($srsini, $srsfin);
    }

    # identifier les dalles du niveau de base à mettre à jour et les associer aux images sources:

    my ($ImgGroundWidth, $ImgGroundHeight) = $self->imgGroundSizeOfLevel($self->{bottomLevelId});

    my $tm = $tms->getTileMatrix($self->{bottomLevelId});

    my @images = $src->getImages();
    foreach my $objImg (@images){
        # On reprojette l'emprise si nécessaire
        my %bbox = $self->computeBBox($objImg, $ct);
        if ($bbox{xMin} == 0 && $bbox{xMax} == 0) {
            ERROR(sprintf "Impossible to compute BBOX for the image '%s'. Probably limits are reached !",
                $objImg->getName());
            return FALSE;
        }

        # pyramid's limits update : we store data's limits in the object Pyramid
        $self->{pyramid}->updateLimits($bbox{xMin},$bbox{yMin},$bbox{xMax},$bbox{yMax});

        # On divise les coord par la taille des dalles de cache pour avoir les indices min et max en x et y
        my $iMin=int(($bbox{xMin} - $tm->getTopLeftCornerX()) / $ImgGroundWidth);   
        my $iMax=int(($bbox{xMax} - $tm->getTopLeftCornerX()) / $ImgGroundWidth);   
        my $jMin=int(($tm->getTopLeftCornerY() - $bbox{yMax}) / $ImgGroundHeight); 
        my $jMax=int(($tm->getTopLeftCornerY() - $bbox{yMin}) / $ImgGroundHeight);

        for (my $i = $iMin; $i<= $iMax; $i++){       
            for (my $j = $jMin; $j<= $jMax; $j++){
                my $idxkey = sprintf "%s_%s", $i, $j;
                push (@{$self->{levels}{$self->{bottomLevelId}}{$idxkey}[0]},$objImg);
                $self->{levels}{$self->{bottomLevelId}}{$idxkey}[1] = 0;

                # { level1 => { x1_y2 => [[list objimage1],w1],
                #               x2_y2 => [[list objimage2],w2],
                #               x3_y2 => [[list objimage3],w3], ...} }
            }
        }
    }

    DEBUG(sprintf "N. Tile Cache to the bottom level : %d", scalar keys( %{$self->{levels}{$self->{bottomLevelId}}} ));

    # Calcul des branches à partir des feuilles et de leur poids:
    for (my $i = $self->{levelIdx}{$self->{bottomLevelId}}; $i <= $self->{levelIdx}{$self->{topLevelId}}; $i++){
        my $levelId = $tmList[$i]->getID();

        if ($i != $self->{levelIdx}{$self->{topLevelId}}) {
            my $aboveLevelId = $tmList[$i+1]->getID();
            foreach my $refnode ($self->getNodesOfLevel($levelId)){
                my $parentNodeId = int($refnode->{x}/2)."_".int($refnode->{y}/2);
                if (! defined($self->{levels}{$aboveLevelId}{$parentNodeId})){
                    # On a un nouveau noeud dans ce niveau
                    $self->{levels}{$aboveLevelId}{$parentNodeId} = [0,0];
                }
            }
        }

        DEBUG(sprintf "N. Tile Cache by level (%s) : %d", $levelId, scalar keys( %{$self->{levels}{$levelId}} ));
    }

    return TRUE;
}

####################################################################################################
#                                         PUBLIC METHODS                                           #
####################################################################################################


# method: computeBBox
#  Renvoie la bbox de l'imageSource en parametre dans le SRS de la pyramide.
#------------------------------------------------------------------------------
sub computeBBox(){
  my $self = shift;
  my $img = shift;
  my $ct = shift;
  
  TRACE;
  
  my %BBox = ();

  if (!defined($ct)){
    $BBox{xMin} = Math::BigFloat->new($img->getXmin());
    $BBox{yMin} = Math::BigFloat->new($img->getYmin());
    $BBox{xMax} = Math::BigFloat->new($img->getXmax());
    $BBox{yMax} = Math::BigFloat->new($img->getYmax());
    DEBUG (sprintf "BBox (xmin,ymin,xmax,ymax) to '%s' : %s - %s - %s - %s", $img->getName(),
        $BBox{xMin}, $BBox{yMin}, $BBox{xMax}, $BBox{yMax});
    return %BBox;
  }
  
  # TODO:
  # Dans le cas où le SRS de la pyramide n'est pas le SRS natif des données, il faut reprojeter la bbox.
  # 1. L'algo trivial consiste à reprojeter les coins et prendre une marge de 20%.
  #    C'est rapide et simple mais comment on justifie les 20% ?
  
  # 2. on fait une densification fixe (ex: 5 pts par coté) et on prend une marge beaucoup plus petite.
  #    Ca reste relativement simple, mais on ne sait toujours pas quoi prendre comme marge.
  
  # 3. on fait une densification itérative avec calcul d'un majorant de l'erreur et on arrête quand on 
  #    a une erreur de moins d'un pixel. Puis on prend une marge d'un pixel. Cette fois ça peut prendre 
  #    du temps et l'algo commence à être compliqué.

  # methode 2.  
  # my ($xmin,$ymin,$xmax,$ymax);
  my $step = 7;
  my $dx= ($img->getXmax() - $img->getXmin())/(1.0*$step);
  my $dy= ($img->getYmax() - $img->getYmin())/(1.0*$step);
  my @polygon= ();
  for my $i (@{[0..$step-1]}) {
    push @polygon, [$img->getXmin()+$i*$dx, $img->getYmin()];
  }
  for my $i (@{[0..$step-1]}) {
    push @polygon, [$img->getXmax(), $img->getYmin()+$i*$dy];
  }
  for my $i (@{[0..$step-1]}) {
    push @polygon, [$img->getXmax()-$i*$dx, $img->getYmax()];
  }
  for my $i (@{[0..$step-1]}) {
    push @polygon, [$img->getXmin(), $img->getYmax()-$i*$dy];
  }

  my ($xmin_reproj, $ymin_reproj, $xmax_reproj, $ymax_reproj);
  for my $i (@{[0..$#polygon]}) {
    # FIXME: il faut absoluement tester les erreurs ici:
    #        les transformations WGS84G vers PM ne sont pas possible au dela de 85.05°.

    my $p = 0;
    eval { $p= $ct->TransformPoint($polygon[$i][0],$polygon[$i][1]); };
    if ($@) {
        ERROR($@);
        ERROR(sprintf "Impossible to transform point (%s,%s). Probably limits are reached !",$polygon[$i][0],$polygon[$i][1]);
        return [0,0,0,0];
    }

    if ($i==0) {
      $xmin_reproj= $xmax_reproj= @{$p}[0];
      $ymin_reproj= $ymax_reproj= @{$p}[1];
    } else {
      $xmin_reproj= @{$p}[0] if @{$p}[0] < $xmin_reproj;
      $ymin_reproj= @{$p}[1] if @{$p}[1] < $ymin_reproj;
      $xmax_reproj= @{$p}[0] if @{$p}[0] > $xmax_reproj;
      $ymax_reproj= @{$p}[1] if @{$p}[1] > $ymax_reproj;
    }
  }

  my $margeX = ($xmax_reproj - $xmin_reproj) * 0.02; # FIXME: la taille de la marge est arbitraire!!
  my $margeY = ($ymax_reproj - $ymax_reproj) * 0.02; # FIXME: la taille de la marge est arbitraire!!

  $BBox{xMin} = Math::BigFloat->new($xmin_reproj - $margeX);
  $BBox{yMin} = Math::BigFloat->new($ymin_reproj - $margeY);
  $BBox{xMax} = Math::BigFloat->new($xmax_reproj + $margeX);
  $BBox{yMax} = Math::BigFloat->new($ymax_reproj + $margeY);
  
  DEBUG (sprintf "BBox (xmin,ymin,xmax,ymax) to '%s' (with proj) : %s ; %s ; %s ; %s", $img->getName(), $BBox{xMin}, $BBox{yMin}, $BBox{xMax}, $BBox{yMax});
  
  return %BBox;
}

# method: imgGroundSizeOfLevel
# Calcule les dimensions terrain (en unité du srs) des dalles du niveau en paramètre.
#------------------------------------------------------------------------------
sub imgGroundSizeOfLevel(){
  my $self = shift;
  my $levelId = shift;
  
  TRACE();
  
  my $tms = $self->{pyramid}->getTileMatrixSet();
  my $tm  = $tms->getTileMatrix($levelId);
  my $xRes = Math::BigFloat->new($tm->getResolution()); 
  my $yRes = Math::BigFloat->new($tm->getResolution());
  my $imgGroundWidth   = $tm->getTileWidth()  * $self->{pyramid}->getTilePerWidth() * $xRes;
  my $imgGroundHeight = $tm->getTileHeight() * $self->{pyramid}->getTilePerHeight() * $yRes;
  
  DEBUG (sprintf "Size ground (level %s): %s * %s SRS unit", $levelId, $imgGroundWidth, $imgGroundHeight);
  
  return ($imgGroundWidth,$imgGroundHeight);
}

####################################################################################################
#                                    TREE WEIGHTER METHODS                                         #
####################################################################################################

# method: weightBranch
#  Parcourt l'arbre et détermine le poids de chaque noeud en fonction des commandes à réaliser
#-------------------------------------------------------------------------------
sub weightBranch {
    my $self = shift;
    my $node = shift;

    TRACE;

    my @childList = $self->getChilds($node);
    if (scalar @childList == 0){
        $self->weightBottomNode($node);
        return TRUE;
    }

    my $accumulatedWeight = 0;

    foreach my $n (@childList){
        if (! $self->weightBranch($n)) {
            return FALSE;
        }
        $accumulatedWeight += $self->getAccumulatedWeightOfNode($n);
    }

    if (! $self->weightAboveNode($node)) {
        return FALSE
    }

    $self->setAccumulatedWeightOfNode($node,$accumulatedWeight);

    return TRUE;

}


# method: weightBottomNode 
#  Détermine le poids de chaque noeud du bas de l'arbre en fonction des commandes à réaliser :
#       - mergeNtiff
#       - wget
#---------------------------------------------------------------------------------------------------
sub weightBottomNode {
    my $self = shift;
    my $node = shift;

    TRACE;

    my $res  = "\n";

    my $bgImgPath=undef;

    if ((defined ($self->{datasource}) && 
            $self->{datasource}->getSRS() ne $self->{pyramid}->getTileMatrixSet()->getSRS())
    ||
        (! $self->{pyramid}->isNewPyramid() &&
            ($self->{pyramid}->getFormat()->getCompression() eq 'jpg' ||
            $self->{pyramid}->getFormat()->getCompression() eq 'png'))
    ) {

        $self->updateWeightOfNode($node,WGET_W);

    } else {
        my $newImgDesc = $self->getImgDescOfNode($node);
        if ( -f $newImgDesc->getFilePath() ){
            $self->updateWeightOfNode($node,TIFFCP_W);
        }

        $self->updateWeightOfNode($node,MERGENTIFF_W);
    }

    $self->updateWeightOfNode($node,TIFF2TILE_W);

    return TRUE;
}

# method: weightAboveNode
#  Détermine le poids de chaque noeud ayant des fils en fonction des commandes à réaliser :
#       - merge4tiff
#       - wget
#---------------------------------------------------------------------------------------------------
sub weightAboveNode {
    my $self = shift;
    my $node = shift;

    TRACE;

    my $newImgDesc = $self->getImgDescOfNode($node);
    my @childList = $self->getChilds($node);

    if (scalar @childList != 4){

        my $tm = $self->getTileMatrix($node->{level});
        if (! defined $tm) {
            ERROR(sprintf "Cannot load the Tile Matrix for the level %s",$node->{level});
            return FALSE;
        }

        my $tooWide =  $tm->getMatrixWidth() < $self->{pyramid}->getTilePerWidth();
        my $tooHigh =  $tm->getMatrixHeight() < $self->{pyramid}->getTilePerHeight();

        if (-f $newImgDesc->getFilePath()) {

            if ($self->{pyramid}->getFormat()->getCompression() eq 'jpg' ||
                $self->{pyramid}->getFormat()->getCompression() eq 'png') {
                if (! $tooWide && ! $tooHigh) {
                    $self->updateWeightOfNode($node,WGET_W);
                }
            } else {
                $self->updateWeightOfNode($node,TIFFCP_W);
            }
        }
    }

    $self->updateWeightOfNode($node,MERGE4TIFF_W);
    $self->updateWeightOfNode($node,TIFF2TILE_W);

    return TRUE;
}


# method: shareNodesOnJobs
#  Détermine le cutLevel afin que la répartition sur les différents scripts et le temps d'exécution
#  de ceux-ci soient, a priori, optimaux.
#  Création du tableau de répartition des noeuds sur les jobs
#-------------------------------------------------------------------------------
sub shareNodesOnJobs {
    my $self = shift;

    TRACE;

    my $optimalWeight = undef;
    my $cutLevelId = undef;
    my @jobsSharing = undef;

    # calcul du poids total de l'arbre : c'est la somme des poids cumulé des noeuds du topLevel
    my $wholeTreeWeight = 0;
    my @topLevelNodeList = $self->getNodesOfTopLevel();
    foreach my $node (@topLevelNodeList) {
        $wholeTreeWeight += $self->getAccumulatedWeightOfNode($node);
    }

    for (my $i = $self->{levelIdx}{$self->{topLevelId}}; $i >= $self->{levelIdx}{$self->{bottomLevelId}}; $i--){
        my $levelId = $self->{tmList}[$i]->getID();
        my @levelNodeList = $self->getNodesOfLevel($levelId);
        
        if (scalar @levelNodeList < $self->{job_number}) {
            next;
        }

        @levelNodeList =
            sort {$self->getAccumulatedWeightOfNode($b) <=> $self->getAccumulatedWeightOfNode($a)} @levelNodeList;

        my @JOBSWEIGHT;
        my @JOBS;
        for (my $j = 0; $j < $self->{job_number}; $j++) {
            push @JOBSWEIGHT, 0;
        }
        
        for (my $j = 0; $j < scalar @levelNodeList; $j++) {
            my $indexMin = $self->minArrayIndex(@JOBSWEIGHT);
            $JOBSWEIGHT[$indexMin] += $self->getAccumulatedWeightOfNode($levelNodeList[$j]);
            push (@{$JOBS[$indexMin]}, $levelNodeList[$j]);
        }
        
        # on additionne le poids du job le plus "lourd" et le poids du finisher pour quantifier le
        # pire temps d'exécution
        my $finisherWeight = $wholeTreeWeight - $self->sumArray(@JOBSWEIGHT);
        my $worstWeight = $self->maxArrayValue(@JOBSWEIGHT) + $finisherWeight;

        # on compare ce pire des cas avec celui obtenu jusqu'ici. S'il est plus petit, on garde ce niveau comme
        # cutLevel (a priori celui qui optimise le temps total de la génération de la pyramide).
        if (! defined $optimalWeight || $worstWeight < $optimalWeight) {
            $optimalWeight = $worstWeight;
            $cutLevelId = $levelId;
            @jobsSharing = @JOBS;
            DEBUG (sprintf "New cutLevel found : %s (worstWeight : %s)",$levelId,$optimalWeight);
        }

    }

    if (! defined $cutLevelId) {
        # Nous sommes dans le cas où même le niveau du bas contient moins de noeuds qu'il y a de jobs.
        # Dans ce cas, on définit le cutLevel comme le niveau du bas.
        INFO("The number of nodes in the bottomLevel is smaller than the number of jobs. Some one will be empty");
        $cutLevelId = $self->{bottomLevelId};
        my @levelNodeList = $self->getNodesOfLevel($cutLevelId);
        for (my $i = 0; $i < scalar @levelNodeList; $i++) {
            push (@{$jobsSharing[$i]}, $levelNodeList[$i]);
        }
    }

    $self->{cutLevelId} = $cutLevelId;

    return @jobsSharing;
}

####################################################################################################
#                                            ARRAY TOOLS                                           #
####################################################################################################

# method: minArrayIndex
#  Renvoie l'indice de l'élément le plus petit du tableau
#-------------------------------------------------------------------------------
sub minArrayIndex {
    my $self = shift;
    my @array = @_;

    TRACE;

    my $min = undef;
    my $minIndex = undef;

    for (my $i = 0; $i < scalar @array; $i++){
        if (! defined $minIndex || $min > $array[$i]) {
            $min = $array[$i];
            $minIndex = $i;
        }
    }

    return $minIndex;
}

# method: maxArrayValue
#  Renvoie la valeur maximale du tableau
#-------------------------------------------------------------------------------
sub maxArrayValue {
    my $self = shift;
    my @array = @_;

    TRACE;

    my $max = undef;

    for (my $i = 0; $i < scalar @array; $i++){
        if (! defined $max || $max < $array[$i]) {
            $max = $array[$i];
        }
    }

    return $max;
}

# method: sumArray
#  Renvoie la somme des élément du tableau tableau
#-------------------------------------------------------------------------------
sub sumArray {
    my $self = shift;
    my @array = @_;

    TRACE;

    my $sum = 0;

    for (my $i = 0; $i < scalar @array; $i++){
        $sum += $array[$i];
    }

    return $sum;
}

# method: oldSharing #TEST#
#-------------------------------------------------------------------------------
sub oldSharing {
    my $self = shift;
    my @nodes = @_;

    TRACE;

    my @nodeRackWeight;
    my $nodeCounter=0;

    foreach my $node (@nodes){
        $nodeRackWeight[$nodeCounter % $self->{job_number}] += $self->getAccumulatedWeightOfNode($node);
        $nodeCounter++;
    }

    print "Ancienne répartition : @nodeRackWeight\n"; 
}

# method: statArray #TEST#
#-------------------------------------------------------------------------------
sub statArray {
    my $self = shift;
    my @array = @_;

    TRACE;

    my $moyenne = 0;

    for (my $i = 0; $i < scalar @array; $i++){
        $moyenne += $array[$i];
    }

    $moyenne /= scalar @array;
    my $variance = 0;

    for (my $i = 0; $i < scalar @array; $i++){
        $variance += ($array[$i]-$moyenne) * ($array[$i]-$moyenne);
    }
    $variance /= scalar @array;
    my $ecarttype = sqrt($variance);
    print "Moyenne : $moyenne, écart type : $ecarttype\n"; 
}

####################################################################################################
#                                         GETTERS / SETTERS                                        #
####################################################################################################

# method: getImgDescOfNode
#  Retourne la description d'une image identifiée par node.
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
#  Renvoie les descripteurs des images source impliquées dans la mise à jour de la
#  dalle désignée par le noeud en parametre.
#  Le niveau de ce noeud doit etre bottomLevel.
#  Cette fonction n'est appelée que si les images sources sont dans la même
#  projetion que la pyramide.
#------------------------------------------------------------------------------
sub getImgDescOfBottomNode(){
  my $self = shift;
  my $node = shift;
  
  my $keyidx = sprintf "%s_%s", $node->{x}, $node->{y};
  return undef if ($node->{level} ne $self->{bottomLevelId});
  return $self->{levels}{$node->{level}}{$keyidx}[0];
}

# method: getWeightOfBottomNode
#  Renvoie le poids de la dalle désignée par le noeud en parametre.
#  Le niveau de ce noeud doit etre bottomLevel.
#------------------------------------------------------------------------------

sub getWeightOfBottomNode(){
  my $self = shift;
  my $node = shift;
  
  my $keyidx = sprintf "%s_%s", $node->{x}, $node->{y};
  return undef if ($node->{level} ne $self->{bottomLevelId});
  return $self->{levels}{$node->{level}}{$keyidx}[1];
}

# method: isInTree
#  Indique si le noeud en paramêtre appartient à l'arbre.
#------------------------------------------------------------------------------
sub isInTree(){
  my $self = shift;
  my $node = shift;
  my $keyidx = sprintf "%s_%s", $node->{x}, $node->{y};
  return $self->{levels}{$node->{level}}{$keyidx};
}

# method: getPossibleChilds
#  Renvoie la liste des 4 noeuds enfants possibles, ou une liste vide si on est 
#  sur une feuille.
#------------------------------------------------------------------------------
sub getPossibleChilds(){
  my $self = shift;
  my $node = shift;
  
  my @res;
  if ($self->{levelIdx}{$node->{level}} <= $self->{levelIdx}{$self->{bottomLevelId}}) {
    return @res;
  }

  my $lowerLevelId = ($self->{tmList}[$self->{levelIdx}{$node->{level}}-1])->getID();
  for (my $i=0; $i<=1; $i++){
    for (my $j=0; $j<=1; $j++){
      my $childNode = {level => $lowerLevelId, x => $node->{x}*2+$j, y => $node->{y}*2+$i};
      push(@res, $childNode);
      DEBUG(sprintf "Possible Child for %s_%s_%s (level_idx) : %s_%s_%s",
            $node->{level}, $node->{x},  $node->{y},
            $childNode->{level}, $childNode->{x},  $childNode->{y});
    }
  }
  return @res;
}

# method: getChilds
#  Renvoie les noeuds enfant du noeud passé en paramêtre.
#  Le noeud est un hash(level,x,y)
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

# method: getCutLevelId
#  Renvoie l'id du cutLevel
#------------------------------------------------------------------------------
sub getCutLevelId(){
  my $self = shift;
  return $self->{cutLevelId};
}

# method: getNodesOfLevel
#  Renvoie les noeuds du niveau en parametre.
#------------------------------------------------------------------------------
sub getNodesOfLevel(){
  my $self = shift;
  my $levelId= shift;

  if (! defined $levelId) {
    ERROR("Level undef ?");
    return undef;
  }
  
  my @nodes;
  my $lvl=$self->{levels}->{$levelId};

  foreach my $k (keys(%$lvl)){
    my ($x,$y) = split(/_/, $k);
    push(@nodes, {level => $levelId, x => $x, y => $y});
  }

  return @nodes;
}
sub getTopLevelId {
  my $self = shift;
  return $self->{topLevelId};
}
# method: getNodesOfTopLevel
#  Renvoie les noeuds du niveau le plus haut: topLevel.
#------------------------------------------------------------------------------
sub getNodesOfTopLevel(){
  my $self = shift;
  return $self->getNodesOfLevel($self->{topLevelId});
}

# method: getNodesOfCutLevel
#  Renvoie les noeuds du niveau cutLevel
#------------------------------------------------------------------------------
sub getNodesOfCutLevel(){
    my $self = shift;
    return $self->getNodesOfLevel($self->{cutLevelId});
}

# method: getTileMatrix
#  return the tile matrix from the supplied ID. This ID is the TMS ID (string) and not the ascending resolution 
#  order (integer).
#---------------------------------------------------------------------------------------------------------------
sub getTileMatrix {
  my $self = shift;
  my $level= shift; # id !
  
  if (! defined $level) {
    return undef;
  }
  
  return undef if (! exists($self->{levelIdx}->{$level}));
  
  return $self->{tmList}[$self->{levelIdx}->{$level}];
}

# method: getAccumulatedWeightOfNode
#  renvoie le poids cumulé du noeud
#------------------------------------------------------------------------------
sub getAccumulatedWeightOfNode(){
    my $self = shift;
    my $node = shift;

    my $keyidx = sprintf "%s_%s", $node->{x}, $node->{y};

    return $self->{levels}{$node->{level}}{$keyidx}[1];

}

# method: updateWeightOfNode
#  Ajoute au poids propre du noeud le poids passé en paramètre
#------------------------------------------------------------------------------
sub updateWeightOfNode(){
    my $self = shift;
    my $node = shift;
    my $weight = shift;

    my $keyidx = sprintf "%s_%s", $node->{x}, $node->{y};

    if ($node->{level} eq $self->{bottomLevelId}) {
        $self->{levels}{$node->{level}}{$keyidx}[1] += $weight;
    } else {
        $self->{levels}{$node->{level}}{$keyidx}[0] += $weight;
    }
}

# method: setAccumulatedWeightOfNode
#  Calcule le poids cumulé du noeud. Il ajoute le poids propre (déjà connu) du noeud à celui
#  passé en paramètre. Ce dernier correspond à la somme des poids cumulé des fils.
#------------------------------------------------------------------------------
sub setAccumulatedWeightOfNode(){
    my $self = shift;
    my $node = shift;
    my $weight = shift;

    my $keyidx = sprintf "%s_%s", $node->{x}, $node->{y};

    return if ($node->{level} eq $self->{bottomLevelId});

    $self->{levels}{$node->{level}}{$keyidx}[1] = $weight + $self->{levels}{$node->{level}}{$keyidx}[0];
    
}

####################################################################################################
#                                         EXPORT METHODS                                           #
####################################################################################################

# method: exportTree
#  Export dans un fichier texte de l'arbre complet.
#  (Orienté maintenance !)
#------------------------------------------------------------------------------
sub exportTree {
  my $self = shift;
  my $file = shift; # filepath !
  
  # sur le bottomlevel, on a :
  # { level1 => { x1_y2 => [[objimage1, objimage2, ...],w1],
  #               x2_y2 => [[objimage2],w1],
  #               x3_y2 => [[objimage3],w1], ...} }
  
  # on exporte dans un fichier la liste des indexes par images sources en projection :
  #  imagesource
  #  - x1_y2 => /OO/IF/ZX.tif => xmin, ymin, xmax, ymax
  #  - x2_y2 => /OO/IF/YX.tif => xmin, ymin, xmax, ymax
  #  ...
  TRACE;

  my $idLevel = $self->{bottomLevelId};
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
