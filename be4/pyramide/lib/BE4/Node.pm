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
use Data::Dumper ;
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
    * i - colonne dans la matrice des images
    * j - ligne dans la matrice des images
    * tm : BE4::TileMatrix - to which node belong
    * graph : BE4::Graph or BE4::QTree - which contain the node
    * w - own node's weight  
    * W - accumulated weight (childs' weights sum)
    * code - commands to execute to generate this node (to write in a script)
    * script - BE4::Script, in which the node is calculated
    * nodeSources : list of BE4::Node - from which this node is calculated
    * geoImages : list of BE4::GeoImage - from which this node is calculated
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
        script => undef,
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
    $self->{w} = 0;
    $self->{W} = 0;
    $self->{code} = '';
    
    return TRUE;
}

####################################################################################################
#                                       GETTERS / SETTERS                                          #
####################################################################################################

# Group: getters - setters

sub getScript {
    my $self = shift;
    return $self->{script}
}

sub writeInScript {
    my $self = shift;
    $self->{script}->print($self->{code});
}

sub setScript {
    my $self = shift;
    my $script = shift;
    
    if (! defined $script || ref ($script) ne "BE4::Script") {
        ERROR("We expect to a BE4::Script object.");
    }
    
    $self->{script} = $script; 
}

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
    
    #ALWAYS(sprintf "BBOX : %s",Dumper(@Bbox)); #TEST#
    
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

sub setOwnWeight {
    my $self = shift;
    my $weight = shift;
    $self->{w} = $weight;
}

sub getScriptID {
    my $self = shift;
    return $self->{script}->getID;
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

####################################################################################################
#                                          EXPORT METHODS                                          #
####################################################################################################

# Group: export methods

# method: exportForMntConf
# la sortie est formatée pour pouvoir être utilisée dans le fichier de conf de mergeNtif
sub exportForMntConf {
    my $self = shift;
    my $filePath = shift;
    
    TRACE;
    
    my @Bbox = $self->getBBox;
    
    my $output = sprintf "%s\t%s\t%s\t%s\t%s\t%s\t%s\n",
        $filePath,
        $Bbox[0],
        $Bbox[3],
        $Bbox[2],
        $Bbox[1],
        $self->getTM()->getResolution(),
        $self->getTM()->getResolution();
    
    return $output;
}

sub exportForDebug {
    my $self = shift ;
    
    print "Objet Node :\n";
    print "\tLevel : ".$self->getLevel()."\n";
    print "\tTM Resolution : ".$self->getTM()->getResolution()."\n";
    print "\tColonne : ".$self->getCol()."\n";
    print "\tLigne : ".$self->getRow()."\n";
    if (defined $self->getScript()) {
        print "\tScript ID : ".$self->getScriptID()."\n";
    } else {
        print "\tScript undefined.\n";
    }
    printf "\t Noeud Source :\n";
    foreach my $node_sup ( @{$self->getNodeSources()} ) {
        printf "\t\tResolution : %s, Colonne ; %s, Ligne : %s\n",$node_sup->getTM()->getResolution(),$node_sup->getCol(),$node_sup->getRow();
    }
    printf "\t Geoimage Source :\n";
    
    foreach my $img ( @{$self->getGeoImages()} ) {
        printf "\t\tNom : %s\n",$img->getName();
    }
}

1;
__END__