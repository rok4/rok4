# Copyright © (2011) Institut national de l'information
#                    géographique et forestière 
# 
# Géoportail SAV <contact.geoservices@ign.fr>
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
File: Layer.pm

Class: COMMON::Layer

(see ROK4GENERATION/libperlauto/COMMON_Layer.png)

Describe a layer for a ROK4 server.

Using:
    (start code)
    use COMMON::Layer;

    # Layer object creation
    my $objLayer = new COMMON::Layer({
        title            => "Ortho IGN en RAW",
        abstract         => "Projection native",
        keywordlist      => ["key","word"],
        style            => "normal",
        minRes           => 0.5,
        maxRes           => 1,
        opaque           => 1,
        authority        => "IGNF",
        srslist          => [ "IGNF:LAMB93","IGNF:RGF93G"],
        resampling       => "lanczos_4",
        geo_bbox         => [0,40,10,50],
        proj             => "IGNF:LAMB93",
        proj_bbox        => [805888,6545408,806912,6546432],
        pyramid          => "/pyramids/ORTHO.pyr",
    });

    my $XMLlayer = $objLayer->exportToXML();
    (end code)

Attributes:
    title - string - Layer's name.
    abstract - string - Abstract, to resume the layer content.
    keywordlist - string array - List of keywords.
    style - string - Layer's style.
    minres - float - Minimal resolution of the pyramid.
    maxres - float - Maximal resolution of the pyramid.
    opaque - boolean - 1 by default.
    authority - string - SRS's authority : IGNF:LAMB93 -> IGNF.
    srslist - string array - List of available SRS for the layer.
    resampling - string - Used interpolation to generate images used by this layer.
    geo_bbox - double array - [minx,miny,maxx,maxy] in WGS84G.
    proj - string - Data native projection.
    proj_bbox - double array - [minx,miny,maxx,maxy] in pyramid's SRS.
    pyramid - string - Pyramid's descriptor path.
=cut

################################################################################

package COMMON::Layer;

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
# Constantes
use constant TRUE  => 1;
use constant FALSE => 0;

# Constant: STRLYRTMPLT
# Define the template XML for a layer
my $STRLYRTMPLT   = <<"TLYR";
<?xml version='1.0' encoding='UTF-8'?>
<layer>
    <title>__TITLE__</title>
    <abstract>__ABSTRACT__</abstract>
    <keywordList>
    <!-- __KEYWORDS__ -->
</keywordList>
    __STYLE__
    <EX_GeographicBoundingBox>
        <westBoundLongitude>__W__</westBoundLongitude>
        <eastBoundLongitude>__E__</eastBoundLongitude>
        <southBoundLatitude>__S__</southBoundLatitude>
        <northBoundLatitude>__N__</northBoundLatitude>
    </EX_GeographicBoundingBox>
    __SRSS__
    <boundingBox CRS="__SRS__" minx="__MINX__" miny="__MINY__" maxx="__MAXX__" maxy="__MAXY__"/>
    <minRes>__MINRES__</minRes>
    <maxRes>__MAXRES__</maxRes>
    __OPAQUE__
    <authority>__AUTHORITY__</authority>
    __RESAMPLING__
    <pyramid>__PYRAMID__</pyramid>
</layer>
TLYR

# Constant: STRTKTMPLT
# Define the template XML for the keywords part of a layer.
my $STRTKTMPLT = <<"TK";
    <keyword>__KEYWORD__</keyword>
    <!-- __KEYWORDS__ -->
TK

# Constant: STRSRSTMPLT
# Define the template XML for the SRS part of a layer.
my $STRSRSTMPLT = <<"TSRS";
        <WMSCRS>__SRS__</WMSCRS>
TSRS

# Constant: DEFAULT
# Define default values for optionnal attributes.
my %DEFAULT;

################################################################################

BEGIN {}
INIT {
    %DEFAULT = (
        abstract         => "",
        keywordlist      => [],
        minres           => 0.0,
        maxres           => 0.0,
        authority        => "IGNF",
        resampling       => "lanczos_4",
    );    
}
END {}

####################################################################################################
#                                        Group: Constructors                                       #
####################################################################################################

=begin nd
Constructor: new

Layer constructor. Bless an instance.

