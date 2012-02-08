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

package BE4::Layer;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
use XML::LibXML;

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
# Global template

my $STRLYRTMPLT   = <<"TLYR";
<?xml version='1.0' encoding='US-ASCII'?>
<layer>
    <title>__TITLE__</title>
    <abstract>__ABSTRACT__</abstract>
    <keywordList>
        <!-- __KEYWORDS__ -->
    </keywordList>
    <style>__STYLE__</style>
    <EX_GeographicBoundingBox>
        <westBoundLongitude>__W__</westBoundLongitude>
        <eastBoundLongitude>__E__</eastBoundLongitude>
        <southBoundLatitude>__S__</southBoundLatitude>
        <northBoundLatitude>__N__</northBoundLatitude>
    </EX_GeographicBoundingBox>
    <WMSCRSList>
        <!-- __SRSS__ -->
    </WMSCRSList>
    <boundingBox CRS="__SRS__" minx="__MINX__" miny="__MINY__" maxx="__MAXX__" maxy="__MAXY__"/>
    <opaque>__OPAQUE__</opaque>
    <authority>__AUTHORITY__</authority>
    <resampling>__RESAMPLING__</resampling>
    <pyramid>__PYRAMID__</pyramid>
</layer>
TLYR

my $STRTKTMPLT   = <<"TK";
    <keyword>__KEYWORD__</keyword>
    <!-- __KEYWORDS__ -->
TK

my $STRSRSTMPLT   = <<"TSRS";  
    <WMSCRS>__SRS__</WMSCRS>
    <!-- __SRSS__ -->
TSRS

#
# Group: constructor
#

################################################################################
# constructor
sub new {
  my $this = shift;

  my $class= ref($this) || $this;
  my $self = {
	title            => undef,       # manadatory !
        abstract         => "",          # by default (optional) !
        keywordlist      => [],          # by default (optional) !
        style            => "normal",    # by default !
        minres           => 0.5,         # by default !
        maxres           => 1,           # by default !
        opaque           => 1,           # by default !
        authority        => "IGNF",      # by default !
        srslist          => [],          # manadatory !
        resampling       => "lanczos_4", # by default !
        geo_bbox         => [],          # manadatory: minx,miny,maxx,maxy !
        proj             => undef,       # by default first param srslist (optional) !
        proj_bbox        => [],          # manadatory: minx,miny,maxx,maxy !
        pyramid          => undef,       # manadatory: path of pyr. desc. !
  };

  bless($self, $class);
  
  TRACE;
  
  # init. class
  if (! $self->_init(@_)) {
    ERROR ("One parameter is missing !");
    return undef;
  }
  
  return $self;
}

