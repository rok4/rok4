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
File: DatabaseSource.pm

Class: COMMON::DatabaseSource

Stores parameters and builds WMS request.

Using:
    (start code)
    use COMMON::DatabaseSource;

    # Image width and height not defined

    # DatabaseSource object creation
    my $objDatabaseSource = COMMON::DatabaseSource->new({
        db_host   => "ORTHO_RAW_LAMB93_PARIS_OUEST",
        db_     => "http://localhost/wmts/rok4",
        wms_version => "1.3.0",
        wms_request => "getMap",
        wms_format  => "image/tiff"
    });

    OR

    # Image width and height defined, style, transparent and background color (for a WMS vector)

    # DatabaseSource object creation
    my $objDatabaseSource = COMMON::DatabaseSource->new({
        wms_layer   => "BDD_WLD_WM",
        wms_url     => "http://localhost/wmts/rok4",
        wms_version => "1.3.0",
        wms_request => "getMap",
        wms_format  => "image/png",
        wms_bgcolor => "0xFFFFFF",
        wms_transparent  => "FALSE",
        wms_style  => "line",
        max_width  => 1024,
        max_height  => 1024
    });
    (end code)

Attributes:
    URL - string -  Left part of a WMS request, before the *?*.
    VERSION - string - Parameter *VERSION* of a WMS request : "1.3.0".
    REQUEST - string - Parameter *REQUEST* of a WMS request : "getMap"
    FORMAT - string - Parameter *FORMAT* of a WMS request : "image/tiff"
    LAYERS - string - Layer name to harvest, parameter *LAYERS* of a WMS request.
    OPTIONS - string - Contains style, background color and transparent parameters : STYLES=line&BGCOLOR=0xFFFFFF&TRANSPARENT=FALSE for example. If background color is defined, transparent must be 'FALSE'.
    min_size - integer - Used to remove too small harvested images (full of nodata), in bytes. Can be zero (no limit).
    max_width - integer - Max image's pixel width which will be harvested, can be undefined (no limit).
    max_height - integer - Max image's pixel height which will be harvested, can be undefined (no limit).

If *max_width* and *max_height* are not defined, images will be harvested all-in-one. If defined, requested image size have to be a multiple of this size.
=cut

################################################################################

package COMMON::DatabaseSource;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
use Data::Dumper;

use COMMON::Database;

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


# Constant: DEFAULT
# Define default values for attributes.
my %DEFAULT;

################################################################################

BEGIN {}
INIT {
    %DEFAULT = (
        db_port => 5432,
        db_schema => "public"
    );
}
END {}

####################################################################################################
#                                        Group: Constructors                                       #
####################################################################################################

=begin nd
Constructor: new

DatabaseSource constructor. Bless an instance.

Parameters (hash):
    wms_layer - string - Layer to harvest.
    wms_url - string - WMS server url.
    wms_version - string - WMS version.
    wms_request - string - Request's type.
    wms_format - string - Result's format.
    wms_bgcolor - string - Optionnal. Hexadecimal red-green-blue colour value for the background color (white = "0xFFFFFF").
    wms_transparent - boolean - Optionnal.
    wms_style - string - Optionnal.
    min_size - integer - Optionnal. 0 by default
    max_width - integer - Optionnal.
    max_height - integer - Optionnal.
See also:
    <_init>
=cut
sub new {
    my $class = shift;
    my $params = shift;

    $class = ref($class) || $class;
    # IMPORTANT : if modification, think to update natural documentation (just above) and pod documentation (bottom)
    my $this = {
        host      => undef,
        port  => undef,
        dbname  => undef,
        schemaname   => undef,
        username => undef,
        password => undef,
        tablenames => [],
        tables => {}
    };

    bless($this, $class);


    # init. class
    return undef if (! $this->_init($params));
    return undef if (! $this->_load());

    return $this;
}

=begin nd
Function: _init

Checks and stores attributes' values.

Parameters (hash):
    wms_layer - string - Layer to harvest.
    wms_url - string - WMS server url.
    wms_version - string - WMS version.
    wms_request - string - Request's type.
    wms_format - string - Result's format.
    wms_bgcolor - string - Optionnal. Hexadecimal red-green-blue colour value for the background color (white = "0xFFFFFF").
    wms_transparent - boolean - Optionnal.
    wms_style - string - Optionnal.
    min_size - integer - Optionnal. 0 by default
    max_width - integer - Optionnal.
    max_height - integer - Optionnal.
