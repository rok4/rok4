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
use COMMON::Harvesting;

######################################################

# Harvesting creation

my $objHarvesting = COMMON::Harvesting->new({
    wms_layer   => "BDD_WLD_WM",
    wms_url     => "http://localhost/wmts/rok4",
    wms_version => "1.3.0",
    wms_request => "getMap",
    wms_format  => "image/png",
    wms_bgcolor => "0xFFFFFF",
    wms_style  => "line",
    max_width  => 1024,
    max_height  => 1024
});

ok (defined $objHarvesting, "Harvesting created");

is ($objHarvesting->getOptions, "STYLES=line&BGCOLOR=0xFFFFFF", "Correct options composition");

######################################################

# Test on request functions

my $request = $objHarvesting->doRequestUrl({
    srs =>"IGNF:LAMB93",
    inversion => 0,
    bbox => [0,200,1000,1200],
    width => 500,
    height => 600
});

is ($request, "http://localhost/wmts/rok4?LAYERS=BDD_WLD_WM&SERVICE=WMS&VERSION=1.3.0&REQUEST=getMap&FORMAT=image/png&CRS=IGNF:LAMB93&BBOX=0,200,1000,1200&WIDTH=500&HEIGHT=600&STYLES=line&BGCOLOR=0xFFFFFF",
    "Well formated request");

$request = $objHarvesting->doRequestUrl({
    srs =>"IGNF:LAMB93",
    inversion => 1,
    bbox => [0,200,1000,1200],
    width => 500,
    height => 600
});

is ($request, "http://localhost/wmts/rok4?LAYERS=BDD_WLD_WM&SERVICE=WMS&VERSION=1.3.0&REQUEST=getMap&FORMAT=image/png&CRS=IGNF:LAMB93&BBOX=200,0,1200,1000&WIDTH=500&HEIGHT=600&STYLES=line&BGCOLOR=0xFFFFFF",
    "Well formated request with coordinates inversion");

######################################################

# Bad parameters

my $objBadHarvesting = COMMON::Harvesting->new({
    wms_layer   => "BDD_WLD_WM",
    wms_url     => "http://localhost/wmts/rok4",
    wms_version => "1.3.0",
    wms_request => "getMap",
    wms_format  => "image/png",
    wms_bgcolor => "0xFF",
    wms_transparent  => "FALSE",
    wms_style  => "line",
    max_width  => 1024,
    max_height  => 1024
});

ok (! defined $objBadHarvesting, "Wrong 'wms_bgcolor' detected");
undef $objBadHarvesting;

$objBadHarvesting = COMMON::Harvesting->new({
    wms_layer   => "BDD_WLD_WM",
    wms_url     => "http://localhost/wmts/rok4",
    wms_version => "1.3.0",
    wms_request => "getMap",
    wms_format  => "image/jpeg",
    wms_bgcolor => "0xFFFFFF",
    wms_transparent  => "FALSE",
    wms_style  => "line",
    max_width  => 1024,
    max_height  => 1024
});

ok (! defined $objBadHarvesting, "Wrong 'wms_format' detected");
undef $objBadHarvesting;

$objBadHarvesting = COMMON::Harvesting->new({
    wms_layer   => "BDD_WLD_WM",
    wms_url     => "http://localhost/wmts/rok4",
    wms_version => "1.3.0",
    wms_request => "getMap",
    wms_format  => "image/png",
    max_width  => 1024
});

ok (! defined $objBadHarvesting, "Wrong maximum pixel sizes (height AND width) detected");
undef $objBadHarvesting;

$objBadHarvesting = COMMON::Harvesting->new({
    wms_layer   => "BDD_WLD_WM",
    wms_url     => "http://localhost/wmts/rok4",
    wms_version => "1.3.0",
    wms_request => "getMap",
    wms_format  => "image/png",
    min_size  => -12
});

ok (! defined $objBadHarvesting, "Wrong minimum size (negative) detected");
undef $objBadHarvesting;



######################################################

done_testing();

