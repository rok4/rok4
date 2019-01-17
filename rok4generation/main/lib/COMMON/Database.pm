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
File: Database.pm

Class: COMMON::Database

Allow to request a PostgreSQL database.

Using:
    (start code)
    use COMMON::Database;

    # DatabaseSource object creation
    my $objDatabase = COMMON::Database->new( $dbname, $host, $port, $username, $password );
    (end code)

Attributes:
    dbname - string - database name
    host - string - PostgreSQL server host
    port - integer - PostgreSQL server port
    username - string - PostgreSQL server user
    password - string - PostgreSQL server user's password

    connection - DBI database - PostgreSQL connection, use to execute requests

    current_results - DBI statement - Use to manage big results
=cut

################################################################################

package COMMON::Database;

use strict;
use warnings;

use Cwd;
use DBI;
use Data::Dumper;
use Log::Log4perl qw(:easy);

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

# Cache for requests
my $CACHE;

################################################################################

BEGIN {}
INIT {}
END {}

####################################################################################################
#                                        Group: Constructors                                       #
####################################################################################################

=begin nd
Constructor: new

Database constructor. Bless an instance. Connect the DBI object

Parameters (list):
    dbname - string - database name
    host - string - PostgreSQL server host
    port - integer - PostgreSQL server port
    username - string - PostgreSQL server user
    password - string - PostgreSQL server user's password

=cut
sub new {
    my $class = shift;

    my ( $dbname, $host, $port, $username, $password ) = @_;

    if (! defined $dbname) {
        ERROR("Parameters de connexion à fournir");
        return undef;   
    }

    $class = ref($class) || $class;

    my $this = {
        id => "$dbname,$host,$port,$username,$password",

        dbname => $dbname,
        host => $host,
        port => $port,
        username => $username,
        password => $password,

        connection => undef,

        current_results => undef
    };

    bless($this, $class);

    $this->{connection} = DBI->connect(
        "dbi:Pg:dbname=$dbname;host=$host;port=$port", $username, $password,
        { AutoCommit => 1, RaiseError => 0, PrintError => 0, pg_enable_utf8 => 1 }
    ) or do {
        ERROR("Impossible de se connecter à la base :");
        ERROR("\t nom : $dbname");
        ERROR("\t url : $host:$port");
        ERROR("\t utilisateur : $username");
        return undef;        
    };

    DEBUG("Connexion à la base OK");
    DEBUG("\t nom : $dbname");
    DEBUG("\t url : $host:$port");
    DEBUG("\t utilisateur : $username");

    return $this;
}

=begin nd
Function: disconnect

Return
    0 if success, 1 if failure
=cut
sub disconnect {
    my $this = shift;

    if ( $this->{connection}->{AutoCommit} == 0 ) {
        WARN("Open transaction, we rollback it before disconnection");
        $this->rollback_transaction();
    }

    $this->{connection}->disconnect() or return 1;

    return 0;
}

####################################################################################################
#                                   Group: Gestion des transactions                                #
####################################################################################################

=begin nd
Function: start_transaction

Commit already open transaction.

Return
    TRUE if success, FALSE if failure
=cut
sub start_transaction {
    my $this = shift;

    if ( $this->{connection}->{AutoCommit} == 0 ) {
        WARN("A transaction is already open, we commit it");
        $this->commit_transaction();
    }

    $this->{connection}->begin_work() or do {
        ERROR("Cannot open a transaction");
        return FALSE;
    };

    return TRUE;
}

=begin nd
Function: commit_transaction

Return
    TRUE if success, FALSE if failure
=cut
sub commit_transaction {
    my $this = shift;

    if ( $this->{connection}->{AutoCommit} == 1 ) {
        WARN("No open transaction to commit");
        return TRUE;
    }

    $this->{connection}->commit() or do {
        ERROR("Cannot commit the open transaction");
        return FALSE;
    };

    return TRUE;
}

=begin nd
Function: rollback_transaction

Return
    TRUE if success, FALSE if failure
