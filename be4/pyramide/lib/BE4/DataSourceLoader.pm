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
File: DataSourceLoader.pm

Class: BE4::DataSourceLoader

Loads, validates and manages data sources. Data sources informations are read from a specific configuration file.

Using:
    (start code)
    use BE4::DataSourceLoader

    # DataSourceLoader object creation
    my $objDataSourceLoader = BE4::DataSourceLoader->new({
        filepath_conf => "/home/IGN/CONF/source.txt",
    });

    (end code)

Attributes:
    FILEPATH_DATACONF - string - Path to the specific datasources configuration file.
    dataSources - <DataSource> array - Data sources ensemble. Can contain just one element.

Limitations:
    Metadata managing not yet implemented.
=cut

################################################################################

package BE4::DataSourceLoader;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
use Data::Dumper;
use List::Util qw(min max);

# My module
use BE4::DataSource;
use BE4::PropertiesLoader;

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

=begin nd
Constructor: new

DataSourceLoader constructor. Bless an instance.

Parameters (list):
    datasource - hash - Section *datasource*, in the general BE4 configuration file. Contains the key "filepath_conf"
|               filepath_conf - string - Path to the data sources configuration file

See also:
    <_init>, <_load>
=cut
sub new {
    my $this = shift;
    my $datasource = shift;

    my $class= ref($this) || $this;
    # IMPORTANT : if modification, think to update natural documentation (just above)
    my $self = {
        FILEPATH_DATACONF => undef,
        dataSources  => []
    };

    bless($self, $class);

    TRACE;

    # init. class
    return undef if (! $self->_init($datasource));

    # load. class
    if (defined $self->{FILEPATH_DATACONF}) {
        return undef if (! $self->_load());
    }
    
    INFO (sprintf "Data sources number : %s",scalar @{$self->{dataSources}});

    return $self;
}

=begin nd
Function: _init

Checks the "datasource" section. Must contain key "filepath_conf" (and path is tested)

Parameters (list):
    datasource - hash - Section *datasource*, in the general BE4 configuration file. Contains the key "filepath_conf" :
|               filepath_conf - string - Path to the data sources configuration file
=cut
sub _init {
    my $self   = shift;
    my $datasource = shift;

    TRACE;
    
    return FALSE if (! defined $datasource);
    
    if (! exists($datasource->{filepath_conf}) || ! defined ($datasource->{filepath_conf})) {
        ERROR("'filepath_conf' is required in the 'datasource' section !");
        return FALSE ;
    }
    if (! -f $datasource->{filepath_conf}) {
        ERROR (sprintf "Data's configuration file ('%s') doesn't exist !",$datasource->{filepath_conf});
        return FALSE;
    }
    $self->{FILEPATH_DATACONF} = $datasource->{filepath_conf};

    return TRUE;
}

=begin nd
Function: _load

Reads the specific data sources configuration file and creates corresponding <DataSource> objects.
=cut
sub _load {
    my $self   = shift;

    TRACE;

    my $propLoader = BE4::PropertiesLoader->new($self->{FILEPATH_DATACONF});

    if (! defined $propLoader) {
        ERROR("Can not load sources' properties !");
        return FALSE;
    }

    my $sourcesProperties = $propLoader->getAllProperties();

    if (! defined $sourcesProperties) {
        ERROR("All parameters properties of sources are empty !");
        return FALSE;
    }

    my $sources = $self->{dataSources};
    my $nbSources = 0;

    while( my ($level,$params) = each(%$sourcesProperties) ) {
        my $datasource = BE4::DataSource->new($level,$params);
        if (! defined $datasource) {
            ERROR(sprintf "Cannot create a DataSource object for the base level %s",$level);
            return FALSE;
        }
        push @{$sources}, $datasource;
        $nbSources++;
    }

    if ($nbSources == 0) {
        ERROR ("No source !");
        return FALSE;
    }

    return TRUE;
}

####################################################################################################
#                                Group: Data sources update                                        #
####################################################################################################

=begin nd
Function: updateDataSources

From data sources, TMS and parameters, we identify top and bottom :
    - bottom level = the lowest level among data source base levels
    - top level = levelID in parameters if defined, top level of TMS otherwise.

For each datasource, we store the order and the ID of the higher level which use this datasource.
The base level (from which datasource is used) is already known.

Example (with 2 data sources) :
    - DataSource1: from level_18 (order 2) to level_16 (order 4)
    - DataSource1: from level_15 (order 5) to level_12 (order 8)

There are no superposition between data sources.

Parameters:
    TMS - <TileMatrixSet> - To know levels' orders
    topID - string - Optionnal, from the *pyramid* section in the configuration file
    
