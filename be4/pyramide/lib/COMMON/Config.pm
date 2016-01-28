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
use Scalar::Util qw/reftype/;

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
	my $parms = shift;
    
    my $class= ref($this) || $this;

	my $self = {
        "filePath" => undef,
        "fileFormat" => undef,
        "configuration" => {},
    };

	bless($self, $class);

    DEBUG(sprintf "COMMON::Config->new called with parameters : %s", Dumper($parms));
    if (!defined $parms || ! keys %{$parms} > 0) {
        ERROR("COMMON::Config->new cannot be called without arguments.");
        return undef;
    }

    my $key = undef;
    my $value = undef;

    # Read the mandatory configuration file's path parameter 
    if (defined ($value = delete $parms->{'-filepath'})) {
        DEBUG(sprintf "Given configuration file's path : '%s'", $value);
        $self->{"filePath"} = $value;
    } else {
        ERROR("Cannot use COMMON::Config->new whithout a valid '-filepath' parameter.");
        return undef;
    }
    $key = undef;
    $value = undef;

    # Check the format in which the configuration file is written
    $value = delete $parms->{'-format'};
    DEBUG(sprintf "Given configuration file's format : '%s'", $value);
    if ( (defined $value) && ($self->_isKnownFormat($value)) ) {
        $self->{"fileFormat"} = uc($value);
    } elsif ( ! defined $value) {
        INFO("No format defined for the configuration file. Switching to default INI-like format ('INI').");
        $self->{"fileFormat"} = "INI";
    } else {
        return undef;
    }
    ($key, $value) = (undef, undef);

    my $loaded = FALSE;
    if ($self->{"fileFormat"} eq "INI") {
        $loaded = $self->_loadINI($self->{"filePath"});
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
    my $fileHandle = undef;

    unless (open($fileHandle, "<", $filepath)){
        ERROR(sprintf "Cannot open configuration's file %s.",$filepath);
        return FALSE;
    }

    my $currentSection = undef;
    my $currentSubSection = undef;

    while( defined( my $l = <$fileHandle> ) ) {
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
                if (exists $self->{"configuration"}->{$currentSection}->{$prop[0]}) {
                    ERROR (sprintf "A property is defined twice in the configuration : section %s, parameter %s", $currentSection, $prop[0]);
                    return FALSE;
                }            
                $self->{"configuration"}->{$currentSection}->{$prop[0]} = $prop[1];                
            } else {
                if (defined $self->{"configuration"}->{$currentSection}->{$currentSubSection}->{$prop[0]}) {
                    ERROR (sprintf "A property is defined twice in the configuration : section %s, subsection %s parameter %s", $currentSection, $currentSubSection, $prop[0]);
                    return FALSE;
                }            
                $self->{"configuration"}->{$currentSection}->{$currentSubSection}->{$prop[0]} = $prop[1];
            }
        } else {
            if (! $self->readCompositionLine($prop[0],$prop[1])) {
                ERROR (sprintf "Cannot read a composition line !");
                return FALSE;
            }
        } 

    }

    close $fileHandle;

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
        if (exists $self->{"configuration"}->{"sourceByLevel"}->{$levelId}) {
            $self->{"configuration"}->{"sourceByLevel"}->{$levelId} += 1;
            $priority = $self->{"configuration"}->{"sourceByLevel"}->{$levelId};
        } else {
            $self->{"configuration"}->{"sourceByLevel"}->{$levelId} = 1;
        }

        $self->{"configuration"}->{"composition"}->{$levelId}->{$priority} = {
            "bbox" => $bboxId,
            "pyr" => $pyr,
        };

        if (! exists $self->{"configuration"}->{"sourcePyramids"}->{$pyr}) {
            # we have a new source pyramid, but not yet information about
            $self->{"configuration"}->{"sourcePyramids"}->{$pyr} = undef;
        }

    }

    return TRUE;
}


################################################################################
#                                Group: Tester                                 #
################################################################################

=begin nd
Function: _isKnownFormat

Checks configuration file's format. Possible values: 'INI'.

Syntax: _isKnownFormat( format )

Parameters (list):
    format - string - format's name
=cut
sub _isKnownFormat {
    my $self = shift;
    my $format = shift;

    TRACE;

    return FALSE if (! defined $format);

    my $allowedFormats = "";
    my $count = 0;
    my @formats = FILE_FORMATS();
    foreach (@formats) {
        return TRUE if (uc($format) eq $_);
        $count++;
        $allowedFormats .= ($count < scalar @formats) ? "'$_', " : "'$_'.";
    }

    ERROR (sprintf "COMMON::Config cannot read Configuration file format '%s'."
       ." The implemented file formats are : %s", uc($format), $allowedFormats);
    return FALSE;
}


=begin_nd
Function: isSection

