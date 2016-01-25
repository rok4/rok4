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
# Constants
use constant TRUE  => 1;
use constant FALSE => 0;
use constant FILE_FORMATS => qw(INI);

################################################################################

BEGIN {}
INIT {}
END {}


################################################################################
#                             Group: Constructors                              #
################################################################################

sub new {
	my $this = shift;
	my %parms = @_; # TODO : -filepath parameter and -format parameter
    # default format would be INI-like (but with subsections), but later on, if
    # needed, JSON might be added, or something similar.

    my $class= ref($this) || $this;

	my $self = {
        filePath => undef,
        fileFormat => undef,
        content => {},
    };

	bless($self, $class);

    my ($key, $value);

    # Read the mandatory configuration file's path parameter 
    if (defined ($value = delete $parms{'-filepath'})) {
        $self->{filePath} = $value;
    } else {
        ERROR("Cannot use COMMON::Config->new whithout a valid '-filepath' parameter.");
        return undef;
    }
    ($key, $value) = (undef, undef);

    # Check the format in which the configuration file is written
    $value = delete $parms{'-format'}
    if ((defined $value) && _isKnownFormat($value)) {
        $self->{fileFormat} = uc($value);
    } elsif ( ! defined $value) {
        INFO("No format defined for the configuration file. Switching to default INI-like format ('INI').");
        $self->{fileFormat} = "INI";
    } else {
        return undef;
    }
    ($key, $value) = (undef, undef);

    my $loaded = FALSE;
    if ($self->{fileFormat} eq "INI") {
        $loaded = _loadINI($self->{fileFormat});
    }

    if ($loaded) {
        return $self;
    } else {
        ERROR("Configuration file wasn't properly loaded.");
        return undef;
    }
}

=begin nd
Function: _loadINI

Read line by line (order is important), no library is used.

Parameters (list):
    filepath - string - INI-like configuration file path, to read

See Also:
    <readCompositionLine>
=cut
sub _loadINI {
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
            $currentSubSection = undef; # Resetting subsection as section changes
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
            if (! defined $currentSubSection) {
                if (exists $self->{content}->{$currentSection}->{$prop[0]}) {
                    ERROR (sprintf "A property is defined twice in the configuration : section %s, parameter %s", $currentSection, $prop[0]);
                    return FALSE;
                }            
                $self->{content}->{$currentSection}->{$prop[0]} = $prop[1];                
            } else {
                if (exists $self->{content}->{$currentSection}->$currentSubSection}->{$prop[0]}) {
                    ERROR (sprintf "A property is defined twice in the configuration : section %s, subsection %s parameter %s", $currentSection, $currentSubSection, $prop[0]);
                    return FALSE;
                }            
                $self->{content}->{$currentSection}->{$currentSubSection}->{$prop[0]} = $prop[1];
            }
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

=begin nd
Function: readCompositionLine

Reads a *composition* section line. Determine sources by level and calculate priorities.

Parameters (list):
    prop - string - Composition's name: levelId.bboxId
    val - string - Composition's value: pyrPath1,pyrPath2,pyrPath3
=cut
sub readCompositionLine {
    my $self = shift;
    my $prop = shift;
    my $val = shift;

    TRACE;

    my ($levelId,$bboxId) = split(/\./,$prop,-1);

    if ($levelId eq '' || $bboxId eq '') {
        ERROR (sprintf "Cannot define a level id and a bbox id (%s). Must be levelId.bboxId",$prop);
        return FALSE;
    }

    my @pyrs = split(/,/,$val,-1);

    foreach my $pyr (@pyrs) {
        if ($pyr eq '') {
            ERROR (sprintf "Invalid list of pyramids (%s). Must be /path/pyr1.pyr,/path/pyr2.pyr",$val);
            return FALSE;
        }
        if (! -f $pyr) {
            ERROR (sprintf "A referenced pyramid's file doesn't exist : %s",$pyr);
            return FALSE;
        }

        my $priority = 1;
        if (exists $self->{content}->{sourceByLevel}->{$levelId}) {
            $self->{content}->{sourceByLevel}->{$levelId} += 1;
            $priority = $self->{content}->{sourceByLevel}->{$levelId};
        } else {
            $self->{content}->{sourceByLevel}->{$levelId} = 1;
        }

        $self->{content}->{composition}->{$levelId}->{$priority} = {
            bbox => $bboxId,
            pyr => $pyr,
        };

        if (! exists $self->{content}->{sourcePyramids}->{$pyr}) {
            # we have a new source pyramid, but not yet information about
            $self->{content}->{sourcePyramids}->{$pyr} = undef;
        }

    }

    return TRUE;
}


