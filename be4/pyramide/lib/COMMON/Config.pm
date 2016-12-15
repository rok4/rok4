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
    # load properties 
    my $cfg = COMMON::Config->new({
        'filepath' => "/mon/fichier/de/configuration.txt",
        'format' => "INI"
    });
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
    
    my $class = ref($this) || $this;

	my $self = {
        "filePath" => undef,
        "fileFormat" => undef,
        "configuration" => {},
        "rawConfiguration" => {},
    };

	bless($self, $class);

    DEBUG(sprintf "COMMON::Config->new called with parameters : %s", Dumper($parms));
    if (! defined $parms || ! keys %{$parms} > 0) {
        ERROR("COMMON::Config->new cannot be called without arguments.");
        return undef;
    }

    my $key = undef;
    my $value = undef;

    # Read the mandatory configuration file's path parameter 
    if (defined ($value = delete $parms->{'filepath'})) {
        DEBUG(sprintf "Given configuration file's path : '%s'", $value);
        $self->{"filePath"} = $value;
    } else {
        ERROR("Cannot use COMMON::Config->new whithout a valid 'filepath' parameter.");
        return undef;
    }
    $key = undef;
    $value = undef;

    # Check the format in which the configuration file is written
    $value = delete $parms->{'format'};
    if ( (defined $value) && ($self->_isKnownFormat($value)) ) {
        DEBUG(sprintf "Given configuration file's format : '%s'", $value);
        $self->{"fileFormat"} = uc($value);
    } elsif ( ! defined $value) {
        INFO("No format defined for the configuration file. Switching to default INI-like format ('INI').");
        $self->{"fileFormat"} = "INI";
    } else {
        return undef;
    }

    if ($self->{"fileFormat"} eq "INI") {
        if (! $self->_loadINI() ) {
            ERROR("Configuration file wasn't properly loaded.");
            return undef;
        }
    }

    return $self;
}

=begin nd
Function: _loadINI

Read line by line (order is important), no library is used.

