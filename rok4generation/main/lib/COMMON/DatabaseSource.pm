# Copyright © (2011) Institut national de l'information
#                    géographique et forestière 
# 
# Géoportail SAV <contact.geoservices@ign.fr>
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

(see ROK4GENERATION/libperlauto/COMMON_DatabaseSource.png)

Stores parameters and builds WMS request.

Using:
    (start code)
    use COMMON::DatabaseSource;

    # Image width and height not defined

    # DatabaseSource object creation
    my $objDatabaseSource = COMMON::DatabaseSource->new({
        "db" => {
            "host" => "postgis.ign.fr",
            "port" => "5433",
            "database" => "geobase",
            "user" => "ign",
            "password" => "pwd"
        },
        "tables" => [
            {
                "schema" => "bdtopo",
                "native_name" => "limite_administrative_simp",
                "attributes" => "nom"
            },
            {
                "schema" => "bdtopo",
                "native_name" => "pai_sante_simp",
                "final_name" => "sante",
                "attributes" => "name",
                "filter" => "urgence = '1'"
            }
        ]
    });

    (end code)

Attributes:
    host - string - postgis server host
    port - integer - postgis server port
    dbname - string - postgis database name
    username - string - postgis server user
    password - string - postgis server user's password
    srs - string - data coordinates' system
    tables - hash - all informations about wanted tables
|       {
|           "schema_name.table" => {
|               "schema" => "schema_name",
|               "native_name" => "table",
|               "final_name" => "public_name",
|               "attributes" => "att1,att2",
|               "geometry" => {
|                   "type" => "MULTIPOLYGON",
|                   "name" => "the_geom",
|               },
|               "attributes_analysis" => {
|                   att1 => {
|                       "type" => "float",
|                       "count" => "1034",
|                       "min" => "1.02",
|                       "max" => "1654.9",
|                   },
|                   att2 => {
|                       "type" => "varchar",
|                       "count" => "2",
|                       "values" => ['yes', 'no'],
|                   }
|               }
|           }
|       }
=cut

################################################################################

package COMMON::DatabaseSource;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
use Data::Dumper;

use COMMON::Database;
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


# Constant: DEFAULT
# Define default values for attributes.
my %DEFAULT;

################################################################################

