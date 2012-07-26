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
use BE4::Harvesting;

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
    * bottomLevelID => undef, # this datasource will be used between bottomLevel and topLevel
    * bottomLevelOrder => undef,
    * topLevelID => undef,
    * topLevelOrder => undef,
    
    * srs => undef,
    * extent => undef, # OGR::Geometry object, in the previous SRS
    * bbox => undef, # array of limits of the previous extent

    * imageSource => undef, # an ImageSource object (can be undefined)

    * harvesting => undef # an Harvesting object (can be undefined)
=cut

####################################################################################################
#                                       CONSTRUCTOR METHODS                                        #
####################################################################################################

# Group: constructor

sub new {
    my $this = shift;
    my $level = shift;
    my $params = shift;

    my $class= ref($this) || $this;
    # IMPORTANT : if modification, think to update natural documentation (just above) and pod documentation (bottom)
    my $self = {
        # Global information
        bottomLevelID => undef,
        bottomLevelOrder => undef,
        topLevelID => undef,
        topLevelOrder => undef,
        bbox => undef,
        extent => undef,
        srs => undef,
        # Image source
        imageSource => undef,
        # Harvesting
        harvesting => undef
    };

    bless($self, $class);

    TRACE;

    # load. class
    return undef if (! $self->_load($level,$params));

    return undef if (! $self->computeGlobalInfo());

    return $self;
}

sub _load {
    my $self   = shift;
    my $level = shift;
    my $params = shift;

    TRACE;
    
    return FALSE if (! defined $params);

    if (! defined $level || $level eq "") {
        ERROR("A data source have to be defined with a level !");
        return FALSE;
    }
    $self->{bottomLevelID} = $level;

    if (! exists $params->{srs} || ! defined $params->{srs}) {
        ERROR("A data source have to be defined with the 'srs' parameter !");
        return FALSE;
    }
    $self->{srs} = $params->{srs};

    # bbox is optionnal if we have an ImageSource (checked in computeGlobalInfo)
    if (exists $params->{extent} && defined $params->{extent}) {
        $self->{extent} = $params->{extent};
    }

    # ImageSource is optionnal
    my $imagesource = undef;
    if (exists $params->{path_image}) {
        $imagesource = BE4::ImageSource->new({
            path_image => $params->{path_image},
            path_metadata => $params->{path_metadata},
        });
        if (! defined $imagesource) {
            ERROR("Cannot create the ImageSource object");
            return FALSE;
        }
    }
    $self->{imageSource} = $imagesource;

    # Harvesting is optionnal, but if we have 'wms_layer' parameter, we suppose that we have others
    my $harvesting = undef;
    if (exists $params->{wms_layer}) {
        $harvesting = BE4::Harvesting->new({
            wms_layer   => $params->{wms_layer},
            wms_url     => $params->{wms_url},
            wms_version => $params->{wms_version},
            wms_request => $params->{wms_request},
            wms_format  => $params->{wms_format},
            wms_style => $params->{wms_style},
            wms_bgcolor => $params->{wms_bgcolor},
            wms_transparent  => $params->{wms_transparent},
            min_size  => $params->{min_size},
            image_width  => $params->{image_width},
            image_height  => $params->{image_height}
        });
        if (! defined $harvesting) {
            ERROR("Cannot create the Harvesting object");
            return FALSE;
        }
    }
    $self->{harvesting} = $harvesting;
    
    if (! defined $harvesting && ! defined $imagesource) {
        ERROR("A data source must have a ImageSource OR a Harvesting !");
        return FALSE;
    }
    
    return TRUE;
}

####################################################################################################
#                                       PUBLIC METHODS                                             #
####################################################################################################

# Group: public methods

#
=begin nd
    method: computeGlobalInfo

    Read the srs, for the box or images.

    Read the extent, 2 cases are possible :
        - extent is a bbox, as xmin,ymin,xmax,ymax
        - extent is a file path, file contains a complex polygon

    We generate an OGR Geometry
