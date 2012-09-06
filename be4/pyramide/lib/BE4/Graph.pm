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
    for (my $k = $src->getBottomOrder; $k <= $src->getTopOrder; $k++){

        my $levelID = $tms->getIDfromOrder($k);
        my $sourceTm = $tms->getTileMatrix($levelID);
        my @targetLevelsID = @{$sourceTm->getTargetsTmId()};
        
        # pyramid's limits update : we store data's limits in the pyramid's levels
        $self->{pyramid}->updateTMLimits($levelID,@{$self->{bbox}});

        foreach my $node ( $self->getNodesOfLevel($levelID) ) {
            
            # On récupère la BBOX du noeud pour calculer les noeuds cibles
            my ($xMin,$yMax,$xMax,$yMin) = $node->getBBox();
            
            foreach my $targetTmID (@targetLevelsID) {
                my $targetTm = $tms->getTileMatrix($targetTmID);
                my $iMin = $targetTm->xToColumn($xMin,$tilesPerWidth);
                my $iMax = $targetTm->xToColumn($xMax,$tilesPerWidth);
                my $jMin = $targetTm->yToRow($yMin,$tilesPerHeight);
                my $jMax = $targetTm->yToRow($yMax,$tilesPerHeight);
                
                for (my $i = $iMin; $i < $iMax + 1; $i++){
                    for (my $j = $jMin ; $j < $jMax +1 ; $j++) {
                        
                      my $idxkey = sprintf "%s_%s",$i,$j;
                      my $newnode = undef;
                      if (! defined $self->{nodes}->{$targetTmID}->{$idxkey}) {
                        $newnode = new BE4::Node({
                          i => $i,
                          j => $j,
                          tm => $targetTm,
                          graph => $self,
                        });
                        $self->{nodes}->{$targetTmID}->{$idxkey} = $newnode ;
                      } else {
                        $newnode = $self->{nodes}->{$targetTmID}->{$idxkey};
                      }
                     $newnode->addNodeSources($node);              
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
#                                          COMPUTE METHODS                                         #
####################################################################################################

# Group: compute methods

#
=begin nd
method: computeWholeTree

Determine codes and weights for each node of the current graph, and share work on scripts, so as to optimize execution time.

Only one step:
    - browse graph and write commands in different scripts.

Parameter:
    NEWLIST - stream to the cache's list, to add new images.
    
See Also:
    <computeBranch>, <shareNodesOnJobs>, <writeBranchCode>, <writeTopCode>
=cut
sub computeWholeTree {
    my $self = shift;
    my $NEWLIST = shift;
    
    my $src = $self->{datasource};

    # Prepare TMP repository
    if (! $self->prepareTMP() ) {
         ERROR(sprintf "Cannot prepare the directories needed to write scripts.");
        return FALSE;
    }
   #Initialisation
   my $Finisher_Index = 0;
   # boucle sur tous les niveaux en partant de ceux du bas
   for(my $i = $src->getBottomOrder; $i <= $src->getTopOrder; $i++) {
       # boucle sur tous les noeuds du niveau
       foreach my $node ( $self->getNodesOfLevel($self->getPyramid()->getTileMatrixSet()->getIDfromOrder($i))) {
           # on détermine dans quel script on l'écrit en se basant sur les poids
           my $Script_Nb = BE4::Array->minArrayIndex(0,@{${$self->{process}->getWeights()}[$node->getLevel()]});
           # on stocke l'information dans l'objet node
           $node->setScriptNb($Script_Nb);
            # on détermine le script à ecrire
           my $code = "\n";
           ### Pour l'instant seulement mergeNtiff
           ### TODO : les autres cas que mergeNtiff
           my ($c,$w) = $self->{process}->mergeNtiff($node);
           if ($w == -1) {
            ERROR(sprintf "Cannot compose mergeNtiff command for the node %s.",$node->getWorkBaseName);
            return FALSE;
           }
           $code .= $c ;
           # on met à jour les poids
           ${${$self->{process}->getWeights()}[$node->getLevel()]}[$Script_Nb] += $w;
           # on ecrit la commande dans le fichier
           my $PRINT = ${${$self->{process}->getStreams()}[$node->getLevel()]}[$Script_Nb] ;
           printf $PRINT "%s",$c;
           
           #TODO
           # on met à jour NEWLIST
           
           # final script with all work2tile commands
           # on ecrit dans chacun des scripts de manière tournante
           $code = "\n";
           ($c,$w) = $self->{process}->work2cache($node,1);
           $code .= $c ;
           # on ecrit la commande dans le fichier
           $PRINT = ${${$self->{process}->getStreams()}[$src->getTopOrder + 1]}[$Finisher_Index] ;
           printf $PRINT "%s",$c;
           #on met à jour l'index
           if ($Finisher_Index == $self->{process}->getJobNumber() - 1) {
               $Finisher_Index = 0;
           } else {
               $Finisher_Index ++;
           }

       }
   }

   #$self->exportGraph();
   
   # on ferme tous les streams
   #for(my $i = $src->getBottomOrder; $i <= $src->getTopOrder; $i++) {
   #  foreach my $stream (@{${$self->{process}->getWeights()}[$i]}){
   #    close($stream);
   #  }
   #}
    
    TRACE;
    
    return TRUE;
};

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

####################################################################################################
#                                         SCRIPT MANAGEMENT                                        #
####################################################################################################

# Prepare TMP repository for temp files
sub prepareTMP {
    my $self = shift ;
    my $src = $self->{datasource};
    
    # creation of tmp directories
    my $TMP_path = $self->{process}->getRootTmpDir();
    if ( ! BE4::Graph::createDirectory($TMP_path) ) {return FALSE;}
    
    for (my $i = $src->getBottomOrder; $i <= $src->getTopOrder + 1; $i++){
        
        ### creation of level sub-directory
        my $Level_path = File::Spec->catfile($TMP_path,"LEVEL_".$i);
        $Level_path = File::Spec->catfile($TMP_path,"FINISHER") if ($i eq $src->getTopOrder + 1);
        
        if ( ! BE4::Graph::createDirectory($Level_path) ) {return FALSE;}
        
        for (my $j = 0; $j < $self->{process}->getJobNumber(); $j++) {
            
          ### creation of script sub-directory
          my $Script_path = File::Spec->catfile($Level_path,"SCRIPT_".$j);
          if ( ! BE4::Graph::createDirectory($Script_path) ) {return FALSE;}
          
          if ($i ne $src->getTopOrder + 1) {
            ### creation of mergeNtiff config sub-directory
            my $Config_path = File::Spec->catfile($Script_path,"mergeNtiff");
            if ( ! BE4::Graph::createDirectory($Config_path) ) {return FALSE;}
        }
            
        }
    }
    return TRUE;
    
}

# Creation of a directory
# TODO : should be place in a Tools.pm file
sub createDirectory {
    my $directory = shift ;
    
    if ( -d $directory) {return TRUE;} # il existe deja
    
    DEBUG (sprintf "Create the directory'%s' !", $directory);
    eval { mkpath([$directory]); };
    if ($@) {
      ERROR(sprintf "Can not create the '%s' : %s !", $directory , $@);
      return FALSE;
    }
    return TRUE;
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

sub getTopID {
    my $self = shift;
    return $self->{topID};
}

sub getBottomID {
    my $self = shift;
    return $self->{bottomID};
}


sub getTopOrder {
    my $self = shift;
    return $self->{pyramid}->getTileMatrixSet->getOrderfromID($self->{topID});
}

sub getBottomOrder {
    my $self = shift;
    return $self->{pyramid}->getTileMatrixSet->getOrderfromID($self->{bottomID});
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
#                                         DEBUG FUNCTIONS                                          #
####################################################################################################

sub exportGraph {
    my $self = shift ;
    my $src = $self->{datasource};
    
   # boucle sur tous les niveaux en partant de ceux du bas
   for (my $i = $src->getBottomOrder; $i <= $src->getTopOrder; $i++) {
       printf "Description du niveau '%s' : \n",$i;
       # boucle sur tous les noeuds du niveau
       foreach my $node ( $self->getNodesOfLevel($i)) {
         printf "\tNoeud : %s_%s ; TM Résolution : %s ; Calculé à partir de : \n",$node->getCol(),$node->getRow(),$node->getTM()->getResolution();
         foreach my $node_sup ( @{$node->getNodeSources()} ) {
             #print Dumper ($node_sup);
             printf "\t\t Noeud :%s_%s , TM Resolution : %s\n",$node_sup->getCol(),$node_sup->getRow(),$node_sup->getTM()->getResolution();
         }
       }
   }
}

1;
__END__