################################################################################
# privates init.
sub _init {
    my $self   = shift;
    my $params = shift;

    TRACE;
    
    return FALSE if (! defined $params);
    
    # init. params
    
    # parameters mandatoy !
    if (! exists($params->{title})) {
      ERROR ("key/value required to 'title' !");
      return FALSE;
    }

    if (! exists($params->{pyramid})) {
      ERROR ("key/value required to 'pyramid' !");
      return FALSE;
    }

    if (! exists($params->{geo_bbox}) ||
        ! scalar (@{$params->{geo_bbox}})){
      ERROR("list empty to 'geo_bbox' !");
      return FALSE;
    }
    
    if (! exists($params->{proj_bbox}) ||
        ! scalar (@{$params->{proj_bbox}})){
      ERROR("list empty to 'proj_bbox' !");
      return FALSE;
    }
    
    if (! scalar (@{$params->{srslist}})){
      ERROR("list empty to 'srslist' !");
      return FALSE;
    }
    
    # parameters optional or by default !
    if (! exists($params->{proj}) ||
        ! defined ($params->{proj})) {
        $self->{proj} = $params->{srslist}->[0];
      WARN (sprintf "key/value optional to 'proj' (first value '%s' of listsrs by default)!",
            $self->{proj});
      $params->{proj} = $self->{proj};
    }
    
    if (! exists($params->{abstract}) ||
        ! defined ($params->{abstract})) {
      WARN (sprintf "key/value optional to 'abstract' ('%s' by default)!",
            $self->{abstract});
      $params->{abstract} = $self->{abstract};
    }
    
    if (! exists($params->{style}) ||
        ! defined ($params->{style})) {
      WARN (sprintf "key/value optional to 'style' ('%s' by default)!",
            $self->{style});
      $params->{style} = $self->{style};
    }
    
    if (! exists($params->{minres}) ||
        ! defined ($params->{minres})) {
      WARN (sprintf "key/value optional to 'minres' ('%s' by default)!",
            $self->{minres});
      $params->{minres} = $self->{minres};
    }
    
    if (! exists($params->{maxres}) ||
        ! defined ($params->{maxres})) {
      WARN (sprintf "key/value optional to 'maxres' ('%s' by default)!",
            $self->{maxres});
      $params->{maxres} = $self->{maxres};
    }
    
    if (! exists($params->{resampling}) ||
        ! defined ($params->{resampling})) {
      WARN (sprintf "key/value optional to '' ('%s' by default)!",
            $self->{resampling});
      $params->{resampling} = $self->{resampling};
    }
    
    if (! exists($params->{authority}) ||
        ! defined ($params->{authority})) {
      WARN (sprintf "key/value optional to 'authority' ('%s' by default)!",
            $self->{authority});
      $params->{authority} = $self->{authority};
    }
    
    if (! exists($params->{opaque}) ||
        ! defined ($params->{opaque})) {
      WARN (sprintf "key/value optional to 'opaque' ('%s' by default)!",
            $self->{opaque});
        $params->{opaque} = $self->{opaque};
    }
    
    if (! exists($params->{keywordlist}) ||
        ! scalar (@{$params->{keywordlist}})){
      WARN("list empty to 'keywordlist' !");
      @{$params->{keywordlist}} = [];
    }
    
    # test
    if (! -f $params->{pyramid}) {
      ERROR ("The path of file doesn't exist !");
      return FALSE;
    }
    
    if (scalar (@{$params->{geo_bbox}}) != 4) {
      ERROR ("bbox not correct !");
      return FALSE;
    }
    
    if (scalar (@{$params->{proj_bbox}}) != 4) {
      ERROR ("bbox not correct !");
      return FALSE;
    }
    
    # save
    $self->{title}     = $params->{title};
    $self->{pyramid}   = $params->{pyramid};
    $self->{geo_bbox}  = $params->{geo_bbox};
    $self->{proj_bbox} = $params->{proj_bbox};
    $self->{srslist}   = $params->{srslist};
    
    $self->{keywordlist} = $params->{keywordlist};
    $self->{authority}   = $params->{authority};
    $self->{minres}      = $params->{minres};
    $self->{maxres}      = $params->{maxres};
    $self->{style}       = $params->{style};
    $self->{abstract}    = $params->{abstract};
    $self->{proj}        = $params->{proj};
    $self->{resampling}  = $params->{resampling};
    $self->{opaque}      = $params->{opaque};
    
    return TRUE;
}
################################################################################
# public function.
sub to_string {
  my $self = shift;
  
  TRACE;
  
  my $string = undef;
  
  # parsing template
  my $parser = XML::LibXML->new();
  
  my $doctpl = eval { $parser->parse_string($STRLYRTMPLT); };
  
  if (!defined($doctpl) || $@) {
    ERROR(sprintf "Can not parse template file of layer : %s !", $!);
    return undef;
  }
  
  my $strlyrtmplt = $doctpl->toString(0);
  
  my $title    = $self->{title};
  $strlyrtmplt =~ s/__TITLE__/$title/;
  
  my $style    = $self->{style};
  $strlyrtmplt =~ s/__STYLE__/$style/;
  
  my $abstract = $self->{abstract};
  $strlyrtmplt =~ s/__ABSTRACT__/$abstract/;
  
  my $minres   = $self->{minres};
  $strlyrtmplt =~ s/__MINRES__/$minres/;
  my $maxres   = $self->{maxres};
  $strlyrtmplt =~ s/__MAXRES__/$maxres/;
  
  my $opaque   = $self->{opaque};
  $strlyrtmplt =~ s/__OPAQUE__/$opaque/;
  
  my $resampling = $self->{resampling};
  $strlyrtmplt   =~ s/__RESAMPLING__/$resampling/;
  
  my $pyramid  = $self->{pyramid};
  $strlyrtmplt =~ s/__PYRAMID__/$pyramid/;
  
  my $authority  = $self->{authority};
  $strlyrtmplt   =~ s/__AUTHORITY__/$authority/;
  
  my ($minx,$miny,$maxx,$maxy) = @{$self->{proj_bbox}};
  $strlyrtmplt =~ s/__MINX__/$minx/;
  $strlyrtmplt =~ s/__MINY__/$miny/;
  $strlyrtmplt =~ s/__MAXX__/$maxx/;
  $strlyrtmplt =~ s/__MAXY__/$maxy/;
  
  my ($w,$s,$e,$n) = @{$self->{geo_bbox}};
  my $srs = $self->{proj};
  $strlyrtmplt =~ s/__SRS__/$srs/;
  $strlyrtmplt =~ s/__E__/$e/;
  $strlyrtmplt =~ s/__W__/$w/;
  $strlyrtmplt =~ s/__N__/$n/;
  $strlyrtmplt =~ s/__S__/$s/;
  
  foreach (@{$self->{srslist}}) {
    $strlyrtmplt =~ s/<!-- __SRSS__ -->\n/$STRSRSTMPLT/;
    $strlyrtmplt =~ s/__SRS__/$_/;
  }
  $strlyrtmplt =~ s/<!-- __SRSS__ -->\n//;
  
  foreach (@{$self->{keywordlist}}) {
    $strlyrtmplt =~ s/<!-- __KEYWORDS__ -->\n/$STRTKTMPLT/;
    $strlyrtmplt =~ s/__KEYWORD__/$_/;
  }
  $strlyrtmplt =~ s/<!-- __KEYWORDS__ -->\n//;

  $strlyrtmplt =~ s/^$//g;
  $strlyrtmplt =~ s/^\n$//g;
  
  $string = $strlyrtmplt;
  
  return $string;
}

