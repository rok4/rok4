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
use COMMON::DataSourceLoader;

# Other used class
use COMMON::TileMatrixSet;

######################################################

# GeoImage creation

my $newDSL = COMMON::DataSourceLoader->new({
    filepath_conf => $Bin."/../../sources/sources.txt"
});
ok (defined $newDSL, "DataSourceLoader created");
is ($newDSL->getNumberDataSources(), 4, "All expected data sources are created");

my $oldDSL = COMMON::DataSourceLoader->new({
        path_image => $Bin."/../../images/BDORTHO",
        srs => "IGNF:LAMB93",
    },{
        wms_layer   => "LAYER",
        wms_url     => "http://url/server/wms",
        wms_version => "1.3.0",
        wms_request => "getMap",
        wms_format  => "image/tiff"
    }, "18"
);
ok (defined $oldDSL, "DataSourceLoader created with old configuration");

######################################################

my $TMS = COMMON::TileMatrixSet->new($Bin."/../../tms/LAMB93_10cm.tms");

my ($bottomOrder,$topOrder) = $newDSL->updateDataSources($TMS);
is_deeply([$bottomOrder,$topOrder],[10,21],
          "Update bottom/top ID/orders for each data source : no global top level specified");

($bottomOrder,$topOrder) = $newDSL->updateDataSources($TMS,"level_3");
is_deeply([$bottomOrder,$topOrder],[10,18],
          "Update bottom/top ID/orders for each data source : global top level specified");

($bottomOrder,$topOrder) = $newDSL->updateDataSources($TMS,"level_6");
is_deeply([$bottomOrder,$topOrder],[-1,-1],
          "Update bottom/top ID/orders for each data source : unconsistent level ID detected");

($bottomOrder,$topOrder) = $newDSL->updateDataSources($TMS,"fake_level");
is_deeply([$bottomOrder,$topOrder],[-1,-1],
          "Update bottom/top ID/orders for each data source : unknown level ID detected");

######################################################

done_testing();

