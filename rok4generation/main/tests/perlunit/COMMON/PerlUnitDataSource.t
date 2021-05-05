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

use FindBin qw($Bin);

use Test::More;

use Log::Log4perl qw(:easy);
# logger by default for unit tests
Log::Log4perl->easy_init({
    level => $WARN,
    layout => '%5p : %m (%M) %n'
});

# My tested class
use COMMON::DataSource;

######################################################

# DataSource creation

my $objDSimage = COMMON::DataSource->new(
    "19",
    {
        srs => "IGNF:LAMB93",
        path_image => $Bin."/../../images/BDORTHO/"
    }
);
ok (defined $objDSimage, "DataSource (just image) created");
ok ($objDSimage->hasImages(), "DataSource has images");
ok (! $objDSimage->hasHarvesting(), "DataSource has not harvesting");

my $objDSharvest = COMMON::DataSource->new(
    "19",
    {
        srs => "IGNF:WGS84G",
        extent =>  $Bin."/../../shape/Polygon.txt",
        wms_layer   => "layer",
        wms_url => "http://url/wms/",
        wms_version => "1.3.0",
        wms_request => "getMap",
        wms_format  => "image/tiff",
    }
);
ok (defined $objDSharvest, "DataSource (just harvesting) created");
ok (! $objDSharvest->hasImages(), "DataSource has not images");
ok ($objDSharvest->hasHarvesting(), "DataSource has harvesting");

######################################################

# Bad parameters

my $error = COMMON::DataSource->new(
    "19",
    {
        srs => "IGNF:LAMB93",
        path_image => $Bin."/../../images/"
    }
);
ok (! defined $error, "Wrong data source detected");
undef $error;

$error = COMMON::DataSource->new(
    "19",
    {
        srs => "IGNF:LAMB93",
    }
);
ok (! defined $error, "No data source detected");
undef $error;

######################################################

done_testing();