BEGIN {}
INIT {
    %DEFAULT = (
        port => 5432,
        schema => "public",
        attributes => ""
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
|   {
|       "srs" => "EPSG:4326",
|       "db" => {
|           "host" => "postgis.ign.fr",
|           "port" => "5433",
|           "database" => "geobase",
|           "user" => "ign",
|           "password" => "pwd"
|       },
|       "tables" => [
|           {
|               "schema" => "bdtopo",
|               "native_name" => "limite_administrative_simp",
|               "attributes" => "nom"
|           },
|           {
|               "schema" => "bdtopo",
|               "native_name" => "pai_sante_simp",
|               "final_name" => "sante",
|               "attributes" => "name",
|               "filter" => "urgence = '1'"
|           }
|       ]
|   }

See also:
    <_init>, <_load>
=cut
sub new {
    my $class = shift;
    my $params = shift;

    $class = ref($class) || $class;
    # IMPORTANT : if modification, think to update natural documentation (just above) and pod documentation (bottom)
    my $this = {
        host => undef,
        port => undef,
        dbname => undef,
        username => undef,
        password => undef,
        srs => undef,
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
|   {
|       "srs" => "EPSG:4326",
|       "db" => {
|           "host" => "postgis.ign.fr",
|           "port" => "5433",
|           "database" => "geobase",
|           "user" => "ign",
|           "password" => "pwd"
|       },
|       "tables" => [
|           {
|               "schema" => "bdtopo",
|               "native_name" => "limite_administrative_simp",
|               "attributes" => "nom"
|           },
|           {
|               "schema" => "bdtopo",
|               "native_name" => "pai_sante_simp",
|               "final_name" => "sante",
|               "attributes" => "name",
|               "filter" => "urgence = '1'"
|           }
|       ]
|   }
=cut
sub _init {
    my $this   = shift;
    my $params = shift;
    
    return FALSE if (! defined $params);
    # SRS
    if (! exists($params->{srs}) || ! defined $params->{srs}) {
        ERROR("Parameter 'srs' is required !");
        return FALSE ;
    }
    $this->{srs} = uc($params->{srs});

    # PORT    
    if (! exists($params->{db}->{port}) || ! defined ($params->{db}->{port})) {
        $this->{port} = $DEFAULT{port};
        INFO(sprintf "Default value for 'db.port' : %s", $this->{port});
    } else {
        if (int($params->{db}->{port}) <= 0) {
            ERROR("If 'db.port' is given, it must be strictly positive.");
            return FALSE ;
        }
        $this->{port} = int($params->{db}->{port});
    }

    # Other parameters are mandatory
    # HOST
    if (! exists($params->{db}->{host}) || ! defined ($params->{db}->{host})) {
        ERROR("Parameter 'db.host' is required !");
        return FALSE ;
    }
    $this->{host} = $params->{db}->{host};
    # DATABASE
    if (! exists($params->{db}->{database}) || ! defined ($params->{db}->{database})) {
        ERROR("Parameter 'db.database' is required !");
        return FALSE ;
    }
    $this->{dbname} = $params->{db}->{database};
    # USERNAME
    if (! exists($params->{db}->{user}) || ! defined ($params->{db}->{user})) {
        ERROR("Parameter 'db.user' is required !");
        return FALSE ;
    }
    $this->{username} = $params->{db}->{user};
    # PASSWORD
    if (! exists($params->{db}->{password}) || ! defined ($params->{db}->{password})) {
        ERROR("Parameter 'db.password' is required !");
        return FALSE ;
    }
    $this->{password} = $params->{db}->{password};

    # TABLES
    if (! exists($params->{tables}) || ! defined ($params->{tables})) {
        ERROR("Parameter 'tables' is required !");
        return FALSE ;
    }
    if (! ref($params->{tables}) eq "ARRAY") {
        ERROR("Parameter 'tables' have to be an array");
        return FALSE ;
    }

    foreach my $t (@{$params->{tables}}) {
        if (! exists($t->{native_name}) || ! defined ($t->{native_name})) {
            ERROR("Parameter 'native_name' is required for a table !");
            return FALSE ;
        }
        if (! exists($t->{final_name}) || ! defined ($t->{final_name})) {
            $t->{final_name} = $t->{native_name};
        }
        if (! exists($t->{schema}) || ! defined ($t->{schema})) {
            $t->{schema} = $DEFAULT{schema};
        }
        if (! exists($t->{attributes}) || ! defined ($t->{attributes})) {
            $t->{attributes} = $DEFAULT{attributes};
        }
        if (! exists($t->{filter}) || ! defined ($t->{filter})) {
            $t->{filter} = "";
        }

        $this->{tables}->{sprintf ("%s.%s", $t->{schema}, $t->{native_name})} = $t;
    }

    if (scalar(keys %{$this->{tables}}) == 0) {
        ERROR("Parameter 'tables' contains no table");
        return FALSE ;
    }
    
    return TRUE;
}

=begin nd
Function: _load

Analyse tables and attributes connecting to the database
=cut
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

    while ( my ($table, $hash) = each(%{$this->{tables}})) {
        DEBUG("Récupération d'informations sur $table");
        if (! $database->is_table_exist($hash->{schema}, $hash->{native_name})) {
            ERROR("Table $table does not exist");
            return FALSE;
        }

        my ($geomname, $geomtype) = $database->get_geometry_column($hash->{schema}, $hash->{native_name});
        if (! defined $geomname) {
            ERROR("No geometry column in table $table");
            return FALSE;
        }

        $hash->{geometry} = {
            type => $geomtype,
            name => $geomname
        };

        my $native_atts = $database->get_attributes_hash($hash->{schema}, $hash->{native_name});
        
        if ($hash->{attributes} eq "*") {
            $hash->{attributes} = join(",", keys(%{$native_atts}));
        }

        my @asked_atts = split(/,/, $hash->{attributes});

        $hash->{attributes_analysis} = {};
        foreach my $a (@asked_atts) {
            if ($a eq "") {next;}

            if (! exists $native_atts->{$a}) {
                ERROR("Attribute $a is not present in table $table");
                return FALSE;
            }

            if ($a eq $geomname) {next;}

            $hash->{attributes_analysis}->{$a} = {
                type => $native_atts->{$a}
            };

            my $count = $database->get_distinct_values_count($hash->{schema}, $hash->{native_name}, $a);
            $hash->{attributes_analysis}->{$a}->{count} = $count;

            my @numerics = ("integer", "real", "double precision", "numeric");
            if (defined COMMON::Array::isInArray($native_atts->{$a}, @numerics)) {
                my ($min, $max) = $database->get_min_max_values($hash->{schema}, $hash->{native_name}, $a);
                $hash->{attributes_analysis}->{$a}->{min} = $min;
                $hash->{attributes_analysis}->{$a}->{max} = $max;
            }

            elsif ($count <= 50) {
                my @distincts = $database->get_distinct_values($hash->{schema}, $hash->{native_name}, $a);
                $hash->{attributes_analysis}->{$a}->{values} = \@distincts;
            }
        }

    }

    $database->disconnect();

    return TRUE;
}

####################################################################################################
#                               Group: Request methods                                             #
####################################################################################################

=begin nd
Function: getDatabaseInfos

Return database url ("host=postgis.ign.fr dbname=bdtopo user=ign password=PWD port=5432") and datasource projection
=cut
sub getDatabaseInfos {
    my $this = shift;

    my $url = sprintf "host=%s dbname=%s user=%s password=%s port=%s",
        $this->{host}, $this->{dbname}, $this->{username}, $this->{password}, $this->{port};

    return ($url, $this->{srs});
}

=begin nd
Function: getSqlExports

Return a string array : SQL request and associated destination table name
=cut
sub getSqlExports {
    my $this = shift;

    my @sqls;

    while (my ($table, $hash) = each(%{$this->{tables}})) {

        my $sql = "";
        if (scalar(keys %{$hash->{attributes_analysis}}) != 0) {
            $sql = sprintf "SELECT %s,%s FROM $table", 
                join(",", keys(%{$hash->{attributes_analysis}})), 
                $hash->{geometry}->{name};
        } else {
            # Cas où l'on ne veut aucun attribut sauf la géométrie
            $sql = sprintf "SELECT %s FROM $table",
                $hash->{geometry}->{name};
        }
            
        if ($hash->{filter} ne "") {
            $sql .= sprintf " WHERE %s", $hash->{filter};
        }

        push(@sqls, $sql, $hash->{final_name});
    }

    return @sqls;
}

####################################################################################################
#                                Group: Getters - Setters                                          #
####################################################################################################

# Function: getTables
sub getTables {
    my $this = shift;
    return $this->{tables};
}


# Function: getTablesNumber
sub getTablesNumber {
    my $this = shift;
    return scalar(keys %{$this->{tables}});
}

1;
__END__
