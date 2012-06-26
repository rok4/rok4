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

package BE4::Harvesting;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
use Data::Dumper;

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
    * URL      => undef, # ie url of rok4 !
    * VERSION  => undef, # ie 1.3.0
    * REQUEST  => undef, # ie getMap
    * FORMAT   => undef, # ie image/png
    * LAYERS   => undef, # ie ORTHOPHOTO,ROUTE,...
    * image_width => 4096, # max images size which will be harvested, can be undefined
    * image_height => 4096
=cut

####################################################################################################
#                                       CONSTRUCTOR METHODS                                        #
####################################################################################################

# Group: constructor

sub new {
    my $this = shift;

    my $class= ref($this) || $this;
    my $self = {
        URL      => undef,
        VERSION  => undef,
        REQUEST  => undef,
        FORMAT   => undef,
        LAYERS    => undef,
        image_width => undef,
        image_height => undef
    };

    bless($self, $class);

    TRACE;

    # init. class
    return undef if (! $self->_init(@_));

    return $self;
}

sub _init {
    my $self   = shift;
    my $params = shift;

    TRACE;
    
    return FALSE if (! defined $params);
    
    # 'image_width' and 'image_height' are optionnal, but if one is defined, the other must be defined
    if (exists($params->{image_width}) && defined ($params->{image_width})) {
        $self->{image_width} = $params->{image_width};
        if (exists($params->{image_height}) && defined ($params->{image_height})) {
            $self->{image_height} = $params->{image_height};
        } else {
            ERROR("If parameter 'image_width' is defined, parameter 'image_height' must be defined !");
            return FALSE ;
        }
    } else {
        if (exists($params->{image_height}) && defined ($params->{image_height})) {
            ERROR("If parameter 'image_height' is defined, parameter 'image_width' must be defined !");
            return FALSE ;
        }
    }

    # Other parameters are mandatory
    if (! exists($params->{wms_url}) || ! defined ($params->{wms_url})) {
        ERROR("Parameter 'wms_url' is required !");
        return FALSE ;
    }
    if (! exists($params->{wms_version}) || ! defined ($params->{wms_version})) {
        ERROR("Parameter 'wms_version' is required !");
        return FALSE ;
    }
    if (! exists($params->{wms_request}) || ! defined ($params->{wms_request})) {
        ERROR("Parameter 'wms_request' is required !");
        return FALSE ;
    }
    if (! exists($params->{wms_format}) || ! defined ($params->{wms_format})) {
        ERROR("Parameter 'wms_format' is required !");
        return FALSE ;
    }
    if (! exists($params->{wms_layer}) || ! defined ($params->{wms_layer})) {
        ERROR("Parameter 'wms_layer' is required !");
        return FALSE ;
    }

    $params->{wms_layer} =~ s/ //;
    my @layers = split (/,/,$params->{wms_layer},-1);
    foreach my $layer (@layers) {
        if ($layer eq '') {
            ERROR(sprintf "Value for 'wms_layer' is not valid (%s) : it must be LAYER[{,LAYER_N}] !",$params->{wms_layer});
            return FALSE ;
        }
        INFO(sprintf "Layer %s will be harvested !",$layer);
    }
    
    # init. params    
    $self->url($params->{wms_url});
    $self->version ($params->{wms_version});
    $self->request($params->{wms_request});
    $self->format($params->{wms_format});
    $self->layers($params->{wms_layer});

    return TRUE;
}

####################################################################################################
#                                      REQUEST METHOD                                              #
####################################################################################################

# Group: request method

#
=begin nd
   method: doRequestUrl

   Abstract

   Parameters:
      srs - bbox's SRS
      bbox - extent of the harvested image
      imagesize - pixel size of the harvested image

   Returns:
      An urls' array : one request if no size restrictions, several if asked image is too big for the service
=cut
sub doRequestUrl {
    my $self = shift;

    my %args = @_;

    TRACE;

    my $srs       = $args{srs}       || ( ERROR ("'srs' parameter required !") && return undef );
    my $bbox      = $args{bbox}      || ( ERROR ("'bbox' parameter required !") && return undef );
    my $imagesize = $args{imagesize} || ( ERROR ("'imagesize' parameter required !") && return undef );

    my ($xmin, $ymin, $xmax, $ymax)  = @{$bbox};
    my ($image_width, $image_height) = @{$imagesize};
    
    if (defined $self->{image_width} && 
        ($self->{image_width} < $image_width && $image_width % $self->{image_width} != 0) ||
        ($self->{image_height} < $image_height && $image_height % $self->{image_height} != 0))
    {
        ERROR(sprintf "Harvesting size is not compatible with the image's size in the request.");
        return ();;
    }
    
    # Size in harvested image of the asked image
    my $imagePerWidth = int($image_width/$self->{image_width});
    my $imagePerHeight = int($image_height/$self->{image_height});
    
    # Ground size of an harvested image
    my $groundHeight = ($xmax-$xmin)/$imagePerHeight;
    my $groundWidth = ($ymax-$ymin)/$imagePerWidth;
    
    my @requests;

    for (my $i = 0; $i < $imagePerHeight; $i++) {
        for (my $j = 0; $j < $imagePerHeight; $j++) {
            my $url = sprintf ("http://%s?LAYERS=%s&SERVICE=WMS&VERSION=%s&REQUEST=%s&FORMAT=%s&CRS=%s&BBOX=%s,%s,%s,%s&WIDTH=%s&HEIGHT=%s&STYLES=",
                    $self->url(),
                    $self->layers(),
                    $self->version(),
                    $self->request(),
                    $self->format(),
                    $srs,
                    $xmin+$j*$groundWidth, $ymax-($i+1)*$groundHeight,
                    $xmin+($j+1)*$groundWidth, $ymax-$i*$groundHeight,
                    $self->{image_width}, $self->{image_height});
            push @requests,$url;
        }
    }

    return @requests;
}