Parameters (hash):
    title - string
    abstract - string - Optionnal, empty string by default.
    keywordlist - string array - Optionnal, empty array by default.
    style - string - Optionnal, "normal" by default.
    minres - float - Optionnal, 0.0 by default.
    maxres - float - Optionnal, 0.0 by default.
    opaque - boolean - Optionnal, 1 by default.
    authority - string - Optionnal, "IGNF" by default.
    srslist - string array
    resampling - string - Optionnal, "lanczos_4" by default.
    geo_bbox - double array
    proj - string - Optionnal, first element of srslist by default.
    proj_bbox - double array
    pyramid - string
    
See also:
    <_init>
=cut
sub new {
    my $class = shift;
    my $params = shift;

    $class = ref($class) || $class;
    # IMPORTANT : if modification, think to update natural documentation (just above)
    my $this = {
        title            => undef,
        abstract         => "",
        keywordlist      => [],
        style            => "normal",
        minres           => 0.0,
        maxres           => 0.0,
        opaque           => 1,
        authority        => "IGNF",
        srslist          => [],
        resampling       => "lanczos_4",
        geo_bbox         => [],
        proj             => undef,
        proj_bbox        => [],
        pyramid          => undef,
    };

    bless($this, $class);


    # init. class
    if (! $this->_init($params)) {
        ERROR ("One parameter is missing !");
        return undef;
    }

    return $this;
}

=begin nd
Function: _init

Check and store layer's attributes values.

Parameters (hash):
    title - string - Mandatory
    abstract - string - Optionnal, empty string by default.
    keywordlist - string array - Optionnal, empty array by default.
    style - string - Optionnal (only for raster pyramid)
    minres - float - Optionnal, 0.0 by default.
    maxres - float - Optionnal, 0.0 by default.
    opaque - boolean - Optionnal, 1 by default.
    authority - string - Optionnal, "IGNF" by default.
    srslist - string array - Optionnal (only for raster pyramid)
    resampling - string - Optionnal, "lanczos_4" by default.
    geo_bbox - double array - Mandatory
    proj - string - Optionnal, first element of srslist by default.
    proj_bbox - double array - Mandatory
    pyramid - string - Mandatory
