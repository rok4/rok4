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

package BE4::Graph;

use Math::BigFloat;
use Geo::OSR;

use strict;
use warnings;

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
use constant CACHE2WORK_PNG_W => 3;
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
    levels => {}, # levelZ => { x_y => node, x'_y' => node', ...}
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
    my @tmList = $tms->getTileMatrixByArray();# tableau de TM trié par résolution croissante
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
        my ($iMin,$jMax) = @{$tm->computeImageCoordFromLandCoord($bbox{xMin},$bbox{yMax},$self->{pyramid})};
        my ($iMax,$jMin) = @{$tm->computeImageCoordFromLandCoord($bbox{xMax},$bbox{yMin},$self->{pyramid})};

        for (my $i = $iMin; $i<= $iMax; $i++){       
            for (my $j = $jMin; $j<= $jMax; $j++){
                my $idxkey = sprintf "%s_%s", $i, $j;
                my $node = undef;
                if (! defined $self->{levels}{$self->{bottomLevelId}}{$idxkey}) {
                  $node = new BE4::Node({
                    i => $i,
                    j => $j,
                    tmObj => $tm,
                    graphObj => $self,
                    filePath => undef,
                    w => undef,
                    W => undef,
                    c => undef
                  });
                  $self->{levels}{$self->{bottomLevelId}}{$idxkey} = $node;
                } else {
                  $node =  $self->{levels}{$self->{bottomLevelId}}{$idxkey};
                }                
                $node->setImageSources($objImg);
            }
        }
    }

    DEBUG(sprintf "N. Tile Cache to the bottom level : %d", scalar keys( %{$self->{levels}{$self->{bottomLevelId}}} ));
    
    # On parcourt chaque niveau/level
    for (my $l = $self->{levelIdx}{$self->{bottomLevelId}}; $l <= $self->{levelIdx}{$self->{topLevelId}}; $l++){
      my $levelId = $tmList[$l]->getID();
      my @parentstmid = @{$tmList[$l]->getParentsTmId()};
      #my @parentstmres = map {$tmList[$self->{levelIdx}{$_}]->getResolution()} @parentstmid;
      #print "On parcours le niveau : ".$tmList[$i]->{resolution}." ";
      #printf "On calcul les images des niveaux %s \n",join(' ',@parentstmres);
      # on parcours toutes images de ce level
      foreach my $refnode ($self->getNodesOfLevel($levelId)){
        my ($xMin,$yMax,$xMax,$yMin) = @{$refnode->getBbox()};
        # pour chaque image du niveau levelId
        # on calcule les images du parentTm qui utiliseront cette image comme source
        foreach my $parentTmId (@parentstmid) {
          my $parentTm = $tms->getTileMatrix($parentTmId);
          my ($iMin,$jMax) = @{$parentTm->computeImageCoordFromLandCoord($xMin,$yMax,$self->{pyramid})};
          my ($iMax,$jMin) = @{$parentTm->computeImageCoordFromLandCoord($xMax,$yMin,$self->{pyramid})};
          for (my $i = $iMin; $i < $iMax + 1; $i++){
            for (my $j = $jMin ; $j < $jMax +1 ; $j++) {
              my $idxkey = sprintf "%s_%s",$i,$j;
              my $node = undef;
              if (! defined $self->{levels}{$levelId}{$idxkey}) {
                $node = new BE4::Node({
                  i => $i,
                  j => $j,
                  tmObj => $parentTm,
                  graphObj => $self,
                  filePath => undef,
                  w => undef,
                  W => undef,
                  c => undef
                });
                $self->{levels}{$levelId}{$idxkey} = $node ;
              } else {
                $node = $self->{levels}{$levelId}{$idxkey};
              }
              $node->setNodeSources($refnode);              
            }
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
sub computeBBox {
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

####################################################################################################
#                                         CUT LEVEL METHOD                                         #
####################################################################################################


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
        INFO(sprintf "The number of nodes in the bottomLevel (%s) is smaller than the number of jobs (%s). Some one will be empty",
                scalar $self->getNodesOfLevel($self->{bottomLevelId}),$self->{job_number});
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
#                                         GETTERS / SETTERS                                        #
####################################################################################################

sub getPyramid {
  my $self = shift;
  return $self->{pyramid};
}

# method: isInTree
#  Indique si le noeud en paramêtre appartient à l'arbre.
#------------------------------------------------------------------------------
sub isInTree(){
  my $self = shift;
  my $node = shift;
  
  my $level = $node->getTmObj()->getId();
  my $keyidx = sprintf "%s_%s", $node->{i}, $node->{j};
  return $self->{levels}{$level}{$keyidx};
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
  
  my $lvl=$self->{levels}->{$levelId};
  my @nodes = values(%$lvl);
  
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

####################################################################################################
#                                         EXPORT METHODS                                           #
####################################################################################################

# method: exportGraph
#  Export dans un fichier texte du Graph
#  (Orienté maintenance !)
#------------------------------------------------------------------------------
sub exportGraph {
  my $self = shift;
  my $file = shift; # filepath !
  
  # On exporte dans un fichier la liste des images à calculer.
  # La nomenclature est la suivante :
  # <Description du Graph> (voir plus bas)
  # <Résolution du TM> <Image ID (i_j)> <xmin> <ymin> <xmax> <ymax> <From [IdSource1|IdSource2|...]>
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
  my ($xmin,$ymin) = $self->getTileMatrix($idLevel)->computeLandCoordFromImageCoord($idxmin,$idymin,$refpyr);
  my ($xmax,$ymax) = $self->getTileMatrix($idLevel)->computeLandCoordFromImageCoord($idxmax,$idymax,$refpyr);
  push (@bboxfinal,($xmin,$ymin,$xmax,$ymax));
  
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
 
  for (my $l = $self->{levelIdx}{$self->{bottomLevelId}}; $l <= $self->{levelIdx}{$self->{topLevelId}}; $l++){
    my @nodes = $self->getNodesOfLevel($l);
    my $tm = $self->getTileMatrix($l);
    my $resolution = $tm->getResolution();
    foreach my $node (@nodes) {
      my $id = sprintf "%s_%s",$node->getCoordI(),$node->getCoordJ();
      my $bbox = join('|',@{$node->getBbox()});
      my $sources = join('|',@{$node->getNodeSources()},@{$node->getImageSources()});
      printf FILE "\nResolution du TM : %s, ID : %s, BBox : [%s], Source :[%s]",$resolution,$id,$bbox,$sources;
    }
  }
  
  close FILE;
  
  return TRUE;
}

1;
__END__
