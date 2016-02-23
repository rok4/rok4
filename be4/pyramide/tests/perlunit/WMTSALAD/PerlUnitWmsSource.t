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

use strict;
use warnings;

use FindBin qw($Bin);

use Test::More;

use Log::Log4perl qw(:easy);
# logger by default for unit tests
Log::Log4perl->easy_init({
    level => $WARN,
    layout => '%5p : %m (%M) %n'
});

# My tested class
use WMTSALAD::WmsSource;

####################### Good ######################

# WmsSource creation with all parameters defined
my $wmsSource = WMTSALAD::WmsSource->new( { 
        type => "WMS",
        level => 7,
        order => 0,
        wms_url => "http://target.server.net/wms" ,
        wms_proxy => "http://proxy.domaine.fr" ,
        wms_timeout => 60 ,
        wms_retry => 10 ,
        wms_interval => 5 ,
        wms_user => "user" ,
        wms_password => "password" ,
        wms_referer => "referer" ,
        wms_userAgent => "UA" ,
        wms_version => "1.3.0" ,
        wms_layers => "LAYER_1,LAYER_2,LAYER_3" ,
        wms_styles => "STYLE_FOR_LAYER_1,STYLE_FOR_LAYER_2,STYLE_FOR_LAYER_3" ,
        wms_crs => "EPSG:2154" ,
        wms_format => "image/png" ,
        wms_channels => 3 ,
        wms_nodata => "0xFFA2FA" ,
        wms_extent => "634500,6855000,636800,6857700" ,
        wms_option => "FAKE_OPTION" ,
    } );
ok (defined $wmsSource, "WmsSource (all parameters) created");
undef $wmsSource;

# WmsSource creation without optionnal parameters
$wmsSource = WMTSALAD::WmsSource->new( { 
        type => "WMS",
        level => 7,
        order => 0,
        wms_url => "http://target.server.net/wms" ,
        wms_version => "1.3.0" ,
        wms_layers => "LAYER_1,LAYER_2,LAYER_3" ,
        wms_styles => "STYLE_FOR_LAYER_1,STYLE_FOR_LAYER_2,STYLE_FOR_LAYER_3" ,
        wms_channels => 3 ,
        wms_nodata => "0xFFA2FA" ,
        wms_extent => "634500,6855000,636800,6857700" ,
    } );
ok (defined $wmsSource, "WmsSource (mandatory parameters only) created");
undef $wmsSource;

################# Bad Mandatory ################

# WmsSource creation without a 'wms_url'
my $errWmsSource = WMTSALAD::WmsSource->new( { 
        type => "WMS",
        level => 7,
        order => 0,
        wms_version => "1.3.0" ,
        wms_layers => "LAYER_1,LAYER_2,LAYER_3" ,
        wms_styles => "STYLE_FOR_LAYER_1,STYLE_FOR_LAYER_2,STYLE_FOR_LAYER_3" ,
        wms_channels => 3 ,
        wms_nodata => "0xFFA2FA" ,
        wms_extent => "634500,6855000,636800,6857700" ,
    } );
ok (!defined $errWmsSource, "WmsSource->new with no 'wms_url'.");
undef $errWmsSource;

# WmsSource creation without a 'wms_version'
$errWmsSource = WMTSALAD::WmsSource->new( { 
        type => "WMS",
        level => 7,
        order => 0,
        wms_url => "http://target.server.net/wms" ,
        wms_layers => "LAYER_1,LAYER_2,LAYER_3" ,
        wms_styles => "STYLE_FOR_LAYER_1,STYLE_FOR_LAYER_2,STYLE_FOR_LAYER_3" ,
        wms_channels => 3 ,
        wms_nodata => "0xFFA2FA" ,
        wms_extent => "634500,6855000,636800,6857700" ,
    } );
ok (!defined $errWmsSource, "WmsSource->new with no 'wms_version'.");
undef $errWmsSource;

