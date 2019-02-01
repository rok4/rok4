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

(see PYR2PYR_PropertiesLoader.png)

Reads a configuration file, to IniFiles format

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
    my $class = shift;
    my $filepath = shift;

    $class = ref($class) || $class;
    # IMPORTANT : if modification, think to update natural documentation (just above)
    my $this = {
        cfgFile   => $filepath,
        cfgObject => undef
    };

    bless($this, $class);

    # init. class
    return undef if (! $this->_init());
    return undef if (! $this->_check());

    return $this;
}

# Function: _init
sub _init {
    my $this = shift;

    my $file = $this->{cfgFile};
    
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

    $this->{cfgObject} = $cfg;
    
    return TRUE;
}

# Function: _init
sub _check {
    my $this = shift;

    if ($this->{cfgObject}->isSection("from") != 2) {
        ERROR("'from' section is missing");
        return FALSE;
    }

    if ($this->{cfgObject}->isSection("to") != 2) {
        ERROR("'to' section is missing");
        return FALSE;
    }

    if ($this->{cfgObject}->isSection("process") != 2) {
        ERROR("'process' section is missing");
        return FALSE;
    }

    if (! $this->{cfgObject}->isProperty({property=>"pyr_desc_file",section=>"from"})) {
       ERROR("'pyr_desc_file' property is missing");
        return FALSE; 
    }

    if (! $this->{cfgObject}->isProperty({property=>"pyr_list_file",section=>"from"})) {
       ERROR("'pyr_list_file' property is missing");
        return FALSE; 
    }


    if ( $this->{cfgObject}->isProperty({property=>"pool_name",section=>"to"})) {
        INFO("'pool_name' is provided : CEPH push");
    }
    elsif ( $this->{cfgObject}->isProperty({property=>"bucket_name",section=>"to"})) {
        INFO("'bucket_name' is provided : S3 push");
    }
    elsif ( $this->{cfgObject}->isProperty({property=>"container_name",section=>"to"})) {
        INFO("'container_name' is provided : SWIFT push");
    }
    else {
        ERROR("Neither pool_name nor bucket_name not container_name provided");       
        return FALSE;
    }


    if (! $this->{cfgObject}->isProperty({property=>"pyr_name",section=>"to"})) {
        ERROR("'pyr_name' property is missing");
        return FALSE; 
    }
    if (! $this->{cfgObject}->isProperty({property=>"pyr_desc_path",section=>"to"})) {
        ERROR("'pyr_desc_path' property is missing");
        return FALSE; 
    }


    if (! $this->{cfgObject}->isProperty({property=>"job_number",section=>"process"})) {
        ERROR("'job_number' property is missing");
        return FALSE; 
    }
    if (! $this->{cfgObject}->isProperty({property=>"path_temp",section=>"process"})) {
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
    my $this = shift;
    return $this->{cfgObject};
}

# Function: getAllProperties
sub getAllProperties {
  my $this = shift;
  
  return $this->{cfgObject}->getConfig();
}

1;
__END__
