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
use BE4::Level;

######################################################

# Level object creation

my $level = BE4::Level->new({
    id                => "level_12",
    order             => 9,
    dir_image         => "/abolute/path/to/imageDir",
    dir_nodata        => "/abolute/path/to/nodataDir",
    dir_mask        => "/abolute/path/to/maskDir",
    size              => [16,8],
    dir_depth         => 2,
});

ok (defined $level, "Level created");

######################################################

# Test on functions

$level->updateExtremTiles(14,21,4,12); # rowMin,rowMax,colMin,colMax #
my ($rowMin,$rowMax,$colMin,$colMax) = $level->getLimits();
is_deeply([$rowMin,$rowMax,$colMin,$colMax],[14,21,4,12],"Update extrem tiles for the first time");

$level->updateExtremTiles(20,22,2,4); # rowMin,rowMax,colMin,colMax #
($rowMin,$rowMax,$colMin,$colMax) = $level->getLimits();
is_deeply([$rowMin,$rowMax,$colMin,$colMax],[14,22,2,12],"Update extrem tiles for the second time");

my $xmlLevel = <<"XMLLEVEL";
    <level>
        <tileMatrix>level_12</tileMatrix>
        <baseDir>../../../abolute/path/to/imageDir</baseDir>
        <mask>
            <baseDir>../../../abolute/path/to/maskDir</baseDir>
            <format>TIFF_ZIP_INT8</format>
        </mask>
        <tilesPerWidth>16</tilesPerWidth>
        <tilesPerHeight>8</tilesPerHeight>
        <pathDepth>2</pathDepth>
        <nodata>
            <filePath>../../../abolute/path/to/nodataDir/nd.tif</filePath>
        </nodata>
        <TMSLimits>
            <minTileRow>14</minTileRow>
            <maxTileRow>22</maxTileRow>
            <minTileCol>2</minTileCol>
            <maxTileCol>12</maxTileCol>
        </TMSLimits>
    </level>
<!-- __LEVELS__ -->
XMLLEVEL

is ($level->exportToXML("/absolute/path/to"), $xmlLevel, "Export level to XML (for pyramid's descriptor)");

######################################################

done_testing();