=cut
sub _init {
    my $this   = shift;
    my $params = shift;

    
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

    if (! exists($params->{geo_bbox}) || ! scalar (@{$params->{geo_bbox}})){
        ERROR("list empty to 'geo_bbox' !");
        return FALSE;
    }
    
    if (! exists($params->{proj_bbox}) || ! scalar (@{$params->{proj_bbox}})){
        ERROR("list empty to 'proj_bbox' !");
        return FALSE;
    }
    
    
    # parameters optional or by default !
    if (! exists($params->{proj}) || ! defined ($params->{proj})) {
        $params->{proj} = $params->{srslist}->[0];
        INFO(sprintf "Default value for 'proj' : %s", $params->{proj});
    }
    
    if (! exists($params->{abstract}) || ! defined ($params->{abstract})) {
        $params->{abstract} = $DEFAULT{abstract};
        INFO(sprintf "Default value for 'abstract' : %s", $params->{abstract});
    }

    if (! exists($params->{minres}) || ! defined ($params->{minres})) {
        $params->{minres} = $DEFAULT{minres};
        INFO(sprintf "Default value for 'minres' : %s", $params->{minres});
    }

    if (! exists($params->{maxres}) || ! defined ($params->{maxres})) {
        $params->{maxres} = $DEFAULT{maxres};
        INFO(sprintf "Default value for 'maxres' : %s", $params->{maxres});
    }


    if (! exists($params->{authority}) || ! defined ($params->{authority})) {
        $params->{authority} = $DEFAULT{authority};
        INFO(sprintf "Default value for 'authority' : %s", $params->{authority});
    }

    
    if (! exists($params->{keywordlist}) || ! defined ($params->{keywordlist})) {
        $params->{keywordlist} = $DEFAULT{keywordlist};
        INFO(sprintf "Default value for 'keywordlist' : %s", Dumper($params->{keywordlist}));
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
    $this->{title}     = $params->{title};
    $this->{pyramid}   = $params->{pyramid};
    $this->{geo_bbox}  = $params->{geo_bbox};
    $this->{proj_bbox} = $params->{proj_bbox};
    $this->{srslist}   = $params->{srslist};
    
    $this->{keywordlist} = $params->{keywordlist};
    $this->{authority}   = $params->{authority};
    $this->{minres}      = $params->{minres};
    $this->{maxres}      = $params->{maxres};
    $this->{style}       = $params->{style};
    $this->{abstract}    = $params->{abstract};
    $this->{proj}        = $params->{proj};
    $this->{resampling}  = $params->{resampling};
    $this->{opaque}      = $params->{opaque};
    
    return TRUE;
}

####################################################################################################
#                                Group: Export methods                                             #
####################################################################################################

=begin nd
Function: exportToXML

Insert layer's attributes in the XML template.

Returns a string to XML format.

Example:
    (start code)
    <layer>
        <title>Ortho IGN en RAW</title>
        <abstract>Projection native</abstract>
        <keywordList>
            <keyword>key</keyword>
            <keyword>word</keyword>
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
            <WMSCRS>IGNF:RGF93G</WMSCRS>
        </WMSCRSList>
        <boundingBox CRS="IGNF:LAMB93" minx="805888" miny="6545408" maxx="806912" maxy="6546432"/>
        <opaque>true</opaque>
        <authority>IGNF</authority>
        <resampling>lanczos_4</resampling>
        <pyramidList>
            <pyramid>/pyramids/ORTHO.pyr</pyramid>
        </pyramidList>
    </layer>
    (end code)
=cut
sub exportToXML {
  my $this = shift;
  
  
  my $string = undef;
  
  # parsing template
  my $parser = XML::LibXML->new();
  
  my $doctpl = eval { $parser->parse_string($STRLYRTMPLT); };
  
  if (!defined($doctpl) || $@) {
    ERROR(sprintf "Can not parse template file of layer : %s !", $!);
    return undef;
  }
  
  my $strlyrtmplt = $doctpl->toString(0);
  
  my $title    = $this->{title};
  $strlyrtmplt =~ s/__TITLE__/$title/;

  my $style = $this->{style};
  if (! defined $style) {$style = "";} else {$style = "<style>$style</style>";}
  $strlyrtmplt =~ s/__STYLE__/$style/;

  my $abstract = $this->{abstract};
  $strlyrtmplt =~ s/__ABSTRACT__/$abstract/;
  
  my $minres   = $this->{minres};
  $strlyrtmplt =~ s/__MINRES__/$minres/;
  my $maxres   = $this->{maxres};
  $strlyrtmplt =~ s/__MAXRES__/$maxres/;
  
  my $opaque   = $this->{opaque};
  if (! defined $opaque) {$opaque = "";} else {$opaque = "<opaque>$opaque</opaque>";}
  $strlyrtmplt =~ s/__OPAQUE__/$opaque/;
  
  my $resampling = $this->{resampling};
  if (! defined $resampling) {$resampling = "";} else {$resampling = "<resampling>$resampling</resampling>";}
  $strlyrtmplt   =~ s/__RESAMPLING__/$resampling/;
  
  my $pyramid  = $this->{pyramid};
  $strlyrtmplt =~ s/__PYRAMID__/$pyramid/;
  
  my $authority  = $this->{authority};
  $strlyrtmplt   =~ s/__AUTHORITY__/$authority/;
  
  my $srs = $this->{proj};
  $strlyrtmplt =~ s/__SRS__/$srs/;
  my ($minx,$miny,$maxx,$maxy) = @{$this->{proj_bbox}};
  $strlyrtmplt =~ s/__MINX__/$minx/;
  $strlyrtmplt =~ s/__MINY__/$miny/;
  $strlyrtmplt =~ s/__MAXX__/$maxx/;
  $strlyrtmplt =~ s/__MAXY__/$maxy/;
  
  my ($w,$s,$e,$n) = @{$this->{geo_bbox}};
  $strlyrtmplt =~ s/__E__/$e/;
  $strlyrtmplt =~ s/__W__/$w/;
  $strlyrtmplt =~ s/__N__/$n/;
  $strlyrtmplt =~ s/__S__/$s/;
  

  my $srsliststring = "";
  if (defined $this->{srslist}){
    $srsliststring .= "<WMSCRSList>\n";
    foreach (@{$this->{srslist}}) {
      $srsliststring .= $STRSRSTMPLT;
      $srsliststring =~ s/__SRS__/$_/;
    }
    $srsliststring .= "    </WMSCRSList>\n";
  }

  $strlyrtmplt =~ s/__SRSS__/$srsliststring/;
  
  foreach (@{$this->{keywordlist}}) {
    $strlyrtmplt =~ s/<!-- __KEYWORDS__ -->\n/$STRTKTMPLT/;
    $strlyrtmplt =~ s/__KEYWORD__/$_/;
  }
  $strlyrtmplt =~ s/<!-- __KEYWORDS__ -->\n//;

  $strlyrtmplt =~ s/^$//g;
  $strlyrtmplt =~ s/^\n$//g;
  
  $string = $strlyrtmplt;
  
  return $string;
}

1;
__END__