####################################################################################################
#                                       GETTERS / SETTERS                                          #
####################################################################################################

# Group: getters - setters

sub url {
    my $self = shift;
    if (@_) {
        my $string = shift;
        $string =~ s/^http:\/\///;
        $string =~ s/\?$//;
        $self->{URL} = $string;
    }
    return $self->{URL};
}

sub version {
    my $self = shift;
    if (@_) { $self->{VERSION} = shift; }
    return $self->{VERSION};
}

sub request {
  my $self = shift;
  if (@_) { $self->{REQUEST} = shift; }
  return $self->{REQUEST};
}
sub format {
    my $self = shift;
    if (@_) { $self->{FORMAT} = shift; }
    return $self->{FORMAT};
}
sub layers {
    my $self = shift;
    if (@_) { $self->{LAYERS} = shift; }
    return $self->{LAYERS};
}


sub getWMSVersion {
    my $self = shift;
    return $self->{VERSION};
}
sub getWMSRequest {
    my $self = shift;
    return $self->{REQUEST};
}
sub getWMSFormat {
    my $self = shift;
    return $self->{FORMAT};
}
sub getWMSLayers {
    my $self = shift;
    return $self->{LAYERS};
}
sub getWMSServer {
    my $self = shift;
    return $self->{URL};
}

1;
__END__

=head1 NAME

BE4::Harvesting - Declare WMS service

=head1 SYNOPSIS

    use BE4::Harvesting;

    # Image width and height not defined

    # Harvesting object creation
    my $objHarvesting = BE4::Harvesting->new({
        wms_layer   => "ORTHO_RAW_LAMB93_PARIS_OUEST",
        wms_url     => "http://localhost/wmts/rok4",
        wms_version => "1.3.0",
        wms_request => "getMap",
        wms_format  => "image/tiff"
    });
    
    # Do a request
    my @requests = $objHarvesting->doRequestUrl(
        srs=> "WGS84",
        bbox => [5,47,6,48],
        imagesize => [4096,4096]
    );
    
    # @requests contains one url
    # http://http://localhost/wmts/rok4?LAYERS=ORTHO_RAW_LAMB93_PARIS_OUEST&SERVICE=WMS&VERSION=1.3.0&
    # REQUEST=getMap&FORMAT=image/tiff&CRS=WGS84&BBOX=5,47,6,48&WIDTH=4096&HEIGHT=4096&STYLES=
    
    OR
    
    # Image width and height defined
    
    # Harvesting object creation
    my $objHarvesting = BE4::Harvesting->new({
        wms_layer   => "ORTHO_RAW_LAMB93_PARIS_OUEST",
        wms_url     => "http://localhost/wmts/rok4",
        wms_version => "1.3.0",
        wms_request => "getMap",
        wms_format  => "image/tiff"
        image_width  => 1024
        image_height  => 1024
    });


    # Do a request
    my @requests = $objHarvesting->doRequestUrl(
        srs=> "WGS84",
        bbox => [5,47,6,48],
        imagesize => [4096,4096]
    );
    
    # @requests contains 16 urls
    # http://http://localhost/wmts/rok4?LAYERS=ORTHO_RAW_LAMB93_PARIS_OUEST&SERVICE=WMS&VERSION=1.3.0&
    # REQUEST=getMap&FORMAT=image/tiff&CRS=WGS84&BBOX=5,47,5.25,47.25&WIDTH=4096&HEIGHT=4096&STYLES=
    # ,
    # http://http://localhost/wmts/rok4?LAYERS=ORTHO_RAW_LAMB93_PARIS_OUEST&SERVICE=WMS&VERSION=1.3.0&
    # REQUEST=getMap&FORMAT=image/tiff&CRS=WGS84&BBOX=5.25,47,5.5,47.25&WIDTH=4096&HEIGHT=4096&STYLES=
    # .
    # .
    # .
    # http://http://localhost/wmts/rok4?LAYERS=ORTHO_RAW_LAMB93_PARIS_OUEST&SERVICE=WMS&VERSION=1.3.0&
    # REQUEST=getMap&FORMAT=image/tiff&CRS=WGS84&BBOX=5.75,47.75,6,48&WIDTH=4096&HEIGHT=4096&STYLES=

=head1 DESCRIPTION

=head2 ATTRIBUTES

=over 4

=item URL

URL of rok4.

=item VERSION

1.3.0 for WMS.

=item REQUEST

"getMap"

=item FORMAT

Image/tiff,png... or image/x-bil;bits=32

=item LAYERS

Name of the harvested resource

=item image_width, image_height

If not defined, images will be harvested all-in-one. If defined, requested image size will have to be a multiple of this size.

=back

=head1 SEE ALSO

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
