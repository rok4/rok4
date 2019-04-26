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
#
################################################################################

=begin nd
File: PropertiesLoader.pm

Class: EXPYR::PropertiesLoader

(see ROK4GENERATION/libperlauto/EXPYR_PropertiesLoader.png)

Stores all information about the pyramid extraction.

Using:
    (start code)
    [ logger ] 
    ; Optionnal section
    log_level =
    log_path =
    log_file =

    [ pyramid ]
    pyr_name =
    pyr_desc_path =
    pyr_data_path =
    extract_mode = ; slink (défaut), hlink, copy

    [ datasource ]
    ; a subsection = one level
    [[ ID ]]
    ; ID = TMS level identifier
    extent = ; BBOX or WKT
    desc_file = ; path to the pyramid descriptor
    (end code)

Attributes:
    cfgFile - string - Configuration file path
    cfgObject - <COMMON::Config> - Configuration reader
   
=cut

################################################################################

package EXPYR::PropertiesLoader;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
use Data::Dumper;

use COMMON::Config;
use COMMON::CheckUtils;
use COMMON::Array;

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
        'filepath' => $file,
        'format' => "INI"
    });

    if (! defined $cfg) {
        ERROR ("Can not load properties !");
        return FALSE;
    }

    $this->{cfgObject} = $cfg;
    
    return TRUE;
}

# Function: _check
sub _check {
    my $this = shift;

    # Pyramid section
    if ($this->{cfgObject}->isSection("pyramid") != 2) {
        ERROR("'pyramid' section is missing");
        return FALSE;
    }

    if (! $this->{cfgObject}->isProperty({ 'section' => "pyramid", 'property' => "pyr_name"}) ) {
        ERROR("'pyramid.pyr_name' property is missing");
        return FALSE;
    }
    if (! $this->{cfgObject}->isProperty({ 'section' => "pyramid", 'property' => "pyr_desc_path"}) ) {
        ERROR("'pyramid.pyr_desc_path' property is missing");
        return FALSE;
    }
    if (! $this->{cfgObject}->isProperty({ 'section' => "pyramid", 'property' => "pyr_data_path"}) ) {
        ERROR("'pyramid.pyr_data_path' property is missing");
        return FALSE;
    }
    if (! $this->{cfgObject}->isProperty({ 'section' => "pyramid", 'property' => "tms_path"}) ) {
        ERROR("'pyramid.tms_path' property is missing");
        return FALSE;
    }

    # Datasource section
    if ($this->{cfgObject}->isSection("datasource") != 2) {
        ERROR("'datasource' section is missing");
        return FALSE;
    }

    my $error = FALSE;

    foreach my $level ($this->{cfgObject}->getSubSections("datasource")) {

        my $descPath = $this->{cfgObject}->getProperty({section => "datasource", subsection => $level, property => "desc_file"});
        my $extent = $this->{cfgObject}->getProperty({section => "datasource", subsection => $level, property => "extent"});

        if (! defined $descPath) {
            ERROR("Undefined descriptor file's path for level '$level'");
            $error = TRUE;
        } elsif ( ! (-e $descPath && -f $descPath) ) {
            ERROR("Descriptor file's path for level '$level' doesn't exist: $descPath");
            $error = TRUE;            
        }

        if (! defined $extent ) {
            ERROR("Undefined extent for level '$level'");
            $error = TRUE;
        } elsif ( COMMON::CheckUtils::isBbox($extent) ) {
            DEBUG("Extent for level '$level' is a bounding box.");
        } elsif ( -e $extent && -f $extent ) {
            DEBUG("Extent for level '$level' is a file.");
        } else {
            ERROR("Unvalid extent for level '$level': $extent");
            $error = TRUE;
        }

    }

    return ! $error;
}

####################################################################################################
#                                Group: Getters - Setters                                          #
####################################################################################################

# Function: getAllProperties
sub getAllProperties {
    my $this = shift;

    return $this->{cfgObject}->getRawConfig();
}


1;
__END__