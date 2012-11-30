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

# My tested class
use BE4::Base36;

######################################################

my @array = (65,465,8,462,165,32,45,123,354);

######################################################

# Test on encodeB10toB36

my $b36 = BE4::Base36->encodeB10toB36(132348);
is ($b36, "2U4C", "Conversion base 10 -> base 36 : no length");

$b36 = BE4::Base36->encodeB10toB36(16549465,3);
is ($b36, "9UPND", "Conversion base 10 -> base 36 : length too small");

$b36 = BE4::Base36->encodeB10toB36(1254,4);
is ($b36, "00YU", "Conversion base 10 -> base 36 : length bigger than needed");

$b36 = BE4::Base36->encodeB10toB36(0,4);
is ($b36, "0000", "Conversion base 10 -> base 36 : 0 with length");

$b36 = BE4::Base36->encodeB10toB36(0);
is ($b36, "0", "Conversion base 10 -> base 36 : 0 without length");

$b36 = BE4::Base36->encodeB10toB36(0,-1);
is ($b36, "0", "Conversion base 10 -> base 36 : ignore negative length");

######################################################

# Test on encodeB36toB10

my $b10 = BE4::Base36->encodeB36toB10("564S");
is ($b10, 241228, "Conversion base 36 -> base 10");

$b10 = BE4::Base36->encodeB36toB10("004FR6G");
is ($b10, 7453528, "Conversion base 36 -> base 10 : begin with 0");

$b10 = BE4::Base36->encodeB36toB10("000000");
is ($b10, 0, "Conversion base 36 -> base 10 : just zeros");

######################################################

# Test on b36PathToIndices

my $b36path = "0FG8/00/MA";
my ($i,$j) = BE4::Base36->b36PathToIndices($b36path);
is_deeply ([$i,$j], [19462,756874], "Conversion base 36 path -> indices");

######################################################

# Test on indicesToB36Path

my $b10path = BE4::Base36->indicesToB36Path(15247,75846,3);
is ($b10path, "0B1M/RI/JU", "Conversion indices -> base 36 path");

is (BE4::Base36->indicesToB36Path(4032, 18217, 3), "3E/42/01", "Conversion indices -> base 36 path");

######################################################

done_testing();

