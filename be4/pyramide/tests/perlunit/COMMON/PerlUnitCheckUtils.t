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
use COMMON::CheckUtils;

######################## True #######################
my $test;

$test = (COMMON::CheckUtils::isInteger(-1856164354684)) && (COMMON::CheckUtils::isInteger(+1856684)) && (COMMON::CheckUtils::isInteger(14684)) && (COMMON::CheckUtils::isInteger(0));
ok($test, "Is a base 10 integer.");
undef $test;

$test = (COMMON::CheckUtils::isPositiveInt(+1856684)) && (COMMON::CheckUtils::isPositiveInt(14684)) && (COMMON::CheckUtils::isPositiveInt(0));
ok($test, "Is a positive base 10 integer.");
undef $test;

$test = (COMMON::CheckUtils::isStrictPositiveInt(+1856684)) && (COMMON::CheckUtils::isStrictPositiveInt(14684));
ok($test, "Is a strictly positive base 10 integer.");
undef $test;

$test = (COMMON::CheckUtils::isNumber(-1856164354684)) && (COMMON::CheckUtils::isNumber(+1856684)) && (COMMON::CheckUtils::isNumber(14684)) && (COMMON::CheckUtils::isNumber(0)) && (COMMON::CheckUtils::isNumber(0.298)) && (COMMON::CheckUtils::isNumber(-15.8)) && (COMMON::CheckUtils::isNumber(+15.8));
ok($test, "Is a base 10 number.");
undef $test;


####################### False #######################

$test = (!COMMON::CheckUtils::isInteger(0.5)) && (!COMMON::CheckUtils::isInteger(+0.5)) && (!COMMON::CheckUtils::isInteger(-0.5)) && (!COMMON::CheckUtils::isInteger("NAN"));
ok($test, "Is not a base 10 integer.");
undef $test;

$test = (!COMMON::CheckUtils::isPositiveInt(0.5)) && (!COMMON::CheckUtils::isPositiveInt(+0.5)) && (!COMMON::CheckUtils::isPositiveInt(-0.5)) && (!COMMON::CheckUtils::isPositiveInt("NAN")) && (!COMMON::CheckUtils::isPositiveInt(-521));
ok($test, "Is not a positive base 10 integer.");
undef $test;

$test = (!COMMON::CheckUtils::isStrictPositiveInt(0.5)) && (!COMMON::CheckUtils::isStrictPositiveInt(+0.5)) && (!COMMON::CheckUtils::isStrictPositiveInt(-0.5)) && (!COMMON::CheckUtils::isStrictPositiveInt("NAN")) && (!COMMON::CheckUtils::isStrictPositiveInt(-521)) && (!COMMON::CheckUtils::isStrictPositiveInt(0));
ok($test, "Is not a strictly positive base 10 integer.");
undef $test;

$test = !COMMON::CheckUtils::isNumber("NAN") && !COMMON::CheckUtils::isNumber("4F2A11");
ok($test, "Is not a base 10 number.");
undef $test;


######################## End ########################

done_testing();