=cut
sub rollback_transaction {
    my $this = shift;

    if ( $this->{connection}->{AutoCommit} == 1 ) {
        WARN("No open transaction to rollback");
        return TRUE;
    }

    $this->{connection}->rollback() or do {
        ERROR("Cannot rollback the open transaction");
        return FALSE;
    };

    DEBUG("Transaction rollbackée");
    return FALSE;
}

####################################################################################################
#                                      Group: Requests executors                                   #
####################################################################################################

=begin nd
Function: select_one_row

Parameter (list):
    sql - string - Request to execute

Return
    If success, first result as array, undef if filaure or no result
=cut
sub select_one_row {
    my $this = shift;
    my $sql = shift;

    my $sth = $this->{connection}->prepare($sql) or do {
        ERROR("Cannot prepare the request SELECT ONE ROW : $sql");
        return undef;
    };

    $sth->execute() or do {
        ERROR("Cannot execute the request SELECT ONE ROW : $sql");
        return undef;
    };

    my @row = $sth->fetchrow_array();

    if (defined $sth->err) {
        ERROR("Problème lors de la récupération des résultats de la requête SELECT ONE ROW : $sql");
        return undef;
    }

    $sth->finish() or do {
        ERROR("Cannot close the request SELECT ONE ROW : $sql");
        return undef;
    };

    return @row;
}

=begin nd
Function: select_all_row

Parameter (list):
    sql - string - Request to execute

Return
    Une référence sur un tableau de référence à des tableaux correspondant à tous les résultats en cas de succès, undef en cas d'échec ou d'absence de résultat
=cut
sub select_all_row {
    my $this = shift;
    my $sql = shift;

    my $sth = $this->{connection}->prepare($sql) or do {
        ERROR("Cannot prepare the request SELECT ALL ROW : $sql");
        return undef;
    };

    $sth->execute() or do {
        ERROR("Cannot execute the request SELECT ALL ROW : $sql");
        return undef;
    };

    my $rows = $sth->fetchall_arrayref();

    if (defined $sth->err) {
        ERROR("Cannot get results of the request SELECT ALL ROW : $sql");
        return undef;
    }

    $sth->finish() or do {
        ERROR("Cannot close the request SELECT ALL ROW : $sql");
        return undef;
    };

    return $rows;
}

=begin nd
Function: select_many_row

Parameter (list):
    sql - string - Request to execute

Return
    0 if success and the result is memorized, 1 otherwise
=cut
sub select_many_row {
    my $this = shift;
    my $sql = shift;

    if ( defined $this->{current_results} ) {
        $this->{current_results}->finish() or do {
            ERROR("Cannot close current memorizedresult SELECT MANY ROW");
            return 1;
        };
    }

    $this->{current_results} = $this->{connection}->prepare($sql) or do {
        ERROR("Cannot prepare the request SELECT MANY ROW : $sql");
        return 1;
    };


    $this->{current_results}->execute() or do {
        ERROR("Cannot execute the request SELECT MANY ROW : $sql");
        return 1;
    };

    return 0;
}

=begin nd
Function: next_row

Return
    Next line of memorized results as array, undef if failure or no result
=cut
sub next_row {
    my $this = shift;

    if ( defined $this->{current_results} ) {
        my @raw = $this->{current_results}->fetchrow_array();

        if (defined $this->{current_results}->err ) {
            DEBUG("Cannot read next line in the memorized results : " . $this->{current_results}->errstr);
            return undef;
        }

        return @raw;
    }
    else {
        WARN("No current memorized results");
        return undef;
    }
}

=begin nd
Function: next_row

Return
    Next line of memorized results as hash reference, undef if failure or no result
=cut
sub next_row_as_hashref {
    my $this = shift;

    if ( defined $this->{current_results} ) {
        my $refHash = $this->{current_results}->fetchrow_hashref();

        if (defined $this->{current_results}->err ) {
            DEBUG("Problème de lecture dans les résultats de la requête : " . $this->{current_results}->errstr);
            return undef;
        }

        return $refHash;
    }
    else {
        WARN("No current memorized results");
        return undef;
    }
}

=begin nd
Function: execute_without_return

Parameter (list):
    sql - string - Request to execute

Return
    0 if success, 1 if failure
