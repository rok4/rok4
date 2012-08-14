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

use File::Spec ;
use BE4::Base36 ;

require Exporter;
use AutoLoader qw(AUTOLOAD);

our @ISA = qw(Exporter);

our %EXPORT_TAGS = ( 'all' => [ qw() ] );
our @EXPORT_OK   = ( @{$EXPORT_TAGS{'all'}} );
our @EXPORT      = qw();

################################################################################
# Constantes
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
    * i => undef, # i colonne dans la matrice des images
    * j => undef, # j ligne dans la matrice des images
    * tm => undef, # BE4::TileMatrix object, to which node belong
    * graph => undef, # BE4::Graph or BE4::QTree object, which contain the node
    * w => 0,     # w = own node's weight  
    * W => 0,     # W = accumulated weight (childs' weights sum)
    * code => '',     # c = commands to generate this node (to write in a script)
    * nodeSources => [], # list of BE4::Node from which this node is calculated
    * geoImages => [], # list of BE4::GeoImage from which this node is calculated
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
        i => undef,
        j => undef,
        tm => undef,
        graph => undef,
        w => 0,
        W => 0,
        code => '',
        nodeSources => [],
        geoImages => [],
    };
    
    bless($self, $class);
    
    TRACE;
    
    # init. class
    return undef if (! $self->_init(@_));
    
    return $self;
}

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
    if (! defined $params->{tm}) {
        ERROR("Node tmid is undef !");
        return FALSE;
    }
    if (! defined $params->{graph}) {
        ERROR("Tree Node is undef !");
        return FALSE;
    }
    
    # init. params    
    $self->{i} = $params->{i};
    $self->{j} = $params->{j};
    $self->{tm} = $params->{tm};
    $self->{graph} = $params->{graph};
    $self->{w} = $params->{w};
    $self->{W} = $params->{W};
    $self->{c} = $params->{c};
    
    return TRUE;
}

####################################################################################################
#                                       GETTERS / SETTERS                                          #
####################################################################################################

# Group: getters - setters

sub getCol {
    my $self = shift;
    return $self->{i};
}

sub getRow {
  my $self = shift;
  return $self->{j};
}

sub getWorkBaseName {
  my $self = shift;
  return (sprintf "%s_%s_%s", $self->getLevel, $self->{i}, $self->{j});
}

sub getWorkName {
  my $self = shift;
  return (sprintf "%s_%s_%s.tif", $self->getLevel, $self->{i}, $self->{j});
}

sub getLevel {
  my $self = shift;
  return $self->{tm}->getID;
}

sub getTM {
  my $self = shift;
  return $self->{tm};
}

sub getGraph {
  my $self = shift;
    return $self->{graph};
}

sub getNodeSources {
  my $self = shift;
  return $self->{nodeSources};
}

sub getGeoImages {
  my $self = shift;
  return $self->{geoImages};
}

sub addNodeSources {
  my $self = shift;
  my @nodes = shift;
  
  push(@{$self->getNodeSources()},@nodes);
  
  return TRUE;
}

sub addGeoImages {
  my $self = shift;
  my @images = shift;
  
  push(@{$self->getGeoImages()},@images);
  
  return TRUE;
}

sub setCode {
    my $self = shift;
    my $code = shift;
    $self->{code} = $code;
}

sub getBBox {
    my $self = shift;
    
    my @Bbox = $self->{tm}->indicesToBBox(
        $self->{i},
        $self->{j},
        $self->{graph}->getPyramid->getTilesPerWidth,
        $self->{graph}->getPyramid->getTilesPerHeight
    );
    
    return @Bbox;
}

sub getCode {
    my $self = shift;
    return $self->{code};
}

sub getOwnWeight {
  my $self = shift;
  return $self->{w};
}

sub getAccumulatedWeight {
  my $self = shift;
  return $self->{W};
}

sub updateOwnWeight {
  my $self = shift;
  my $weight = shift;
  $self->{w} += $weight;
}

# method: setAccumulatedWeightOfNode
#  Calcule le poids cumulé du noeud. Il ajoute le poids propre (déjà connu) du noeud à celui
#  passé en paramètre. Ce dernier correspond à la somme des poids cumulé des fils.
#------------------------------------------------------------------------------
sub setAccumulatedWeight {
    my $self = shift;
    my $weight = shift;
    $self->{W} = $weight + $self->getOwnWeight;
}

# to_mergentif_string
# la sortie est formatée pour pouvoir être utilisée dans le fichier de conf de mergeNtif
#---------------------------------------------------------------------------------------------------
sub to_mergentif_string {
    my $self = shift;
    my $filePath = shift;
    
    TRACE;
    
    my @Bbox = $self->getBBox;
    
    my $output = sprintf "%s\t%s\t%s\t%s\t%s\t%s\t%s\n",
        $filePath,
        $Bbox[0],
        $Bbox[1],
        $Bbox[2],
        $Bbox[3],
        $self->getTM->getResolution(),
        $self->getTM->getResolution();
    
    return $output;
}

1;
__END__