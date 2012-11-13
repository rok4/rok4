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
use FindBin qw($Bin); # aboslute path of the present testfile in $Bin

# My tested class
use BE4::QTree;

#Other Used Class
use BE4::Forest;
use BE4::Pyramid;
use BE4::Commands;
use BE4::DataSource;

######################################################

# QTree Object Creation

my $pyramid = BE4::Pyramid->new({

    tms_path => $Bin."/../tms",
    tms_name => "LAMB93_10cm.tms",

    dir_depth => 2,

    pyr_data_path => $Bin."/../pyramid",
    pyr_desc_path => $Bin."/../pyramid",
    pyr_name_new => "newPyramid",

    dir_image => "IMAGE",
    dir_nodata => "NODATA",
    dir_metadata => "METADATA",

    pyr_level_bottom => "19",

    compression => "raw",
    image_width => 16,
    image_height => 16,
    bitspersample => 8,
    sampleformat => "uint",
    photometric => "rgb",
    samplesperpixel => 3,
    interpolation => "bicubic",

    color => "FFFFFF"
});

my $commands = BE4::Commands->new($pyramid);

my $datasource = BE4::DataSource->new(
    "19",
    {
        srs => "IGNF:LAMB93",
        path_image => $Bin."/../images/BDORTHO/"
    }
);

my $DSL = BE4::DataSourceLoader->new({
    path_image => $Bin."/../images/BDORTHO",
    srs => "IGNF:LAMB93",
},{
    wms_layer   => "LAYER",
    wms_url     => "http://url/server/wms",
    wms_version => "1.3.0",
    wms_request => "getMap",
    wms_format  => "image/tiff"
}, "18");

my $job_number = 16;
my $path_temp = $Bin."/../temp/";
my $path_shell = $Bin."/../temp";


my $forest = BE4::Forest->new($DSL,$pyramid,{
	job_number => $job_number,
	path_temp => $path_temp,
	path_shell => $path_shell,
});

my $qtree = BE4::Graph->new($forest,$datasource,$pyramid,$commands);

ok (defined $qtree, "QTree Object created");

######################################################

done_testing();