=cut
sub execute_without_return {
    my $this = shift;
    my $sql = shift;

    my $sth = $this->{connection}->prepare($sql) or do {
        ERROR("Cannot prepare the requestSANS RETOUR : $sql");
        return 1;
    };

    $sth->execute() or do {
        ERROR("Cannot execute the requestSANS RETOUR : $sql");
        return 1;
    };

    $sth->finish() or do {
        ERROR("Cannot close the request SANS RETOUR : $sql");
        return 1;
    };

    return 0;
}

=begin nd
Function: run_sql_dump

Parameters (list):
    schema_name - string - Schema name in which the file have to be execute
    sql_file - string - SQL script to execute

Return
    0 if success, 1 if failure
=cut
sub run_sql_dump {
    my $this = shift;
    my $schema_name = shift;
    my $sql_file = shift;

    $schema_name = lc $schema_name;

    local $ENV{"PGDATABASE"} = $this->{dbname};
    local $ENV{"PGHOST"}     = $this->{host};
    local $ENV{"PGPORT"}     = $this->{port};
    local $ENV{"PGUSER"}     = $this->{username};
    local $ENV{"PGPASSWORD"} = $this->{password};

    my $cmd_psql = "PGOPTIONS='-c search_path=$schema_name,public' psql -v ON_ERROR_STOP=1 -q -f $sql_file";

    DEBUG("Call : " . $cmd_psql);

    my @logs = `$cmd_psql`;

    if ( $? != 0 ) {
        ERROR("Cannot execute the SQL script : " . $sql_file);
        ERROR(Dumper(\@logs));
        return 1;
    }

    return 0;
}

####################################################################################################
#                                          Group: Tests                                            #
####################################################################################################

=begin nd
Function: is_schema_exist

Parameter (list):
    schema_name - string - Schema to test

Return
    TRUE if schema exists, FALSE otherwise
=cut
sub is_schema_exist {
    my $this = shift;
    my $schema_name = shift;

    my $sql = "SELECT count(*) FROM pg_namespace WHERE nspname = lower('$schema_name')";
    ( my $schema_exist ) = $this->select_one_row($sql);
    if ( $schema_exist != 0 ) {
        return TRUE;
    } else {
        return FALSE;
    }
}

=begin nd
Function: is_table_exist

Parameters (list):
    schema_name - string - Schema where to test the table
    table_name - string - Table to tests

Return
    TRUE if table exists, FALSE otherwise
=cut
sub is_table_exist {
    my $this = shift;
    my $schema_name = shift;
    my $table_name = shift;

    my $sql = "SELECT count(*) FROM pg_tables WHERE schemaname = '$schema_name' AND tablename = '$table_name';";
    ( my $table_exist ) = $this->select_one_row($sql);
    if ( $table_exist != 0 ) {
        return TRUE;
    }

    $sql = "SELECT count(*) FROM pg_views WHERE schemaname = '$schema_name' AND viewname = '$table_name';";
    ( $table_exist ) = $this->select_one_row($sql);
    if ( $table_exist != 0 ) {
        return TRUE;
    }

    $sql = "SELECT count(*) FROM pg_matviews WHERE schemaname = '$schema_name' AND matviewname = '$table_name';";
    ( $table_exist ) = $this->select_one_row($sql);
    if ( $table_exist != 0 ) {
        return TRUE;
    }

    return FALSE;
}

=begin nd
Function: is_table_empty

Parameters (list):
    schema_name - string - Schema where to test the table
    table_name - string - Table to tests

Return
    TRUE if table is empty, FALSE otherwise
=cut
sub is_table_empty {
    my $this = shift;
    my $schema_name = shift;
    my $table_name = shift;

    my $sql = "SELECT count(*) FROM (SELECT * FROM $schema_name.$table_name LIMIT 1) AS tmp;";
    ( my $table_empty ) = $this->select_one_row($sql);
    if ( $table_empty == 0 ) {
        return TRUE;
    } else {
        return FALSE;
    }
}

=begin nd
Function: is_column_exist

Parameters (list):
    schema_name - string - Schema where to test the table's column
    table_name - string - Table where to tests the column
    column_name - string - Column to test

Return
    TRUE if column exists, FALSE otherwise
