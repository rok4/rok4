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

(see COMMON_Config.png)

Using:
    (start code)
    # load properties from INI file
    my $cfgINI = COMMON::Config->new({
        'filepath' => "/mon/fichier/de/configuration.txt",
        'format' => "INI"
    });

    # load properties from INI file
    my $cfgJSON = COMMON::Config->new({
        'filepath' => "/mon/fichier/de/configuration.json",
        'format' => "JSON"
    });
    (end code)

Attributes:
    filePath - string - Path to the configuration file
    fileFormat - string - Configuration format : INI, JSON
    configuration - string hash - Configuration stored in string hash, with section and sub section, with orders
    rawConfiguration - string hash - Configuration stored in string hash, with section and sub section, without orders
    
Limitations:
    
=cut

################################################################################

package COMMON::Config;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
use Data::Dumper;
use Scalar::Util qw/reftype/;
use JSON::Parse qw(assert_valid_json parse_json);

use COMMON::Array;

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

# Constant: FILEFORMATS
# Define allowed values for configuration format
my @FILEFORMATS = ('INI', 'JSON');

################################################################################

BEGIN {}
INIT {}
END {}

################################################################################
#                             Group: Constructors                              #
################################################################################

=begin nd
Constructor: new

Config constructor. Bless an instance.

Parameters (hash):
    filepath - string - Configuration file path
    format - string - File format, INI or JSON

See also:
    <_loadINI>, <_loadJSON>
=cut
sub new {
    my $class = shift;
    my $params = shift;
    
    $class = ref($class) || $class;

    my $this = {
        filePath => undef,
        fileFormat => undef,
        configuration => {},
        rawConfiguration => {},
    };
    
    bless($this, $class);

    DEBUG(sprintf "COMMON::Config->new called with parameters : %s", Dumper($params));
    if (! defined $params || ! keys %{$params} > 0) {
        ERROR("COMMON::Config->new cannot be called without arguments.");
        return undef;
    }

    my $value = undef;

    # Read the mandatory configuration file's path parameter 
    if (defined ($value = delete $params->{'filepath'})) {
        DEBUG(sprintf "Given configuration file's path : '%s'", $value);
        $this->{filePath} = $value;
    } else {
        ERROR("Cannot use COMMON::Config->new whithout a valid 'filepath' parameter.");
        return undef;
    }
    $value = undef;

    # Check the format in which the configuration file is written
    $value = delete $params->{'format'};

    if ( ! defined $value) {
        INFO("No format defined for the configuration file. Switching to default INI-like format ('INI').");
        $value = "INI";
    }
    
    if ( ! defined COMMON::Array::isInArray(uc($value), @FILEFORMATS) ) {
        ERROR("Unknown file format: $value");
        return undef;
    }
    $this->{fileFormat} = uc($value);

    if ($this->{fileFormat} eq "INI") {
        if (! $this->_loadINI() ) {
            ERROR("Configuration file wasn't properly loaded.");
            return undef;
        }
    }
    elsif ($this->{fileFormat} eq "JSON") {
        if (! $this->_loadJSON() ) {
            ERROR("Configuration file wasn't properly loaded.");
            return undef;
        }
    }

    return $this;
}

=begin nd
Function: _loadINI

Read line by line (order is important), no library is used.

=cut
sub _loadINI {
    my $this = shift;


    my $filepath = $this->{filePath};
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

            if (exists $this->{configuration}->{$l}) {
                ERROR (sprintf "A section is defined twice in the configuration : section '%s'", $l);
                return FALSE;
            }
            $currentSection = $l;
            $currentSubSection = undef; # Resetting subsection as section changes
            $this->{configuration}->{$currentSection}->{'_props'} = []; # Array of properties name, to index their order
            next;
        }

        if ($l =~ m/^\[\[([\w-]*)\]\]$/) {
            $l =~ s/[\[\]]//g;

            if (exists $this->{configuration}->{$currentSection}->{$l}) {
                ERROR (sprintf "A subsection is defined twice in the configuration : section '%s', subsection '%s'", $currentSection, $l);
                return FALSE;
            }
            $currentSubSection = $l;            
            $this->{configuration}->{$currentSection}->{$currentSubSection}->{'_props'} = []; # Array of properties name, to index their order
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
            if (exists $this->{configuration}->{$currentSection}->{$prop[0]}) {
                ERROR (sprintf "A property is defined twice in the configuration : section %s, parameter %s", $currentSection, $prop[0]);
                return FALSE;
            }            
            $this->{configuration}->{$currentSection}->{$prop[0]} = $prop[1];
            push (@{$this->{configuration}->{$currentSection}->{'_props'}}, $prop[0]);

            $this->{rawConfiguration}->{$currentSection}->{$prop[0]} = $prop[1];
        } else {
            if (defined $this->{configuration}->{$currentSection}->{$currentSubSection}->{$prop[0]}) {
                ERROR (sprintf "A property is defined twice in the configuration : section %s, subsection %s parameter %s", $currentSection, $currentSubSection, $prop[0]);
                return FALSE;
            }
            $this->{configuration}->{$currentSection}->{$currentSubSection}->{$prop[0]} = $prop[1];
            push (@{$this->{configuration}->{$currentSection}->{$currentSubSection}->{'_props'}}, $prop[0]);
            
            $this->{rawConfiguration}->{$currentSection}->{$currentSubSection}->{$prop[0]} = $prop[1];
        }
    }

    close $fileHandle;

    return TRUE;
}


