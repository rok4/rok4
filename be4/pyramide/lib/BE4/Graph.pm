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
|   nX = BE4::Node

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
    objForest - BE4::Forest in which this graph is.
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
method: _init

Check DataSource, Pyramid and Commands parameters.

Parameters :
    objForest - a BE4::DataForest object
    objSrc - a BE4::DataSource object
    objPyr - a BE4::Pyramid object
    ObjCommands - a BE4::Commands object
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

#
=begin nd
method: _load

Determine all nodes from the bottom level to the top level, thanks to the data source.

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

        # si un niveau est vide on a une erreur
        if (scalar(@{$self->getNodesOfLevel($levelID)}) == 0) {
            ERROR (sprintf "The level %s has no nodes. Invalid use of TMS for nearest neighbour interpolation.",$levelID);
            return FALSE;
        }
        
        # on n'a plus rien à calculer, on sort
        last if ($k == $src->getTopOrder );
        
        foreach my $node ( $self->getNodesOfLevel($levelID) ) {
            
            # On récupère la BBOX du noeud pour calculer les noeuds cibles
            my ($xMin,$yMax,$xMax,$yMin) = $node->getBBox();
            
            foreach my $targetTmID (@targetLevelsID) {
                next if ($tms->getOrderfromID($targetTmID) > $src->getTopOrder());
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
                        ## intersection avec la bbox des données initiales
                        if ( $newnode->isBboxIntersectingNodeBbox($self->getBbox())) {
                          $self->{nodes}->{$targetTmID}->{$idxkey} = $newnode ;
                          $newnode->addNodeSources($node); 
                        }
                      } else {
                        $newnode = $self->{nodes}->{$targetTmID}->{$idxkey};
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
#                                          GRAPH COMPUTING METHODS                                 #
####################################################################################################

# Group: graph computing methods

#
=begin nd
method: computeYourself

Determine codes and weights for each node of the current graph, and share work on scripts, so as to optimize execution time.

Only one step:
    - browse graph and write commands in different scripts.

Parameter:
    NEWLIST - stream to the cache's list, to add new images.
=cut
sub computeYourself {
    my $self = shift;
    my $NEWLIST = shift;
    
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
           my $script_index = BE4::Array->minArrayIndex(0,@WeightsOfLevel);
           my $script = $ScriptsOfLevel[$script_index];
           # on stocke l'information dans l'objet node
           $node->setScript($script);
           # on détermine le script à ecrire
           my ($c,$w) ;
           if ($self->getDataSource->hasHarvesting) {
               # Datasource has a WMS service : we have to use it
               ($c,$w) = $self->{commands}->wms2work($node,$self->getDataSource->getHarvesting,FALSE);
               if (! defined $c) {
                   ERROR(sprintf "Cannot harvest image for node %s",$node->getWorkBaseName);
                   return FALSE;
               }
           } else {
               ($c,$w) = $self->{commands}->mergeNtiff($node);
               if ($w == -1) {
                   ERROR(sprintf "Cannot compose mergeNtiff command for the node %s.",$node->getWorkBaseName);
                   return FALSE;
               }
           }
           # on met à jour les poids
           $script->addWeight($w);
           # on ecrit la commande dans le fichier
           $script->print($c);       
                   
           # final script with all work2tile commands
           # on ecrit dans chacun des scripts de manière tournante
           my $finisher = $self->getForest()->getScript($Finisher_Index);
           ($c,$w) = $self->{commands}->work2cache($node,"\${ROOT_TMP_DIR}/".$node->getScript()->getID(),1);
           # on ecrit la commande dans le fichier
           $finisher->print($c);
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

#
=begin nd
method: containsNode

Parameters:
    level - level of the node we want to know if it is in the graph.
    x     - x coordinate of the node we want to know if it is in the graph.
    y     - y coordinate of the node we want to know if it is in the graph.

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

####################################################################################################
#                               GEOGRAPHIC TOOLS                                                   #
####################################################################################################

# Group: GEOGRAPHIC TOOLS

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
#                                         GETTERS / SETTERS                                        #
####################################################################################################

# Group: getters - setters

sub getPyramid{
    my $self = shift;
    return $self->{pyramid};
}

sub getCommands{
    my $self = shift;
    return $self->{commands};
}

sub getForest{
    my $self = shift;
    return $self->{forest};
}

sub getDataSource{
    my $self = shift;
    return $self->{datasource};
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

sub getNodesOfBottomLevel {
    my $self = shift;
    return $self->getNodesOfLevel($self->{bottomID});
}

sub getBbox {
    my $self =shift;
    return ($self->{bbox}[0],$self->{bbox}[1],$self->{bbox}[2],$self->{bbox}[3]);
}

#
=begin_nd
method: getScriptsOfLevel

Return the scripts for a given Level.

Parameters:
    - level : levelID 
    
Returns:
    An array of BE4::Script
=cut
sub getScriptsOfLevel {
    my $self = shift;
    my $levelID = shift;
    my $order =  $self->getPyramid()->getTileMatrixSet()->getOrderfromID($levelID);
    
    my $numberOfScriptByLevel = $self->getForest()->getSplitNumber();
    my $numberOfFinisher = $self->getForest()->getSplitNumber();
    
    my $start_index = $numberOfFinisher + ($order - $self->getBottomOrder) * $numberOfScriptByLevel ;
    my $end_index = $start_index + $numberOfScriptByLevel - 1;

    return @{$self->getForest()->getScripts()}[$start_index .. $end_index];
};


####################################################################################################
#                                         EXPORT METHODS                                           #
####################################################################################################

# Group : EXPORT METHODS

#
=begin nd
method: exportForDebug

Export in a string the content of the graph object

=cut
sub exportForDebug {
    my $self = shift ;
    my $src = $self->{datasource};
    
    my $output = "";
    
   # boucle sur tous les niveaux en partant de ceux du bas
   for (my $i = $src->getBottomOrder; $i <= $src->getTopOrder; $i++) {
       $output .= sprintf "Description du niveau '%s' : \n",$i;
       # boucle sur tous les noeuds du niveau
       foreach my $node ( $self->getNodesOfLevel($i)) {
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

=head1 NAME

BE4::Graph - representation of the cache as a graph : cache image = node

=head1 SYNOPSIS

    use BE4::Graph;
    
    my $job_number = 4; # 4 split scripts by level + 4 finisher = 4*(number_of_level +1) scripts
  
    # Graph object creation
    my $objGraph = BE4::Graph->new($objDataSource, $objPyramid, $job_number);
    

=head1 DESCRIPTION

=head2 ATTRIBUTES

=over 4

=item pyramid

A Pyramid object.

=item commands

A Commands object.

=item datasource

A Datasource object.

=item bbox

Array [xmin,ymin,xmax,ymax], bbox of datasource in the TMS' SRS.

=item nodes

An hash, composition of each node in the tree (code to generate the node, own weight, accumulated weight):

    {
        levelID => { x1_y2 => objNode, ...}
        ...
    }
    
    with objNode = a BE4::Node object

=item bottomID, topID

Extrem levels identifiants of the graph.

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

=head1 AUTHORS

Chevereau Simon, E<lt>simon.chevereaun@ign.frE<gt>
Satabin Théo, E<lt>theo.satabin@ign.frE<gt>

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2011 by Satabin Théo

This library is free software; you can redistribute it and/or modify it under the same terms as Perl itself, either Perl version 5.10.1 or, at your option, any later version of Perl 5 you may have available.

=cut