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
File: SourcePyramid.pm

Class: JOINCACHE::SourcePyramid

Store all informations about a pyramid.

Using:
    (start code)
    use JOINCACHE::SourcePyramid;

    my $objPyr = JOINCACHE::SourcePyramid->new("/home/IGN/descriptors/SOURCE1.pyr");
    (end code)

Attributes:
    pyramidDescriptor - string - Absolute path to the source pyramid descriptor.
    levels - <JOINCACHE::SourceLevel> hash - Used levels in the source pyramid. Key is level's ID.
    formatCode - string - Format code of the source pyramid.
    photometric  - string - Photometrc of the source pyramid.
    samplesperpixel - integer -  Number of samples per pixel of the source pyramid.
    compatible - boolean - Precise if this source pyramid is compatible with the final pyramid. If the final pyramid is compatible with the source pyramid, we can make links. We compare format, samplesperpixel, and photometric.

=cut

################################################################################

package JOINCACHE::SourcePyramid;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
use XML::LibXML;
use File::Basename;

use BE4::Pyramid;
use JOINCACHE::SourceLevel;

require Exporter;
use AutoLoader qw(AUTOLOAD);

our @ISA = qw(Exporter);

our %EXPORT_TAGS = ( 'all' => [ qw() ] );
our @EXPORT_OK   = ( @{$EXPORT_TAGS{'all'}} );
our @EXPORT      = qw();

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

Pyramid constructor. Bless an instance. Checks and stores informations.

Parameters (list):
    file - string - Absolute file path to the source pyramid's descriptor (extension .pyr)

=cut
sub new {
    my $this = shift;
    my $file = shift;

    my $class= ref($this) || $this;
    # IMPORTANT : if modification, think to update natural documentation (just above)
    my $self = {
        pyramidDescriptor => undef,
        levels => {},
        formatCode => undef,
        photometric => undef,
        samplesperpixel => undef,
        compatible => FALSE,
    };

    bless($self, $class);

    TRACE;

    if (! defined $file) {
        ERROR("The parameter 'file' is required!");
        return undef;
    }

    if (! -f  $file) {
        ERROR(sprintf "The file '%s' does not exist",$file);
        return undef;
    }

    $self->{pyramidDescriptor} = $file;
    
    return $self;   
}

=begin nd
Function: loadAndCheck

We have to collect pyramid's attributes' values, parsing the XML file. We control values, in order to have the same as the final pyramid. Returns FALSE if there is a problem to load or if source pyramid is not consistent with the final pyramid.

Parameters (list):
    pyramid - <BE4::Pyramid> - Final pyramid, to compare components