=begin nd
Function: _loadJSON

Parse JSON file using library.

=cut
sub _loadJSON {
    my $this = shift;

    my $filepath = $this->{filePath};
    my $json_text = do {
        open(my $json_fh, "<", $this->{filePath}) or do {
            ERROR(sprintf "Cannot open JSON file : %s (%s)", $this->{filePath}, $! );
            return FALSE;
        };
        local $/;
        <$json_fh>
    };

    eval {
        assert_valid_json ($json_text);
    };
    if ($@) {
        ERROR("File $filepath is not a valid JSON");
        ERROR($@);
        return FALSE;
    }

    $this->{rawConfiguration} = parse_json ($json_text);

    return TRUE;
}


################################################################################
#                                Group: Tester                                 #
################################################################################

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
    my $this = shift;
    my $sectionName = shift;

    if ($this->{fileFormat} ne "INI") {
        ERROR("isSection is implemented only for INI configuration format");
        return 0;
    }

    if (! defined $sectionName) {
        ERROR("No section name provided");
        return 0;
    }

    if (! exists $this->{configuration}->{$sectionName}) {
        return 0;
    }

    if (! defined reftype($this->{configuration}->{$sectionName}) || reftype($this->{configuration}->{$sectionName}) ne 'HASH') {
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
    my $this = shift;
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

    if ($this->{fileFormat} ne "INI") {
        ERROR("isSubSection is implemented only for INI configuration format");
        return 0;
    }

    if ($this->isSection($sectionName) != 2) {
        return 0;
    }

    if (! exists $this->{configuration}->{$sectionName}->{$subSectionName}) {
        return 0;
    }

    if (
        ! defined reftype($this->{configuration}->{$sectionName}->{$subSectionName}) || 
        reftype($this->{configuration}->{$sectionName}->{$subSectionName}) ne 'HASH'
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
    section - string - section's name
    subsection - string - subsection's name (optionnal)
    property - string - the property to check

Returns:
    TRUE if properties exists and is defined, FALSE otherwise
=cut
sub isProperty {
    my $this = shift;
    my $params = shift;

    if ($this->{fileFormat} ne "INI") {
        ERROR("isProperty is implemented only for INI configuration format");
        return FALSE;
    }

    if (! exists $params->{section} || ! defined $params->{section}) {
        ERROR("No section name provided");
        return FALSE;
    }
    my $sec = $params->{section};
    
    if (! exists $params->{property} || ! defined $params->{property}) {
        ERROR("No property name provided");
        return FALSE;
    }
    my $prop = $params->{property};

    if (exists $params->{subsection} && defined $params->{subsection}) {
        my $subsec = $params->{subsection};
        return (exists $this->{configuration}->{$sec}->{$subsec}->{$prop} && defined $this->{configuration}->{$sec}->{$subsec}->{$prop});
    } else {
        return (exists $this->{configuration}->{$sec}->{$prop} && defined $this->{configuration}->{$sec}->{$prop});
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
    my $this = shift;
    my $section = shift;

    if ($this->{fileFormat} ne "INI") {
        ERROR("getSection is implemented only for INI configuration format");
        return undef;
    }

    if(! defined $section) {
        ERROR("Wrong argument number : syntax = getSection('section_name')");
        return undef;
    }

    if ($this->isSection($section) != 2) {
        ERROR("'$section' is not a section");
        return undef;
    }

    my $refSectionHash = $this->{configuration}->{$section};
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
    my $this = shift;
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

    if ($this->{fileFormat} ne "INI") {
        ERROR("getSubSection is implemented only for INI configuration format");
        return undef;
    }

    if ($this->isSubSection($section, $subSection) != 2) {
        ERROR("'$section.$subSection' is not a subsection");
        return undef;
    }

    my $refSubSectionHash = $this->{configuration}->{$section}->{$subSection};
    my %subSectionHash = %{$refSubSectionHash};

    return %subSectionHash;
}


=begin nd
Function: getProperty

Returns the value of a property in a section or a section-subsection pair.

Syntax: getProperty({ 'section' => sectionName, 'property' => propertyName [, 'subsection' => subSectionName] })

Parameters (hash):
    section - string - section's name
    subsection - string - subsection's name (optionnal)
    property - string - the property to get
=cut
sub getProperty {
    my $this = shift;
    my $params = shift;

    if ($this->{fileFormat} ne "INI") {
        ERROR("getProperty is implemented only for INI configuration format");
        return undef;
    }

    if (! $this->isProperty($params)) {
        return undef;
    }

    my $sec = $params->{section};
    my $prop = $params->{property};

    if (exists $params->{subsection} && defined $params->{subsection}) {
        my $subsec = $params->{subsection};
        return $this->{configuration}->{$sec}->{$subsec}->{$prop};
    } else {
        return $this->{configuration}->{$sec}->{$prop};
    }
}

=begin nd
Function: setProperty

Store the value of a property in a section or a section-subsection pair.

Syntax: setProperty({ 'section' => sectionName, 'property' => propertyName [, 'subsection' => subSectionName], 'value' = value })

Parameters (hash):
    section - string - section's name
    subsection - string - subsection's name (optionnal)
    property - string - the property to set
    value - string - the value to set
=cut
sub setProperty {
    my $this = shift;
    my $params = shift;

    if ($this->{fileFormat} ne "INI") {
        ERROR("setProperty is implemented only for INI configuration format");
    }

    my $sec = $params->{section};
    my $prop = $params->{property};
    my $val = $params->{value};

    if (exists $params->{subsection} && defined $params->{subsection}) {
        my $subsec = $params->{subsection};
        $this->{configuration}->{$sec}->{$subsec}->{$prop} = $val;
        $this->{rawConfiguration}->{$sec}->{$subsec}->{$prop} = $val;
    } else {
        $this->{configuration}->{$sec}->{$prop} = $val;
        $this->{rawConfiguration}->{$sec}->{$prop} = $val;
    }
}

=begin nd
Function: getProperty

Returns the list of existing sections.

Syntax: getSections()
  
=cut
sub getSections {
    my $this = shift;

    if ($this->{fileFormat} ne "INI") {
        ERROR("getSections is implemented only for INI configuration format");
        return ();
    }

    my @sections;
    foreach my $item (keys %{$this->{configuration}}) {
        if ( $this->isSection($item) == 2 ) {
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
    my $this = shift;
    my $section = shift;

    if ($this->{fileFormat} ne "INI") {
        ERROR("getSubSections is implemented only for INI configuration format");
        return ();
    }

    if ($this->isSection($section) != 2) {
        ERROR("'$section' is not a section, cannot obtain list of subsections in");
        return undef;
    } 

    my @subSections;
    foreach my $item (keys %{$this->{configuration}->{$section}}) {
        if ( $this->isSubSection($section, $item) == 2 ) {
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
    my $this = shift;
    my $section = shift;
    my $subsection = shift;

    if ($this->{fileFormat} ne "INI") {
        ERROR("getProperties is implemented only for INI configuration format");
        return ();
    }

    if (defined $subsection) {
        if ($this->isSubSection($section, $subsection) != 2) {
            ERROR("'$section.$subsection' is not a subsection, cannot obtain list of properties in");
            return undef;
        }
        return @{$this->{configuration}->{$section}->{$subsection}->{'_props'}};
    } else {
        if ($this->isSection($section) != 2) {
            ERROR("'$section' is not a section, cannot obtain list of properties in");
            return undef;
        }
        return @{$this->{configuration}->{$section}->{'_props'}};
    }
}

=begin nd
Function: getConfig

Returns a hash copy of the the part of the COMMON::Config object that actually contains the configuration.

In JSON case, we return the raw configuration.

Syntax: getConfig()
 
=cut
sub getConfig {
    my $this = shift;

    my $refConfig;
    if ($this->{fileFormat} eq "JSON") {
        $refConfig = $this->{rawConfiguration};
    } else {
        $refConfig = $this->{configuration};
    }

    return %{$refConfig};
}

=begin nd
Function: getRawConfig

Returns a hash reference of the the part of the COMMON::Config object that actually contains the raw configuration (without order).
=cut
sub getRawConfig {
    my $this = shift;

    return $this->{rawConfiguration};
}


1;
__END__