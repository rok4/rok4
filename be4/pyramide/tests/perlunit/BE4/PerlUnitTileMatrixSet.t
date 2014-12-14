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
use BE4::TileMatrixSet;

######################################################

# Quad tree TileMatrixSet

my $tmsQuadTree = BE4::TileMatrixSet->new($Bin."/../../tms/LAMB93_10cm.tms");
ok (defined $tmsQuadTree, "Quad tree TileMatrixSet created");
ok ($tmsQuadTree->isQTree(), "TileMatrixSet recognized as a QTree one");
is ($tmsQuadTree->getName(), "LAMB93_10cm", "TMS' name extraction");

######################################################

# TileMatrixSet for DTM

my $tmsDTM = BE4::TileMatrixSet->new($Bin."/../../tms/LAMB93_1M_MNT.tms");
ok (defined $tmsDTM, "TileMatrixSet for DTM created");

ok (! $tmsDTM->isQTree(),"TileMatrixSet recognized as a DTM one");

######################################################

# Bad TileMatrixSet

my $badTMS = BE4::TileMatrixSet->new($Bin."/../../tms/BadTMS.tms");
ok (! defined $badTMS, "Unvalid TMS detected");

my $badSRS = BE4::TileMatrixSet->new($Bin."/../../tms/BadSRS.tms");
ok (! defined $badSRS, "Unvalid SRS detected");

######################################################

# Test on ID/order functions

is ($tmsQuadTree->getIDfromOrder(2), "level_19", "Conversion order -> ID");
is ($tmsQuadTree->getOrderfromID("level_4"), 17, "Conversion ID -> order");
is ($tmsQuadTree->getBelowLevelID("level_16"), "level_17", "Find the below level ID");

######################################################

# Test on inversion SRS and TMS with just one TM

my $inversionSRS = BE4::TileMatrixSet->new($Bin."/../../tms/InversionSRS.tms");
ok (defined $inversionSRS, "TileMatrixSet with just one tile matrix created");
ok ($inversionSRS->getInversion(),"SRS which need reversed coordinates detected");

######################################################

done_testing();

