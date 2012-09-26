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

package BE4::DataSourceLoader;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
use Data::Dumper;
use List::Util qw(min max);

use Data::Dumper;
use Geo::GDAL;

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

################################################################################
=begin nd
Group: variable

variable: $self
    * FILEPATH_DATACONF - path of data's configuration file
    * dataSources : array of BE4::DataSource
=cut

####################################################################################################
#                                       CONSTRUCTOR METHODS                                        #
####################################################################################################

# Group: constructor

sub new {
    my $this = shift;

    my $class= ref($this) || $this;
    # IMPORTANT : if modification, think to update natural documentation (just above) and pod documentation (bottom)
    my $self = {
        FILEPATH_DATACONF => undef,
        dataSources  => []
    };

    bless($self, $class);

    TRACE;

    # init. class
    return undef if (! $self->_init(@_));

    # load. class
    if (defined $self->{FILEPATH_DATACONF}) {
        return undef if (! $self->_load());
    } else {
        # Old datasource definition
        return undef if (! $self->_loadOld(@_));
    }
    
    INFO (sprintf "Data sources number : %s",scalar @{$self->{dataSources}});

    return $self;
}

#
=begin nd
method: _init

Check the DataSource and Harvesting objects.

Parameters:
    datasource - BE4 configuration section 'datasource' = path_image + srs or filepath_conf
=cut
sub _init {
    my $self   = shift;
    my $datasource = shift;

    TRACE;
    
    return FALSE if (! defined $datasource);
    
    if (exists($datasource->{path_image})) {
        WARN("Old method is using to define a datasource (without datasource configuration file), convert it.");
        return TRUE;
    }
    
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

#
=begin nd
method: _load

Create BE4::PropertiesLoader and BE4::DataSource objects .

Parameters:
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

#
=begin nd
method: _loadOld

Allow to use old method to define datasource (just one). Use be4 configuration sections 'datasource', 'harvesting' and parameter 'pyr_level_bottom'.

Parameters:
    datasource - be4 configuration section 'datasource' = path_image + srs
    harvesting - be4 configuration section 'harvesting' (can be undefined)
    bottomId - be4 configuration parameter 'pyr_level_bottom' (section 'pyramid')
=cut
sub _loadOld {
    my $self   = shift;
    my $datasource = shift;
    my $harvesting = shift;
    my $bottomId = shift;

    TRACE;

    if (! defined $bottomId) {
        ERROR("We need a bottom level identifiant (section 'pyramid', parameter 'pyr_level_bottom') !");
        return FALSE;
    }
    
    my $params;
    
    $params = { map %$_, grep ref $_ eq 'HASH', ($datasource, $params) };
    $params = { map %$_, grep ref $_ eq 'HASH', ($harvesting, $params) };

    my $sources = $self->{dataSources};

    my $objDataSource = BE4::DataSource->new($bottomId,$params);
    if (! defined $objDataSource) {
        ERROR(sprintf "Cannot create the DataSource object for the base level %s (old method)",$bottomId);
        return FALSE;
    }
    push @{$sources}, $objDataSource;

    return TRUE;
}

####################################################################################################
#                                       DATASOURCES UPDATE                                         #
####################################################################################################

# Group: datasources update

#
=begin nd
method: updateDataSources

From data sources, TMS and parameters, we identify top and bottom.
    - bottom level = the lowest level among data source base levels
    - top level = levelID in parameters if defined, top level of TMS otherwise.

For each datasource, we store the order and the ID of the higher level which use this datasource.
The base level (from which datasource is used) is already known.

Example (with 2 data sources):
    - DataSource1: from level_18 (order 2) to level_16 (order 4)
    - DataSource1: from level_15 (order 5) to level_12 (order 8)

There are no superposition between data sources.

Parameters:
    TMS - a TileMatrixSet object, to know levels' orders
    topID - optionnal, from the 'pyramid' section in the configuration file
    
Return:
    Global bottom and top order, in a list : (bottomOrder,topOrder)
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
        $topID = $TMS->getLevelTop();
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
#                                       GETTERS / SETTERS                                          #
####################################################################################################

# Group: getters - setters

sub getDataSources {
    my $self = shift;
    return $self->{dataSources}; 
}

sub getNumberDataSources {
    my $self = shift;
    return scalar $self->{dataSources}; 
}

####################################################################################################
#                                          EXPORT METHODS                                          #
####################################################################################################

# Group: export methods

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

=head1 NAME

BE4::DataSourceLoader - Load and validate data sources

=head1 SYNOPSIS

    use BE4::DataSourceLoader

    # DataSourceLoader object creation
    my $objDataSourceLoader = BE4::DataSourceLoader->new({
        filepath_conf => "/home/IGN/CONF/source.txt",
    });
    
    # DataSourceLoader object creation for old configuration (just one data source)
    my $objDataSourceLoader = BE4::DataSourceLoader->new(
        { # datasource section
            SRS         => "IGNF:LAMB93",
            path_image  => "/home/IGN/DATA/IMAGES/",
        },
        { # harvesting section
            wms_layer   => ORTHO_RAW_LAMB93_PARIS_OUEST
            wms_url     => http://localhost/wmts/rok4
            wms_version => 1.3.0
            wms_request => getMap
            wms_format  => image/tiff
        },
        $bottomLevel, # bottom level for this one data source
    );

=head1 DESCRIPTION

=over 4

=item FILEPATH_DATACONF

Complete file's path, which contain all informations about data sources

=item dataSources

An array of DataSource objects

=back

=head1 FILE CONFIGURATION

=over 4

=item In the be4 configuration, section datasource (multidata.conf)

    [ datasource ]
    filepath_conf       = /home/IGN/CONF/source.txt

=item In the source configuration (source.txt)

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


=back

=head1 LIMITATION & BUGS

Metadata managing not yet implemented.

=head1 SEE ALSO

=head2 POD documentation

=begin html

<ul>
<li><A HREF="./lib-BE4-DataSource.html">BE4::DataSource</A></li>
</ul>

=end html

=head2 NaturalDocs

=begin html

<A HREF="../Natural/Html/index.html">Index</A>

=end html

=head1 AUTHOR

Satabin Théo, E<lt>theo.satabin@ign.frE<gt>

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2011 by Satabin Théo

This library is free software; you can redistribute it and/or modify it under the same terms as Perl itself, either Perl version 5.10.1 or, at your option, any later version of Perl 5 you may have available.

=cut
