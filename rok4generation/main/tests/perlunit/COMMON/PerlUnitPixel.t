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
use COMMON::Pixel;

######################################################

# Correct parameters 1
my $params = {
    samplesperpixel => 1,
    bitspersample => 32,
    sampleformat => "float",
    photometric => "gray"
};
my $pixel = COMMON::Pixel->new($params);

ok (defined $pixel, "Float32 Pixel created");
undef $pixel;

# Correct parameters 2
$params = {
    samplesperpixel => 3,
    bitspersample => 8,
    sampleformat => "uint",
    photometric => "rgb"
};
$pixel = COMMON::Pixel->new($params);

ok (defined $pixel, "UInt8 Pixel created");
undef $pixel;

######################################################

# Test on parameter 'samplesperpixel'
$params->{samplesperpixel} = 14;
$pixel = COMMON::Pixel->new($params);

ok (! defined $pixel, "Incorrect value detected for 'samplesperpixel'");
undef $pixel;

# Test on parameter 'photometric'
$params->{samplesperpixel} = 3;
$params->{photometric} = "??";
$pixel = COMMON::Pixel->new($params);

ok (! defined $pixel, "Incorrect value detected for 'photometric'");
undef $pixel;

# Test on parameter 'bitspersample'
$params->{bitspersample} = 17;
$params->{photometric} = "rgb";
$pixel = COMMON::Pixel->new($params);

ok (! defined $pixel, "Incorrect value detected for 'bitspersample'");
undef $pixel;

# Test on parameter 'sampleformat'
$params->{bitspersample} = 8;
$params->{sampleformat} = "??";
$pixel = COMMON::Pixel->new($params);

ok (! defined $pixel, "Incorrect value detected for 'sampleformat'");
undef $pixel;

######################################################

done_testing();