=cut
sub _init {
    my $this   = shift;
    my $params = shift;
    
    return FALSE if (! defined $params);
    
    # PORT    
    if (! exists($params->{db_port}) || ! defined ($params->{db_port})) {
        $this->{db_port} = $DEFAULT{db_port};
        INFO(sprintf "Default value for 'db_port' : %s", $this->{db_port});
    } else {
        if (int($params->{db_port}) <= 0) {
            ERROR("If 'db_port' is given, it must be strictly positive.");
            return FALSE ;
        }
        $this->{port} = int($params->{db_port});
    }

    # SCHEMA    
    if (! exists($params->{db_schema}) || ! defined ($params->{db_schema})) {
        $this->{db_schema} = $DEFAULT{db_schema};
        INFO(sprintf "Default value for 'db_schema' : %s", $this->{db_schema});
    } else {
        $this->{schemaname} = $params->{db_schema};
    }

    # Other parameters are mandatory
    # HOST
    if (! exists($params->{db_host}) || ! defined ($params->{db_host})) {
        ERROR("Parameter 'db_host' is required !");
        return FALSE ;
    }
    $this->{host} = $params->{db_host};
    # DBNAME
    if (! exists($params->{db_name}) || ! defined ($params->{db_name})) {
        ERROR("Parameter 'db_name' is required !");
        return FALSE ;
    }
    $this->{dbname} = $params->{db_name};
    # USERNAME
    if (! exists($params->{db_user}) || ! defined ($params->{db_user})) {
        ERROR("Parameter 'db_user' is required !");
        return FALSE ;
    }
    $this->{username} = $params->{db_user};
    # PASSWORD
    if (! exists($params->{db_pwd}) || ! defined ($params->{db_pwd})) {
        ERROR("Parameter 'db_pwd' is required !");
        return FALSE ;
    }
    $this->{password} = $params->{db_pwd};

    # TABLES
    if (! exists($params->{db_tables}) || ! defined ($params->{db_tables})) {
        ERROR("Parameter 'db_tables' is required !");
        return FALSE ;
    }

    $params->{db_tables} =~ s/ //;
    my @tables = split (/,/,$params->{db_tables},-1);
    foreach my $t (@tables) {
        if ($t eq '') {
            next;
        }
        push(@{$this->{tablenames}}, $t);
    }

    if (scalar(@{$this->{tablenames}}) == 0) {
        ERROR("Parameter 'db_tables' contains no table name");
        return FALSE ;
    }
    
    return TRUE;
}

sub _load {
    my $this   = shift;

    my $database = COMMON::Database->new(
        $this->{dbname},
        $this->{host},
        $this->{port},
        $this->{username},
        $this->{password}
    );

    if (! defined $database) {
        ERROR( "Cannot connect database to extract attributes and type for tables" );
        return FALSE;
    }

    foreach my $t (@{$this->{tablenames}}) {
        my $atts = $database->get_attributes_hash($this->{schemaname}, $t);

        if (! defined $atts) {
            ERROR( sprintf "Cannot get attributes of table %s.%s", $this->{schemaname}, $t );
            return FALSE;
        }

        $this->{tables}->{$t} = $atts;
    }

    $database->disconnect();

    return TRUE;
}

####################################################################################################
#                               Group: Request methods                                             #
####################################################################################################

=begin nd
Function: getCommandMakeJsons

=cut
sub getCommandMakeJsons {
    my $this = shift;
    my $bbox = join(" ", @_);

    my $dburl = sprintf "dbname=%s user=%s password=%s",
        $this->{dbname}, $this->{username}, $this->{password};

    my $cmd = "MakeJsons \"$bbox\" \"$dburl\"";

    foreach my $t (@{$this->{tablenames}}) {
        $cmd .= sprintf " %s.$t", $this->{schemaname};
    }

    return "$cmd\n";
}

####################################################################################################
#                                Group: Getters - Setters                                          #
####################################################################################################

# Function: getTables
sub getTables {
    my $this = shift;
    return $this->{tables};
}


1;
__END__
