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

use Test::More;

use Log::Log4perl qw(:easy);
# logger by default for unit tests
Log::Log4perl->easy_init({
    level => $WARN,
    layout => '%5p : %m (%M) %n'
});

# My tested class
use BE4::NoData;
use BE4::Pixel;

######################################################

# Pixel objects creations

my $pixelFloat32 = BE4::Pixel->new({
    samplesperpixel => 1,
    bitspersample => 32,
    sampleformat => "float",
    photometric => "gray"
});

ok (defined $pixelFloat32, "Float32 Pixel created");

my $pixelUInt8 = BE4::Pixel->new({
    samplesperpixel => 3,
    bitspersample => 8,
    sampleformat => "uint",
    photometric => "rgb"
});

ok (defined $pixelUInt8, "UInt8 Pixel created");

######################################################

my $nodataFloat32 = BE4::NoData->new({
    pixel => $pixelFloat32,
    value => "-99999",
});

ok (defined $nodataFloat32, "Float32 NoData created");

my $nodataUInt8 = BE4::NoData->new({
    pixel => $pixelUInt8,
    value => "255,23,48",
});

ok (defined $nodataUInt8, "UInt8 NoData created");

######################################################

# Test on parameter 'value'
my $nodata = BE4::NoData->new({
    pixel => $pixelUInt8,
    value => "255,63",
});

ok (! defined $nodata, "Incorrect value detected for 'value' : UInt8 Pixel, 3 samples per pixel <=> '255,63'");
undef $nodata;

$nodata = BE4::NoData->new({
    pixel => $pixelUInt8,
    value => "201,63,-4",
});

ok (! defined $nodata, "Incorrect value detected for 'value' : UInt8 Pixel, 3 samples per pixel <=> '201,63,-4'");
undef $nodata;

$nodata = BE4::NoData->new({
    pixel => $pixelFloat32,
    value => "FFFF",
});

ok (! defined $nodata, "Incorrect value detected for 'value' : UInt8 Pixel, 3 samples per pixel <=> 'FFFF'");
undef $nodata;

$nodata = BE4::NoData->new({
    pixel => $pixelFloat32,
    value => "FFFFFF",
});

ok (! defined $nodata, "Incorrect value detected for 'value' : Float32 Pixel, 1 sample per pixel <=> 'FFFFFF'");
undef $nodata;

######################################################

# Value conversion tests

$nodata = BE4::NoData->new({
    pixel => $pixelUInt8,
    value => "FF8F12",
});

ok (defined $nodata, "UInt8 NoData created with value conversion");

is ($nodata->getValue(), "255,143,18", "Value conversion (HEX FF8F12 -> DEC 255,143,18)");

undef $nodata;

######################################################

done_testing();

