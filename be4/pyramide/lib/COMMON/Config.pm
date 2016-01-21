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
File: Config.pm

Class: COMMON::Config


Using:
    (start code)
    (end code)

Attributes:
    

Limitations:
    
=cut

################################################################################

package COMMON::Config;

use strict;
use warnings;

use vars qw($VERSION);

my $VERSION = '0.1';

use Log::Log4perl qw(:easy);
use Data::Dumper;
use List::Util qw(min max);

  # inheritance
our @ISA;
use parent 'Exporter';

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

sub new {
	my $this = shift;
	my $parms = shift; # TODO : -filepath parameter and -format parameter
    # default format would be INI-like (but with subsections), but later on, if
    # needed, JSON might be added, or something similar.

    my $class= ref($this) || $this;

	my $self = {

    };

	bless($self, $class);

    return $self;
}

=begin nd
Function: _load

Read line by line (order is important), no library is used.

Parameters (list):
    filepath - string - Configuration file path, to read

See Also:
    <isConfSection>, <readCompositionLine>
=cut
sub _load {
    my $self = shift;
    my $filepath = shift;

    if (! open CFGF, "<", $filepath ){
        ERROR(sprintf "Cannot open configurations' file %s.",$filepath);
        return FALSE;
    }

    my $currentSection = undef;
    my $currentSubSection = undef;

    while( defined( my $l = <CFGF> ) ) {
        chomp $l;
        $l =~ s/\s+//g; # we remove all spaces
        $l =~ s/;\S*//; # we remove comments

        next if ($l eq '');

        if ($l =~ m/^\[(\w*)\]$/) {
            $l =~ s/[\[\]]//g;

            $currentSection = $l;
            $currentSubSection = undef;
            next;
        }

        if ($l =~ m/^\[\[(\w*)\]\]$/) {
            $l =~ s/[\[\]]//g;

            $currentSubSection = $l;
            next;
        }

        if (! defined $currentSection) {
            ERROR (sprintf "A property must always be in a section (%s)",$l);
            return FALSE;
        }

        my @prop = split(/=/,$l,-1);
        if (scalar @prop != 2 || $prop[0] eq '' || $prop[1] eq '') {
            ERROR (sprintf "A line is invalid (%s). Must be prop = val",$l);
            return FALSE;
        }

        if ($currentSection ne 'composition') {
            if (exists $self->{$currentSection}->{$prop[0]}) {
                ERROR (sprintf "A property is defined twice in the configuration : section %s, parameter %s", $currentSection,$prop[0]);
                return FALSE;
            }
            $self->{$currentSection}->{$prop[0]} = $prop[1];
        } else {
            if (! $self->readCompositionLine($prop[0],$prop[1])) {
                ERROR (sprintf "Cannot read a composition line !");
                return FALSE;
            }
        }

    }

    close CFGF;

    return TRUE;
}


1;