=cut
sub is_column_exist {
    my $this = shift;
    my $schema_name = shift;
    my $table_name = shift;
    my $column_name = shift;

    my $sql = "SELECT count(*) FROM information_schema.columns WHERE table_schema = '$schema_name' AND table_name = '$table_name' AND column_name = '$column_name';";
    ( my $column_exist ) = $this->select_one_row($sql);
    if ( $column_exist != 0 ) {
        return TRUE;
    } else {
        return FALSE;
    }
}

####################################################################################################
#                                     Group: Action sur les schémas                                #
####################################################################################################

=begin nd
Function: create_schema

Parameter (list):
    schema_name - string - Schema to create

Return
    0 if schema is created, 1 otherwise
=cut
sub create_schema {
    my $this = shift;
    my $schema_name = shift;

    $schema_name = lc $schema_name;

    my $sql = "CREATE SCHEMA $schema_name;";

    return $this->execute_without_return($sql);
}

=begin nd
Function: add_column_to_table

Parameters (list):
    schema_name - string - Schema in which the column have to be created
    table_name - string - Table in which the column have to be created
    attribute_name - string - Column name to create
    attribute_type - string - Column type to create
    default_value - string - New column default value (optionnal)

Return
    0 if column is created, 1 otherwise
=cut
sub add_column_to_table {
    my $this = shift;
    my $schema_name = shift;
    my $table_name = shift;
    my $attribute_name = shift;
    my $attribute_type = shift;
    my $default_value = shift;

    if ( ! defined $schema_name || ! defined $table_name || ! defined $attribute_name || ! defined $attribute_type ) {
        ERROR("Missing parameters to create a new column");
        return 1;
    }

    my $req_sql = "ALTER TABLE $schema_name.$table_name ADD COLUMN $attribute_name $attribute_type";

    if (defined $default_value && $default_value ne "") {
        $req_sql .= " DEFAULT '$default_value'";
    }

    $req_sql .= ";";

    return $this->execute_without_return($req_sql);
}

=begin nd
Function: drop_schema_and_inherits

Parameter (list):
    schema_name - string - Schema to remove, including inherits

Return
    0 if schema is droped, 1 otherwise
=cut
sub drop_schema_and_inherits {

    my $this = shift;
    my $schema_name = shift;

    if ( ! defined $schema_name ) {
        ERROR("Schema to remove is missing");
        return 1;
    }

    # Is the schema exist in the database ?
    if ( $this->is_schema_exist() ) {
        WARN("Schema to remove does not exists");
        return 0;
    }

    $this->start_transaction();

    # Suppression propre des héritages
    my $sql = "SELECT DISTINCT f_table_name FROM geometry_columns WHERE f_table_schema = '$schema_name';";
    ( my @geometricTables ) = @{ $this->select_all_row($sql) };

    DEBUG(scalar @geometricTables . " tables in the schema to remove");

    foreach my $table (@geometricTables) {

        # Récupération des tables héritant de table dans schema
        $sql = sprintf 
            "SELECT nc.nspname||'.'||c.relname as name FROM pg_inherits ".
            "JOIN pg_class AS c ON (inhrelid=c.oid) ".
            "JOIN pg_class as p ON (inhparent=p.oid) ".
            "JOIN pg_catalog.pg_namespace nc ON nc.oid = c.relnamespace ".
            "JOIN pg_catalog.pg_namespace np ON np.oid = p.relnamespace ".
            "WHERE np.nspname='%s' AND p.relname='%s';", $schema_name, $table->[0];

        ( my @linkedTables ) = @{ $this->select_all_row($sql) };

        INFO((sprintf "%s tables liked to table %s", scalar @linkedTables, $table->[0]));

        for my $subtable (@linkedTables) {

            $sql = sprintf "ALTER TABLE %s NO INHERIT %s.%s", $subtable->[0], $schema_name, $table->[0];

            if ( $this->execute_without_return($sql) ) {
                ERROR(sprintf "Cannot remove the link between %s and %s", $table->[0], $subtable->[0]);
                $this->rollback_transaction();
                return 1;
            }
        }
    }

    # Delete schéma
    if ( $this->drop_schema( $schema_name, "true" ) ) {
        ERROR("Cannot remove the schema " . $schema_name);
        $this->rollback_transaction();
        return 1;
    }

    $this->commit_transaction();

    return 0;
}

