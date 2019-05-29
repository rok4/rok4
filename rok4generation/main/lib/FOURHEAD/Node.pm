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

################################################################################

=begin nd
File: Node.pm

Class: FOURHEAD::Node

(see ROK4GENERATION/libperlauto/FOURHEAD_Node.png)

Using:
    (start code)
    use FOURHEAD::Node

    my $tm = COMMON::TileMatrix->new(...)
    
    my $node = FOURHEAD::Node->new({
        col => 51,
        row => 756,
        level => 12
    });
    (end code)

Attributes:
    level - string - Level ID, according to the TMS grid.
    col - integer - Column, according to the TMS grid.
    row - integer - Row, according to the TMS grid.

    workImageFilename - string - 
    workMaskFilename - string - 

    ownSourceNodes - boolean - Precise if node have source nodes
    sourceNodes - <FOURHEAD::Node> array - Nodes from which this node is generated
=cut

################################################################################

package FOURHEAD::Node;

use strict;
use warnings;

use Log::Log4perl qw(:easy);

use File::Spec ;
use Data::Dumper ;
use COMMON::Base36 ;

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

####################################################################################################
#                                        Group: Constructors                                       #
####################################################################################################

=begin nd
Constructor: new

Node constructor. Bless an instance.

Parameters (hash):
    level - string - Level ID
    col - integer - Node's column
    row - integer - Node's row

See also:
    <_init>
=cut
sub new {
    my $class = shift;
    my $params = shift;
    
    $class = ref($class) || $class;
    # IMPORTANT : if modification, think to update natural documentation (just above)
    my $this = {
        col => undef,
        row => undef,
        level => undef,

        # Sources pour générer ce noeud
        ownSourceNodes => FALSE,
        sourceNodes => [undef, undef, undef, undef],

        workImageFilename => undef,
        workMaskFilename => undef
    };
    
    bless($this, $class);
    
    # mandatory parameters !
    if (! defined $params->{col}) {
        ERROR("Node's column is undef !");
        return undef;
    }
    if (! defined $params->{row}) {
        ERROR("Node's row is undef !");
        return undef;
    }
    if (! defined $params->{level}) {
        ERROR("Node's level is undef !");
        return undef;
    }
    
    # init. params    
    $this->{col} = $params->{col};
    $this->{row} = $params->{row};
    $this->{level} = $params->{level};

    $this->{workImageFilename} = sprintf "%s_%s_%s_I.tif", $this->{level}, $this->{col}, $this->{row};
    $this->{workMaskFilename} = sprintf "%s_%s_%s_M.tif", $this->{level}, $this->{col}, $this->{row};
        
    return $this;
}

####################################################################################################
#                                Group: Getters - Setters                                          #
####################################################################################################

# Function: getCol
sub getCol {
    my $this = shift;
    return $this->{col};
}

# Function: getLevel
sub getLevel {
    my $this = shift;
    return $this->{level};
}

# Function: getRow
sub getRow {
    my $this = shift;
    return $this->{row};
}

# Function: getWorkImageFilename
sub getWorkImageFilename {
    my $this = shift;
    return $this->{workImageFilename};
}

# Function: getWorkMaskFilename
sub getWorkMaskFilename {
    my $this = shift;
    return $this->{workMaskFilename};
}

# Function: addSourceNode
sub addSourceNode {
    my $this = shift;
    my $child = shift;

    my $place = ($child->getCol() % 2) + 2 * ($child->getRow() % 2);

    if (defined $this->{sourceNodes}->[$place]) {
        ERROR(sprintf "Node %s:%s,%s have already a source node for place $place", $this->{level}, $this->{col}, $this->{row});
        ERROR(sprintf "    the node %s:%s,%s", $this->{sourceNodes}->[$place]->getLevel(), $this->{sourceNodes}->[$place]->getCol(), $this->{sourceNodes}->[$place]->getRow());
        ERROR(sprintf "    and the node %s:%s,%s", $child->getLevel(), $child->getCol(), $child->getRow());
        return FALSE;
    }

    $this->{ownSourceNodes} = TRUE;

    $this->{sourceNodes}->[$place] = $child;
    return TRUE;
}

# Function: writeCode
sub writeCode {
    my $this = shift;
    my $pyramid = shift;
    my $STREAM = shift;

    if (! $this->{ownSourceNodes}) {
        # Un noeud sans noeud source est une dalle du niveau de référence, il suffit de la détuiler (avec le masque éventuel)
        printf $STREAM "PullSlab %s %s\n", 
            $pyramid->getSlabPath("IMAGE", $this->{level}, $this->{col}, $this->{row}, FALSE),
            $this->{workImageFilename};

        if ($pyramid->ownMasks()) {
            printf $STREAM "PullSlab %s %s\n", 
                $pyramid->getSlabPath("MASK", $this->{level}, $this->{col}, $this->{row}, FALSE),
                $this->{workMaskFilename};
        }

        return;
    }

    for (my $i = 0; $i < 4; $i++) {
        if (defined $this->{sourceNodes}->[$i]) {
            $this->{sourceNodes}->[$i]->writeCode($pyramid, $STREAM);
        }
    }

    # Un noeud avec des noeuds source est une dalle qui doit être générée avec un merge4tiff, et tuilée dans le stockage final (avec le masque éventuel)
    printf $STREAM "Merge4tiff %s", $this->{workImageFilename};
    if ($pyramid->ownMasks()) {
        printf $STREAM " %s", $this->{workMaskFilename};
    } else {
        print $STREAM " 0";
    }
    
    for (my $i = 0; $i < 4; $i++) {
        if (defined $this->{sourceNodes}->[$i]) {
            printf $STREAM " %s", $this->{sourceNodes}->[$i]->getWorkImageFilename();
            if ($pyramid->ownMasks()) {
                printf $STREAM " %s", $this->{sourceNodes}->[$i]->getWorkMaskFilename();
            } else {
                print $STREAM " 0";
            }
        } else {
            print $STREAM " 0 0";
        }
    }
    print $STREAM "\n";

    printf $STREAM "PushSlab %s %s \"\${WORK2CACHE_IMAGE_OPTIONS}\"\n",
        $this->{workImageFilename},
        $pyramid->getSlabPath("IMAGE", $this->{level}, $this->{col}, $this->{row}, FALSE);
        

    $pyramid->modifySlab("IMAGE", $this->{level}, $this->{col}, $this->{row});

    if ($pyramid->ownMasks()) {
        printf $STREAM "PushSlab %s %s \"\${WORK2CACHE_MASK_OPTIONS}\"\n",
            $this->{workImageFilename},
            $pyramid->getSlabPath("MASK", $this->{level}, $this->{col}, $this->{row}, FALSE);

        $pyramid->modifySlab("MASK", $this->{level}, $this->{col}, $this->{row});
    }

}

1;
__END__