Checks if the given name really matches a section.
Returns a boolean answer.

Syntax: isSection ( sectionName [, errorMessage] )

Parameters (list):
    sectionName - string - hypothetical section's name
    messageType - string - log output's level (default: debug; case insensitive allowed values : debug, info, error, none)
=cut
sub isSection {
    my $self = shift;
    my $sectionName = shift;
    my $messageType = shift;

    my $message;

    if ( (! defined ($messageType)) || ($messageType eq '') ) {
        $messageType = 'debug';
    }

    if ( ! exists $self->{'configuration'}->{$sectionName} ) {
        $message = sprintf( "No section named '%s' exists in configuration file '%s'.", $sectionName, $self->{'filePath'} );
        if ( lc($messageType) eq 'error' ) {
            ERROR($message);
        } elsif ( lc($messageType) eq 'debug' ) {
            DEBUG($message);
        } elsif ( lc($messageType) eq 'info' ) {
            INFO($message);
        } elsif ( lc($messageType) eq 'none' ) {
        } else {
            DEBUG(sprintf ("Unrecognized message type : '%s'. Switching to default ('debug').", $messageType));
            DEBUG($message);
        }
        return FALSE;
    } elsif ( ! defined $self->{'configuration'}->{$sectionName} ) {
        $message = sprintf( "Item named '%s' exists in root of configuration file '%s', but is undefined.", $sectionName, $self->{'filePath'} );
        if ( lc($messageType) eq 'error' ) {
            ERROR($message);
        } elsif ( lc($messageType) eq 'debug' ) {
            DEBUG($message);
        } elsif ( lc($messageType) eq 'info' ) {
            INFO($message);
        } elsif ( lc($messageType) eq 'none' ) {
        } else {
            DEBUG(sprintf ("Unrecognized message type : '%s'. Switching to default ('debug').", $messageType));
            DEBUG($message);
        }
        return FALSE;
    } elsif ( (! defined reftype($self->{'configuration'}->{$sectionName})) || (! reftype($self->{'configuration'}->{$sectionName}) eq 'HASH') ) {
        $message = sprintf( "Item named '%s' exists in root of configuration file '%s', but is not a section.", $sectionName, $self->{'filePath'} );
        if ( lc($messageType) eq 'error' ) {
            ERROR($message);
        } elsif ( lc($messageType) eq 'debug' ) {
            DEBUG($message);
        } elsif ( lc($messageType) eq 'info' ) {
            INFO($message);
        } elsif ( lc($messageType) eq 'none' ) {
        } else {
            DEBUG(sprintf ("Unrecognized message type : '%s'. Switching to default ('debug').", $messageType));
            DEBUG($message);
        }
        return FALSE;
    }

    $message = sprintf( "Item '%s' is a section in configuration file '%s'.", $sectionName, $self->{'filePath'} );
    if ( (lc($messageType) eq 'error') || (lc($messageType) eq 'none') ) {
    } elsif ( lc($messageType) eq 'debug' ) {
        DEBUG($message);
    } elsif ( lc($messageType) eq 'info' ) {
        INFO($message);
    } else {
        DEBUG(sprintf ("Unrecognized message type : '%s'. Switching to default ('debug').", $messageType));
        DEBUG($message);
    }
    return TRUE;
}

################################################################################
#                           Group: Getters - Setters                           #
################################################################################

=begin nd
Function: getSection

Returns the hash slice contained in a specific section.

Syntax: getSection( section )

Parameters (list):
    section - string - section's name
=cut
sub getSection {
    my $self = shift;
    my $section = shift;

    if(! defined $section) {
        ERROR("Wrong argument number : syntax = getSection('section_name')");
        return undef;
    }

    if (! defined $self->{"configuration"}->{$section}) {
        ERROR(sprintf "Section '%s' isn't defined in the configuration file '%s'.", $section, $self->{"filePath"});
        return undef;
    }

    DEBUG(sprintf "Content of section '%s' : %s", $section, Dumper($self->{"configuration"}->{$section}));

    return $self->{"configuration"}->{$section};
}

=begin nd
Function: getSubSection

Returns the hash slice contained in a specific section-subsection pair.

Syntax: getSubSection( section, subsection )

Parameters (list):
    section - string - section's name
    subsection - string - subsection's name
