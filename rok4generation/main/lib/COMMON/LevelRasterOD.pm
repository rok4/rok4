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

=begin nd
File: LevelRasterOD.pm

Class: COMMON::LevelRasterOD

(see ROK4GENERATION/libperlauto/COMMON_LevelRasterOD.png)

Describe a level in a raster pyramid.

Using:
    (start code)
    use COMMON::LevelRasterOD;
    (end code)

Attributes:
    id - string - Level identifiant.
    order - integer - Level order (ascending resolution)
    tm - <COMMON::TileMatrix> - Binding Tile Matrix to this level
    limits - integer array - Extrems columns and rows for the level (Extrems tiles which contains data) : [rowMin,rowMax,colMin,colMax]
    sources - <WMTSALAD::PyrSource> or <WMTSALAD::WmsSource> array - Sources to use for this level

    desc_path - string - Directory path of the pyramid's descriptor containing this level

    dir_depth - integer - Number of subdirectories from the level root to the image if FILE storage type : depth = 2 => /.../LevelID/SUB1/SUB2/IMG.tif
    size - integer array - Number of tile in one image for this level, widthwise and heightwise : [width, height].
    dir_image - string - Directory in which we write the pyramid's images if FILE storage type   
=cut

################################################################################

package COMMON::LevelRasterOD;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
use XML::LibXML;
use File::Spec;
use Cwd;
use Data::Dumper;

require Exporter;
use AutoLoader qw(AUTOLOAD);

our @ISA = qw(Exporter);

our %EXPORT_TAGS = ( 'all' => [ qw() ] );
our @EXPORT_OK   = ( @{$EXPORT_TAGS{'all'}} );
our @EXPORT      = qw();

use COMMON::TileMatrix;
use COMMON::TileMatrixSet;

use WMTSALAD::WmsSource;
use WMTSALAD::PyrSource;

################################################################################
# Constantes
use constant TRUE  => 1;
use constant FALSE => 0;

################################################################################

BEGIN {}
INIT {}
END {}

####################################################################################################
#                                        Group: Constructors                                       #
####################################################################################################

=begin nd
Constructor: new

Level constructor. Bless an instance.