################################################################################
# Preloaded methods go here.
BEGIN {}
INIT {}
END {}

1;
__END__

# Below is stub documentation for your module. You'd better edit it!

=head1 NAME

  BE4::Layer -

=head1 SYNOPSIS

  use BE4::Layer;
  
  my $params = {
            title            => "Ortho IGN en RAW",
            abstract         => "Projection native",
            keywordlist      => [],
            style            => "normal",    # normal by default !
            minRes           => 0.5,
            maxRes           => 1,
            opaque           => 1,           # 0|1 !
            authority        => "IGNF",      # IGNF by default !
            srslist          => [ "IGNF:LAMB93",
                                  "IGNF:RGF93G"],
            resampling       => "lanczos_4", # type of values : ...
            geo_bbox         => [, , , ],    # minx,miny,maxx,maxy
            proj             => "IGNF:LAMB93",
            proj_bbox        => [, , , ],    # minx,miny,maxx,maxy
            pyramid          => "",          # path of pyr. desc. !
    };
    
  my $objLayer = new BE4::Layer($params);
  $objLayer->to_string();

=head1 DESCRIPTION

=head2 EXPORT

None by default.

=head1 SAMPLE

* Sample Pyramid file (.pyr) :

  eg SEE ASLO

* Sample TMS file (.tms) :

  eg SEE ASLO

* Sample LAYER file (.lay) :

  [SCAN_RAW_TEST.lay]
  
  <layer>
	<title>Scan IGN en RAW</title>
	<abstract></abstract>
	<keywordList>
		<keyword></keyword>
	</keywordList>
	<style>normal</style>
	<minRes>0.5</minRes>
	<maxRes>1</maxRes>
	<EX_GeographicBoundingBox>
                <westBoundLongitude>0</westBoundLongitude>
                <eastBoundLongitude>10</eastBoundLongitude>
                <southBoundLatitude>40</southBoundLatitude>
                <northBoundLatitude>50</northBoundLatitude>
        </EX_GeographicBoundingBox>
	<WMSCRSList>
		<WMSCRS>IGNF:LAMB93</WMSCRS>
	</WMSCRSList>
	<boundingBox CRS="IGNF:LAMB93" minx="805888" miny="6545408" maxx="806912" maxy="6546432"/>
	<opaque>true</opaque>
	<authority>IGNF</authority>
	<resampling>lanczos_4</resampling>
	<pyramidList>
		<pyramid>../config/pyramids/SCAN_RAW_TEST.pyr</pyramid>
	</pyramidList>
  </layer>

=head1 SEE ALSO

 eg package module following :
 
 BE4::Pyramid
 BE4::TileMatrixSet
 BE4::Level

=head1 LIMITATIONS AND BUGS

=head1 AUTHOR

Bazonnais Jean Philippe, E<lt>jpbazonnais@E<gt>

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2011 by Bazonnais Jean Philippe

This library is free software; you can redistribute it and/or modify
it under the same terms as Perl itself, either Perl version 5.10.1 or,
at your option, any later version of Perl 5 you may have available.

=cut

