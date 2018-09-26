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

my $test;

######################## isPositiveInt

$test = COMMON::CheckUtils::isPositiveInt(+1856684);
ok($test, "Is a positive base 10 integer.");
$test = COMMON::CheckUtils::isPositiveInt(14684);
ok($test, "Is a positive base 10 integer.");
$test = COMMON::CheckUtils::isPositiveInt(0));
ok($test, "Is a positive base 10 integer.");

$test = COMMON::CheckUtils::isPositiveInt(0.5);
ok(! $test, "Is not a positive base 10 integer.");
$test = COMMON::CheckUtils::isPositiveInt(+0.5);
ok(! $test, "Is not a positive base 10 integer.");
$test = COMMON::CheckUtils::isPositiveInt(-0.5);
ok( !$test, "Is not a positive base 10 integer.");
$test = COMMON::CheckUtils::isPositiveInt("NAN");
ok( !$test, "Is not a positive base 10 integer.");
$test = COMMON::CheckUtils::isPositiveInt(-521));
ok(! $test, "Is not a positive base 10 integer.");

######################## isStrictPositiveInt

$test = COMMON::CheckUtils::isStrictPositiveInt(+1856684);
ok($test, "Is a strictly positive base 10 integer.");
$test = COMMON::CheckUtils::isStrictPositiveInt(14684));
ok($test, "Is a strictly positive base 10 integer.");

$test = COMMON::CheckUtils::isStrictPositiveInt(0.5);
ok(! $test, "Is not a strictly positive base 10 integer.");
$test = COMMON::CheckUtils::isStrictPositiveInt(+0.5);
ok(! $test, "Is not a strictly positive base 10 integer.");
$test = COMMON::CheckUtils::isStrictPositiveInt(-0.5);
ok(! $test, "Is not a strictly positive base 10 integer.");
$test = COMMON::CheckUtils::isStrictPositiveInt("NAN");
ok(! $test, "Is not a strictly positive base 10 integer.");
$test = COMMON::CheckUtils::isStrictPositiveInt(-521);
ok(! $test, "Is not a strictly positive base 10 integer.");
$test = COMMON::CheckUtils::isStrictPositiveInt(0));
ok(! $test, "Is not a strictly positive base 10 integer.");

######################## isInteger

$test = COMMON::CheckUtils::isInteger(-1856164354684);
ok($test, "Is a base 10 integer.");
$test = COMMON::CheckUtils::isInteger(+1856684);
ok($test, "Is a base 10 integer.");
$test = COMMON::CheckUtils::isInteger(14684);
ok($test, "Is a base 10 integer.");
$test = COMMON::CheckUtils::isInteger(0));
ok($test, "Is a base 10 integer.");

$test = COMMON::CheckUtils::isInteger(0.5);
ok(! $test, "Is not a base 10 integer.");
$test = COMMON::CheckUtils::isInteger(+0.5);
ok(! $test, "Is not a base 10 integer.");
$test = COMMON::CheckUtils::isInteger(-0.5);
ok(! $test, "Is not a base 10 integer.");
$test = COMMON::CheckUtils::isInteger("NAN"));
ok(! $test, "Is not a base 10 integer.");

######################## isNumber

$test = COMMON::CheckUtils::isNumber(-1856164354684);
ok($test, "Is a base 10 number.");
$test = COMMON::CheckUtils::isNumber(+1856684);
ok($test, "Is a base 10 number.");
$test = COMMON::CheckUtils::isNumber(14684);
ok($test, "Is a base 10 number.");
$test = COMMON::CheckUtils::isNumber(0);
ok($test, "Is a base 10 number.");
$test = COMMON::CheckUtils::isNumber(0.298);
ok($test, "Is a base 10 number.");
$test = COMMON::CheckUtils::isNumber(-15.8);
ok($test, "Is a base 10 number.");
$test = COMMON::CheckUtils::isNumber(+15.8));
ok($test, "Is a base 10 number.");

$test = COMMON::CheckUtils::isNumber("NAN") && !COMMON::CheckUtils::isNumber("4F2A11");
ok(! $test, "Is not a base 10 number.");

######################## isBbox

$test = COMMON::CheckUtils::isBbox("452.3,-89,9856,+45.369"));
ok($test, "Is a bbox.");

$test = COMMON::CheckUtils::isInteger("idsjfhg,oeziurh,uydcsq,iuerhg");
ok(! $test, "Is not a bbox (not numbers)");
$test = COMMON::CheckUtils::isInteger("452.3,-89,9856");
ok(! $test, "Is not a bbox (not 4 numbers)");
$test = COMMON::CheckUtils::isInteger("9856,-89,452.3,+45.369");
ok(! $test, "Is not a bbox (min > max)");

######################## isEmpty

$test = COMMON::CheckUtils::isEmpty(undef));
ok($test, "undef is empty.");
$test = COMMON::CheckUtils::isEmpty(""));
ok($test, "'' is empty.");

$test = COMMON::CheckUtils::isEmpty(0));
ok(! $test, "0 is not empty.");
$test = COMMON::CheckUtils::isEmpty({}));
ok(! $test, "{} is not empty.");

######################## End ########################

done_testing();

