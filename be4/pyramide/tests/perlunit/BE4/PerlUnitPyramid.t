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
use BE4::Pyramid;

######################################################

use constant TRUE  => 1;
use constant FALSE => 0;

# Pyramid creation

my $newPyr = BE4::Pyramid->new({
    tms_path => $Bin."/../../tms",
    tms_name => "LAMB93_10cm.tms",

    dir_depth => 2,

    pyr_data_path => $Bin."/../../pyramid",
    pyr_desc_path => $Bin."/../../pyramid",
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
ok (defined $newPyr, "New Pyramid created");
is ($newPyr->getNodataValue(),"255,255,255","Nodata conversion hex -> dec");

my $updatePyr = BE4::Pyramid->new({
    tms_path => $Bin."/../../tms",

    pyr_data_path => $Bin."/../../pyramid",
    pyr_desc_path => $Bin."/../../pyramid",
    pyr_name_new => "updatePyramid",
    pyr_level_bottom => 19,

    pyr_data_path_old => $Bin."/../../pyramid",
    pyr_desc_path_old => $Bin."/../../pyramid",
    pyr_name_old => "oldPyramid",

    # Parameters switched by old pyramid's ones
    dir_image => "HIMAJE",
    dir_nodata => "NAUDATTA",
    dir_metadata => "MAITADATA",

    tms_name => "WRONG.tms",
    dir_depth => 5,

    pyr_level_bottom => "level_19",

    compression => "jpg",
    image_width => 4,
    image_height => 4,
    bitspersample => 8,
    sampleformat => "float",
    photometric => "gray",
    samplesperpixel => 1,
    interpolation => "nn",

    color => "0,1,2"
    
});
ok (defined $updatePyr, "Update Pyramid created");

my $badUpdatePyr = BE4::Pyramid->new({
    tms_path => $Bin."/../../tms",

    pyr_data_path => $Bin."/../../pyramid",
    pyr_desc_path => $Bin."/../../pyramid",
    pyr_name_new => "updatePyramid",
    pyr_level_bottom => 19,

    pyr_data_path_old => $Bin."/../../pyramid",
    pyr_desc_path_old => $Bin."/../../pyramid",
    pyr_name_old => "badOldPyramid",
});
ok (! defined $badUpdatePyr, "Wrong level ID in hte old pyramid's descriptor detected");

######################################################

# Getters

is ($updatePyr->getTmsName(), "LAMB93_10cm",
    "TMS in the old pyramid's descriptor take precedence over configuration");

is ($updatePyr->getNodataValue(), "0,255,255",
    "Nodata value in the old pyramid's descriptor take precedence over configuration");

is ($updatePyr->getDirImage(), "IMG",
    "Image directory name in the old pyramid's descriptor take precedence over configuration");
is ($updatePyr->getDirNodata(), "NDT",
    "Nodata directory name in the old pyramid's descriptor take precedence over configuration");

is ($updatePyr->getDirDepth(), 2,
    "Directory depth in the old pyramid's descriptor take precedence over configuration");

is ($updatePyr->getInterpolation(), "linear",
    "Interpolation in the old pyramid's descriptor take precedence over configuration");
is ($updatePyr->getCompression(), "png",
    "Compression in the old pyramid's descriptor take precedence over configuration");
is ($updatePyr->getFormatCode(), "TIFF_PNG_INT8",
    "Format code in the old pyramid's descriptor take precedence over configuration");
is ($updatePyr->getSamplesPerPixel(), 3,
    "Samples per pixel in the old pyramid's descriptor take precedence over configuration");
is ($updatePyr->getPhotometric(), "rgb",
    "Photometric in the old pyramid's descriptor take precedence over configuration");
is ($updatePyr->getBitsPerSample(), 8,
    "Bits per sample in the old pyramid's descriptor take precedence over configuration");
is ($updatePyr->getSampleFormat(), "uint",
    "Sample format in the old pyramid's descriptor take precedence over configuration");

is ($updatePyr->getTilesPerWidth(), 16,
    "Tiles per width in the old pyramid's descriptor take precedence over configuration");
is ($updatePyr->getTilesPerHeight(), 16,
    "Tiles per height in the old pyramid's descriptor take precedence over configuration");

is (scalar keys %{$updatePyr->getLevels()}, 20, "Levels fetched from the old pyramid's descriptor");

######################################################

is ($updatePyr->getDirImage(), "IMG", "Compose path of image");
is ($updatePyr->getDirImage(FALSE), "IMG", "Compose path of image");
is ($updatePyr->getDirImage(TRUE), $Bin."/../../pyramid/updatePyramid/IMG", "Compose path of image");

######################################################

ok ($updatePyr->createLevels(0,5),"Levels updated with new top/bottom level's orders");

is (scalar keys %{$updatePyr->getLevels()}, 22, "Levels number expected");

######################################################

done_testing();

