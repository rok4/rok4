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
File: SourceLevel.pm

Class: COMMON::SourceLevel

Describe a level in a pyramid.

Using:
    (start code)
    use COMMON::SourceLevel;

    my $params = {
        id => "level_5",
        dir_image => "/home/IGN/BDORTHO/IMAGE/level_5/",
        dir_mask => "/home/IGN/BDORTHO/MASK/level_5/",
        limits => [365,368,1026,1035]
    };

    my $objSourceLevel = COMMON::SourceLevel->new($params);
    (end code)

Attributes:
    id - string - Level identifiant, present in the TMS.
    dir_image - string - Absolute images' directory path for this level.
    dir_mask - string - Optionnal (if masks are present in the source pyramid). Absolute masks' directory path for this level.
    limits - integer array - Level's extrem _tiles_ : [iMin, jMin, iMax, jMax]

Limitations:

Metadata not implemented.
=cut

################################################################################

package COMMON::SourceLevel;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
use List::Util qw[min max];

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

####################################################################################################
#                                        Group: Constructors                                       #
####################################################################################################

=begin nd
Constructor: new

Level constructor. Bless an instance. Check and store level's attributes values.

Parameters (hash):
    id - string - Level identifiant
    limits - integer array - Level's extrem tiles : [colMin, rowMin, colMax, rowMax]
    dir_image - string - Absolute images' directory path for this level
    dir_mask - string - Optionnal. Absolute mask' directory path for this level
=cut
sub new {
    my $this = shift;
    my $params = shift;
    
    my $class= ref($this) || $this;
    # IMPORTANT : if modification, think to update natural documentation (just above)
    my $self = {
        id => undef,
        dir_image => undef,
        dir_mask => undef,
        limits => undef
    };
    
    bless($self, $class);
    
    TRACE;
    
    return undef if (! defined $params);

    # Mandatory parameters !
    if (! exists($params->{id})) {
        ERROR ("The parameter 'id' is required");
        return undef;
    }
    $self->{id} = $params->{id};

    if (! exists($params->{dir_image})) {
        ERROR ("The parameter 'dir_image' is required");
        return undef;
    }
    $self->{dir_image} = $params->{dir_image};

    if (! exists($params->{limits})) {
        ERROR ("The parameter 'limits' is required");
        return undef;
    }
    if (scalar @{$params->{limits}} != 4) {
        ERROR ("The parameter 'limits' have to contain 4 defined values");
        return undef;
    }
    $self->{limits} = $params->{limits};

    # parameters optional !
    if (exists $params->{dir_mask} && defined $params->{dir_mask}){
        $self->{dir_mask} = $params->{dir_mask};
    }
    
    return $self;
}


####################################################################################################
#                                  Group: BBOX tools                                               #
####################################################################################################

=begin nd
Function: intersectBboxIndices

Intersects provided indices bbox with the extrem tiles of this source level. Provided list is directly modified.

Parameters (list):
    bbox - list reference - Bounding box to intersect with the level's limits : (colMin,rowMin,colMax,rowMax).
=cut
sub intersectBboxIndices {
    my $self = shift;
    my $bbox = shift;

    $bbox->[0] = max($bbox->[0], $self->{limits}[0]);
    $bbox->[1] = max($bbox->[1], $self->{limits}[1]);
    $bbox->[2] = min($bbox->[2], $self->{limits}[2]);
    $bbox->[3] = min($bbox->[3], $self->{limits}[3]);
}

####################################################################################################
#                                Group: Getters - Setters                                          #
####################################################################################################

# Function: getID
sub getID {
    my $self = shift;
    return $self->{id};
}

# Function: getDirImage
sub getDirImage {
    my $self = shift;
    return $self->{dir_image};
}

# Function: getDirMask
sub getDirMask {
    my $self = shift;
    return $self->{dir_mask};
}

####################################################################################################
#                                Group: Export methods                                             #
####################################################################################################

=begin nd
Function: exportForDebug

Returns all source level's informations. Useful for debug.

Example:
    (start code)
    (end code)
=cut
sub exportForDebug {
    my $self = shift ;
    
    my $export = "";
    
    $export .= "\nObject COMMON::SourceLevel :\n";
    $export .= sprintf "\t ID (string) : %s\n", $self->{id};

    $export .= sprintf "\t Directories (absolute paths): \n";
    $export .= sprintf "\t\t- Images : %s\n",$self->{dir_image};
    $export .= sprintf "\t\t- Masks : %s\n",$self->{dir_mask} if (defined $self->{dir_mask});
    
    $export .= "\t Tile limits : \n";
    $export .= sprintf "\t\t- Column min : %s\n",$self->{limits}[2];
    $export .= sprintf "\t\t- Column max : %s\n",$self->{limits}[3];
    $export .= sprintf "\t\t- Row min : %s\n",$self->{limits}[0];
    $export .= sprintf "\t\t- Row max : %s\n",$self->{limits}[1];
    
    return $export;
}

1;
__END__