=cut
sub _loadINI {
    my $self = shift;


    my $filepath = $self->{"filePath"};
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

        if ($l =~ m/^\[([\w-]*)\]$/) {
            $l =~ s/[\[\]]//g;

            if (exists $self->{"configuration"}->{$l}) {
                ERROR (sprintf "A section is defined twice in the configuration : section '%s'", $l);
                return FALSE;
            }
            $currentSection = $l;
            $currentSubSection = undef; # Resetting subsection as section changes
            $self->{"configuration"}->{$currentSection}->{'_props'} = []; # Array of properties name, to index their order
            next;
        }

        if ($l =~ m/^\[\[([\w-]*)\]\]$/) {
            $l =~ s/[\[\]]//g;

            if (exists $self->{"configuration"}->{$currentSection}->{$l}) {
                ERROR (sprintf "A subsection is defined twice in the configuration : section '%s', subsection '%s'", $currentSection, $l);
                return FALSE;
            }
            $currentSubSection = $l;            
            $self->{"configuration"}->{$currentSection}->{$currentSubSection}->{'_props'} = []; # Array of properties name, to index their order
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

        if (! defined $currentSubSection) {
            if (exists $self->{"configuration"}->{$currentSection}->{$prop[0]}) {
                ERROR (sprintf "A property is defined twice in the configuration : section %s, parameter %s", $currentSection, $prop[0]);
                return FALSE;
            }            
            $self->{"configuration"}->{$currentSection}->{$prop[0]} = $prop[1];
            push (@{$self->{"configuration"}->{$currentSection}->{'_props'}}, $prop[0]);

            $self->{"rawConfiguration"}->{$currentSection}->{$prop[0]} = $prop[1];
        } else {
            if (defined $self->{"configuration"}->{$currentSection}->{$currentSubSection}->{$prop[0]}) {
                ERROR (sprintf "A property is defined twice in the configuration : section %s, subsection %s parameter %s", $currentSection, $currentSubSection, $prop[0]);
                return FALSE;
            }
            $self->{"configuration"}->{$currentSection}->{$currentSubSection}->{$prop[0]} = $prop[1];
            push (@{$self->{"configuration"}->{$currentSection}->{$currentSubSection}->{'_props'}}, $prop[0]);
            
            $self->{"rawConfiguration"}->{$currentSection}->{$currentSubSection}->{$prop[0]} = $prop[1];
        }
        

    }

    close $fileHandle;

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

Syntax: isSection ( sectionName )

Parameters (list):
    sectionName - string - hypothetical section's name

Returns:
    0 if sectionName is nothing (or sectionName is undefined), 1 if sectionName is a properties, 2 if sectionName is a section
=cut
sub isSection {
    my $self = shift;
    my $sectionName = shift;

    if (! defined $sectionName) {
        ERROR("No section name provided");
        return 0;
    }

    if (! exists $self->{'configuration'}->{$sectionName}) {
        return 0;
    }

    if (! defined reftype($self->{'configuration'}->{$sectionName}) || reftype($self->{'configuration'}->{$sectionName}) ne 'HASH') {
        return 1;
    }

    return 2;
}

=begin_nd
Function: isSubSection

Checks if the given name really matches a subsection.

Syntax: isSubSection ( sectionName, subSectionName )

Parameters (list):
    sectionName - string - hypothetical section's name
    subSectionName - string - hypothetical subsection's name

Returns:
    0 if subSectionName is nothing (or subSectionName is undefined or sectionName is undefined), 1 if subSectionName is a properties, 2 if subSectionName is a sub section
=cut
sub isSubSection {
    my $self = shift;
    my $sectionName = shift;
    my $subSectionName = shift;

    if (! defined $sectionName) {
        ERROR("No section name provided");
        return 0;
    }

    if (! defined $subSectionName) {
        ERROR("No subsection name provided");
        return 0;
    }

    if ($self->isSection($sectionName) != 2) {
        return 0;
    }

    if (! exists $self->{'configuration'}->{$sectionName}->{$subSectionName}) {
        return 0;
    }

    if (
        ! defined reftype($self->{'configuration'}->{$sectionName}->{$subSectionName}) || 
        reftype($self->{'configuration'}->{$sectionName}->{$subSectionName}) ne 'HASH'
        ) {
        return 1;
    }

    return 2;
}

=begin_nd
Function: isProperty

Checks if the given name really matches a properties.

Syntax: isProperty ({ 'section' => sectionName, 'property' => propertyName [, 'subsection' => subSectionName] })

Parameters (hash):
    'section' => sectionName - string - section's name
    'subsection' => subSectionName - string - subsection's name (optionnal)
    'property' => propertyName - string - the property to check

Returns:
    TRUE if properties exists and is defined, FALSE otherwise
=cut
sub isProperty {
    my $self = shift;
    my $parms = shift;

    if (! exists $parms->{section} || ! defined $parms->{section}) {
        ERROR("No section name provided");
        return FALSE;
    }
    my $sec = $parms->{section};
    
    if (! exists $parms->{property} || ! defined $parms->{property}) {
        ERROR("No property name provided");
        return FALSE;
    }
    my $prop = $parms->{property};

    if (exists $parms->{subsection} && defined $parms->{subsection}) {
        my $subsec = $parms->{subsection};
        return (exists $self->{'configuration'}->{$sec}->{$subsec}->{$prop} && defined $self->{'configuration'}->{$sec}->{$subsec}->{$prop});
    } else {
        return (exists $self->{'configuration'}->{$sec}->{$prop} && defined $self->{'configuration'}->{$sec}->{$prop});
    }
}

################################################################################
#                           Group: Getters - Setters                           #
################################################################################

=begin nd
Function: getSection

Returns a copy of the hash slice contained in a specific section.

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

    if ($self->isSection($section) != 2) {
        ERROR("'$section' is not a section");
        return undef;
    }

    my $refSectionHash = $self->{"configuration"}->{$section};
    my %sectionHash = %{$refSectionHash};

    return %sectionHash;
}


=begin nd
Function: getSubSection

Returns a copy of the hash slice contained in a specific section-subsection pair.

Syntax: getSubSection( section, subSection )

Parameters (list):
    section - string - section's name
    subSection - string - subsection's name
=cut
sub getSubSection {
    my $self = shift;
    my $section = shift;
    my $subSection = shift;

    if(! defined $section) {
        ERROR("section is not defined");
        return undef;
    }

    if(! defined $subSection) {
        ERROR("subSection is not defined");
        return undef;
    }

    if ($self->isSubSection($section, $subSection) != 2) {
        ERROR("'$section.$subSection' is not a subsection");
        return undef;
    }

    my $refSubSectionHash = $self->{"configuration"}->{$section}->{$subSection};
    my %subSectionHash = %{$refSubSectionHash};

    return %subSectionHash;
}