Parameters (hash):
    type - string - XML
    params - <XML::LibXML::Element> - XML node of the level (from the pyramid's descriptor)
        or
    type - string - VALUES
    params - string hash - Hash containg all needed level informations

    descDirectory - string - Directory path of the pyramid's descriptor containing this level

See also:
    <_loadXML>, <_loadValues>
=cut
sub new {
    my $class = shift;
    my $type = shift;
    my $params = shift;
    my $descDirectory = shift;
    
    $class= ref($class) || $class;
    # IMPORTANT : if modification, think to update natural documentation (just above)
    my $this = {
        id => undef,
        order => undef,
        tm => undef,

        limits => undef, # rowMin,rowMax,colMin,colMax

        sources => [],

        # ABOUT PYRAMID
        desc_path => undef,

        # SI STOCKAGE FICHIER
        size => [undef, undef],
        dir_depth => undef,
        dir_image => undef
    };
    
    bless($this, $class);

    if (! defined $descDirectory) {
        ERROR ("The 'desc_path' is required to create a level");
        return undef;
    }
    $this->{desc_path} = $descDirectory;

    if ($type eq "XML") {
        if ( ! $this->_loadXML($params) ) {
            ERROR("Cannot load Level XML");
            return undef;
        }
    } else {
        if (! $this->_loadValues($params)) {
            ERROR ("One parameter is missing !");
            return undef;
        }
    }

    return $this;
}

=begin nd
Function: _loadValues

Check and store level's attributes values.

Parameter:
    params - hash - Hash containg all needed level informations
=cut
sub _loadValues {
    my $this   = shift;
    my $params = shift;
  
    return FALSE if (! defined $params);
    
    if (! exists($params->{tm})) {
        ERROR ("The parameter 'tm' is required");
        return FALSE;
    }
    if (ref ($params->{tm}) ne "COMMON::TileMatrix") {
        ERROR ("The parameter 'tm' is not a COMMON:TileMatrix");
        return FALSE;
    }
    $this->{tm} = $params->{tm};
    $this->{order} = $params->{tm}->getOrder();
    $this->{id} = $params->{tm}->getID();

    if (! exists($params->{persistent}) || ! defined $params->{persistent}) {
        ERROR ("The parameter 'persistent' is required");
        return FALSE;
    }

    if (! exists($params->{limits})) {
        ERROR ("The parameter 'limits' is required");
        return FALSE;
    }
    # check values
    if (scalar (@{$params->{limits}}) != 4){
        ERROR("List for 'limits' have to be [rowMin,rowMax,colMin,colMax] !");
        return FALSE;
    }
    $this->{limits} = $params->{limits};


    if (! exists($params->{sources}) || ! defined $params->{sources}) {
        ERROR ("The parameter 'sources' is required");
        return FALSE;
    }
    $this->{sources} = $params->{sources};

    # Si stockage, il faut :
    # - dir_data
    # - dir_depth
    # - size ( = [ image_width, image_height ] )
    if ($params->{persistent}) {

        if (! exists($params->{dir_data}) || ! defined $params->{dir_data}) {
            ERROR ("If persistent, the parameter 'dir_data' is required");
            return FALSE;
        }

        $this->{dir_image} = File::Spec->catdir($params->{dir_data}, "IMAGE", $this->{id});

        if (! exists($params->{dir_depth}) || ! defined $params->{dir_depth}) {
            ERROR ("If persistent, the parameter 'dir_depth' is required");
            return FALSE;
        }

        $this->{dir_depth} = $params->{dir_depth};

        if (! exists($params->{size})) {
            ERROR ("If persistent, the parameter 'size' is required");
            return FALSE;
        }
        # check values
        if (! scalar (@{$params->{size}})){
            ERROR("List empty for 'size' !");
            return FALSE;
        }
        $this->{size} = $params->{size};
    }

    return TRUE;
}

=begin nd
Function: _loadXML

Extract level's information from the XML element

Parameter:
    levelRoot - <XML::LibXML::Element> - XML node of the level (from the pyramid's descriptor)
=cut
sub _loadXML {
    my $this   = shift;
    my $levelRoot = shift;

    $this->{id} = $levelRoot->findvalue('tileMatrix');
    if (! defined $this->{id} || $this->{id} eq "" ) {
        ERROR ("Cannot extract 'tileMatrix' from the XML level");
        return FALSE;
    }

    $this->{limits} = [
        $levelRoot->findvalue('TMSLimits/minTileRow'),
        $levelRoot->findvalue('TMSLimits/maxTileRow'),
        $levelRoot->findvalue('TMSLimits/minTileCol'),
        $levelRoot->findvalue('TMSLimits/maxTileCol')
    ];
    if (! defined $this->{limits}->[0] || $this->{limits}->[0] eq "") {
        ERROR ("Cannot extract extrem tiles from the XML level");
        return FALSE;
    }

    if (defined $levelRoot->findvalue('onFly') && $levelRoot->findvalue('onFly') eq "true") {
        
        my $dirimg = $levelRoot->findvalue('baseDir');
        if (! defined $dirimg || $dirimg eq "" ) {
            ERROR ("Cannot extract 'baseDir' from an onFly level");
            return FALSE;
        }
        $this->{dir_image} = File::Spec->rel2abs(File::Spec->rel2abs( $dirimg , $this->{desc_path} ) );

        $this->{dir_depth} = $levelRoot->findvalue('pathDepth');
        if (! defined $this->{dir_depth} || $this->{dir_depth} eq "" ) {
            ERROR ("Cannot extract 'pathDepth' from an onFly level");
            return FALSE;
        }

        $this->{size} = [$levelRoot->findvalue('tilesPerWidth'), $levelRoot->findvalue('tilesPerHeight')];
        if (! defined $this->{size}->[0] || ! defined $this->{size}->[1] ) {
            ERROR ("Cannot extract 'tilesPerWidth' and 'tilesPerHeight' from an onFly level");
            return FALSE;
        }
    }

    elsif (! defined $levelRoot->findvalue('onDemand') || $levelRoot->findvalue('onDemand') ne "true") {
        ERROR("The level is neither onDemand, nor onFly.");
        return FALSE;
    }

    my $sourcesNode = $levelRoot->findnodes('sources')->[0];

    if (! defined $sourcesNode) {
        ERROR("A level onFly or onDemand have to own a 'sources' part");
        return FALSE;
    }

    foreach my $s ($sourcesNode->getChildrenByTagName('*')) {
        my $name = $s->nodeName;
        
        my $source = undef;

        if ($name eq "webService") {
            $source = WMTSALAD::WmsSource->new("XML", $s);
        }
        elsif ($name eq "basedPyramid") {
            $source = WMTSALAD::PyrSource->new("XML", $s);
        }
        else {
            ERROR("A source have to be webService or basedPyramid, '$name' is unknown");
            return FALSE;
        }

        if (! defined $source) {
            ERROR("Cannot create a source from the XML element");
            return FALSE;
        }

        push(@{$this->{sources}}, $source);
    }

    return TRUE;
}

####################################################################################################
#                                Group: Getters - Setters                                          #
####################################################################################################

# Function: getID
sub getID {
    my $this = shift;
    return $this->{id};
}

=begin nd
method: isPersistent
=cut
sub isPersistent {
    my $this = shift;
    if (defined $this->{dir_image}) {
        return TRUE;
    }
    return FALSE;
}

=begin nd
Function: getLimits

Return extrem tiles as an array: rowMin,rowMax,colMin,colMax
=cut
sub getLimits {
    my $this = shift;
    return ($this->{limits}->[0], $this->{limits}->[1], $this->{limits}->[2], $this->{limits}->[3]);
}

# Function: getOrder
sub getOrder {
    my $this = shift;
    return $this->{order};
}

# Function: getImageWidth
sub getImageWidth {
    my $this = shift;
    return $this->{size}->[0];
}

# Function: getImageHeight
sub getImageHeight {
    my $this = shift;
    return $this->{size}->[1];
}


# Function: getDirsInfo
sub getDirsInfo {
    my $this = shift;

    my @dirs = File::Spec->splitdir($this->{dir_image});
    pop(@dirs);pop(@dirs);pop(@dirs);
    my $dir_data = File::Spec->catdir(@dirs);

    return ($this->{dir_depth}, $dir_data);
}

=begin nd
method: updateLimits

Compare old extrems rows/columns with the news and update values.

Parameters (list):
    rowMin, rowMax, colMin, colMax - integer list - Tiles indices to compare with current extrems
=cut
sub updateLimits {
    my $this = shift;
    my ($rowMin,$rowMax,$colMin,$colMax) = @_;

    if (! defined $this->{limits}->[0] || $rowMin < $this->{limits}->[0]) {$this->{limits}->[0] = $rowMin;}
    if (! defined $this->{limits}->[1] || $rowMax > $this->{limits}->[1]) {$this->{limits}->[1] = $rowMax;}
    if (! defined $this->{limits}->[2] || $colMin < $this->{limits}->[2]) {$this->{limits}->[2] = $colMin;}
    if (! defined $this->{limits}->[3] || $colMax > $this->{limits}->[3]) {$this->{limits}->[3] = $colMax;}
}

=begin nd
method: updateLimitsFromBbox
=cut
sub updateLimitsFromBbox {
    my $this = shift;
    my ($xmin, $ymin, $xmax, $ymax) = @_;

    my $colMin = $this->{tm}->xToColumn($xmin);
    my $colMax = $this->{tm}->xToColumn($xmax);
    my $rowMin = $this->{tm}->yToRow($ymax);
    my $rowMax = $this->{tm}->yToRow($ymin);

    $this->updateLimits($rowMin,$rowMax,$colMin,$colMax);
}

=begin nd
method: bindTileMatrix

For levels loaded from an XML element, we have to link the Tile Matrix (we only have the level ID).

We control if level exists in the provided TMS, and we calculate the level's order in this TMS.

Parameter:
    tms - <COMMON::TileMatrixSet> - TMS containg the Tile Matrix to link to this level.
=cut
sub bindTileMatrix {
    my $this = shift;
    my $tms = shift;

    if (defined $this->{tm} ) {
        # le tm est déjà présent pour le niveau
        return TRUE;
    }

    $this->{tm} = $tms->getTileMatrix($this->{id});
    if (! defined $this->{tm}) {
        ERROR(sprintf "Cannot find a level with the id %s in the TMS", $this->{id});
        return FALSE;
    }

    $this->{order} = $tms->getOrderfromID($this->{id});

    return TRUE;
}


####################################################################################################
#                                Group: Export methods                                             #
####################################################################################################

=begin nd
method: exportToXML

Export Level's attributes in XML format.

Example:
    Niveau sans stockage, source pyramide
    (start code)
    <level>
        <tileMatrix>6</tileMatrix>
        <onDemand>true</onDemand>
        <TMSLimits>
            <minTileRow>12</minTileRow>
            <maxTileRow>12</maxTileRow>
            <minTileCol>1</minTileCol>
            <maxTileCol>1</maxTileCol>
        </TMSLimits>
        <sources>
            <basedPyramid>
                <file>/home/ign/PYRAMIDS/SOURCE.pyr</file>
                <style>normal</style>
                <transparent>false</transparent>
            </basedPyramid>
        </sources>
    </level>
    (end code)
    Niveau sans stockage, source pyramide
    (start code)
    <level>
        <tileMatrix>6</tileMatrix>
        <onDemand>true</onDemand>
        <TMSLimits>
            <minTileRow>12</minTileRow>
            <maxTileRow>12</maxTileRow>
            <minTileCol>1</minTileCol>
            <maxTileCol>1</maxTileCol>
        </TMSLimits>
        <sources>
            <webService>
                <url>wms.ign.fr</url>
                <timeout>60</timeout>
                <retry>10</retry>
                <wms>
                    <version>1.3.0</version>
                    <layers>LAYER</layers>
                    <styles>normal</styles>
                    <channels>3</channels>
                    <noDataValue>255,255,255</noDataValue>
                    <bbox minx=640000 miny=6840000 maxx=680000 maxy=6870000 />
                    <crs>EPSG:2154</crs>
                    <format>image/jpeg</format>
                </wms>
            </webService>
        </sources>
    </level>
    (end code)

=cut
sub exportToXML {
    my $this = shift;

    my $string =               "    <level>\n";
    $string .= sprintf         "        <tileMatrix>%s</tileMatrix>\n", $this->{id};

    if (defined $this->{dir_image}) {
        $string .=             "        <onFly>true</onFly>\n";
        $string .= sprintf     "        <tilesPerWidth>%s</tilesPerWidth>\n", $this->{size}->[0];
        $string .= sprintf     "        <tilesPerHeight>%s</tilesPerHeight>\n", $this->{size}->[1];
        $string .= sprintf     "        <baseDir>%s</baseDir>\n", File::Spec->abs2rel($this->{dir_image}, $this->{desc_path});
        $string .= sprintf     "        <pathDepth>%s</pathDepth>\n", $this->{dir_depth};

    } else {
        $string .=             "        <onDemand>true</onDemand>\n";
    }

    $string .=                 "        <TMSLimits>\n";
    if (defined $this->{limits}->[0]) {
        $string .= sprintf     "            <minTileRow>%s</minTileRow>\n", $this->{limits}->[0];
        $string .= sprintf     "            <maxTileRow>%s</maxTileRow>\n", $this->{limits}->[1];
        $string .= sprintf     "            <minTileCol>%s</minTileCol>\n", $this->{limits}->[2];
        $string .= sprintf     "            <maxTileCol>%s</maxTileCol>\n", $this->{limits}->[3];
    } else {
        $string .=             "            <minTileRow>0</minTileRow>\n";
        $string .=             "            <maxTileRow>0</maxTileRow>\n";
        $string .=             "            <minTileCol>0</minTileCol>\n";
        $string .=             "            <maxTileCol>0</maxTileCol>\n";
    }
    $string .=                 "        </TMSLimits>\n";

    $string .=                 "        <sources>\n";

    foreach my $s (@{$this->{sources}}) {
        $string .= $s->exportToXML();
    }
    $string .=                 "        </sources>\n";

    $string .=                 "    </level>\n";

    return $string;
}

1;
__END__