# WmsSource creation with a bad 'wms_version'
$errWmsSource = WMTSALAD::WmsSource->new( { 
        type => "WMS",
        level => 7,
        order => 0,
        wms_url => "http://target.server.net/wms" ,
        wms_version => "4.2.1" ,
        wms_layers => "LAYER_1,LAYER_2,LAYER_3" ,
        wms_styles => "STYLE_FOR_LAYER_1,STYLE_FOR_LAYER_2,STYLE_FOR_LAYER_3" ,
        wms_channels => 3 ,
        wms_nodata => "0xFFA2FA" ,
        wms_extent => "634500,6855000,636800,6857700" ,
    } );
ok (!defined $errWmsSource, "WmsSource->new with wrong 'wms_version'.");
undef $errWmsSource;

# WmsSource creation without a 'wms_layers'
$errWmsSource = WMTSALAD::WmsSource->new( { 
        type => "WMS",
        level => 7,
        order => 0,
        wms_url => "http://target.server.net/wms" ,
        wms_version => "1.3.0" ,
        wms_styles => "STYLE_FOR_LAYER_1,STYLE_FOR_LAYER_2,STYLE_FOR_LAYER_3" ,
        wms_channels => 3 ,
        wms_nodata => "0xFFA2FA" ,
        wms_extent => "634500,6855000,636800,6857700" ,
    } );
ok (!defined $errWmsSource, "WmsSource->new with no 'wms_layers'.");
undef $errWmsSource;

# WmsSource creation without a 'wms_styles'
$errWmsSource = WMTSALAD::WmsSource->new( { 
        type => "WMS",
        level => 7,
        order => 0,
        wms_url => "http://target.server.net/wms" ,
        wms_version => "1.3.0" ,
        wms_layers => "LAYER_1,LAYER_2,LAYER_3" ,
        wms_channels => 3 ,
        wms_nodata => "0xFFA2FA" ,
        wms_extent => "634500,6855000,636800,6857700" ,
    } );
ok (!defined $errWmsSource, "WmsSource->new with no 'wms_styles'.");
undef $errWmsSource;

# WmsSource creation with an unmathcing number of 'wms_styles'
$errWmsSource = WMTSALAD::WmsSource->new( { 
        type => "WMS",
        level => 7,
        order => 0,
        wms_url => "http://target.server.net/wms" ,
        wms_version => "1.3.0" ,
        wms_layers => "LAYER_1,LAYER_2,LAYER_3" ,
        wms_styles => "STYLE_FOR_LAYER_1,STYLE_FOR_LAYER_2",
        wms_channels => 3 ,
        wms_nodata => "0xFFA2FA" ,
        wms_extent => "634500,6855000,636800,6857700" ,
    } );
my $errWmsSource2 = WMTSALAD::WmsSource->new( { 
        type => "WMS",
        level => 7,
        order => 0,
        wms_url => "http://target.server.net/wms" ,
        wms_version => "1.3.0" ,
        wms_layers => "LAYER_1,LAYER_2,LAYER_3" ,
        wms_styles => "STYLE_FOR_LAYER_1,STYLE_FOR_LAYER_2,STYLE_FOR_LAYER_3,STYLE_FOR_LAYER_4",
        wms_channels => 3 ,
        wms_nodata => "0xFFA2FA" ,
        wms_extent => "634500,6855000,636800,6857700" ,
    } );
ok ((!defined $errWmsSource) && (!defined $errWmsSource2), "WmsSource->new with unmathcing number of 'wms_styles'.");
undef $errWmsSource;
undef $errWmsSource2;

# WmsSource creation without a 'wms_channels'
$errWmsSource = WMTSALAD::WmsSource->new( { 
        type => "WMS",
        level => 7,
        order => 0,
        wms_url => "http://target.server.net/wms" ,
        wms_version => "1.3.0" ,
        wms_layers => "LAYER_1,LAYER_2,LAYER_3" ,
        wms_styles => "STYLE_FOR_LAYER_1,STYLE_FOR_LAYER_2,STYLE_FOR_LAYER_3" ,
        wms_nodata => "0xFFA2FA" ,
        wms_extent => "634500,6855000,636800,6857700" ,
    } );
ok (!defined $errWmsSource, "WmsSource->new with no 'wms_channels'.");
undef $errWmsSource;

