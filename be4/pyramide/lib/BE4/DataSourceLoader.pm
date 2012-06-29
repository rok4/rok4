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
    * FILEPATH_DATACONF => undef, # path of data's configuration file
    * sources  => [], # array of DataSource objects
=cut


####################################################################################################
#                                       CONSTRUCTOR METHODS                                        #
####################################################################################################

# Group: constructor

sub new {
    my $this = shift;

    my $class= ref($this) || $this;
    my $self = {
        FILEPATH_DATACONF => undef,
        sources  => []
    };

    bless($self, $class);

    TRACE;

    # init. class
    return undef if (! $self->_init(@_));

    # load. class
    return undef if (! $self->_load());

    return $self;
}


sub _init {
    my $self   = shift;
    my $params = shift;

    TRACE;
    
    return FALSE if (! defined $params);
    
    if (! exists($params->{filepath_conf}) || ! defined ($params->{filepath_conf})) {
        ERROR("key/value required to 'filepath_conf' !");
        return FALSE ;
    }
    if (! -f $params->{filepath_conf}) {
        ERROR (sprintf "Data's configuration file ('%s') doesn't exist !",$params->{filepath_conf});
        return FALSE;
    }
    $self->{FILEPATH_DATACONF} = $params->{filepath_conf};

    return TRUE;
}


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

    my $sources = $self->{sources};
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

1;
__END__

=head1 NAME

BE4::DataSourceLoader - Load and validate data sources

=head1 SYNOPSIS

    use BE4::DataSourceLoader

    # DataSourceLoader object creation
    my $objDataSource = BE4::DataSource->new({
        filepath_conf => "/home/IGN/CONF/source.txt",
    });

=head1 DESCRIPTION

=over 4

=item FILEPATH_DATACONF

Complete file's path, which contain all informations about data sources

=item sources

An array of DataSource objects

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
    image_width = 2048
    image_height = 2048
    
    [ 14 ]
    
    srs = IGNF:WGS84G
    extent = /home/IGN/SHAPE/Polygon.txt
    
    wms_layer   = ORTHO_RAW_LAMB93_D075-O
    wms_url     = http://localhost/wmts/rok4
    wms_version = 1.3.0
    wms_request = getMap
    wms_format  = image/tiff
    image_width = 4096
    image_height = 4096


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
