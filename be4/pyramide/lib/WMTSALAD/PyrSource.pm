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

Class: BE4::PyrSource



Using:
    (start code)
    use BE4::PyrSource;

    
    (end code)

Attributes:
    
=cut

################################################################################

package WMTSALAD::PropertiesLoader;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
use Data::Dumper;
#use Scalar::Util qw/reftype/;


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


=begin_nd

Constructor: new

<WMTSALAD::PyrSource's> constructor.

Using:
    (start code)
    new(path [, style] [,transparent])
    (end code)

Parameters:
    path - string - Path to the source pyramid's descriptor file
    style - string - The style to apply to source images when streaming them (optionnal)
    transparent - boolean - Another style parameter, whose name is explicit (optionnal)

Returns:
    The newly created PyrSource object. 'undef' in case of failure.
    
=cut
sub new() {
    my $this = shift;
    my $path = shift;
    my $style = shift;
    my $transparent = shift;

    my $class= ref($this) || $this;

    # IMPORTANT : if modification, think to update natural documentation (just above)
    my $self = {
        filePath => undef;
        style => undef;
        transparent => undef;
    };

    bless($self, $class);

    if (!defined $path) {
        return undef;
    }

    if (!$self->_load($path, $style, $transparent)) {
        ERROR("Could not load pyramid source.");
        return undef;
    }

    return $self;
}

=begin_nd

Function: _load

<WMTSALAD::PyrSource's> constructor's annex. Checks parameters passed to 'new', 
then load them in the new PyrSource object.

Using:
    (start code)
    _load(path [, style] [,transparent])
    (end code)

Parameters:
    path - string - Path to the source pyramid's descriptor file
    style - string - The style to apply to source images when streaming them (optionnal)
    transparent - boolean - Another style parameter, whose name is explicit (optionnal)

Returns:
    TRUE in case of success, FALSE in case of failure.
    
=cut
sub _load() {
    my $self = shift;
    my $path = shift;
    my $style = shift;
    my $transparent = shift;

    if (!exists $path || !defined $path) {
        ERROR("A pyramid descriptor's file path must be passed to load a pryamid source.");
        return FALSE;
    }

    $self->{filePath} = $path;
    if (exists $style && defined $style && $style ne '') {
        $self->{style} = $style;
    }
    if (exists $transparent && defined $transparent) {
        if (lc $transparent eq "true" || $transparent == TRUE ) {
            $self->{transparent} = "true";
        } elsif (lc $transparent eq "false" || $transparent == FALSE ) {
            $self->{transparent} = "false";
        } else {
            WARN(sprintf "Unhandled value for 'transparent' attribute : %s. Ignoring it.", $transparent);
        }
    }

    return TRUE;
}


####################################################################################################
#                                        Group: Output                                             #
####################################################################################################

