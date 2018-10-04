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

Request a PostgreSQL database.

Using:
    (start code)
    use COMMON::Database;

    # DatabaseSource object creation
    my $objDatabase = COMMON::Database->new( $dbname, $host, $port, $username, $password );
    (end code)

Attributes:
    dbname - string - 
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

################################################################################

BEGIN {}
INIT {}
END {}

####################################################################################################
#                                        Group: Constructors                                       #
####################################################################################################

sub new {
    my $class = shift;

    my ( $dbname, $host, $port, $username, $password ) = @_;

    if (! defined $dbname) {
        ERROR("Paramètres de connexion à fournir");
        return undef;   
    }

    $class = ref($class) || $class;

    my $this = {

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

    INFO("Connexion à la base OK");
    INFO("\t nom : $dbname");
    INFO("\t url : $host:$port");
    INFO("\t utilisateur : $username");

    return $this;
}

sub disconnect {
    my $this = shift;

    if ( $this->{connection}->{AutoCommit} == 0 ) {
        WARN("Une transaction est en cours au moment de la déconnexion : on la rollbacke");
        $this->rollback_transaction();
    }

    DEBUG("Deconnexion demandée");
    $this->{connection}->disconnect() or return 1;
    DEBUG(" Deconnexion effectuée");

    return 0;
}

####################################################################################################
#                                   Group: Gestion des transactions                                #
####################################################################################################

sub start_transaction {
    my $this = shift;

    if ( $this->{connection}->{AutoCommit} == 0 ) {
        WARN("Une transaction est déjà en cours. Commit de cette dernière");
        $this->commit_transaction();
    }

    $this->{connection}->begin_work() or do {
        ERROR("Impossible de démarrer une transaction");
        return FALSE;
    };

    DEBUG("Transaction démarrée");
    return TRUE;
}

sub commit_transaction {
    my $this = shift;

    if ( $this->{connection}->{AutoCommit} == 1 ) {
        WARN("Aucune transaction en cours. Commit annulé");
        return TRUE;
    }

    $this->{connection}->commit() or do {
        ERROR("Impossible de commiter la transaction en cours");
        return FALSE;
    };

    DEBUG("Transaction commitée");
    return TRUE;
}

sub rollback_transaction {
    my $this = shift;

    if ( $this->{connection}->{AutoCommit} == 1 ) {
        WARN("Aucune transaction en cours. Rollback annulé");
        return TRUE;
    }

    $this->{connection}->rollback() or do {
        ERROR("Impossible de rollbacker la transaction en cours");
        return FALSE;
    };

    DEBUG("Transaction rollbackée");
    return FALSE;
}

####################################################################################################
#                                      Group: Exécuteurs de requêtes                               #
####################################################################################################

=begin nd
Function: select_one_row

Paramètre(s) (liste):
    sql - string - Requête à exécuter

Retourne
    Un tableau correspondant à la première ligne en cas de succès, undef en cas d'échec ou d'absence de résultat
=cut
sub select_one_row {
    my $this = shift;
    my $sql = shift;

    my $sth = $this->{connection}->prepare($sql) or do {
        ERROR("Impossible de préparer la requête SELECT ONE ROW : $sql");
        return undef;
    };

    $sth->execute() or do {
        ERROR("Impossible d'exécuter la requête SELECT ONE ROW : $sql");
        return undef;
    };

    my @row = $sth->fetchrow_array();

    if (defined $sth->err) {
        ERROR("Problème lors de la récupération des résultats de la requête SELECT ONE ROW : $sql");
        return undef;
    }

    $sth->finish() or do {
        ERROR("Impossible de clore la requête SELECT ONE ROW : $sql");
        return undef;
    };

    return @row;
}

=begin nd
Function: select_all_row

Paramètre(s) (liste):
    sql - string - Requête à exécuter

Retourne
    Une référence sur un tableau de référence à des tableaux correspondant à tous les résultats en cas de succès, undef en cas d'échec ou d'absence de résultat
=cut
sub select_all_row {
    my $this = shift;
    my $sql = shift;

    my $sth = $this->{connection}->prepare($sql) or do {
        ERROR("Impossible de préparer la requête SELECT ALL ROW : $sql");
        return undef;
    };

    $sth->execute() or do {
        ERROR("Impossible d'exécuter la requête SELECT ALL ROW : $sql");
        return undef;
    };

    my $rows = $sth->fetchall_arrayref();

    if (defined $sth->err) {
        ERROR("Problème lors de la récupération des résultats de la requête SELECT ALL ROW : $sql");
        return undef;
    }

    $sth->finish() or do {
        ERROR("Impossible de clore la requête SELECT ALL ROW : $sql");
        return undef;
    };

    return $rows;
}

=begin nd
Function: select_many_row

Paramètre(s) (liste):
    sql - string - Requête à exécuter

Retourne
    0 si la requête a bien été exécutée et le résultat gardé en mémoire, 1 sinon
=cut
sub select_many_row {
    my $this = shift;
    my $sql = shift;

    if ( defined $this->{current_results} ) {
        $this->{current_results}->finish() or do {
            ERROR("Impossible de clore la requête en cours en mémoire SELECT MANY ROW");
            return 1;
        };
    }

    $this->{current_results} = $this->{connection}->prepare($sql) or do {
        ERROR("Impossible de préparer la requête SELECT MANY ROW : $sql");
        return 1;
    };


    $this->{current_results}->execute() or do {
        ERROR("Impossible d'exécuter la requête SELECT MANY ROW : $sql");
        return 1;
    };

    return 0;
}

=begin nd
Function: next_row

Retourne
    Un tableau correspondant à la ligne suivante dans le résultat gardé en mémoire, undef en cas d'échec ou d'absence de résultat
=cut
sub next_row {
    my $this = shift;

    if ( defined $this->{current_results} ) {
        my @raw = $this->{current_results}->fetchrow_array();

        if (defined $this->{current_results}->err ) {
            DEBUG("Problème de lecture dans les résultats de la requête : " . $this->{current_results}->errstr);
            return undef;
        }

        return @raw;
    }
    else {
        WARN(" Aucune requête n'est en cours");
        return undef;
    }
}

=begin nd
Function: next_row

Retourne
    Une référence d'un hash correspondant à la ligne suivante dans le résultat gardé en mémoire, undef en cas d'échec ou d'absence de résultat
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
        WARN("Aucune requête n'est en cours");
        return undef;
    }
}