=begin nd
Function: drop_schema

Parameters (list):
    schema_name - string - Schema to remove
    cascade - string - If 'true', drop cascade is called (false if not provided)

Return
    0 if schema is droped, 1 otherwise
=cut
sub drop_schema {
    my $this = shift;
    my $schema_name = shift;
    my $cascade = shift;

    $schema_name = lc $schema_name;

    if ( ! defined $cascade ) {
        $cascade = "false";
    }

    my $geometries = $this->select_all_row( "SELECT f_table_name, f_geometry_column FROM geometry_columns WHERE f_table_schema = '$schema_name';");
    if ( defined $geometries ) {
        foreach my $row ( @{$geometries} ) {
            my $geom_table_name;
            my $geom_geometry_column;
            ( $geom_table_name, $geom_geometry_column ) = @{$row};

            $this->execute_without_return( "SELECT DropGeometryColumn('$schema_name','$geom_table_name','$geom_geometry_column');" );
        }
    }

    my $sql = "DROP SCHEMA $schema_name";

    if ( $cascade && $cascade eq "true" ) {
        $sql = $sql . " CASCADE";
    }

    return $this->execute_without_return( $sql );
}

####################################################################################################
#                                     Group: Modification des droits                               #
####################################################################################################

=begin nd
Function: revoke_schema_permissions

Parameters (list):
    schema_name - string - Schema for which rights have to be removed
    username - string - User who loses rights
    permissions - string - Rights to lose

Return
    0 if success, 1 otherwise
=cut
sub revoke_schema_permissions {
    my $this = shift;
    my $schema_name = shift;
    my $username = shift;
    my $permissions = shift;

    $schema_name = lc $schema_name;

    my $sql = "REVOKE $permissions ON SCHEMA $schema_name FROM $username;";

    return $this->execute_without_return( $sql );
}

=begin nd
Function: grant_schema_permissions

Parameters (list):
    schema_name - string - Schema for which rights have to be added
    username - string - User who wins rights
    permissions - string - Rights to win

Return
    0 if success, 1 otherwise
=cut
sub grant_schema_permissions {
    my $this = shift;
    my $schema_name = shift;
    my $username = shift;
    my $permissions = shift;

    $schema_name = lc $schema_name;

    my $sql = "GRANT $permissions ON SCHEMA $schema_name TO $username;";

    return $this->execute_without_return( $sql );
}

=begin nd
Function: set_permissions_on_tables_from_schema

Parameters (list):
    schema_name - string - Schema for which rights have to be set for all tables
    username - string - User concerned by rights
    permissions - string - Rights to set. If undefined, all rights are removed for all tables in the schema

Return
    0 if success, 1 otherwise
=cut
sub set_permissions_on_tables_from_schema {
    my $this = shift;
    my $schema_name = shift;
    my $username = shift;
    my $permissions = shift;
    
    $schema_name = lc $schema_name;

    my @table_list = $this->get_tables_array($schema_name);

    foreach my $table_name ( @table_list ) {
        $this->revoke_all_permissions_on_table( $schema_name, $table_name, $username );
        if ( defined $permissions ) {
            my $return = $this->grant_table_permissions( $schema_name, $table_name, $username , $permissions);
            if ( $return != 0 ) {
                ERROR(" Erreur lors de la mise de droits sur la table $schema_name.$table_name");
                return 1;
            }
        }
    }

    return 0;
}

=begin nd
Function: revoke_all_permissions_on_table

Parameters (list):
    schema_name - string - Schema for which rights have to be removed
    table_name - string - Table for which rights have to be removed
    username - string - User who loses rights

Return
    0 if success, 1 otherwise
=cut
sub revoke_all_permissions_on_table {
    my $this = shift;
    my $schema_name = shift;
    my $table_name = shift;
    my $username = shift;

    $schema_name = lc $schema_name;

    my $sql = "REVOKE ALL PRIVILEGES ON $schema_name.$table_name FROM $username;";

    return $this->execute_without_return($sql);
}