# WmsSource creation with a bad 'wms_channels'
$errWmsSource = WMTSALAD::WmsSource->new( { 
        type => "WMS",
        level => 7,
        order => 0,
        wms_url => "http://target.server.net/wms" ,
        wms_version => "1.3.0" ,
        wms_layers => "LAYER_1,LAYER_2,LAYER_3" ,
        wms_styles => "STYLE_FOR_LAYER_1,STYLE_FOR_LAYER_2,STYLE_FOR_LAYER_3" ,
        wms_channels => 0 ,
        wms_nodata => "0xFFA2FA" ,
        wms_extent => "634500,6855000,636800,6857700" ,
    } );
$errWmsSource2 = WMTSALAD::WmsSource->new( { 
        type => "WMS",
        level => 7,
        order => 0,
        wms_url => "http://target.server.net/wms" ,
        wms_version => "1.3.0" ,
        wms_layers => "LAYER_1,LAYER_2,LAYER_3" ,
        wms_styles => "STYLE_FOR_LAYER_1,STYLE_FOR_LAYER_2,STYLE_FOR_LAYER_3" ,
        wms_channels => "NaN" ,
        wms_nodata => "0xFFA2FA" ,
        wms_extent => "634500,6855000,636800,6857700" ,
    } );
ok ((!defined $errWmsSource) && (!defined $errWmsSource2), "WmsSource->new with a bad 'wms_channels'.");
undef $errWmsSource;
undef $errWmsSource2;

# WmsSource creation without a 'wms_nodata'
$errWmsSource = WMTSALAD::WmsSource->new( { 
        type => "WMS",
        level => 7,
        order => 0,
        wms_url => "http://target.server.net/wms" ,
        wms_version => "1.3.0" ,
        wms_layers => "LAYER_1,LAYER_2,LAYER_3" ,
        wms_styles => "STYLE_FOR_LAYER_1,STYLE_FOR_LAYER_2,STYLE_FOR_LAYER_3" ,
        wms_channels => 3 ,
        wms_extent => "634500,6855000,636800,6857700" ,
    } );
ok (!defined $errWmsSource, "WmsSource->new with no 'wms_nodata'.");
undef $errWmsSource;

# WmsSource creation without a 'wms_extent'
$errWmsSource = WMTSALAD::WmsSource->new( { 
        type => "WMS",
        level => 7,
        order => 0,
        wms_url => "http://target.server.net/wms" ,
        wms_version => "1.3.0" ,
        wms_layers => "LAYER_1,LAYER_2,LAYER_3" ,
        wms_styles => "STYLE_FOR_LAYER_1,STYLE_FOR_LAYER_2,STYLE_FOR_LAYER_3" ,
        wms_channels => 3 ,
        wms_nodata => "0xFFA2FA" ,
    } );
ok (!defined $errWmsSource, "WmsSource->new with no 'wms_extent'.");
undef $errWmsSource;

# WmsSource creation with wrong 'wms_extent' coordinates count.
$errWmsSource = WMTSALAD::WmsSource->new( { 
        type => "WMS",
        level => 7,
        order => 0,
        wms_url => "http://target.server.net/wms" ,
        wms_version => "1.3.0" ,
        wms_layers => "LAYER_1,LAYER_2,LAYER_3" ,
        wms_styles => "STYLE_FOR_LAYER_1,STYLE_FOR_LAYER_2,STYLE_FOR_LAYER_3" ,
        wms_channels => 3 ,
        wms_nodata => "0xFFA2FA" ,
        wms_extent => "634500,6855000,636800,6857700,6857700" ,
    } );
ok (!defined $errWmsSource, "WmsSource->new with wrong 'wms_extent' coordinates count.");
undef $errWmsSource;

# WmsSource creation with wrong 'wms_extent' coordinates type.
$errWmsSource = WMTSALAD::WmsSource->new( { 
        type => "WMS",
        level => 7,
        order => 0,
        wms_url => "http://target.server.net/wms" ,
        wms_version => "1.3.0" ,
        wms_layers => "LAYER_1,LAYER_2,LAYER_3" ,
        wms_styles => "STYLE_FOR_LAYER_1,STYLE_FOR_LAYER_2,STYLE_FOR_LAYER_3" ,
        wms_channels => 3 ,
        wms_nodata => "0xFFA2FA" ,
        wms_extent => "634500,6855000,636800,irrelevent" ,
    } );
