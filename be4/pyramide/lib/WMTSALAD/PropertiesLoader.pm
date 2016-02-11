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
File: PropertiesLoader.pm

Class: BE4::PropertiesLoader

Reads a configuration file, to sIniFiles* format

Using:
    (start code)
    use BE4::PropertiesLoader;

    my $proptxt << EOF
        [section 1]
        param1=value1
        param2=value2
        [section 2]
        ; param21=value21
        ; param22=value22
    EOF

    open FILE, ">", $propfile;
    printf FILE "%s",  $proptxt;
    close FILE;

    my $objprop = BE4::PropertiesLoader->new($propfile);

    # {section 1 => {...}, section 2 => {...}}
    my $config     = $objprop->getAllProperties();

    my @sections   = $objprop->getSections();  # [section 1, section 2]
    my @parameters = $objprop->getKeyParameters("section 1"); # [param1, param2]
    my @values     = $objprop->getValueParameters("section 1"); # [value1, value2]

    # {param1=>value1, param2=>value2}
    my $config_section = $objprop->getPropertiesBySection("section 1");
    ...
    (end code)

Attributes:
    CFGFILE - string - Configuration file path
    HDLFILE - <Config::IniFiles> - Configuration reader
    CFGPARAMS - hash - File properties (sections...)
=cut

################################################################################

package WMTSALAD::PropertiesLoader;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
use Data::Dumper;
use Scalar::Util qw/reftype/;

use COMMON::Config;

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

<WMTSALAD::PropertiesLoader's> contructor.

Using:
    (start code)
    new(file [, filetype])
    (end code)

Parameters:
    file - string - Path to the configuration file
    filetype - string - The type of configuration file (optionnal, default to INI-like)  

Returns:
    The newly created PropertiesLoader object. 'undef' in case of failure.
    
=cut
sub new {
    my $this = shift;
    my $file = shift;
    my $fileType = shift;

    my $class= ref($this) || $this;
    # IMPORTANT : if modification, think to update natural documentation (just above)
    my $self = {
      CFGFILE   => undef,
      CFGOBJ => undef,
    };

    bless($self, $class);
    
    TRACE;
    
    # init. class
    return undef if (! $self->_initParams($file));
    return undef if (! $self->LoadProperties($file, $fileType));
    
    return $self;
}

# Function: _initParams
sub _initParams {
    my $self = shift;
    my $file = shift;

    TRACE;
    
    if (! defined $file || $file eq "") {
        ERROR ("A configuration file path must be passed as parameter.");
        return FALSE;
    }
    
    # init. params
    if (! -f $file) {
        ERROR (sprintf "Properties file '%s' doesn't exist !", $file);
        return FALSE;
    }
    $self->{CFGFILE} = $file;
    
    return TRUE;
}


####################################################################################################
#                                      Group: Loader                                               #
####################################################################################################

# Function: LoadProperties
sub LoadProperties {
  
  my $self     = shift;
  my $fileconf = shift;
  my $fileType = shift;

  TRACE;

  # load properties 
  my $cfg = COMMON::Config->new( {
                        -filepath => $fileconf,
                        -format => $fileType,
                        } );
    
  if (! defined $cfg) {
    ERROR ("Can not load properties !");
    return FALSE;
  }
    
  # save properties (COMMON::Config object)
  $self->{CFGOBJ} = $cfg;
  
  return TRUE;
}

####################################################################################################
#                                Group: Getters - Setters                                          #
####################################################################################################

=begin nd
Function: getAllProperties

Returns a hash describing the content of the configuration file.

Syntax: getAllProperties()
 
=cut
sub getAllProperties {
    my $self = shift;
    return $self->{CFGOBJ}->getConfig();
}

=begin nd
Function: getPropertiesBySection

Returns a hash describing a section from the configuration file.

Syntax: getPropertiesBySection(section)

Parameters (list):
    section - string - the requested configuration section
 
=cut
sub getPropertiesBySection {
    my $self = shift;
    my $section = shift;
    
    return $self->{CFGOBJ}->getSection($section);
}

=begin nd
Function: getPropertiesBySubSection

Returns a hash describing a sub-section from the configuration file.

Syntax: getPropertiesBySubSection(section)

Parameters (list):
    section - string - the requested configuration section
    subsection - string - the desired subsection of the given section
 
=cut
sub getPropertiesBySubSection {
    my $self = shift;
    my $section = shift;
    my $subsection = shift;
    
    return $self->{CFGOBJ}->getSubSection($section, $subsection);
}

=begin nd
Function: getSections

Returns an array listing the configuration file's sections.

Syntax: getSections()
 
=cut
sub getSections {
    my $self = shift; 

    return $self->{CFGOBJ}->getSections();
}

=begin nd
Function: getSections

Returns an array listing the subsections found in a given configuration file's section.

Syntax: getSections(section)

Parameters (list):
    section - string - the configuration file's section
 
=cut
sub getSubSections {
    my $self = shift; 
    my $section = shift;

    return $self->{CFGOBJ}->getSubSections($section);
}


=begin nd
Function: getKeyParameters

Returns an array listing the parameters keys (=names) found in a given (sub)section, 
in the same order as in the configuration file.

Syntax: getKeyParameters(section [, subsection])

Parameters (list):
    section - string - the configuration file's section
    subsection - string - the configuration file's subsection (optionnal)
 
=cut
sub getKeyParameters {
    my $self = shift;
    my $section = shift;
    my $subsection = shift;

    if (defined $subsection) {
        return $self->{CFGOBJ}->getProperties($section, $subsection);
    } else {
        return $self->{CFGOBJ}->getProperties($section);
    }
}


=begin nd
Function: getValueParameters

Returns an array listing the parameters values found in a given (sub)section, 
in the same order as in the configuration file (matching getKeyParameters()).

Syntax: getValueParameters(section [, subsection])

Parameters (list):
    section - string - the configuration file's section
    subsection - string - the configuration file's subsection (optionnal)
 
=cut
sub getValueParameters {
    my $self = shift;
    my $section = shift;
    my $subsection = shift;
    
    my @parmKeys = $self->getKeyParameters($section, $subsection);
    my @parmVals;

    if (! @parmKeys) {
        return undef
    }
    if (defined $subsection) {
        foreach my $parmKey (@parmKeys) {
            push (@parmVals, $self->{CFGOBJ}->getProperty($section, $subsection, $parmKey));
        }
    } else {
        foreach my $parmKey (@parmKeys) {
            push (@parmVals, $self->{CFGOBJ}->getProperty($section, $parmKey));
        }
    }
    
    return @parmVals;
}

1;
__END__