=begin nd
Function: execute_without_return

Paramètre(s) (liste):
    sql - string - Requête à exécuter

Retourne
    0 si la requête a été exécutée avec succès, 1 sinon
=cut
sub execute_without_return {
    my $this = shift;
    my $sql = shift;

    my $sth = $this->{connection}->prepare($sql) or do {
        ERROR("Impossible de préparer la requête SANS RETOUR : $sql");
        return 1;
    };

    $sth->execute() or do {
        ERROR("Impossible d'exécuter la requête SANS RETOUR : $sql");
        return 1;
    };

    $sth->finish() or do {
        ERROR("Impossible de clore la requête SANS RETOUR : $sql");
        return 1;
    };

    return 0;
}

=begin nd
Function: run_sql_dump

Paramètre(s) (liste):
    schema_name - string - Schéma principal dans lequel exécuter le fichier SQL
    sql_file - string - Fichier SQL à exécuter

Retourne
    0 si l'exécution est en succès, 1 sinon
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

    DEBUG(" Appel à la commande : " . $cmd_psql);

    my @logs = `$cmd_psql`;

    if ( $? != 0 ) {
        ERROR("Erreur lors de l'exécution du fichier SQL : " . $sql_file);
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

Paramètre(s) (liste):
    schema_name - string - Schéma dont l'existence doit être testée

Retourne
    TRUE (1) si le schéma existe, FALSE (0) sinon
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
Function: is_schema_exist

Paramètre(s) (liste):
    schema_name - string - Schéma dans lequel l'existence de la table doit être testée
    table_name - string - Table dont l'existence doit être testée

Retourne
    TRUE (1) si la table existe, FALSE (0) sinon
=cut
sub is_table_exist {
    my $this = shift;
    my $schema_name = shift;
    my $table_name = shift;

    my $sql = "SELECT count(*) FROM pg_tables WHERE schemaname = '$schema_name' AND tablename = '$table_name';";
    ( my $table_exist ) = $this->select_one_row($sql);
    if ( $table_exist != 0 ) {
        return TRUE;
    } else {
        return FALSE;
    }
}

=begin nd
Function: is_table_empty

Paramètre(s) (liste):
    schema_name - string - Schéma dans lequel le contenu de la table doit être testé
    table_name - string - Table dont le contenu doit être testé

Retourne
    TRUE (1) si la table est vide, FALSE (0) sinon
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

Paramètre(s) (liste):
    schema_name - string - Schéma dans lequel l'existence de la colonne doit être testée
    table_name - string - Table dans laquelle l'existence de la colonne doit être testée
    column_name - string - Colonne dont l'existence doit être testée

Retourne
    TRUE (1) si la colonne existe, FALSE (0) sinon
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

Paramètre(s) (liste):
    schema_name - string - Nom du schéma à créer

Retourne
    0 si le schéma a été créé avec succès, 1 sinon
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

Paramètres (liste):
    schema_name - string - Schéma dans lequel créer une colonne
    table_name - string - Table dans laquelle créer une colonne
    attribute_name - string - Nom de la colonne à créer
    attribute_type - string - Type de la colonne à créer
    default_value - string - Valeur par défaut de la colonne à créer (optionnel)

Retourne
    0 si la colonne a été ajoutée avec succès, 1 sinon
=cut
sub add_column_to_table {
    my $this = shift;
    my $schema_name = shift;
    my $table_name = shift;
    my $attribute_name = shift;
    my $attribute_type = shift;
    my $default_value = shift;

    if ( ! defined $schema_name || ! defined $table_name || ! defined $attribute_name || ! defined $attribute_type ) {
        $this->{logger}->log( "ERROR", "Il manque des paramètres pour ajouter une colonne à une table");
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

Paramètre(s) (liste):
    schema_name - string - Nom du schéma à supprimer, avec ses éventuels héritages

Retourne
    0 si le schéma a été supprimé avec succès, 1 sinon
=cut
sub drop_schema_and_inherits {

    my $this = shift;
    my $schema_name = shift;

    if ( ! defined $schema_name ) {
        ERROR("Il manque le nom du schéma à supprimer");
        return 1;
    }

    # Is the schema exist in the database ?
    if ( $this->is_schema_exist() ) {
        WARN("Le schéma à supprimer ($schema_name) n'existe pas");
        return 0;
    }

    $this->start_transaction();

    # Suppression propre des héritages
    my $sql = "SELECT DISTINCT f_table_name FROM geometry_columns WHERE f_table_schema = '$schema_name';";
    ( my @geometricTables ) = @{ $this->select_all_row($sql) };

    INFO("Il existe " . scalar @geometricTables . " tables géométriques dans le schéma");

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

        INFO((sprintf "Il existe %s tables liées à la  table %s", scalar @linkedTables, $table->[0]));

        for my $subtable (@linkedTables) {

            $sql = sprintf "ALTER TABLE %s NO INHERIT %s.%s", $subtable->[0], $schema_name, $table->[0];

            if ( $this->execute_without_return($sql) ) {
                $this->{logger}->log(
                    "ERROR",
                    (sprintf "Erreur lors de la suppression du lien de %s depuis %s", $table->[0], $subtable->[0])
                );
                $this->rollback_transaction();
                return 1;
            }
        }
    }

    # Delete schéma
    if ( $this->drop_schema( $schema_name, "true" ) ) {
        ERROR("Erreur lors de la suppression du schema " . $schema_name);
        $this->rollback_transaction();
        return 1;
    }

    $this->commit_transaction();

    return 0;
}

=begin nd
Function: drop_schema

Paramètre(s) (liste):
    schema_name - string - Nom du schéma à supprimer
    cascade - string - Si 'true', la suppression du schéma est appelée en mode cascade (optionnel, false par défaut)

Retourne
    0 si le schéma a été supprimé avec succès, 1 sinon
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

Paramètre(s) (liste):
    schema_name - string - Nom du schéma sur lequel supprimer des droits
    username - string - Utilisateur à qui enlever des droits
    permissions - string - Droits à supprimer

Retourne
    0 si les droits ont été supprimés avec succès, 1 sinon
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

Paramètre(s) (liste):
    schema_name - string - Nom du schéma sur lequel ajouter des droits
    username - string - Utilisateur à qui ajouter des droits
    permissions - string - Droits à ajouter

Retourne
    0 si les droits ont été ajoutés avec succès, 1 sinon
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

Paramètre(s) (liste):
    schema_name - string - Nom du schéma dans lequel mettre les droits sur toutes les tables
    username - string - Utilisateur à qui mettre les droits sur toutes les tables
    permissions - string - Droits à donner. Optionnel : si non défini, tous les droits sont supprimés sur toutes les tables

Retourne
    0 si les droits ont été mis avec succès, 1 sinon
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

Paramètre(s) (liste):
    schema_name - string - Nom du schéma dans lequel supprimer tous les droits sur la table
    table_name - string - Nom de la table sur laquelle supprimer tous les droits
    username - string - Utilisateur à qui supprimer tous les droits sur la table

Retourne
    0 si les droits ont été supprimés avec succès, 1 sinon
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

Paramètre(s) (liste):
    schema_name - string - Nom du schéma dans lequel ajouter les droits sur la tables
    table_name - string - Nom de la table sur laquelle ajouter les droits
    username - string - Utilisateur à qui ajouter les droits sur la table
    permissions - string - Droits à ajouter

Retourne
    0 si les droits ont été supprimés avec succès, 1 sinon
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
Function: get_geometry_column_name

Paramètre(s) (liste):
    schema_name - string - Nom du schéma dans lequel on veut connaître la colonne géométrique
    table_name - string - Nom de la table dans laquelle on veut connaître la colonne géométrique
Retourne
    le nom de la colonne géométrique, undef si pas de colonne géométrique dans la table
=cut
sub get_geometry_column_name {
    my $this = shift;
    my $schema_name = shift;
    my $table_name = shift;

    my $sql = "SELECT f_geometry_column FROM geometry_columns WHERE f_table_schema = '$schema_name' AND f_table_name = '$table_name';";

    my @line = $this->select_one_row($sql);

    return $line[0];
}

=begin nd
Function: get_attributes_list

Paramètre(s) (liste):
    schema_name - string - Nom du schéma dans lequel on veut la liste des attributs
    table_name - string - Nom de la table dont on veut la liste des attributs

Retourne
    une chaîne de caractères correpondant à la liste des attributs de la table séparés par des virgules, undef en cas d'échec
=cut
sub get_attributes_list {
    my $this = shift;
    my $schema_name = shift;
    my $table_name = shift;

    my $sql = "SELECT column_name FROM information_schema.columns WHERE table_schema = '$schema_name' AND table_name = '$table_name' ORDER BY ordinal_position;";

    my $atts = $this->select_all_row($sql);
    if ( ! defined $atts) {
        $this->{logger}->log(
            "ERROR", 
            (sprintf "Échec de la requête pour lister les attributs de la table %s.%s", $schema_name, $table_name)
        );
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

Paramètre(s) (liste):
    schema_name - string - Nom du schéma dans lequel on veut la liste des attributs
    table_name - string - Nom de la table dont on veut la liste des attributs

Retourne
    une référence de hash dont les clés sont les attributs de la table, undef en cas d'échec
=cut
sub get_attributes_hash {
    my $this = shift;
    my $schema_name = shift;
    my $table_name = shift;

    my $sql = "SELECT column_name, data_type FROM information_schema.columns WHERE table_schema = '$schema_name' AND table_name = '$table_name' ORDER BY ordinal_position;";

    my $atts = $this->select_all_row($sql);
    if ( ! defined $atts) {
        $this->{logger}->log( 
            "ERROR",
            (sprintf "Échec de la requête pour lister les attributs de la table %s.%s", $schema_name, $table_name)
        );
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

Paramètre(s) (liste):
    schema_name - string - Nom du schéma dont on veut la liste des tables

Retourne
    un tableau listant les tables dans le schéma
=cut
sub get_tables_array {
    my $this = shift;
    my $schema_name = shift;

    my $sql = "SELECT tablename FROM pg_tables WHERE schemaname='$schema_name';";

    my $tables = $this->select_all_row($sql);
    if ( ! defined $tables) {
        $this->{logger}->log(
            "ERROR",
            (sprintf "Échec de la requête pour lister les tables du schéma %s", $schema_name)
        );
        return undef;
    }

    my @array;

    foreach my $val (@{$tables}) {
        push @array, $val->[0];
    }

    return @array;
}

=begin nd
Function: get_attributes_hash

Paramètre(s) (liste):
    schema_name - string - Nom du schéma dans lequel on veut les valeurs distincte prises par l'attibut
    table_name - string - Nom de la table dans laquelle on veut les valeurs distincte prises par l'attibut
    att_name - string - Nom de l'attribut dont on veut les valeurs disinctes

Retourne
    un tableau des valeurs distinctes prises par l'attribut, un tableau vide en cas d'échec
=cut
sub get_distinct_values {
    my $this = shift;
    my $schema_name = shift;
    my $table_name = shift;
    my $att_name = shift;

    my $sql = sprintf "SELECT DISTINCT $att_name FROM $schema_name.$table_name;";

    my $att_values = $this->select_all_row($sql);
    if ( ! defined $att_values) {
        $this->{logger}->log(
            "ERROR",
            (sprintf "Échec de la requête pour lister les valeurs de l'attribut %s de la table %s.%s", $att_name, $schema_name, $table_name)
        );
        return ();
    }

    my @array;

    foreach my $val (@{$att_values}) {
        push @array, $val->[0];
    }

    return @array;
}

=begin nd
Function: get_schema_size

Paramètre(s) (liste):
    schema_name - string - Nom du schéma dont on veut la taille complète (+ index)

Retourne
    la taille du schéma, -1 en cas d'erreur
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
        $this->{logger}->log(
            "ERROR",
            "Erreur durant la récupération de la taille du schema $schema_name"
        );
        return -1;
    }

    chomp($size);

    return $size;
}

1;
__END__

