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
File: DataSource.pm

Class: WMTSALAD::DataSource

Abstract class. Describes a data source and its priority at a given level of the output tile matrix set.

Using:
    (start code)
    use parent qw(WMTSALAD::DataSource);

    new {
        ...
        $class->SUPER->new();
        ....
    }

    _init {
        ...
        $self->SUPER->_init( {type => "WMS", level => 7, order => 0});
        ...
    }    
    (end code)

Attributes:
    type - string - the type of datasource, to more easily identify it
    level - string - the level ID for this source in the tile matrix sytem (TMS)
    order - positive integer (starts at 0) - the priority order for this source at this level

Related:
    <WMTSALAD:WmsSource>, <WMTSALAD::PyrSource>
    
=cut

################################################################################

package WMTSALAD::DataSource;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
use Data::Dumper;
use COMMON::CheckUtils;

require Exporter;

our @ISA = qw(Exporter);

our %EXPORT_TAGS = ( 'all' => [ qw() ] );
our @EXPORT_OK   = ( @{$EXPORT_TAGS{'all'}} );
our @EXPORT      = qw();

################################################################################
# Constants
use constant TRUE  => 1;
use constant FALSE => 0;

# Constant: SRC_TYPE
# Define allowed values for attribute "type".
my @SRC_TYPE = ("WMS", "pyr");

################################################################################

BEGIN {}
INIT {
}
END {}

####################################################################################################
#                                        Group: Constructors                                       #
####################################################################################################


=begin nd

Constructor: new

<WMTSALAD::DataSource's> constructor.

Using:
    (start code)
    my $dataSource = WMTSALAD::DataSource->new();
    (end code)

Returns:
    The newly created DataSource object, with undefined parameters.
    
=cut
sub new() {
    my $this = shift;
    my $params = shift;

    my $class= ref($this) || $this;

    # IMPORTANT : if modification, think to update natural documentation (just above)
    my $self = {
        type => undef,
        level => undef,
        order => undef,
    };

    bless($self, $class);

    return $self;
}

=begin nd

Function: _init

<WMTSALAD::DataSource's> initializer.

Using:
    (start code)
    my $initSuccess = $dataSource->_init( { 
        type => "WMS",
        level => 7,
        order => 0,
    } )
    (end code)

Parameters:
    params - hash reference, containing the following properties :
        {
            type - string - the type of datasource, to more easily identify it
            level - string - the level ID for this source in the tile matrix sytem (TMS)
            order - positive integer (starts at 0) - the priority order for this source at this level          
        }

Returns:
    1 (TRUE) in case of success, 0 (FALSE) in case of failure.
    
=cut
sub _init() {
    my $self = shift;
    my $params = shift;

    # Tests type
    if (!exists $params->{type} || !defined $params->{type}) {
        ERROR("Data source's type is undefined.");
        return FALSE;
    } elsif (!$self->isKnownType($params->{type})) {
        ERROR(sprintf "Unrecognized data source's type : %s.", $params->{type});
        return FALSE;
    }
    # Tests level
    if (!exists $params->{level} || !defined $params->{level}) {
        ERROR("Data source's level is undefined.");
        return FALSE;
    }
    # Tests order
    if (!exists $params->{order} || !defined $params->{order}) {
        ERROR("Data source's priority order is undefined.");
        return FALSE;
    } elsif (!COMMON::CheckUtils::isPositiveInt($params->{order})) {
        ERROR(sprintf "Data source's priority order isn't a positive integer: %s.", $params->{order});
        return FALSE;
    }

    # Initializes
    $self->{type} = $params->{type};
    $self->{level} = $params->{level};
    $self->{order} = $params->{order};

    return TRUE;
}


####################################################################################################
#                                        Group: Tests                                              #
####################################################################################################

=begin nd

Function: isKnownType

Tests if the given type is recognized.

Using:
    (start code)
    my $answer = $dataSource->isKnownType( $type );
    (end code)

Parameter:
    type - string - the type to test

Returns:
    1 (TRUE) if the type is recognized, 0 (FALSE) if not.

=cut
sub isKnownType {
    my $self = shift;
    my $type = shift;

    return FALSE if (!defined $type);

    foreach (@SRC_TYPE) {
        return TRUE if ((lc $type) eq (lc $_));
    }

    return FALSE;
}



####################################################################################################
#                                        Group: Output                                             #
####################################################################################################

=begin nd

Function: writeInXml

Abstract function. See child classes for implemented counteparts.

=cut
sub writeInXml() {
    1;
}



1;
