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

Class: PYR2PYR::PropertiesLoader

Reads a configuration file, to sIniFiles* format

Using:
    (start code)
    use PYR2PYR::PropertiesLoader;

    my $config = PYR2PYR::PropertiesLoader->new("/home/ign/file.txt");
    (end code)

Attributes:
    cfgFile - string - Configuration file path
    cfgObject - <COMMON::Config> - Configuration reader
=cut

################################################################################

package PYR2PYR::PropertiesLoader;

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
    my $filepath = shift;

    my $class= ref($this) || $this;
    # IMPORTANT : if modification, think to update natural documentation (just above)
    my $self = {
        cfgFile   => $filepath,
        cfgObject => undef
    };

    bless($self, $class);

    # init. class
    return undef if (! $self->_init());
    return undef if (! $self->_check());

    return $self;
}

# Function: _init
sub _init {
    my $self = shift;

    my $file = $self->{cfgFile};
    
    if (! defined $file || $file eq "") {
        ERROR ("Filepath undefined");
        return FALSE;
    }
    
    # init. params
    if (! -f $file) {
        ERROR (sprintf "File properties '%s' doesn't exist !?", $file);
        return FALSE;
    }

    # load properties 
    my $cfg = COMMON::Config->new({
        'filepath' => $file
    });

    if (! defined $cfg) {
        ERROR ("Can not load properties !");
        return FALSE;
    }

    $self->{cfgObject} = $cfg;
    
    return TRUE;
}

# Function: _init
sub _check {
    my $self = shift;

    if ($self->{cfgObject}->isSection("from") != 2) {
        ERROR("'from' section is missing");
        return FALSE;
    }

    if ($self->{cfgObject}->isSection("to") != 2) {
        ERROR("'to' section is missing");
        return FALSE;
    }

    if ($self->{cfgObject}->isSection("process") != 2) {
        ERROR("'process' section is missing");
        return FALSE;
    }

    if (! $self->{cfgObject}->isProperty({property=>"pyr_desc_file",section=>"from"})) {
       ERROR("'pyr_desc_file' property is missing");
        return FALSE; 
    }

    if (! $self->{cfgObject}->isProperty({property=>"pyr_list_file",section=>"from"})) {
       ERROR("'pyr_list_file' property is missing");
        return FALSE; 
    }


    if (! $self->{cfgObject}->isProperty({property=>"pool_name",section=>"to"})) {
       ERROR("'pool_name' property is missing");
        return FALSE; 
    }
    if (! $self->{cfgObject}->isProperty({property=>"pyr_name",section=>"to"})) {
       ERROR("'pyr_name' property is missing");
        return FALSE; 
    }
    if (! $self->{cfgObject}->isProperty({property=>"pyr_desc_path",section=>"to"})) {
       ERROR("'pyr_desc_path' property is missing");
        return FALSE; 
    }


    if (! $self->{cfgObject}->isProperty({property=>"job_number",section=>"process"})) {
       ERROR("'job_number' property is missing");
        return FALSE; 
    }
    if (! $self->{cfgObject}->isProperty({property=>"path_temp",section=>"process"})) {
       ERROR("'path_temp' property is missing");
        return FALSE; 
    }


    
    return TRUE;
}

####################################################################################################
#                                Group: Getters - Setters                                          #
####################################################################################################

# Function: getCfgObject
sub getCfgObject {
    my $self = shift;
    return $self->{cfgObject};
}

# Function: getAllProperties
sub getAllProperties {
  my $self = shift;
  
  return $self->{cfgObject}->getConfig();
}

1;
__END__
