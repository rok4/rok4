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

package BE4::Node;

use strict;
use warnings;

use Log::Log4perl qw(:easy);

require Exporter;
use AutoLoader qw(AUTOLOAD);

our @ISA = qw(Exporter);

our %EXPORT_TAGS = ( 'all' => [ qw() ] );
our @EXPORT_OK   = ( @{$EXPORT_TAGS{'all'}} );
our @EXPORT      = qw();

################################################################################
# version
our $VERSION = '0.0.1';

################################################################################
# constantes
use constant TRUE  => 1;
use constant FALSE => 0;

################################################################################
# Preloaded methods go here.
BEGIN {}
INIT {}
END {}


################################################################################
# constructor
sub new {
  my $this = shift;
  
  ## simon
  my $class= ref($this) || $this;
  my $self = {
    i => undef, # i colonne dans la matrice des images
    j => undef, # j ligne dans la matrice des images
    tmObj => undef, # the tm linked to the node
    graphObj => undef, # the graph linked to the node (graph can be a graph obj or a qtree obj)
    filePath => undef, # filePath for the image/node
    w => undef,     # w = own node's weight  
    W => undef,     # W = accumulated weight (childs' weights sum)
    c => undef,     # c = commands to generate this node (to write in a script)
    nodeSources => [], # list of nodes from which this node is calculated
    imageSources => [], # list of ImageSource from which this node is calculated
  };

  bless($self, $class);
  
  TRACE;
  
  # init. class
  return undef if (! $self->_init(@_));
  # load 
  #return undef if (! $self->_load());
  
  # DEBUG(Dumper($self));
  
  return $self;
}
# method: _init.
#  Define the number of level
#  get source images.
#-------------------------------------------------------------------------------
sub _init {
  
  my $self = shift;
  my $params = shift ; # Hash
  
  TRACE;
  
  # mandatory parameters !
  if (! defined $params->{i}) {
    ERROR("Node Coord i is undef !");
    return FALSE;
  }
  if (! defined $params->{j}) {
    ERROR("Node Coord j is undef !");
    return FALSE;
  }
  if (! defined $params->{tmObj}) {
    ERROR("Node tmid is undef !");
    return FALSE;
  }
  if (! defined $params->{filePath}) {
    ERROR("Node filePath is undef !");
    return FALSE;
  }
  if (! defined $params->{graphObj}) {
    ERROR("Tree Node is undef !");
    return FALSE;
  }
  
  # init. params    
  $self->{i} = $params->{i};
  $self->{j} = $params->{j};
  $self->{tmObj} = $params->{tmObj};
  $self->{graphObj} = $params->{graphObj};
  $self->{filePath} = $params->{filePath};
  $self->{w} = $params->{w};
  $self->{W} = $params->{W};
  $self->{c} = $params->{c};

  return TRUE;
}


sub getBbox {
  my $self = shift;

  TRACE;
  
  # Format of bbox : (Upper Left, Lower Right)   
  my $tilePerWidth = $self->getGraphObj()->getPyramid()->getTilePerWidth();
  my $tilePerHeight = $self->getGraphObj()->getPyramid()->getTilePerHeight();
  my ($xmin,$ymax) = $self->getTmObj()->computeLandCoordFromImageCoord($self->getCoordImageI(),$self->getCoordImageJ(),$tilePerWidth,$tilePerHeight);
  my ($xmax,$ymin) = $self->getTmObj()->computeLandCoordFromImageCoord($self->getCoordImageI() + 1,$self->getCoordImageJ() + 1,$tilePerWidth,$tilePerHeight);
   
  return ($xmin,$ymax,$ymin,$xmax);
}
sub copy {
  my $this = shift;
  my $filePath = shift;

  my $class= ref($this) || $this;
  my $self = {
    filePath => $filePath,
    i => $this->{i},
    j => $this->{j},
    graphObj => $this->{graphObj},
    tmObj => $this->{tmObj},
    nodeSources => @{$this->{nodeSources}},
    imageSources => @{$this->{imageSources}},
    w => $this->{w},
    W => $this->{W},
    c => $this->{c},
  };

  bless($self, $class);
  
  TRACE;
  
  return $self;
}
sub getCoordImageI {
  my $self = shift;
  return $self->{i};
}
sub getCoordImageJ {
  my $self = shift;
  return $self->{j};
}
sub getFilePath {
  my $self = shift;
  return $self->{filePath};
}
sub getTmObj {
  my $self = shift;
  return $self->{tmObj};
}
sub getGraphObj {
  my $self = shift;
    return $self->{graphObj};
}
sub getNodeSources {
  my $self = shift;
  return @{$self->{nodeSources}};
}
sub getImageSources {
  my $self = shift;
  return @{$self->{imageSources}};
}
sub setNodeSources {
  my $self = shift;
  my @nodes = shift;
  push(@{$self->getNodeSources()},@nodes);
  return TRUE;
}
sub setImageSources {
  my $self = shift;
  my @images = shift;
  push(@{$self->getImageSources()},@images);
  return TRUE;
}
sub setComputingCode(){
    my $self = shift;
    my $code = shift;
    $self->{c} = $code;
}
sub getComputingCode(){
    my $self = shift;
    return $self->{c};
}
sub getWeightOfNode(){
  my $self = shift;
  return $self->{w};
}
sub getAccumulatedWeightOfNode(){
  my $self = shift;
  return $self->{W};
}
sub updateWeightOfNode(){
  my $self = shift;
  my $weight = shift;
  $self->{w} += $weight;
}
# method: setAccumulatedWeightOfNode
#  Calcule le poids cumulé du noeud. Il ajoute le poids propre (déjà connu) du noeud à celui
#  passé en paramètre. Ce dernier correspond à la somme des poids cumulé des fils.
#------------------------------------------------------------------------------
sub setAccumulatedWeightOfNode(){
    my $self = shift;
    my $weight = shift;
    $self->{W} = $weight + $self->getWeightOfNode();
}

# to_string methode
# la sortie est formatée pour pouvoir être utilisée dans le fichier de conf de mergeNtif
#---------------------------------------------------------------------------------------------------
sub to_string {
  my $self = shift;
  
  TRACE;
  my @Bbox = $self->getBbox();
  
  my $output = sprintf "%s\t%s\t%s\t%s\t%s\t%s\t%s\n",
          $self->{filePath},
          $Bbox[0],
          $Bbox[1],
          $Bbox[2],
          $Bbox[3],
          $self->getTmObj()->getResolution(),
          $self->getTmObj()->getResolution();
  #DEBUG ($output);
  return $output;

}
1;
__END__
