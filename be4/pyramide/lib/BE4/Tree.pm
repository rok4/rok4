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
# constantes
use constant TRUE  => 1;
use constant FALSE => 0;

#-------------------------------------------------------------------------------
# Global

#-------------------------------------------------------------------------------
# Preloaded methods go here.
BEGIN {}
INIT {}
END {}

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
    levels        => {}, # level1 => { x1_y2 => [objimage1], x2_y2 => [objimage2], x3_y2 => [objimage3], ...}
                         # level2 => {...}
                         # with objimage = Class ImageSource 
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

    # tilematrix list sort by resolution
    my @tmList = $tms->getTileMatrixByArray();
    @{$self->{tmList}} = @tmList;

    # on fait un hash pour retrouver l'ordre d'un niveau a partir de son id.
    for (my $i=0; $i < scalar @tmList; $i++){
        $self->{levelIdx}{$tmList[$i]->getID()} = $i;
    }


    # initialisation de la transfo de coord du srs des données initiales vers
    # le srs de la pyramide. Si les srs sont identique on laisse undef.
    my $ct = undef;  
    if ($tms->getSRS() ne $src->getSRS()){
        my $srsini= new Geo::OSR::SpatialReference;
        eval { $srsini->ImportFromProj4('+init='.$src->getSRS().' +wktext'); };
        if ($@) {
            ERROR(Geo::GDAL::GetLastErrorMsg());
            return FALSE;
        }
        my $srsfin= new Geo::OSR::SpatialReference;
        eval { $srsfin->ImportFromProj4('+init='.$tms->getSRS().' +wktext'); };
        if ($@) {
            ERROR(Geo::GDAL::GetLastErrorMsg());
            return FALSE;
        }
        $ct = new Geo::OSR::CoordinateTransformation($srsini, $srsfin);
    }

    # Intitialisation du topLevel:
    #  - En priorité celui fourni en paramètre
    #  - Par defaut, c'est le plus haut niveau du TMS, 
    my $toplevel = $self->{pyramid}->getTopLevel();

    if (defined $toplevel) {
        $self->{topLevelId} = $toplevel;
    } else {
        $self->{topLevelId} = $tmList[$#tmList]->getID();
    }


    # Intitialisation du bottomLevel:
    #  - En priorité celui fourni en paramètre
    #  - Par defaut, le niveau de base du calcul est le premier niveau dont la résolution
    #  (réduite de 5%) est meilleure que celle des données sources.
    #  S'il n'y a pas de niveau dont la résolution est meilleure, on prend le niveau
    #  le plus bas de la pyramide.
    my $bottomlevel = $self->{pyramid}->getBottomLevel();
        if (defined $bottomlevel) {
        $self->{bottomLevelId} = $bottomlevel;
    } else {
        my $projSrcRes = $self->computeSrcRes($ct);
        if ($projSrcRes < 0) {
            ERROR("La resolution reprojetee est negative");
            return FALSE;
        }

        $self->{bottomLevelId} = $tmList[0]->getID(); 
        foreach my $tm (@tmList){
            next if ($tm->getResolution() * 0.95  > $projSrcRes);
            $self->{bottomLevelId} = $tm->getID();
        }
    }

    # identifier les dalles du niveau de base à mettre à jour et les associer aux images sources:

    my ($ImgGroundWidth, $ImgGroundHeight) = $self->imgGroundSizeOfLevel($self->{bottomLevelId});

    my $tm = $tms->getTileMatrix($self->{bottomLevelId});

    my @images = $src->getImages();

    foreach my $objImg (@images){
        # On reprojette l'emprise si nécessaire 
        my %bbox = $self->computeBBox($objImg, $ct);

        # pyramid's limits update : we store data's limits in the object Pyramid
        $self->{pyramid}->updateLimits($bbox{xMin},$bbox{yMin},$bbox{xMax},$bbox{yMax});

        # On divise les coord par la taille des dalles de cache pour avoir les indices min et max en x et y
        my $iMin=int(($bbox{xMin} - $tm->getTopLeftCornerX()) / $ImgGroundWidth);   
        my $iMax=int(($bbox{xMax} - $tm->getTopLeftCornerX()) / $ImgGroundWidth);   
        my $jMin=int(($tm->getTopLeftCornerY() - $bbox{yMax}) / $ImgGroundHeight); 
        my $jMax=int(($tm->getTopLeftCornerY() - $bbox{yMin}) / $ImgGroundHeight);

        my $level  = $self->{bottomLevelId};

        for (my $i = $iMin; $i<= $iMax; $i++){       
            for (my $j = $jMin; $j<= $jMax; $j++){
                my $idxkey = sprintf "%s_%s", $i, $j;
                push (@{$self->{levels}{$self->{bottomLevelId}}{$idxkey}},$objImg);

                # { level1 => { x1_y2 => [list objimage1],
                #               x2_y2 => [list objimage2],
                #               x3_y2 => [list objimage3], ...} }
            }
        }
    }


    DEBUG(sprintf "N. Tile Cache to the bottom level : %d", scalar keys( %{$self->{levels}{$self->{bottomLevelId}}} ));


    # Si au bottomLevelId il y a moins de noeud que le nombre de processus demande,
    # on le definit tout de meme comme cutLevel. Tant pis, on aura des scripts vides.
    $self->{cutLevelId}=$self->{bottomLevelId};
    # Calcul des branches à partir des feuilles et de leur poids:
    for (my $i = $self->{levelIdx}{$self->{bottomLevelId}}; $i < scalar(@tmList)-1; $i++){
        my $levelId = $tmList[$i]->getID();

        # On compare au passage le nombre de noeuds du niveau courrant avec le nombre de processus demandé 
        # pour déterminer le cutLevel. A noter que tel que c'est fait ici, le cutLevel ne peut pas etre 
        # le topLevel. On s'evite ainsi des complications avec un script finisher vide et de toute façon
        # le cas n'arrive jamais avec les TMS GPP3.
        if (keys(%{$self->{levels}->{$levelId}}) >= $self->{job_number}){
            $self->{cutLevelId}=$levelId;
            DEBUG(sprintf "cutLevelId become: %s", $levelId);
        }
        my $aboveLevelId = $tmList[$i+1]->getID();
        foreach my $refnode ($self->getNodesOfLevel($levelId)){
            my $parentNodeId = int($refnode->{x}/2)."_".int($refnode->{y}/2);
            if (!defined($self->{levels}{$aboveLevelId}{$parentNodeId})){
                # On a une nouvelle image a calculer (merge4tiff)
                $self->{levels}{$aboveLevelId}{$parentNodeId}=1;
            }
            if ($levelId eq $self->{bottomLevelId}){
                # On ajoute le poids du calcul de l'image de base (mergeNtiff)
                # TODO: ce poids est ici égal a celui de merge4tiff, pour bien faire
                #       il faudrait lui trouver une ponderation plus realiste.
                #       Ce n'est pas evident car dans le cas d'un moissonnage, on 
                #       ne fait pas de mergeNtiff mais un wget.
                $self->{levels}{$aboveLevelId}{$parentNodeId} += 1;
            } else {
                # On ajoute le poids calcule pour les noeuds du dessous
                $self->{levels}{$aboveLevelId}{$parentNodeId} += $self->{levels}{$levelId}{$refnode->{x}."_".$refnode->{y}}; 
            }
        }

        DEBUG(sprintf "N. Tile Cache by level (%s) : %d", $levelId, scalar keys( %{$self->{levels}{$levelId}} ));
    }  

    return TRUE;
}