=cut
sub getSubSection {
    my $self = shift;
    my @address = @_;

    if (2 != scalar @address) {
        ERROR("Syntax : COMMON::Config::getSubSection(section, subsection); There must be exacly 2 arguments.");
        return undef;
    }

    my $section = $address[0];
    my $subSection = $address[1];

    if (! defined $self->{"configuration"}->{$section}) {
        ERROR(sprintf "Section '%s' isn't defined in the configuration file '%s'.", $section, $self->{"filePath"});
        return undef;
    } elsif (! defined $self->{"configuration"}->{$section}->{$subSection}) {
        ERROR(sprintf "Subsection '%s' isn't defined in section '%s' of the configuration file %s.", $subSection, $section, $self->{"filePath"});
        return undef;
    } elsif (! reftype ($self->{"configuration"}->{$section}->{$subSection}) ne 'HASH') {
        ERROR(sprintf "In section '%s', an item named '%s' exists, but is not a subsection. It is probably a property.", $section, $subSection);
        return undef;
    }

    DEBUG(sprintf "Content of section '%s', subsection '%s' : %s", $section, $subSection, Dumper($self->{"configuration"}->{$section}->{$subSection}));

    return $self->{"configuration"}->{$section}->{$subSection};
}

=begin nd
Function: getProperty

Returns the value of a property in a section or a section-subsection pair.

Syntax: getProperty( section, [subsection,] property )

Parameters (list):
    section - string - section's name
    subsection - string - subsection's name (optionnal)
    property - string - property's name
=cut
sub getProperty {
    my $self = shift;
    my @address = @_;

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

    if (! defined $self->{"configuration"}->{$section}) {
        ERROR(sprintf "Section '%s' isn't defined in the configuration file '%s'.", $section, $self->{"filePath"});
        return undef;
    } elsif ((defined $subSection) && (! defined $self->{"configuration"}->{$section}->{$subSection})) {
        ERROR(sprintf "Subsection '%s' isn't defined in section '%s' of the configuration file '%s'.", $subSection, $section, $self->{"filePath"});
        return undef;
    } elsif ((defined $subSection) && (! reftype ($self->{"configuration"}->{$section}->{$subSection}) ne 'HASH')) {
        ERROR(sprintf "In section '%s', an item named '%s' exists, but is not a subsection. It is probably a property.", $section, $subSection);
        return undef;
    } elsif ((! defined $subSection) && (! defined $self->{"configuration"}->{$section}->{$property})) {
        ERROR(sprintf "Property '%s' isn't defined in section '%s' of the configuration file '%s'.", $property, $section, $self->{"filePath"});
        return undef;
    } elsif ((! defined $subSection) && (! reftype ($self->{"configuration"}->{$section}->{$property}) eq 'HASH')) {
        ERROR(sprintf "In section '%s', an item named '%s' exists, but is not a property. It is probably a subsection.", $section, $property);
        return undef;
    } elsif ((defined $subSection) && (! defined $self->{"configuration"}->{$section}->{$subSection}->{$property})) {
        ERROR(sprintf "Property '%s' isn't defined in section '%s', subsection '%s' of the configuration file '%s'.", $property, $section, $subSection, $self->{"filePath"});
        return undef;
    }

    if (2 == scalar @address) {
        DEBUG(sprintf "Value of property '%s' in section '%s' : '%s'", $property, $section, $self->{"configuration"}->{$section}->{$property});
        return $self->{"configuration"}->{$section}->{$property};
    } else {
        DEBUG(sprintf "Value of property '%s' in section '%s', subsection '%s' : '%s'", $property, $section, $subSection, $self->{"configuration"}->{$section}->{$subSection}->{$property});
        return $self->{"configuration"}->{$section}->{$subSection}->{$property};
    }
}

=begin nd
Function: getProperty

Returns the list of existing sections.

Syntax: getSections()

Parameters (list):    
=cut
sub getSections {
    my $self = shift;

    my @sections = keys $self->{"configuration"};
    return @sections;
}

=begin nd
Function: getSubSections

Returns the list of existing sub-sections in a section.

Syntax: getSubSections( section )

Parameters (list):  
    section - string - section's name  
=cut
sub getSubSections {
    my $self = shift;
    my $section = shift;

    my @subSections;
    if (! defined $self->{"configuration"}->{$section}) {
        ERROR(sprintf "Section '%s' isn't defined in the configuration file '%s'.", $section, $self->{"filePath"});
        return undef;
    } 
    foreach my $item (keys $self->{"configuration"}->{$section}) {
        if ( reftype($self->{"configuration"}->{$section}->{$item}) eq 'HASH' ) {
            push (@subSections, $item);
        }
    }

    return @subSections;
}

=begin nd
Function: getProperties

Returns the list of existing properties in a section or a subsection.

Syntax: getProperties( section, [subsection] )

Parameters (list): 
    section - string - section's name
    subsection - string - subsection's name (optionnal)   
=cut
sub getProperties {
    my $self = shift;
    my @address = @_;

    my @properties;

    if ( scalar @_ == 1 ) {
        if (! defined $self->{"configuration"}->{$address[0]}) {
            ERROR(sprintf "Section '%s' is not defined in ")
        }
    }

    return @properties;
}


1;
__END__