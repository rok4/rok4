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
File: PyrSource.pm

Class: WMTSALAD::PyrSource

Using:
    (start code)
    use WMTSALAD::PyrSource;

    my $pyrSource = WMTSALAD::PyrSource->new( { 
        type => "pyr",
        level => 7,
        order => 0,
        file => "/path/to/source_pyramid.pyr",
        style => "normal",
        transparent => "true",
    } );

    $pyrSource->write();    
    (end code)

Attributes:
    type - string - the type of datasource, to more easily identify it (inherited from <WMTSALAD::DataSource>)
    level - positive integer (including 0) - the level ID for this source in the tile matrix sytem (TMS) (inherited from <WMTSALAD::DataSource>
    order - positive integer (starts at 0) - the priority order for this source at this level (inherited from <WMTSALAD::DataSource>
    file - string - Path to the source pyramid's descriptor file
    style - string - The style to apply to source images when streaming them (default : normal)
    transparent - boolean - Another style parameter, whose name is explicit (default : false)

Related:
    <WMTSALAD::DataSource> - Mother class
    
=cut

################################################################################

package WMTSALAD::PyrSource;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
use Data::Dumper;

use parent qw(WMTSALAD::DataSource Exporter);

our %EXPORT_TAGS = ( 'all' => [ qw() ] );
our @EXPORT_OK   = ( @{$EXPORT_TAGS{'all'}} );
our @EXPORT      = qw();

################################################################################
# Constants
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

<WMTSALAD::PyrSource's> constructor.

Using:
    (start code)
    my $pyrSource = WMTSALAD::PyrSource->new( { 
        level => 7,
        order => 0,
        file => "/path/to/source_pyramid.pyr",
        style => "normal",
        transparent => "true",
    } );
    (end code)

Parameters:
    params - hash reference, containing the following properties :
        {
            level - positive integer (including 0) - the level ID for this source in the tile matrix sytem (TMS)
            order - positive integer (starts at 0) - the priority order for this source at this level 
            file - string - Path to the source pyramid's descriptor file
            style - string - The style to apply to source images when streaming them (default : normal)
            transparent - boolean - Another style parameter, whose name is explicit (default : false)
        }

Returns:
    The newly created PyrSource object. 'undef' in case of failure.
    
=cut
sub new() {
    my $this = shift;
    my $params = shift;

    my $class= ref($this) || $this;

    $params->{type}='pyr';

    # IMPORTANT : if modification, think to update natural documentation (just above)
    my $self = $class->SUPER::new($params);
    $self->{file} = undef;
    $self->{style} = undef;
    $self->{transparent} = undef;

    bless($self, $class);

    if (!$self->_init($params)) {
        ERROR("Could not load pyramid source.");
        return undef;
    }

    return $self;
}

=begin nd

Function: _init

<WMTSALAD::PyrSource's> initializer. Checks parameters passed to 'new', 
then load them in the new PyrSource object.

Using:
    (start code)
    my loadSucces = pyrSource->_init( { 
        level => 7,
        order => 0,
        file => "/path/to/source_pyramid.pyr",
        style => "normal",
        transparent => true,
    } )
    (end code)

Parameters:
    params - hash reference, containing the following properties :
        {
            level - positive integer (including 0) - the level ID for this source in the tile matrix sytem (TMS)
            order - positive integer (starts at 0) - the priority order for this source at this level 
            file - string - Path to the source pyramid's descriptor file
            style - string - The style to apply to source images when streaming them (default : normal)
            transparent - boolean - Another style parameter, whose name is explicit (default : false)
        }

Returns:
    TRUE in case of success, FALSE in case of failure.
    
=cut
sub _init() {
    my $self = shift;
    my $params = shift;

    return FALSE if(!$self->SUPER::_init($params));

    if (!exists $params->{file} || !defined $params->{file}) {
        ERROR("A pyramid descriptor's file file must be passed to load a pryamid source.");
        return FALSE;
    }

    $self->{file} = $params->{file};
    if (exists $params->{style} && defined $params->{style} && $params->{style} ne '') {
        $self->{style} = $params->{style};
    } else {
        $self->{style} = "normal";
    }
    if (exists $params->{transparent} && defined $params->{transparent} && $params->{transparent} ne '') {
        if ($params->{transparent} =~ m/\A(1|t|true)\z/i) {
            $self->{transparent} = "true";
        } elsif ($params->{transparent} =~ m/\A(0|f|false)\z/i) {
            $self->{transparent} = "false";
        } else {
            WARN(sprintf "Unhandled value for 'transparent' attribute : %s. Ignoring it.", $params->{transparent});
        }
    } else {
        $self->{transparent} = "false";
    }

    return TRUE;
}


####################################################################################################
#                                        Group: Output                                             #
####################################################################################################

=begin nd

Function: write

=cut
sub write() {
    return TRUE;
}


1;
