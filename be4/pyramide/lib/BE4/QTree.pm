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
    * pyramid    => undef, # object Pyramid !
    * process    => undef, # object Process !
    * datasource => undef, # object DataSource !
    
    * bbox => [], # datasource bbox, [xmin,ymin,xmax,ymax], in TMS' SRS
    * nodes => {},
|   level1 => {
|      x1_y2 => n1,
|      x2_y2 => n2,
|      x3_y2 => n3, ...}
|   level2 => { 
|      x1_y2 => n4,
|      x2_y2 => n5, ...}
|
|   nX = BE4::Node object

    * cutLevelID    => undef, # top level for the parallele processing
    * bottomID => undef, # first level under the source images resolution
    * topID    => undef, # top level of the pyramid (ie of its tileMatrix)
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
        process    => undef,
        datasource => undef,
        # out
        bbox => [],
        nodes => {},
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

sub _init {
    my $self = shift;
    my $objSrc  = shift;
    my $objPyr  = shift;
    my $objProcess  = shift;

    TRACE;

    # mandatory parameters !
    if (! defined $objSrc || ref ($objSrc) ne "BE4::DataSource") {
        ERROR("Can not load DataSource !");
        return FALSE;
    }
    if (! defined $objPyr || ref ($objPyr) ne "BE4::Pyramid") {
        ERROR("Can not load Pyramid !");
        return FALSE;
    }
    if (! defined $objProcess || ref ($objProcess) ne "BE4::Process") {
        ERROR("Can not load Process !");
        return FALSE;
    }

    # init. params    
    $self->{pyramid} = $objPyr;
    $self->{datasource} = $objSrc; 
    $self->{process} = $objProcess;    

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
    if (! $self->identifyBottomTiles($ct)) {
        ERROR(sprintf "Cannot determine bottom tiles for the level %s",$src->getBottomID);
        return FALSE;
    }

    INFO(sprintf "Number of cache images to the bottom level (%s) : %d",
         $self->{bottomID},scalar keys(%{$self->{nodes}{$self->{bottomID}}}));

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
    
    my $bottomID = $self->{bottomID};
    my $tm = $self->{pyramid}->getTileMatrixSet->getTileMatrix($bottomID);
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
            my $iMin = $tm->xToColumn($bbox[0],$TPW);
            my $iMax = $tm->xToColumn($bbox[2],$TPW);
            my $jMin = $tm->yToRow($bbox[3],$TPH);
            my $jMax = $tm->yToRow($bbox[1],$TPH);
            
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
    
    my $tm = $self->{pyramid}->getTileMatrixSet->getTileMatrix($levelID);
    
    my $imgGroundWidth = $tm->getImgGroundWidth($self->{pyramid}->getTilesPerWidth);
    my $imgGroundHeight = $tm->getImgGroundHeight($self->{pyramid}->getTilesPerHeight);
    
    return ($imgGroundWidth,$imgGroundHeight);
}

####################################################################################################
#                                          COMPUTE METHODS                                         #
####################################################################################################

# Group: compute methods

#
=begin nd
method: computeWholeTree

Determine codes and weights for each node of the current graph, and share work on scripts, so as to optimize execution time.

Three steps:
    - browse tree : add weight and code to the nodes.
    - determine the cut level, to distribute fairly work.
    - browse the tree once again: we write commands in different scripts.

Parameter:
    NEWLIST - stream to the cache's list, to add new images.
    
See Also:
    <computeBranch>, <shareNodesOnJobs>, <writeBranchCode>, <writeTopCode>