=begin nd
Function: getProperty

Returns the value of a property in a section or a section-subsection pair.

Syntax: getProperty({ 'section' => sectionName, 'property' => propertyName [, 'subsection' => subSectionName] })

Parameters (hash):
    'section' => sectionName - string - section's name
    'subsection' => subSectionName - string - subsection's name (optionnal)
    'property' => propertyName - string - the property to get
=cut
sub getProperty {
    my $self = shift;
    my $parms = shift;

    if (! $self->isProperty($parms)) {
        return undef;
    }

    my $sec = $parms->{section};
    my $prop = $parms->{property};

    if (exists $parms->{subsection} && defined $parms->{subsection}) {
        my $subsec = $parms->{subsection};
        return $self->{"configuration"}->{$sec}->{$subsec}->{$prop};
    } else {
        return $self->{"configuration"}->{$sec}->{$prop};
    }
}

=begin nd
Function: setProperty

Store the value of a property in a section or a section-subsection pair.

Syntax: setProperty({ 'section' => sectionName, 'property' => propertyName [, 'subsection' => subSectionName], 'value' = value })

Parameters (hash):
    'section' => sectionName - string - section's name
    'subsection' => subSectionName - string - subsection's name (optionnal)
    'property' => propertyName - string - the property to set
    'value' => propertyName - string - the value to set
=cut
sub setProperty {
    my $self = shift;
    my $parms = shift;

    my $sec = $parms->{section};
    my $prop = $parms->{property};
    my $val = $parms->{value};

    if (exists $parms->{subsection} && defined $parms->{subsection}) {
        my $subsec = $parms->{subsection};
        $self->{"configuration"}->{$sec}->{$subsec}->{$prop} = $val;
        $self->{"rawConfiguration"}->{$sec}->{$subsec}->{$prop} = $val;
    } else {
        $self->{"configuration"}->{$sec}->{$prop} = $val;
        $self->{"rawConfiguration"}->{$sec}->{$prop} = $val;
    }
}

=begin nd
Function: getProperty

Returns the list of existing sections.

Syntax: getSections()
  
=cut
sub getSections {
    my $self = shift;

    my @sections;
    foreach my $item (keys %{$self->{"configuration"}}) {
        if ( $self->isSection($item) == 2 ) {
            push (@sections, $item);
        }
    }
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

    if ($self->isSection($section) != 2) {
        ERROR("'$section' is not a section, cannot obtain list of subsections in");
        return undef;
    } 

    my @subSections;
    foreach my $item (keys %{$self->{"configuration"}->{$section}}) {
        if ( $self->isSubSection($section, $item) == 2 ) {
            push (@subSections, $item);
        }
    }

    return @subSections;
}

=begin nd
Function: getProperties

Returns the list of existing properties in a section or a subsection.
The properties listing order is the same than in the original configuration file.

Syntax: getProperties( section, [subsection] )

Parameters (list): 
    section - string - section's name
    subsection - string - subsection's name (optionnal)   
=cut
sub getProperties {
    my $self = shift;
    my $section = shift;
    my $subsection = shift;

    if (defined $subsection) {
        if ($self->isSubSection($section, $subsection) != 2) {
            ERROR("'$section.$subsection' is not a subsection, cannot obtain list of properties in");
            return undef;
        }
        return @{$self->{"configuration"}->{$section}->{$subsection}->{'_props'}};
    } else {
        if ($self->isSection($section) != 2) {
            ERROR("'$section' is not a section, cannot obtain list of properties in");
            return undef;
        }
        return @{$self->{"configuration"}->{$section}->{'_props'}};
    }
}

=begin nd
Function: getConfig

Returns a hash copy of the the part of the COMMON::Config object that actually contains the configuration.

Syntax: getConfig()
 
=cut
sub getConfig {
    my $self = shift;

    my $refConfig = $self->{'configuration'};
    my %hashConfig = %{$refConfig};
    return %hashConfig;
}

=begin nd
Function: getRawConfig

Returns a hash reference of the the part of the COMMON::Config object that actually contains the raw configuration (without order).
=cut
sub getRawConfig {
    my $self = shift;

    return $self->{'rawConfiguration'};
}


1;
__END__