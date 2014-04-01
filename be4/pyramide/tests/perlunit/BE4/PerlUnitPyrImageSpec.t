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
use BE4::PyrImageSpec;

######################################################

# PyrImageSpec objects creations

# classic
my $pis = BE4::PyrImageSpec->new({
    samplesperpixel => 1,
    bitspersample => 32,
    sampleformat => "float",
    photometric => "gray",
    compression => "lzw",
    interpolation => "bicubic",
    gamma => 1
});

ok (defined $pis, "PyrImageSpec created");

is ($pis->getCompressionOption(), "none", "Default value for 'compressionoption' : 'none'");

is ($pis->getFormatCode(), "TIFF_LZW_FLOAT32", "Format code verification");

# with format code
my $pis2 = BE4::PyrImageSpec->new({
    formatCode => "TIFF_PKB_INT8",
    samplesperpixel => 3,
    photometric => "rgb",
    interpolation => "bicubic",
    gamma => 1
});

ok (defined $pis2, "PyrImageSpec created from formatCode");

is ($pis2->getCompression(), "pkb", "Compression extracted from format code");
is ($pis2->getPixel->getSampleFormat(), "uint", "Sample format extracted from format code");
is ($pis2->getPixel->getBitsPerSample(), "8", "Bits per sample extracted from format code");

######################################################

# Test on format decoder

my ($comp,$sf,$bps) = $pis2->decodeFormat("TIFF_INT8");
is_deeply ([$comp,$sf,$bps],["raw","uint",8], "Deprecated format code 'TIFF_INT8' decoded");
($comp,$sf,$bps) = $pis2->decodeFormat("TIFF_FLOAT32");
is_deeply ([$comp,$sf,$bps],["raw","float",32], "Deprecated format code 'TIFF_FLOAT32' decoded");

######################################################

# Test on parameter 'interpolation'
$pis = BE4::PyrImageSpec->new({
    samplesperpixel => 1,
    bitspersample => 32,
    sampleformat => "float",
    photometric => "gray",
    compression => "lzw",
    interpolation => "bicubique",
    gamma => 1
});

is ($pis->getInterpolation(), "bicubic", "Interpolation 'bicubique' is deprecated and change into 'bicubic'");
undef $pis;

$pis = BE4::PyrImageSpec->new({
    samplesperpixel => 1,
    bitspersample => 32,
    sampleformat => "float",
    photometric => "gray",
    compression => "lzw",
    interpolation => "??",
    gamma => 1
});

ok (! defined $pis, "Incorrect value detected for 'interpolation'");
undef $pis;

######################################################

# Test on parameter 'compression'
$pis = BE4::PyrImageSpec->new({
    samplesperpixel => 1,
    bitspersample => 32,
    sampleformat => "float",
    photometric => "gray",
    compression => "floatraw",
    interpolation => "bicubique",
    gamma => 1
});

is ($pis->getCompression(), "raw", "Compression 'floatraw' is deprecated and change into 'raw'");
undef $pis;

$pis = BE4::PyrImageSpec->new({
    samplesperpixel => 1,
    bitspersample => 32,
    sampleformat => "float",
    photometric => "gray",
    compression => "??",
    interpolation => "nn",
    gamma => 1
});

ok (! defined $pis, "Incorrect value detected for 'compression'");
undef $pis;

######################################################

# Test on parameter 'compressionoption'
$pis = BE4::PyrImageSpec->new({
    samplesperpixel => 1,
    bitspersample => 32,
    sampleformat => "float",
    photometric => "gray",
    compression => "floatraw",
    compressionoption => "??",
    interpolation => "bicubique",
    gamma => 1
});

ok (! defined $pis, "Incorrect value detected for 'compressionoption'");
undef $pis;

$pis = BE4::PyrImageSpec->new({
    samplesperpixel => 1,
    bitspersample => 8,
    sampleformat => "uint",
    photometric => "gray",
    compression => "jpg",
    compressionoption => "crop",
    interpolation => "bicubique",
    gamma => 1
});

ok (defined $pis, "Compression option 'crop' is accepted for jpeg compression");
undef $pis;

$pis = BE4::PyrImageSpec->new({
    samplesperpixel => 1,
    bitspersample => 8,
    sampleformat => "uint",
    photometric => "gray",
    compression => "raw",
    compressionoption => "crop",
    interpolation => "bicubique",
    gamma => 1
});

ok (! defined $pis, "Compression option 'crop' is accepted ONLY for jpeg compression");
undef $pis;

######################################################

# Test on parameter 'gamma'
$pis = BE4::PyrImageSpec->new({
    samplesperpixel => 1,
    bitspersample => 32,
    sampleformat => "float",
    photometric => "gray",
    compression => "floatraw",
    interpolation => "bicubique",
    gamma => "-9"
});

cmp_ok ($pis->getGamma(), '==', 0, "Gamma cannot be negative");
undef $pis;

$pis = BE4::PyrImageSpec->new({
    samplesperpixel => 1,
    bitspersample => 32,
    sampleformat => "float",
    photometric => "gray",
    compression => "raw",
    interpolation => "nn",
    gamma => "NaN"
});

ok (! defined $pis, "Incorrect value detected for 'gamma', have to be a number");
undef $pis;

$pis = BE4::PyrImageSpec->new({
    samplesperpixel => 1,
    bitspersample => 32,
    sampleformat => "float",
    photometric => "gray",
    compression => "raw",
    interpolation => "nn",
    gamma => "1.3"
});

cmp_ok ($pis->getGamma(), '>', 1, "Gamma can be bigger than 1");
undef $pis;

######################################################

done_testing();