=cut
sub computeWholeTree {
    my $self = shift;
    my $NEWLIST = shift;

    TRACE;
    
    # -------------------------------------------------------------------
    # Pondération de l'arbre en fonction des opérations à réaliser,
    # création du code script (ajouté aux noeuds de l'arbre) et mise à
    # jour de la liste des fichiers du cache avec les nouvelles images.
    my @topLevelNodeList = $self->getNodesOfTopLevel;
    
    foreach my $topNode (@topLevelNodeList) {
        if (! $self->computeBranch($topNode,$NEWLIST)) {
            ERROR(sprintf "Can not compute the node of the top level '%s'!", Dumper($topNode));
            return FALSE;
        }
    }
    
    # -------------------------------------------------------------------
    # Détermination du cutLevel optimal et répartition des noeuds sur les jobs,
    # en tenant compte du fait qu'ils peuvent déjà contenir du travail, du fait
    # de la pluralité des arbres à traiter.
    
    my @nodeRack;
    
    $self->shareNodesOnJobs(\@nodeRack,\@{$self->{process}->getWeights});
    
    if (! scalar @nodeRack) {
        ERROR("Cut Level Node List is empty !");
        return FALSE;
    }
    INFO (sprintf "CutLevel : %s", $self->{cutLevelID});
    
    # -------------------------------------------------------------------
    # Split scripts
    for (my $scriptCount = 1; $scriptCount <= $self->{process}->getJobNumber; $scriptCount++){

        if (! defined($nodeRack[$scriptCount-1])) {
            $self->{process}->printInScript("echo \"Nothing to do : not a problem.\"\n",$scriptCount);
        } else {
            foreach my $node (@{$nodeRack[$scriptCount-1]}) {
                $self->{process}->printInScript(
                    sprintf("echo \"NODE : LEVEL:%s X:%s Y:%s\"\n", $node->getLevel, $node->getCol, $node->getRow)
                    ,$scriptCount);
                $self->writeBranchCode($node,$scriptCount);
                # NOTE : pas d'image à collecter car non séparation par script du dossier temporaire, de travail
                # A rétablir si possible
                # on récupère l'image de travail finale pour le job de fin.
                # $finishScriptCode .= $self->collectWorkImage($node, $scriptId, $finishScriptId);
            }
        }
    }
    
    # -------------------------------------------------------------------
    # Final script    
    if ($self->getTopID eq $self->getCutLevelID){
        INFO("Final script will be empty");
        $self->{process}->printInScript("echo \"Final script have nothing to do.\" \n",0);
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
    - the node does not belong to the bottom level -> computeBranch on each child, and computeAboveImage

Parameter:
    node - node to treat.
    NEWLIST - stream to the cache's list, to add new images.
    
See Also:
    <computeBottomImage>, <computeAboveImage>
=cut
sub computeBranch {
    
    my $self = shift;
    my $node = shift;
    my $NEWLIST = shift;

    my $weight = 0;

    TRACE;
    
    printf $NEWLIST "0/%s\n", $self->{pyramid}->getCacheNameOfImage($node,'data');
    
    my $res = '';
    my @childList = $self->getChildren($node);
    if (scalar @childList == 0){
        if (! $self->computeBottomImage($node)) {
            ERROR(sprintf "Cannot compute the bottom image : %s_%s, level %s)",
                  $node->getCol, $node->getRow, $node->getLevel);
            return FALSE;
        }
        return TRUE;
    }
    foreach my $n (@childList){
        if (! $self->computeBranch($n,$NEWLIST)) {
            ERROR(sprintf "Cannot compute the branch from node %s)", $node->getWorkBaseName);
            return FALSE;
        }
        $weight += $n->getAccumulatedWeight;
    }

    if (! $self->computeAboveImage($node)) {
        ERROR(sprintf "Cannot compute the above image : %s)", $node->getWorkBaseName);
        return FALSE;
    }

    $node->setAccumulatedWeight($weight);

    return TRUE;
}

#
=begin nd
method: computeBottomImage

Treat a bottom node : determine code and weight.

2 cases.
    - native projection, lossless compression and images as data -> mergeNtiff
    - reprojection or lossy compression or just a WMS service as data -> wget

Parameter:
    node - bottom level's node, to treat.
    
See Also:
    <wms2work>, <Process:cache2work>, <mergeNtiff>, <work2cache>
=cut
sub computeBottomImage {
    
    my $self = shift;
    my $node = shift;

    TRACE;
    
    # Temporary weight and code
    my ($c,$w);
    # Final weight and code
    my $weight  = 0;
    my $code  = "\n";
    
    if ($self->getDataSource->hasHarvesting) {
        # Datasource has a WMS service : we have to use it
        ($c,$w) = $self->{process}->wms2work($node,$self->getDataSource->getHarvesting);
        $code .= $c;
        $weight += $w;
    } else {    
        ($c,$w) = $self->{process}->mergeNtiff($node);
        if ($w == -1) {
            ERROR(sprintf "Cannot compose mergeNtiff command for the node %s.",$node->getWorkBaseName);
            return FALSE;
        }
        $code .= $c;
        $weight += $w;
    }

    # copie de l'image de travail créée dans le rep temp vers l'image de cache dans la pyramide.
    ($c,$w) = $self->{process}->work2cache($node,($node->getLevel eq $self->getTopID));
    
    $code .= $c;
    $weight += $w;

    $node->updateOwnWeight($weight);
    $node->setCode($code);

    return TRUE;
}

#
=begin nd
method: computeAboveImage

Treat an above node (different to the bottom level) : determine code and weight.

To generate an above node, we use children (merge4tiff). If we have not 4 children or if children contain nodata, we have to supply a background, a color or an image if exists.

Parameter:
    node - above level's node, to treat.
    
See Also:
    <wms2work>, <cache2work>, <merge4tiff>, <work2cache>
=cut
sub computeAboveImage {
    
    my $self = shift;
    my $node = shift;

    TRACE;

    # Temporary weight and code
    my ($c,$w);
    # Final weight and code
    my $weight  = 0;
    my $code  = "\n";

    my $workBgPath=undef;
    my $workBgName=undef;

    # On renseigne dans tous les cas la couleur de nodata, et on donne un fond s'il existe, même s'il y a 4 images,
    # si on a l'option nowhite
    my $bg='-n ' . $self->{pyramid}->getNodata->getValue;
    
    my @childList = $self->getChildren($node);

    if (scalar @childList != 4 || $self->{pyramid}->getNodata->getNoWhite) {
        # Pour cela, on va récupérer le nombre de tuiles (en largeur et en hauteur) du niveau, et 
        # le comparer avec le nombre de tuile dans une image (qui est potentiellement demandée à 
        # rok4, qui n'aime pas). Si l'image contient plus de tuile que le niveau, on ne demande plus
        # (c'est qu'on a déjà tout ce qui faut avec les niveaux inférieurs).

        # WARNING (TOS) cette solution n'est valable que si la structure de l'image (nombre de tuile dans l'image si
        # tuilage il y a) est la même entre le cache moissonné et la pyramide en cours d'écriture.
        # On a à l'heure actuelle du 16 sur 16 sur toute les pyramides et pas de JPEG 2000. 

        my $tm = $self->{pyramid}->getTileMatrixSet->getTileMatrix($node->getLevel);

        my $tooWide =  $tm->getMatrixWidth() < $self->{pyramid}->getTilesPerWidth();
        my $tooHigh =  $tm->getMatrixHeight() < $self->{pyramid}->getTilesPerHeight();
        
        my $cacheImgPath = $self->{pyramid}->getCachePathOfImage($node,'data');
        my $workImgPath = File::Spec->catfile($self->{process}->getScriptTmpDir, $node->getWorkName);

        if (-f $cacheImgPath) {
            # Il y a dans la pyramide une dalle pour faire image de fond de notre nouvelle dalle.
            $workBgName = join("_","bgImg",$node->getWorkName);
            $workBgPath = File::Spec->catfile('${TMP_DIR}',$workBgName);

            if ($self->{pyramid}->getCompression() eq 'jpg') {
                # On vérifie d'abord qu'on ne veut pas moissonner une zone trop grande
                if ($tooWide || $tooHigh) {
                    WARN(sprintf "The image would have been too high or too wide to harvest it (level %s)",
                         $node->getLevel);
                } else {
                    # On peut et doit chercher l'image de fond sur le WMS
                    $bg.=" -b $workBgPath";
                    ($c,$w) = $self->{process}->wms2work($node,$self->getDataSource->getHarvesting);
                    $code .= $c;
                    $weight += $w;
                }
            } else {
                # copie avec tiffcp ou untile+montage pour passer du format de cache au format de travail.
                $bg.=" -b $workBgPath";
                ($c,$w) = $self->{process}->cache2work($node);
                $code .= $c;
                $weight += $w;
            }
        }
    }


    # Maintenant on constitue la liste des images à passer à merge4tiff.
    my $childImgParam=''; 
    my $imgCount=0;
    
    foreach my $childNode ($self->getPossibleChildren($node)) {
        $imgCount++;
        if (defined $childNode){
            $childImgParam.=' -i'.$imgCount.' $TMP_DIR/' . $childNode->getWorkName;
        }
    }
    ($c,$w) = $self->{process}->merge4tiff('$TMP_DIR/'.$node->getWorkName, $bg, $childImgParam);
    $code .= $c;
    $weight += $w;

    # Suppression des images de travail dont on a plus besoin.
    foreach my $node (@childList){
        $code .= sprintf "rm -f \${TMP_DIR}/%s \n", $node->getWorkName;
    }

    # Si on a copié une image pour le fond, on la supprime maintenant
    if ( defined $workBgName ){
        $code.= "rm -f $workBgPath \n";
    }

    # copie de l'image de travail crée dans le rep temp vers l'image de cache dans la pyramide.
    ($c,$w) = $self->{process}->work2cache($node,($node->getLevel eq $self->getTopID));
    $code .= $c;
    $weight += $w;

    $node->updateOwnWeight($weight);
    $node->setCode($code);

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
    node - node whose code is written.
    ind - indice (integer) of the stream in which we want to write code.
=cut
sub writeBranchCode {
    my $self = shift;
    my $node = shift;
    my $ind = shift;

    TRACE;

    my $code = '';
    my @childList = $self->getChildren($node);

    # Le noeud est une feuille
    if (scalar @childList == 0){
        $self->{process}->printInScript($node->getCode,$ind);
        return TRUE;
    }

    # Le noeud a des enfants
    foreach my $n (@childList){
        $self->writeBranchCode($n,$ind);
    }
    
    $self->{process}->printInScript($node->getCode,$ind);

    return TRUE;
}

#
=begin nd
method: writeTopCode

Recursive method, which allow to browse downward the tree, from the top, to the cut level and write commands in the script finisher.

Parameter:
    node - node whose code is written.
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
    
    $self->{process}->printInScript($node->getCode,0);

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

Parameters:
    nodeRack - reference to array, to return nodes' sharing (length = number of jobs).
    weights - reference to array, contains current weights (jobs are already filled if several data sources) and to return new weights (length = number of jobs + 1, the script finisher).
=cut
sub shareNodesOnJobs {
    my $self = shift;
    my ($nodeRack,$weights) = @_;

    TRACE;

    my $tms = $self->{pyramid}->getTileMatrixSet;
    my $jobNumber = $self->{process}->getJobNumber;
    
    my $optimalWeight = undef;
    my $cutLevelID = undef;
    
    my @jobsSharing = undef;
    my @jobsWeights = undef;

    # calcul du poids total de l'arbre : c'est la somme des poids cumulé des noeuds du topLevel
    my $wholeTreeWeight = 0;
    my @topLevelNodeList = $self->getNodesOfTopLevel;
    foreach my $node (@topLevelNodeList) {
        $wholeTreeWeight += $node->getAccumulatedWeight;
    }

    for (my $i = $self->getTopOrder(); $i >= $self->getBottomOrder(); $i--){
        my $levelID = $tms->getIDfromOrder($i);
        my @levelNodeList = $self->getNodesOfLevel($levelID);
        
        if ($levelID ne $self->{bottomID} && scalar @levelNodeList < $jobNumber) {
            next;
        }
        
        @levelNodeList =
            sort {$b->getAccumulatedWeight <=> $a->getAccumulatedWeight} @levelNodeList;

        my @TMP_WEIGHTS;
        
        for (my $j = 0; $j <= $jobNumber; $j++) {
            # On initialise les poids avec ceux des jobs
            $TMP_WEIGHTS[$j] = $weights->[$j];
        }
        
        my $finisherWeight = $wholeTreeWeight;
        my @TMP_JOBS;
        
        for (my $j = 0; $j < scalar @levelNodeList; $j++) {
            my $indexMin = BE4::Array->minArrayIndex(1,@TMP_WEIGHTS);
            my $nodeWeight = $levelNodeList[$j]->getAccumulatedWeight;
            $TMP_WEIGHTS[$indexMin] += $nodeWeight;
            $finisherWeight -= $nodeWeight;
            push @{$TMP_JOBS[$indexMin-1]}, $levelNodeList[$j];
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
            @jobsSharing = @TMP_JOBS;            
            @jobsWeights = @TMP_WEIGHTS;
            DEBUG (sprintf "New cutLevel found : %s (worstWeight : %s)",$levelID,$optimalWeight);
        }
    }

    $self->{cutLevelID} = $cutLevelID;
    
    # We store results in array references
    for (my $i = 0; $i < $jobNumber; $i++) {
        $nodeRack->[$i] = $jobsSharing[$i];
        $weights->[$i] = $jobsWeights[$i];
    }
    # Weights' array is longer
    $weights->[$jobNumber] = $jobsWeights[$jobNumber];

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

#
=begin nd
method: containsNode

Parameters:
    node - node we want to know if it is in the tree.

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
method: getPossibleChildren

Parameters:
    node - node we want to know children.

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
    node - node we want to know children.

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

=item process

A Process object.

=item datasource

A Datasource object.

=item bbox

Array [xmin,ymin,xmax,ymax], bbox of datasource in the TMS' SRS.

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

=item bottomID, topID

Extrem levels identifiants of the tree.

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