=cut
sub computeGlobalInfo {
    my $self = shift;

    TRACE;

    # Bounding polygon
    if (defined $self->{imageSource}) {
        # We have real images for source, bbox will be calculated from them.
        my ($xmin,$ymin,$xmax,$ymax);

        my @BBOX = $self->{imageSource}->computeBBox();
        $xmin = $BBOX[0] if (! defined $xmin || $xmin < $BBOX[0]);
        $ymin = $BBOX[1] if (! defined $ymin || $xmin < $BBOX[1]);
        $xmax = $BBOX[2] if (! defined $xmax || $xmin > $BBOX[2]);
        $ymax = $BBOX[3] if (! defined $ymax || $xmin > $BBOX[3]);
        
        $self->{extent} = sprintf "%s,%s,%s,%s",$xmin,$ymin,$xmax,$ymax;
    }    

    # Bounding polygon
    if (! defined $self->{extent}) {
        ERROR("'extent' required in the sources configuration file if no image source !");
        return FALSE ;
    }

    my $GMLextent;

    $self->{extent} =~ s/ //;
    my @limits = split (/,/,$self->{extent},-1);

    if (scalar @limits == 4) {
        # user supplied a BBOX
        if ($limits[0] !~ m/[+-]?\d+\.?\d*/ || $limits[1] !~ m/[+-]?\d+\.?\d*/ ||
            $limits[2] !~ m/[+-]?\d+\.?\d*/ || $limits[3] !~ m/[+-]?\d+\.?\d*/ ) {
            ERROR(sprintf "If 'extent' is a bbox, value must be a string like 'xmin,ymin,xmax,ymax' : %s !",$self->{extent});
            return FALSE ;
        }

        my $xmin = $limits[0];
        my $ymin = $limits[1];
        my $xmax = $limits[2];
        my $ymax = $limits[3];

        if ($xmax <= $xmin || $ymax <= $ymin) {
            ERROR(sprintf "'box' value is not logical for a bbox (max < min) : %s !",$self->{extent});
            return FALSE ;
        }

        $GMLextent = sprintf "<gml:Polygon><gml:outerBoundaryIs><gml:LinearRing><gml:coordinates>%s,%s %s,%s %s,%s %s,%s %s,%s</gml:coordinates></gml:LinearRing></gml:outerBoundaryIs></gml:Polygon>",
            $xmin,$ymin,
            $xmin,$ymax,
            $xmax,$ymax,
            $xmax,$ymin,
            $xmin,$ymin;
    }
    elsif (scalar @limits == 1) {
        # user supplied a file which contains bounding polygon
        if (! -f $self->{extent}) {
            ERROR (sprintf "Shape file ('%s') doesn't exist !",$self->{extent});
            return FALSE;
        }
        
        if (! open SHAPE, "<", $self->{extent} ){
            ERROR(sprintf "Cannot open the shape file %s.",$self->{extent});
            return FALSE;
        }

        $GMLextent = '';
        while( defined( my $line = <SHAPE> ) ) {
            $GMLextent .= $line;
        }
        close(SHAPE);
    } else {
        ERROR(sprintf "The value for 'extent' is not valid (must be a BBOX or a file with a GML shape) : %s.",
            $self->{extent});
        return FALSE;
    }

    if (! defined $GMLextent) {
        ERROR(sprintf "Cannot define the string from the parameter 'extent' (GML) => %s.",$self->{extent});
        return FALSE;
    }

    # We use extent to define a GML string, Now, we store in this attribute the equivalent OGR Geometry
    $self->{extent} = undef;

    eval { $self->{extent} = Geo::OGR::Geometry->create(GML=>$GMLextent); };
    if ($@) {
        ERROR(sprintf "GML geometry (%s) is not valid : %s",$GMLextent,$@);
        return FALSE;
    }

    if (! defined $self->{extent}) {
        ERROR(sprintf "Cannot create a Geometry from the string : %s.",$GMLextent);
        return FALSE;
    }

    my $bboxref = $self->{extent}->GetEnvelope();
    my ($xmin,$xmax,$ymin,$ymax) = ($bboxref->[0],$bboxref->[1],$bboxref->[2],$bboxref->[3]);
    if (! defined $xmin) {
        ERROR("Cannot calculate bbox from the OGR Geometry");
        return FALSE;
    }
    $self->{bbox} = [$xmin,$ymin,$xmax,$ymax];

    return TRUE;

}