=cut
sub loadAndCheck {
    my $self = shift;
    my $pyramid = shift;

    my $dirDepth = $pyramid->getDirDepth();
    my $tilesPerWidth = $pyramid->getTilesPerWidth();
    my $tilesPerHeight = $pyramid->getTilesPerHeight();
    my $tmsName = $pyramid->getTmsName();

    my $pyramidDescFile = $self->{pyramidDescriptor};

    # read xml pyramid
    my $parser  = XML::LibXML->new();
    my $xmltree =  eval { $parser->parse_file($pyramidDescFile); };

    if (! defined ($xmltree) || $@) {
        ERROR(sprintf "Can not read the XML file Pyramid : %s", $@);
        return FALSE;
    }

    my $root = $xmltree->getDocumentElement;

    # FORMAT
    my $tagformat = $root->findnodes('format')->to_literal;
    if ($tagformat eq '') {
        ERROR(sprintf "Can not find parameter 'format' in the XML file Pyramid (%s) !", $pyramidDescFile);
        return FALSE;
    }
    # Format have to be the same as the final pyramid
    if ($tagformat =~ m/_INT8/) {
        if (! ($pyramid->getBitsPerSample() == 8 && $pyramid->getSampleFormat eq "uint") ) {
            ERROR("Source pyramids have to be in 8-bits unsigned integer (format = TIFF_XXX_INT8");
            return FALSE;
        }
    }
    if ($tagformat =~ m/_FLOAT32/) {
        if (! ($pyramid->getBitsPerSample() == 32 && $pyramid->getSampleFormat eq "float") ) {
            ERROR("Source pyramids have to be in 32-bits float (format = TIFF_XXX_FLOAT32");
            return FALSE;
        }
    }
    $self->{formatCode} = $tagformat;

    # SAMPLES PER PIXEL
    my $tagchannels = $root->findnodes('channels')->to_literal;
    if ($tagchannels eq '') {
        ERROR(sprintf "Can not find parameter 'channels' in the XML file Pyramid (%s) !", $pyramidDescFile);
        return FALSE;
    }
    $self->{samplesperpixel} = int($tagchannels);

    # TMS
    my $tagTMS = $root->findnodes('tileMatrixSet')->to_literal;
    if ($tagTMS eq '') {
        ERROR(sprintf "Can not find parameter 'tileMatrixSet' in the XML file Pyramid (%s) !", $pyramidDescFile);
        return FALSE;
    }
    if ($tagTMS ne $tmsName) {
        ERROR(sprintf "The TMS in the source pyramid '%s' (%s) is different from the TMS in the configuration (%s) !",
              $pyramidDescFile,$tagTMS,$tmsName);
        return FALSE;
    }

    # PHOTOMETRIC
    my $tagphotometric = $root->findnodes('photometric')->to_literal;
    if ($tagphotometric eq '') {
        WARN("Can not find parameter 'photometric', determine it from the samples per pixel .");

        my %sppToPh = (
            1 => "gray",
            3 => "rgb",
            4 => "rgb"
        );

        $tagphotometric = $sppToPh{$tagchannels};
    }
    $self->{photometric} = $tagphotometric;

    # LEVELS
    my @levels = $root->getElementsByTagName('level');

    # Reads and controls tilesPerWidth, tilesPerHeight and dirDepth, using the first level
    
    my $tagtilesPerWidth = $levels[0]->findvalue('tilesPerWidth');
    if ($tagtilesPerWidth eq '') {
        ERROR(sprintf "Can not find parameter 'tilesPerWidth' in the XML file Pyramid (%s) !", $pyramidDescFile);
        return FALSE;
    } elsif ($tagtilesPerWidth != $tilesPerWidth) {
        ERROR(sprintf "Tiles per width in the source pyramid '%s' (%s) != tiles per width in the configuration (%s) !",
              $pyramidDescFile,$tagtilesPerWidth,$tilesPerWidth);
        return FALSE;
    }
    
    my $tagtilesPerHeight = $levels[0]->findvalue('tilesPerHeight');
    if ($tagtilesPerHeight eq '') {
        ERROR(sprintf "Can not find parameter 'tilesPerHeight' in the XML file Pyramid (%s) !", $pyramidDescFile);
        return FALSE;
    } elsif ($tagtilesPerHeight != $tilesPerHeight) {
        ERROR(sprintf "Tiles per height in the source pyramid '%s' (%s) != tiles per height in the configuration (%s) !",
              $pyramidDescFile,$tagtilesPerHeight,$tilesPerHeight);
        return FALSE;
    }
    
    my $tagdepth = $levels[0]->findvalue('pathDepth');
    if ($tagdepth eq '') {
        ERROR(sprintf "Can not find parameter 'pathDepth' in the XML file Pyramid (%s) !", $pyramidDescFile);
        return FALSE;
    } elsif ($tagdepth != $dirDepth) {
        ERROR(sprintf "Directory depth in the source pyramid '%s' (%s) != directory depth in the configuration (%s) !",
              $pyramidDescFile,$tagdepth,$dirDepth);
        return FALSE;
    }

    my $directory = dirname($pyramidDescFile);

    foreach my $v (@levels) {
        my $levelId = $v->findvalue('tileMatrix');
        
        my $imageDir = $v->findvalue('baseDir');
        $imageDir = File::Spec->rel2abs($imageDir,$directory);

        my $maskDir = $v->findvalue('mask/baseDir');
        if ($maskDir eq '') {
            undef $maskDir;
        } else {
            $maskDir = File::Spec->rel2abs($maskDir,$directory);
        }

        my @taglimits = [
            $v->findvalue('TMSLimits/minTileCol'),
            $v->findvalue('TMSLimits/minTileRow'),
            $v->findvalue('TMSLimits/maxTileCol'),
            $v->findvalue('TMSLimits/maxTileRow'),
        ];

        my $level = JOINCACHE::SourceLevel->new({
            id => $levelId,
            dir_image => $imageDir,
            dir_mask => $maskDir,
            limits => @taglimits
        });
        if (! defined $level) {
            ERROR ("Cannot create the SourceLevel object !");
            return FALSE;
        }

        $self->{levels}->{$levelId} = $level;
    }

    # Is it compatible with the final pyramid
    if ($self->{formatCode} eq $pyramid->getFormatCode &&
        $self->{samplesperpixel} == $pyramid->getSamplesPerPixel &&
        $self->{photometric} eq $pyramid->getPhotometric) {
            $self->{compatible} = TRUE;
    }

    return TRUE;
}

