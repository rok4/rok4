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

# My tested class
use BE4::GeoImage;

######################################################

# GeoImage creation

my $BDP = BE4::GeoImage->new($Bin."/../../images/BDPARCELLAIRE/BDPARCELLAIRE.tif");
ok (defined $BDP, "BDPARCELLAIRE GeoImage created");

my $BDA = BE4::GeoImage->new($Bin."/../../images/BDALTI/BDALTI.tif");
ok (defined $BDA, "BDALTI GeoImage created");

my $BDO = BE4::GeoImage->new($Bin."/../../images/BDORTHO/BDORTHO.tif");
ok (defined $BDO, "BDO GeoImage created (with associated mask)");

my $error = BE4::GeoImage->new($Bin."/../../fake/path.tif");
ok (! defined $error, "Wrong path detected");

######################################################

# Test on computeInfo

my ($bps,$ph,$sf,$spp) = $BDP->computeInfo();
is_deeply ([$bps,$ph,$sf,$spp], [8,"gray","uint",1],"Extract information from BDPARCELLAIRE GeoImage");

($bps,$ph,$sf,$spp) = $BDA->computeInfo();
is_deeply ([$bps,$ph,$sf,$spp], [32,"gray","float",1],"Extract information from BDALTI GeoImage");

($bps,$ph,$sf,$spp) = $BDO->computeInfo();
is_deeply ([$bps,$ph,$sf,$spp], [8,"rgb","uint",3],"Extract information from BDORTHO GeoImage");

######################################################

# Test on exportForMntConf

my $expectedResult = sprintf "%s\t%.12f\t%.12f\t%.12f\t%.12f\t%.12f\t%.12f\n",
    "IMG $Bin/../../images/BDPARCELLAIRE/BDPARCELLAIRE.tif", 653000, 6858000, 654000, 6857000, 0.1,0.1;
    
is ($BDP->exportForMntConf(), $expectedResult,"Export for mergeNtiff configuration");

$expectedResult = sprintf "%s\t%.12f\t%.12f\t%.12f\t%.12f\t%.12f\t%.12f\n%s\n",
    "IMG $Bin/../../images/BDORTHO/BDORTHO.tif", 642000, 6862000, 643000, 6861000, 0.5, 0.5,
    "MSK $Bin/../../images/BDORTHO/BDORTHO.msk";
    
is ($BDO->exportForMntConf(), $expectedResult,"Export for mergeNtiff configuration (image + mask)");

######################################################

done_testing();