ok (!defined $errWmsSource, "WmsSource->new with wrong 'wms_extent' coordinates type.");
undef $errWmsSource;

# WmsSource creation with wrong 'wms_extent' coordinates type.
$errWmsSource = WMTSALAD::WmsSource->new( { 
        type => "WMS",
        level => 7,
        order => 0,
        wms_url => "http://target.server.net/wms" ,
        wms_version => "1.3.0" ,
        wms_layers => "LAYER_1,LAYER_2,LAYER_3" ,
        wms_styles => "STYLE_FOR_LAYER_1,STYLE_FOR_LAYER_2,STYLE_FOR_LAYER_3" ,
        wms_channels => 3 ,
        wms_nodata => "0xFFA2FA" ,
        wms_extent => "634500,6857700,636800,6855000" ,
    } );
ok (!defined $errWmsSource, "WmsSource->new with wrong 'wms_extent' coordinates order.");
undef $errWmsSource;

################## Bad Optionnal #################

# WmsSource creation with a bad 'wms_timeout'
$errWmsSource = WMTSALAD::WmsSource->new( { 
        type => "WMS",
        level => 7,
        order => 0,
        wms_url => "http://target.server.net/wms" ,
        wms_version => "1.3.0" ,
        wms_layers => "LAYER_1,LAYER_2,LAYER_3" ,
        wms_styles => "STYLE_FOR_LAYER_1,STYLE_FOR_LAYER_2,STYLE_FOR_LAYER_3" ,
        wms_channels => 3 ,
        wms_nodata => "0xFFA2FA" ,
        wms_extent => "634500,6855000,636800,6857700" ,
        wms_timeout => -5,
    } );
ok (!defined $errWmsSource, "WmsSource->new with wrong 'wms_timeout'.");
undef $errWmsSource;

# WmsSource creation with a bad 'wms_interval'
$errWmsSource = WMTSALAD::WmsSource->new( { 
        type => "WMS",
        level => 7,
        order => 0,
        wms_url => "http://target.server.net/wms" ,
        wms_version => "1.3.0" ,
        wms_layers => "LAYER_1,LAYER_2,LAYER_3" ,
        wms_styles => "STYLE_FOR_LAYER_1,STYLE_FOR_LAYER_2,STYLE_FOR_LAYER_3" ,
        wms_channels => 3 ,
        wms_nodata => "0xFFA2FA" ,
        wms_extent => "634500,6855000,636800,6857700" ,
        wms_interval => -5,
    } );
ok (!defined $errWmsSource, "WmsSource->new with wrong 'wms_interval'.");
undef $errWmsSource;

# WmsSource creation with a bad 'wms_retry'
$errWmsSource = WMTSALAD::WmsSource->new( { 
        type => "WMS",
        level => 7,
        order => 0,
        wms_url => "http://target.server.net/wms" ,
        wms_version => "1.3.0" ,
        wms_layers => "LAYER_1,LAYER_2,LAYER_3" ,
        wms_styles => "STYLE_FOR_LAYER_1,STYLE_FOR_LAYER_2,STYLE_FOR_LAYER_3" ,
        wms_channels => 3 ,
        wms_nodata => "0xFFA2FA" ,
        wms_extent => "634500,6855000,636800,6857700" ,
        wms_retry => -5,
    } );
ok (!defined $errWmsSource, "WmsSource->new with wrong 'wms_retry'.");
undef $errWmsSource;

# WmsSource creation with a bad 'wms_format'
$errWmsSource = WMTSALAD::WmsSource->new( { 
        type => "WMS",
        level => 7,
        order => 0,
        wms_url => "http://target.server.net/wms" ,
        wms_version => "1.3.0" ,
        wms_layers => "LAYER_1,LAYER_2,LAYER_3" ,
        wms_styles => "STYLE_FOR_LAYER_1,STYLE_FOR_LAYER_2,STYLE_FOR_LAYER_3" ,
        wms_channels => 3 ,
        wms_nodata => "0xFFA2FA" ,
        wms_extent => "634500,6855000,636800,6857700" ,
        wms_format => "WTFS",
    } );
ok (!defined $errWmsSource, "WmsSource->new with wrong 'wms_format'.");
undef $errWmsSource;

####################### End ######################

done_testing();

