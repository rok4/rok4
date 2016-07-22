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
use COMMON::Array;

######################################################

my @array = (65,465,8,462,165,32,45,123,354);

######################################################

# Test on maxArrayValue

my $maxValue = COMMON::Array::maxArrayValue(0,@array);
is ($maxValue, 465, "Find the maximum value from the beginning");

$maxValue = COMMON::Array::maxArrayValue(4,@array);
is ($maxValue, 354, "Find the maximum value from the fourth element");

$maxValue = COMMON::Array::maxArrayValue(12,@array);
is ($maxValue, undef, "maxArrayValue : identify bad value for the first element index");

######################################################

# Test on minArrayIndex

my $minIndex = COMMON::Array::minArrayIndex(0,@array);
is ($minIndex, 2, "Find the index of the minimum from the beginning");

$minIndex = COMMON::Array::minArrayIndex(3,@array);
is ($minIndex, 5, "Find the index of the minimum from the third element");

$minIndex = COMMON::Array::minArrayIndex(12,@array);
is ($minIndex, undef, "minArrayIndex : identify bad value for the first element index");

######################################################

done_testing();

