package BE4::Harvesting;

use strict;
use warnings;

use Log::Log4perl qw(:easy);

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
#    [ harvesting ]
#    ; wms_url     =
#    ; wms_version =
#    ; wms_request =
#    ; wms_format  =
#    ; wms_layer =

################################################################################
# Preloaded methods go here.
BEGIN {}
INIT {}
END {}

#
# Group: variable
#

#
# variable: $self
#
#    *   URL      => undef, # ie url of rok4 !
#    *   VERSION  => undef, # ie 1.3.0
#    *   REQUEST  => undef, # ie getMap
#    *   FORMAT   => undef, # ie image/png
#    *   LAYER    => undef, # ie ORTHOPHOTO
#

#
# Group: constructor
#

################################################################################
# constructor
sub new {
  my $this = shift;

  my $class= ref($this) || $this;
  my $self = {
    URL      => undef, # ie url of rok4 !
    VERSION  => undef, # ie 1.3.0
    REQUEST  => undef, # ie getMap
    FORMAT   => undef, # ie image/png
    LAYER    => undef, # ie ORTHOPHOTO
  };

  bless($self, $class);
  
  TRACE;
  
  # init. class
  return undef if (! $self->_init(@_));
  
  return $self;
}

################################################################################
# privates init.
sub _init {
    my $self   = shift;
    my $params = shift;

    TRACE;
    
    return FALSE if (! defined $params);
    
    # parameters mandatoy !
    if (! exists($params->{wms_url})     || ! defined ($params->{wms_url})) {
      ERROR("key/value required to 'wms_url' !");
      return FALSE ;
    }
    if (! exists($params->{wms_version}) || ! defined ($params->{wms_version})) {
      ERROR("key/value required to 'wms_version' !");
      return FALSE ;
    }
    if (! exists($params->{wms_request}) || ! defined ($params->{wms_request})) {
      ERROR("key/value required to 'wms_request' !");
      return FALSE ;
    }
    if (! exists($params->{wms_format})  || ! defined ($params->{wms_format})) {
      ERROR("key/value required to 'wms_format' !");
      return FALSE ;
    }
    if (! exists($params->{wms_layer})   || ! defined ($params->{wms_layer})) {
      ERROR("key/value required to 'wms_layer' !");
      return FALSE ;
    }
    
    # init. params    
    $self->url(     $params->{wms_url});
    $self->version ($params->{wms_version});
    $self->request( $params->{wms_request});
    $self->format(  $params->{wms_format});
    $self->layer(   $params->{wms_layer});
    
    # FIXME
    #   it's just an information ! Not an error !
    #return FALSE if (! $self->_doRequest());
    $self->_doRequest();
    
    return TRUE;
}
################################################################################
# private method
sub _doRequest {
 my $self = shift;
 
 use HTTP::Request;
 use LWP::UserAgent;
 
 my $url = sprintf ("http://%s?VERSION=%s&SERVICE=WMS&REQUEST=GetCapabilities", $self->url(), $self->version());
 my $request = HTTP::Request->new(GET => $url);
 my $ua = LWP::UserAgent->new;
 my $response = $ua->request($request);
 
 if ($response->is_error()) {
    ERROR (sprintf "The response is an error :%s (url : '%s')!", $response->status_line(), $url);
    return FALSE;
 }
 
 TRACE (sprintf "Response is : %s (-> url : %s) !", $response->status_line(), $url);
 
 return TRUE;
}
################################################################################
# public method
sub doRequestUrl {
  my $self = shift;
  
  # ie "http://".$URL."?LAYERS=".$LAYER."
  #  &SERVICE=WMS
  #  &VERSION=".$VERSION."
  #  &REQUEST=GetMap
  #  &FORMAT=image/tiff
  #  &CRS=".$srs."
  #  &BBOX=".$xmin.",".$ymin.",".$xmax.",".$ymax."
  #  &WIDTH=".$image_width."
  #  &HEIGHT=".$image_height.";

  # FIXME : no validity test of url !
  
  my %args = @_;
  
  TRACE;

  my $srs       = $args{srs}       || ( ERROR ("'srs' parameter required !") && return undef );
  my $bbox      = $args{bbox}      || ( ERROR ("'bbox' parameter required !") && return undef );
  my $imagesize = $args{imagesize} || ( ERROR ("'imagesize' parameter required !") && return undef );
  
  my ($xmin, $ymin, $xmax, $ymax)  = @{$bbox};
  my ($image_width, $image_height) = @{$imagesize};
  
  my $url = sprintf ("http://%s?LAYERS=%s&SERVICE=WMS&VERSION=%s&REQUEST=%s&FORMAT=%s&CRS=%s&BBOX=%s,%s,%s,%s&WIDTH=%s&HEIGHT=%s",
                     $self->url(),
                     $self->layer(),
                     $self->version(),
                     $self->request(),
                     $self->format(),
                     $srs,
                     $xmin, $ymin, $xmax, $ymax,
                     $image_width, $image_height);
  
  return $url;
}

################################################################################
# get/set
# TODO : check parameters...
sub url {
  my $self = shift;
  if (@_) {
    my $string = shift;
    $string =~ s/^http:\/\///; # ...
    $string =~ s/\?$//;        # ...
    $self->{URL} = $string;
  }
  return $self->{URL};
}

sub version {
  my $self = shift;
  if (@_) { $self->{VERSION} = shift }
  return $self->{VERSION};
}

sub request {
  my $self = shift;
  if (@_) { $self->{REQUEST} = shift }
  return $self->{REQUEST};
}
sub format {
  my $self = shift;
  if (@_) { $self->{FORMAT} = shift }
  return $self->{FORMAT};
}
sub layer {
  my $self = shift;
  if (@_) { $self->{LAYER} = shift }
  return $self->{LAYER};
}

################################################################################
# mapping
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
sub getWMSLayer {
  my $self = shift;
  return $self->{LAYER};
}
sub getWMSServer {
  my $self = shift;
  return $self->{URL};
}
1;
__END__

# Below is stub documentation for your module. You'd better edit it!

=head1 NAME

  BE4::Harvesting - 

=head1 SYNOPSIS

  use BE4::Harvesting;
  
  my $params = {
    wms_url     => "deuchars.ign.fr/rok4/bin/rok4",
    wms_version => "1.3.0",
    wms_request => "GetMap",
    wms_format  => "image/tiff",
    wms_layer   => "test",
  };

  my $objH = BE4::Harvesting->new($params);
  my $url  = $objH->doRequestUrl(srs=> "WGS84", bbox => [0,0,500,500], imagesize => [100,100]);
  

=head1 DESCRIPTION

=head2 EXPORT

None by default.

=head1 SEE ALSO

=head1 AUTHOR

Bazonnais Jean Philippe, E<lt>jpbazonnais@E<gt>

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2011 by Bazonnais Jean Philippe

This library is free software; you can redistribute it and/or modify
it under the same terms as Perl itself, either Perl version 5.10.1 or,
at your option, any later version of Perl 5 you may have available.

=cut