Returns the global bottom and top order, in a integer list : (bottomOrder,topOrder), (-1,-1) if failure.
=cut
sub updateDataSources {
    my $self = shift;
    my $TMS = shift;
    my $topID = shift;

    TRACE();
    
    if (! defined $TMS || ref ($TMS) ne "BE4::TileMatrixSet") {
        ERROR("We need a TileMatrixSet object to update data sources");
        return (-1, -1);
    }
    
    ######## DETERMINE GLOBAL TOP/BOTTOM LEVELS ########
    
    # Définition du topLevel :
    #  - En priorité celui fourni en paramètre
    #  - Par defaut, c'est le plus haut niveau du TMS,
    my $topOrder = undef;
    if (defined $topID) {
        $topOrder = $TMS->getOrderfromID($topID);
        if (! defined $topOrder) {
            ERROR(sprintf "The top level defined in configuration ('%s') does not exist in the TMS !",$topID);
            return (-1, -1);
        }
    } else {
        $topID = $TMS->getTopLevel();
        $topOrder = $TMS->getOrderfromID($topID);
    }

    # Définition du bottomLevel :
    #  - celui fournit dans la configuration des sources : le niveau le plus bas de toutes les sources
    # En plus :
    #  - on vérifie la cohérence des niveaux défini dans la configuration des sources avec le niveau du haut
    #  - on renseigne l'ordre du niveau du bas pour chaque source de données
    my $bottomID = undef;
    my $bottomOrder = undef;
    
    foreach my $datasource (@{$self->{dataSources}}) {
        my $dsBottomID = $datasource->getBottomID;
        my $dsBottomOrder = $TMS->getOrderfromID($dsBottomID);
        if (! defined $dsBottomOrder) {
            ERROR(sprintf "The level present in source configuration ('%s') does not exist in the TMS !",
                $dsBottomID);
            return (-1, -1);
        }

        if ($topOrder < $dsBottomOrder) {
            ERROR(sprintf "A level in sources configuration (%s) is higher than the top level defined in the be4 configuration (%s).",$dsBottomID,$topID);
            return (-1, -1);
        }
        
        $datasource->setBottomOrder($dsBottomOrder);
        
        if (! defined $bottomOrder || $dsBottomOrder < $bottomOrder) {
            $bottomID = $dsBottomID;
            $bottomOrder = $dsBottomOrder;
        }
    }

    if ($topOrder == $bottomOrder) {
        INFO(sprintf "Top and bottom levels are identical (%s) : just one level will be generated",$bottomID);
    }
    
    ######## DETERMINE FOR EACH DATASOURCE TOP/BOTTOM LEVELS ########
    
    @{$self->{dataSources}} = sort {$a->getBottomOrder <=> $b->getBottomOrder} ( @{$self->{dataSources}});
    
    for (my $i = 0; $i < scalar @{$self->{dataSources}} -1; $i++) {
        my $dsTopOrder = $self->{dataSources}[$i+1]->getBottomOrder - 1;
        $self->{dataSources}[$i]->setTopOrder($dsTopOrder);
        $self->{dataSources}[$i]->setTopID($TMS->getIDfromOrder($dsTopOrder));
    }
    
    $self->{dataSources}[-1]->setTopID($topID);
    $self->{dataSources}[-1]->setTopOrder($TMS->getOrderfromID($topID));
    
    if ($topOrder < $bottomOrder) {
        ERROR("Pas bon ça : c'est sens dessus dessous ($topOrder - $topID < $bottomOrder - $bottomID)");
        ERROR("Et ça, ça c'est pas normal !");
        return (-1, -1);
    }
    
    return ($bottomOrder,$topOrder);
}

####################################################################################################
#                                Group: Getters - Setters                                          #
####################################################################################################

# Function: getDataSources
sub getDataSources {
    my $self = shift;
    return $self->{dataSources}; 
}

# Function: getNumberDataSources
sub getNumberDataSources {
    my $self = shift;
    return scalar @{$self->{dataSources}}; 
}

####################################################################################################
#                                Group: Export methods                                             #
####################################################################################################

=begin nd
Function: exportForDebug

Returns all informations about the data sources loader. Useful for debug.

Example:
    (start code)
    (end code)
=cut
sub exportForDebug {
    my $self = shift ;
    
    my $export = "";
    
    $export .= sprintf "\n Object BE4::DataSourceLoader :\n";
    $export .= sprintf "\t Configuration file :\n", $self->{FILEPATH_DATACONF};
    $export .= sprintf "\t Sources number : %s\n", scalar @{$self->{dataSources}};
    
    return $export;
}

1;
__END__

=begin nd

Group: details

Configuration file exmaples.

_In the be4 configuration, section *datasource* (multidata.conf)_
    (start code)
    [ datasource ]
    filepath_conf       = /home/IGN/CONF/source.txt
    (end code)

_In the source configuration (source.txt)_
    (start code)
    [ 19 ]
    
    srs                 = IGNF:LAMB93
    path_image          = /home/theo/DONNEES/BDORTHO_PARIS-OUEST_2011_L93/DATA
    
    wms_layer   = ORTHO_RAW_LAMB93_PARIS_OUEST
    wms_url     = http://localhost/wmts/rok4
    wms_version = 1.3.0
    wms_request = getMap
    wms_format  = image/tiff
    max_width = 2048
    max_height = 2048
    
    [ 14 ]
    
    srs = IGNF:WGS84G
    extent = /home/IGN/SHAPE/Polygon.txt
    
    wms_layer   = ORTHO_RAW_LAMB93_D075-O
    wms_url     = http://localhost/wmts/rok4
    wms_version = 1.3.0
    wms_request = getMap
    wms_format  = image/tiff
    max_width = 4096
    max_height = 4096
    (end code)
    
=cut