=begin nd
Function: grant_table_permissions

Parameters (list):
    schema_name - string - Schema for which rights have to be added
    table_name - string - Table for which rights have to be added
    username - string - User who wins rights
    permissions - string - Rights to win

Return
    0 if success, 1 otherwise
=cut
sub grant_table_permissions {
    my $this = shift;
    my $schema_name = shift;
    my $table_name = shift;
    my $username = shift;
    my $permissions = shift;

    $schema_name = lc $schema_name;

    my $sql = "GRANT $permissions ON $schema_name.$table_name TO $username;";

    return $this->execute_without_return($sql);
}

####################################################################################################
#                                 Group: Récupération d'informations                               #
####################################################################################################

=begin nd
Function: get_geometry_column

Parameter (list):
    schema_name - string - Schema in which we want to know the geometry column
    table_name - string - Table for which we want to know the geometry column

Return (array)
    Geometry coumn name and its type, undef if no geometry column for this table
=cut
sub get_geometry_column {
    my $this = shift;
    my $schema_name = shift;
    my $table_name = shift;

    my $sql = "SELECT f_geometry_column, type FROM geometry_columns WHERE f_table_schema = '$schema_name' AND f_table_name = '$table_name';";

    my @line = $this->select_one_row($sql);

    if (scalar(@line) == 0) {
        return (undef, undef);
    }

    return ($line[0], $line[1]);
}

=begin nd
Function: get_attributes_list

Parameter (list):
    schema_name - string - Schema in which we want the attribute list
    table_name - string - Table for which we want the attribute list

Return
    Attributes list as a string (attributes separated by comma), undef if failure
=cut
sub get_attributes_list {
    my $this = shift;
    my $schema_name = shift;
    my $table_name = shift;

    my $sql = "SELECT a.attname FROM pg_attribute a JOIN pg_class t on a.attrelid = t.oid JOIN pg_namespace s on t.relnamespace = s.oid WHERE a.attnum > 0  AND NOT a.attisdropped AND t.relname = '$table_name' AND s.nspname = '$schema_name' ORDER BY a.attnum;";

    my $atts = $this->select_all_row($sql);
    if ( ! defined $atts) {
        ERROR("Cannot list attributes for the table $schema_name.$table_name");
        return undef;
    }

    my @array;

    foreach my $att (@{$atts}) {
        push @array, $att->[0];
    }

    return join(',',@array);
}

=begin nd
Function: get_attributes_hash

Parameter (list):
    schema_name - string - Schema in which we want the attribute list
    table_name - string - Table for which we want the attribute list

Return
    Attributes list as a hash reference, undef if failure
=cut
sub get_attributes_hash {
    my $this = shift;
    my $schema_name = shift;
    my $table_name = shift;

    my $sql = "SELECT a.attname, pg_catalog.format_type(a.atttypid, a.atttypmod) FROM pg_attribute a JOIN pg_class t on a.attrelid = t.oid JOIN pg_namespace s on t.relnamespace = s.oid WHERE a.attnum > 0  AND NOT a.attisdropped AND t.relname = '$table_name' AND s.nspname = '$schema_name' ORDER BY a.attnum;";

    my $atts = $this->select_all_row($sql);
    if ( ! defined $atts) {
        ERROR("Cannot list attributes for the table $schema_name.$table_name");
        return undef;
    }

    my $hash = {};

    foreach my $att (@{$atts}) {
        $hash->{$att->[0]} = $att->[1];
    }

    return $hash;
}

=begin nd
Function: get_tables_array

Parameter (list):
    schema_name - string - Schema for which we want to list the tables

Return
    Table list as array, undef if failure
=cut
sub get_tables_array {
    my $this = shift;
    my $schema_name = shift;

    my $sql = "SELECT tablename FROM pg_tables WHERE schemaname='$schema_name';";

    my $tables = $this->select_all_row($sql);
    if ( ! defined $tables) {
        ERROR("Cannot list tables of schema $schema_name");
        return undef;
    }

    my @array;

    foreach my $val (@{$tables}) {
        push @array, $val->[0];
    }

    return @array;
}

=begin nd
Function: get_min_max_values

