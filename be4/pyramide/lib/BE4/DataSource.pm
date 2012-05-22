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

package BE4::DataSource;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
use Data::Dumper;
use List::Util qw(min max);

use Data::Dumper;
use Geo::GDAL;

# My module
use BE4::ImageSource;
use BE4::HarvestSource;
use BE4::PropertiesLoader;

require Exporter;
use AutoLoader qw(AUTOLOAD);

our @ISA = qw(Exporter);

our %EXPORT_TAGS = ( 'all' => [ qw() ] );
our @EXPORT_OK   = ( @{$EXPORT_TAGS{'all'}} );
our @EXPORT      = qw();

################################################################################
# version
our $VERSION = '0.0.1';

################################################################################
# constantes
use constant TRUE  => 1;
use constant FALSE => 0;

################################################################################
# Global
my %SOURCE;

################################################################################
# Preloaded methods go here.
BEGIN {}
INIT {

%SOURCE = (
    type => ['image','harvest']
);

}
END {}

####################################################################################################
#                                       CONSTRUCTOR METHODS                                        #
####################################################################################################

sub new {
    my $this = shift;

    my $class= ref($this) || $this;
    my $self = {
        FILEPATH_DATACONF => undef, # path of data's configuration file
        type => undef, # (image or harvest)
        sources  => {},    # hash of ImageSource or HarvestSource objects
        SRS => undef,
        bottomExtent => undef, # OGR::Geometry object, in the previous SRS
    };

    bless($self, $class);

    TRACE;

    # init. class
    return undef if (! $self->_init(@_));

    # load. class
    return undef if (! $self->_load());

    return $self;
}

################################################################################
# privates init.
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

    if (! exists($params->{type}) || ! defined ($params->{type})) {
        ERROR("key/value required to 'type' !");
        return FALSE ;
    }
    if (! $self->is_type($params->{type})) {
        ERROR("Invalid data's type !");
        return FALSE ;
    }
    $self->{type} = $params->{type};

    return TRUE;
}

################################################################################
# privates load.
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

    while( my ($k,$v) = each(%$sourcesProperties) ) {
        if ($k eq "global") {
            if (! $self->computeGlobalInfo($v)) {
                ERROR("Cannot compute the global information");
                return FALSE;
            }
            next;
        }

        if ($self->{type} eq "harvest") {
            my $harvestSource = BE4::HarvestSource->new($v);
            if (! defined $harvestSource) {
                ERROR(sprintf "Cannot create an harvest source for the base level %s",$k);
                return FALSE;
            }
            $sources->{$k} = $harvestSource;
            $nbSources++;
        }
        elsif ($self->{type} eq "image") {
            my $imageSource = BE4::ImageSource->new($v);
            if (! defined $imageSource) {
                ERROR(sprintf "Cannot create an image source for the base level %s",$k);
                return FALSE;
            }
            $sources->{$k} = $imageSource;
            $nbSources++;
        }
    }

    if (!defined $sources || $nbSources == 0) {
        ERROR ("No source !");
        return FALSE;
    }

    return TRUE;
}



####################################################################################################
#                                              METHODS                                             #
####################################################################################################

# method: computeGlobalInfo
# * read the srs, for the box or images
# * read the box, 2 cases are possible :
#       - box is a bbox, as xmin,ymin,xmax,ymax
#       - box is a file path, file contains a complex polygon
#   We generate a OGR Geometry
#---------------------------------------------------------------------------------------------------
sub computeGlobalInfo {
    my $self = shift;
    my $params = shift;

    TRACE;

    return FALSE if (! defined $params);

    # SRS
    if (! exists($params->{srs}) || ! defined ($params->{srs})) {
        ERROR("'srs' required in the sources configuration file (section 'global') !");
        return FALSE ;
    }
    $self->{SRS} = $params->{srs};

    # Bounding polygon
    if (! exists($params->{box}) || ! defined ($params->{box})) {
        ERROR("'box' required in the sources configuration file (section 'global') !");
        return FALSE ;
    }

    my $wktextent;

    my @limits = split (/,/,$params->{box},-1);

    if (scalar @limits == 4) {
        # user supplied a BBOX
        if ($limits[0] !~ m/[+-]?\d+\.?\d*/ || $limits[1] !~ m/[+-]?\d+\.?\d*/ ||
            $limits[2] !~ m/[+-]?\d+\.?\d*/ || $limits[3] !~ m/[+-]?\d+\.?\d*/ ) {
            ERROR(sprintf "'box' value must contain only numbers : %s !",$params->{box});
            return FALSE ;
        }

        my $xmin = $limits[0];
        my $ymin = $limits[1];
        my $xmax = $limits[2];
        my $ymax = $limits[3];

        if ($xmax <= $xmin || $ymax <= $ymin) {
            ERROR(sprintf "'box' value is not logical for a bbox (max < min) : %s !",$params->{box});
            return FALSE ;
        }

        $wktextent = sprintf "POLYGON((%s %s,%s %s,%s %s,%s %s,%s %s))",
            $xmin,$ymin,
            $xmin,$ymax,
            $xmax,$ymax,
            $xmax,$ymin,
            $xmin,$ymin;
    }
    elsif (scalar @limits == 1) {
        # user supplied a file which contains bounding polygon
        if (! -f $params->{box}) {
            ERROR (sprintf "Shape file ('%s') doesn't exist !",$params->{box});
            return FALSE;
        }
        
        if (! open SHAPE, "<", $params->{box} ){
            ERROR(sprintf "Cannot open the shape file %s.",$params->{box});
            return FALSE;
        }

        $wktextent = '';
        while( defined( my $line = <SHAPE> ) ) {
            $wktextent .= $line;
        }
        close(SHAPE);
    }

    if (! defined $wktextent) {
        ERROR(sprintf "Cannot define the bottom extent from the parameter 'box' => %s.",$params->{box});
        return FALSE;
    }
    
    my $bottomExtent = Geo::OGR::Geometry->create(WKT=>$wktextent);;
    if (! defined $bottomExtent) {
        ERROR(sprintf "Cannot create a Geometry from the string => %s.",$wktextent);
        return FALSE;
    }

    $self->{bottomExtent} = $bottomExtent;

    return TRUE;
}

################################################################################
# tests
sub is_type {
    my $self = shift;
    my $type = shift;

    TRACE;

    return FALSE if (! defined $type);

    foreach (@{$SOURCE{type}}) {
        return TRUE if ($type eq $_);
    }
    ERROR (sprintf "Unknown 'type' of data (%s) !",$type);
    return FALSE;
}

####################################################################################################
#                                          GETTER/SETTER                                           #
####################################################################################################

sub getSRS {
  my $self = shift;
  return $self->{SRS};
}


1;
__END__

=pod

=head1 NAME

  BE4::DataSource - Managing data sources

=head1 SYNOPSIS

  use BE4::DataSource;
  
  my $objImplData = BE4::DataSource->new(path_conf => $path,
                                         type => image);

=head1 DESCRIPTION


  
=head2 EXPORT

None by default.

=head1 LIMITATION & BUGS

* Support data source multiple !

* Does not implement the managing of metadata !

=head1 SEE ALSO

  eg BE4::HarvestSource
    eg BE4::Harvesting
  eg BE4::ImageSource
    eg BE4::GeoImage

=head1 AUTHOR

Satabin Théo, E<lt>tsatabin@E<gt>

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2011 by Satabin Théo

This library is free software; you can redistribute it and/or modify
it under the same terms as Perl itself, either Perl version 5.10.1 or,
at your option, any later version of Perl 5 you may have available.

=cut