################################################################################
#                                Group: Tester                                 #
################################################################################

=begin nd
Function: _isKnownFormat

Check configuration file's format. Possible values: 'INI'.

Parameters (list):
    format - string - format's name
=cut
sub _isKnownFormat {
    my $self = shift;
    my $format = shift;

    TRACE;

    return FALSE if (! defined $format);

    my $allowedFormats = "(";
    my $count = 0;
    foreach (FILE_FORMATS) {
        return TRUE if (uc($format) eq $_);
        $count++;
        $allowedFormats .= ($count < scalar FILE_FORMATS) ? "$_, " : "$_)";
    }

    ERROR (sprintf "COMMON::Config cannot read Configuration file format (%s)."
       ." The implemented file formats are : %s", uc($format), $allowedFormats);
    return FALSE;
}

################################################################################
#                           Group: Getters - Setters                           #
################################################################################

sub getSection {
    my $self = shift;
    my $section = shift;

    if (! defined $self->{content}->{$section}) {
        ERROR(sprintf "Section '%s' isn't defined in the configuration file %s.", $section, $self->{filePath});
        return undef;
    }

    DEBUG(sprintf "Content of section %s : %s", $section, Dumper($self->{content}->{$section}));

    return $self->{content}->{$section};
}

sub getSubSection {
    my $self = shift;
    my @address = shift;

    if (2 != scalar @address) {
        ERROR("Syntax : COMMON::Config::getSubSection(section, subsection); There must be exacly 2 arguments.");
        return undef;
    }

    my $section = $address[0];
    my $subSection = $address[1];

    if (! defined $self->{content}->{$section}) {
        ERROR(sprintf "Section '%s' isn't defined in the configuration file %s.", $section, $self->{filePath});
        return undef;
    } elsif (! defined $self->{content}->{$section}->{$subSection}) {
        ERROR(sprintf "Subsection '%s' isn't defined in section '%s' of the configuration file %s.", $subSection, $section, $self->{filePath});
        return undef;
    }

    DEBUG(sprintf "Content of section %s, subsection %s : %s", $section, $subSection, Dumper($self->{content}->{$section}->{$subSection}));

    return $self->{content}->{$section}->{$subSection};
}

sub getProperty {
    my $self = shift;
    my @address = shift;

    if ((2 != scalar @address) && (3 != scalar @address)) {
        ERROR("Syntax : COMMON::Config::getSubSection(section, [subsection,] property); There must be either 2 or 3 arguments.");
        return undef;
    }

    my $section = $address[0];
    my $subSection = undef;
    my $property = undef;

    if (2 == scalar @address) {
        $property = $address[1];
    } else {
        $subSection = $address[1];
        $property = $address[2];
    }

    if (! defined $self->{content}->{$section}) {
        ERROR(sprintf "Section '%s' isn't defined in the configuration file %s.", $section, $self->{filePath});
        return undef;
    } elsif ((defined $subSection) && (! defined $self->{content}->{$section}->{$subSection})) {
        ERROR(sprintf "Subsection '%s' isn't defined in section '%s' of the configuration file %s.", $subSection, $section, $self->{filePath});
        return undef;
    } elsif ((! defined $subSection) && (! defined $self->{content}->{$section}->{$property})) {
        ERROR(sprintf "Property '%s' isn't defined in section '%s' of the configuration file %s.", $property, $section, $self->{filePath});
        return undef;
    } elsif ((defined $subSection) && (! defined $self->{content}->{$section}->{$subSection}->{$property})) {
        ERROR(sprintf "Property '%s' isn't defined in section '%s', subsection '%s' of the configuration file %s.", $property, $section, $subSection, $self->{filePath});
        return undef;
    }

    if (2 == scalar @address) {
        DEBUG(sprintf "Value of property '%s' in section %s : %s", $property, $section, Dumper($self->{content}->{$section}->{$property}));
        return $self->{content}->{$section}->{$property};
    } else {
        DEBUG(sprintf "Value of property '%s' in section %s, subsection %s : %s", $property, $section, $subSection, Dumper($self->{content}->{$section}->{$subSection}->{$property}));
        return $self->{content}->{$section}->{$subSection}->{$property};
    }
}


1;
__END__