# method: exportTree
#  Export dans un fichier texte de l'arbre complet.
#  (Orienté maintenance !)
#------------------------------------------------------------------------------
sub exportTree {
  my $self = shift;
  my $file = shift; # filepath !
  
  # sur le bottomlevel, on a :
  # { level1 => { x1_y2 => [objimage1, objimage2, ...],
  #               x2_y2 => [objimage2],
  #               x3_y2 => [objimage3], ...} }
  
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
    my $p= $ct->TransformPoint($polygon[$i][0],$polygon[$i][1]);

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
  #my $xRes = $tm->getResolution(); 
  #my $yRes =$tm->getResolution();
  my $xRes = Math::BigFloat->new($tm->getResolution()); 
  my $yRes = Math::BigFloat->new($tm->getResolution());
  my $imgGroundWidth   = $tm->getTileWidth()  * $self->{pyramid}->getTilePerWidth() * $xRes;
  my $imgGroundHeight = $tm->getTileHeight() * $self->{pyramid}->getTilePerHeight() * $yRes;
  
  DEBUG (sprintf "Size ground (level %s): %s * %s SRS unit", $levelId, $imgGroundWidth, $imgGroundHeight);
  
  return ($imgGroundWidth,$imgGroundHeight);
}

# method: computeSrcRes
#  Retourne la meilleure résolution des images source. Ceci implique une 
#  reprojection dans le cas où le SRS des images source n'est pas le même 
#  que celui de la pyramide.
#------------------------------------------------------------------------------
sub computeSrcRes(){
  my $self = shift;
  my $ct = shift;
  
  TRACE();
  
  my $srcRes = $self->{datasource}->getResolution();
  if (!defined($ct)){
    return $srcRes;
  }
  my @imgs = $self->{datasource}->getImages();
  my $res = 50000000.0;  # un pixel plus gros que la Terre en m ou en deg.
  foreach my $img (@imgs){
    # FIXME: il faut absoluement tester les erreurs ici:
    #        les transformations WGS84G (PlanetObserver) vers PM ne sont pas possible au delà  de 85.05°.
    
    my $p1 = $ct->TransformPoint($img->getXmin(),$img->getYmin());

    my $p2 = $ct->TransformPoint($img->getXmax(),$img->getYmax());

    # JPB : FIXME attention au erreur d'arrondi avec les divisions 
    my $xRes = $srcRes * (@{$p2}[0]-@{$p1}[0]) / ($img->getXmax()-$img->getXmin());
    my $yRes = $srcRes * (@{$p2}[1]-@{$p1}[1]) / ($img->getYmax()-$img->getYmin());
    
    $res=$xRes if $xRes < $res;
    $res=$yRes if $yRes < $res;
  }
 
  return $res;
}

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
#
#  Le niveau de ce noeud doit etre bottomLevel.
#
#  Cette fonction n'est appelée que si les images sources sont dans la même
#  projetion que la pyramide.
#------------------------------------------------------------------------------
sub getImgDescOfBottomNode(){
  my $self = shift;
  my $node = shift;
  
  my $keyidx = sprintf "%s_%s", $node->{x}, $node->{y};
  return undef if ($node->{level} ne $self->{bottomLevelId});
  return $self->{levels}{$node->{level}}{$keyidx};
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

1;
__END__