####################################################################################################
#                                       GETTERS / SETTERS                                          #
####################################################################################################

# Group: getters - setters

sub getSRS {
    my $self = shift;
    return $self->{srs};
}

sub getImages {
    my $self = shift;
    return $self->{imageSource}->getImages;
}

sub hasImages {
    my $self = shift;
    return (defined $self->{imageSource});
}

sub hasHarvesting {
    my $self = shift;
    return (defined $self->{harvesting});
}

sub removeHarvesting {
    my $self = shift;
    $self->{harvesting} = undef;
}


1;
__END__

=head1 NAME

BE4::DataSource - Managing a data source

=head1 SYNOPSIS

    use BE4::DataSource;

    # DataSource object creation : 3 cases
    
    # Real Data and no harvesting : native SRS and lossless compression
    my $objDataSource = BE4::DataSource->new(
        "19",
        {
            srs => "IGNF:LAMB93",
            path_image => "/home/ign/DATA/BDORTHO"
        }
    );
    
    # No Data, just harvesting (here for a WMS vector)
    my $objDataSource = BE4::DataSource->new(
        "19",
        {
            srs => IGNF:WGS84G,
            extent => /home/ign/SHAPE/GMLPolygon.txt,
            
            wms_layer   => "tp:TRONCON_ROUTE",
            wms_url => "http://geoportail/wms/",
            wms_version => "1.3.0",
            wms_request => "getMap",
            wms_format  => "image/png",
            wms_bgcolor => "0xFFFFFF",
            wms_transparent  => "FALSE",
            wms_style  => "line",
            min_size => 9560,
            image_width => 1024,
            image_height => 1024
        }
    );
    
    # Real Data and harvesting : reprojection or lossy compression
    my $objDataSource = BE4::DataSource->new(
        "19",
        {
            srs => "IGNF:LAMB93",
            path_image => "/home/ign/DATA/BDORTHO"
            wms_layer => "ORTHO_XXX",
            wms_url => "http://geoportail/wms/",
            wms_version => "1.3.0",
            wms_request => "getMap",
            wms_format => "image/tiff"
        }
    );

=head1 DESCRIPTION

=head2 ATTRIBUTES

=over 4
  
=item bottomLevelID, bottomLevelOrder

ID (in TMS) and order (integer) of the base level, from which this datasource is used.

=item topLevelID, topLevelOrder

ID (in TMS) and order (integer) of the top level, to which this datasource is used, calculated in relation to other datasource
        
=item extent

An OGR geometry, extent of the data source (calculate from ImageSource or supplied in configuration)

=item bbox

Bbox of extent, an array [xmin,ymin,xmax,ymax].

=item srs

SRS of the bottom extent (and ImageSource objects if exists)
    
=item imageSource

An ImageSource object, can be undefined
    
=item harvesting

An Harvestingobject, can be undefined. If it is useless, it will be remove.

=back

'extent' is mandatory (a bbox or a file which contains a GML geometry) if there are no images. We have to know area to harvest. If images, extent is calculated thanks data.

=head1 LIMITATION & BUGS

Metadata managing not yet implemented.

=head1 SEE ALSO

=head2 POD documentation

=begin html

<ul>
<li><A HREF="./lib-BE4-Harvesting.html">BE4::Harvesting</A></li>
<li><A HREF="./lib-BE4-ImageSource.html">BE4::ImageSource</A></li>
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