Parameter (list):
    schema_name - string - Schema in which determine min and max
    table_name - string - Table in which determine min and max
    att_name - string - Attribute for which we want to determine min and max

Return
    An array (min, max)
=cut
sub get_min_max_values {
    my $this = shift;
    my $schema_name = shift;
    my $table_name = shift;
    my $att_name = shift;

    if (exists $CACHE->{$this->{id}}->{get_min_max_values}->{$schema_name}->{$table_name}->{$att_name}) {
        return @{$CACHE->{$this->{id}}->{get_min_max_values}->{$schema_name}->{$table_name}->{$att_name}};
    }

    my $sql = sprintf "SELECT min($att_name), max($att_name) FROM $schema_name.$table_name;";

    my @line = $this->select_one_row($sql);

    $CACHE->{$this->{id}}->{get_distinct_values_count}->{$schema_name}->{$table_name}->{$att_name} = [$line[0], $line[1]];
    return ($line[0], $line[1]);

}

=begin nd
Function: get_distinct_values_count

Parameter (list):
    schema_name - string - Schema in which count distinct values
    table_name - string - Table in which count distinct values
    att_name - string - Attribute for which we want to count distinct values

Return
    Distinct values count
=cut
sub get_distinct_values_count {
    my $this = shift;
    my $schema_name = shift;
    my $table_name = shift;
    my $att_name = shift;

    if (exists $CACHE->{$this->{id}}->{get_distinct_values_count}->{$schema_name}->{$table_name}->{$att_name}) {
        return $CACHE->{$this->{id}}->{get_distinct_values_count}->{$schema_name}->{$table_name}->{$att_name};
    }

    my $sql = sprintf "SELECT count(*) FROM (SELECT DISTINCT $att_name FROM $schema_name.$table_name) as tmp;";

    my @line = $this->select_one_row($sql);

    $CACHE->{$this->{id}}->{get_distinct_values_count}->{$schema_name}->{$table_name}->{$att_name} = $line[0];
    return $line[0];
}


=begin nd
Function: get_distinct_values

Parameter (list):
    schema_name - string - Schema in which list distinct values
    table_name - string - Table in which list distinct values
    att_name - string - Attribute for which we want to list distinct values

Return
    Distinct values in an array, an empty array if failure
=cut
sub get_distinct_values {
    my $this = shift;
    my $schema_name = shift;
    my $table_name = shift;
    my $att_name = shift;

    if (exists $CACHE->{$this->{id}}->{get_distinct_values}->{$schema_name}->{$table_name}->{$att_name}) {
        return @{$CACHE->{$this->{id}}->{get_distinct_values}->{$schema_name}->{$table_name}->{$att_name}};
    }

    my $sql = sprintf "SELECT DISTINCT $att_name FROM $schema_name.$table_name;";

    my $att_values = $this->select_all_row($sql);
    if ( ! defined $att_values) {
        ERROR("Cannot list distinct values of $att_name in $schema_name.$table_name");
        return ();
    }

    my @array;

    foreach my $val (@{$att_values}) {
        if (defined $val->[0]) {
            push @array, $val->[0];
        }
    }

    $CACHE->{$this->{id}}->{get_distinct_values}->{$schema_name}->{$table_name}->{$att_name} = \@array;
    return @array;
}

=begin nd
Function: get_schema_size

Parameter (list):
    schema_name - string - Schema for which we want size

Return
    size of the schema, -1 if failure
=cut
sub get_schema_size {
    my $this = shift;
    my $schema_name = shift;

    my $sql = "SELECT sum(t.table_size) FROM (
        SELECT pg_total_relation_size(pg_catalog.pg_class.oid) as table_size FROM pg_catalog.pg_class
        JOIN pg_catalog.pg_namespace ON relnamespace = pg_catalog.pg_namespace.oid
        WHERE pg_catalog.pg_namespace.nspname = '$schema_name') t";

    my @row  = $this->select_one_row($sql);
    my $size = $row[0];
    if ( $size eq "" ) {
        ERROR("Cannot get size of schema $schema_name");
        return -1;
    }

    chomp($size);

    return $size;
}

1;
__END__

