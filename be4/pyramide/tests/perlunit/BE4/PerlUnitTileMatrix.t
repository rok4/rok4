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
use BE4::TileMatrix;

######################################################

# TileMatrix objects creations

my $tm = BE4::TileMatrix->new({
    id             => "level4",
    resolution     => 0.324,
    topLeftCornerX => 0,
    topLeftCornerY => 12000,
    tileWidth      => 256,
    tileHeight     => 128,
    matrixWidth    => 100,
    matrixHeight   => 47
});

ok (defined $tm, "TileMatrix created");

######################################################

# Test on size functions

cmp_ok ($tm->getImgGroundWidth(4), "==", 331.776, "Image ground width calculation");
cmp_ok ($tm->getImgGroundHeight(3), "==", 124.416, "Image ground height calculation");

######################################################

# Test on target tile matrix functions

$tm->addTargetTm($tm);
is (scalar @{$tm->getTargetsTm()}, "1", "Add target tile matrix");

######################################################

# Test on conversion functions

cmp_ok ($tm->columnToX(127,4), "==", 42135.552, "Conversion column -> X");
cmp_ok ($tm->rowToY(127,3), "==", -3800.832, "Conversion row -> Y");

cmp_ok ($tm->xToColumn(42155.7,4), "==", 127, "Conversion X -> column");
cmp_ok ($tm->yToRow(-3801.14,3), "==", 127, "Conversion Y -> row");

my ($xMin,$yMin,$xMax,$yMax) = $tm->indicesToBBox(127,12,16,16);
is_deeply([$xMin,$yMin,$xMax,$yMax],[168542.208,3373.824,169869.312,4037.376],"Conversion indices -> bbox");

my ($iMin,$jMin,$iMax,$jMax) = $tm->bboxToIndices(3241.0,132.4,6365.45,213.986,16,16);
is_deeply([$iMin,$jMin,$iMax,$jMax],[2,17,4,17],"Conversion bbox -> indices");

######################################################

done_testing();