####################################################################################################
#                                Group: Getters - Setters                                          #
####################################################################################################

=begin nd
Function: hasLevel

Precises if the provided level exists in the source pyramid.

Parameters (list):
    levelID - string - Identifiant of the asked level
=cut
sub hasLevel {
    my $self = shift;
    my $levelID = shift;

    if (defined $levelID && exists $self->{levels}->{$levelID}) {
        return TRUE;
    }

    return FALSE;
}

=begin nd
Function: getLevel

Returns the <JOINCACHE::SourceLevel> object for the provided level ID.

Parameters (list):
    levelID - string - Identifiant of the asked level
=cut
sub getLevel {
    my $self = shift;
    my $levelID = shift;

    return $self->{levels}->{$levelID};
}

=begin nd
Function: getImageDirectory

Returns the image directory for the provided level, using <JOINCACHE::SourceLevel::getDirImage>.

Parameters (list):
    levelID - string - Identifiant of the asked level
=cut
sub getImageDirectory {
    my $self = shift;
    my $levelID = shift;

    return $self->{levels}->{$levelID}->getDirImage();
}

=begin nd
Function: getMaskDirectory

Returns the mask directory for the provided level, using <JOINCACHE::SourceLevel::getDirMask>.

Parameters (list):
    levelID - string - Identifiant of the asked level
=cut
sub getMaskDirectory {
    my $self = shift;
    my $levelID = shift;

    return $self->{levels}->{$levelID}->getDirMask();
}

# Function: isCompatible
sub isCompatible {
    my $self = shift;

    return $self->{compatible};
}

# Function: getFormatCode
sub getFormatCode {
    my $self = shift;
    return $self->{formatCode};
}

# Function: getBitsPerSample
sub getBitsPerSample {
    my $self = shift;

    if ($self->{formatCode} =~ m/FLOAT32/) {
        return 32;
    } elsif ($self->{formatCode} =~ m/INT8/) {
        return 8;
    }

    ERROR(sprintf "Unknown sample format, in the format code %s", $self->{formatCode});
    return 0;
}

# Function: getSamplesPerPixel
sub getSamplesPerPixel {
    my $self = shift;
    return $self->{samplesperpixel};
}

####################################################################################################
#                                Group: Export methods                                             #
####################################################################################################

=begin nd
Function: exportForDebug

Returns all source pyramid's information. Useful for debug.

Example:
    (start code)
    Object JOINCACHE::SourcePyramid :
        Descriptor : /home/ign/desc/ORTHO_RAW_LAMB93_D075-O.pyr
        Image components :
            Format code : TIFF_RAW_INT8
            Samples per pixel : 3
            Photometric : rgb
        Is NOT compatible with the final pyramid
        Number of levels : 20
    (end code)
=cut
sub exportForDebug {
    my $self = shift ;
    
    my $export = "";
    
    $export .= "\nObject JOINCACHE::SourcePyramid :\n";
    $export .= sprintf "\t Descriptor : %s\n", $self->{pyramidDescriptor};

    $export .= sprintf "\t Image components :\n";
    $export .= sprintf "\t\t Format code : %s\n", $self->{formatCode};
    $export .= sprintf "\t\t Samples per pixel : %s\n", $self->{samplesperpixel};
    $export .= sprintf "\t\t Photometric : %s\n", $self->{photometric};
    $export .= sprintf "\t Is NOT compatible with the final pyramid\n" if (! $self->{compatible});
    $export .= sprintf "\t Is compatible with the final pyramid\n" if ($self->{compatible});
    
    $export .= sprintf "\t Number of levels : %s\n", scalar (keys %{$self->{levels}});
    
    return $export;
}

1;
__END__

