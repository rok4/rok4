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

# Function: new
sub new {
  my $this = shift;

  my $class= ref($this) || $this;
  # IMPORTANT : if modification, think to update natural documentation (just above)
  my $self = {
    CFGFILE   => undef,
    HDLFILE   => undef,
    CFGPARAMS => {},
  };

  bless($self, $class);
  
  TRACE;
  
  # init. class
  return undef if (! $self->_initParams(@_));
  return undef if (! $self->_initCfg());
  
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

# Function: _initCfg
sub _initCfg {
  my $self = shift;

  TRACE;
  
  return FALSE if (! $self->LoadProperties($self->{CFGFILE}));
  
  return TRUE;
}

####################################################################################################
#                                      Group: Loader                                               #
####################################################################################################

# Function: LoadProperties
sub LoadProperties {
  
  my $self     = shift;
  my $fileconf = shift;

  TRACE;

  # load properties 
  my $cfg = COMMON::Config->new( {
                        -filepath => $fileconf,
                        -format => 'INI',
                        } );
    
  if (! defined $cfg) {
    ERROR ("Can not load properties !");
    return FALSE;
  }
  
  # save params
  my $params = $self->{CFGPARAMS};
  
  foreach my $section ($cfg->getSections()) {
    TRACE ("section > $section");
    $params->{$section} = $cfg->getSection($section);
  }
    
  # save handler
  $self->{HDLFILE} = $cfg;
  
  return TRUE;
}

####################################################################################################
#                                Group: Getters - Setters                                          #
####################################################################################################

# Function: getAllProperties
sub getAllProperties {
  my $self = shift;
  return $self->{CFGPARAMS};
}

# Function: getPropertiesBySection
sub getPropertiesBySection {
  my $self = shift;
  my $section = shift;
  
  return undef if (! defined $section);
  return undef if (! exists($self->{CFGPARAMS}->{$section}));
  
  return $self->{CFGPARAMS}->{$section};
}

# Function: getSections
sub getSections {
  my $self = shift;
  
  my @sections;
  my $param = $self->{CFGPARAMS};
  foreach (keys %$param) {
    push @sections, $_;
  }
  return @sections;
}

# Function: getKeyParameters
sub getKeyParameters {
  my $self = shift;
  my $section = shift;
  
  return undef if (! defined $section);
  return undef if (! exists($self->{CFGPARAMS}->{$section}));
  
  my @params;
  my $param = $self->{CFGPARAMS}->{$section};
  foreach (keys %$param) {
    push @params, $_;
  }
  
  return @params;
}

# Function: getValueParameters
sub getValueParameters {
  my $self = shift;
  my $section = shift;
  
  return undef if (! defined $section);
  return undef if (! exists($self->{CFGPARAMS}->{$section}));
  
  my @params;
  my $param = $self->{CFGPARAMS}->{$section};
  foreach (values %$param) {
    push @params, $_;
  }
  
  return @params;
}

1;
__END__
