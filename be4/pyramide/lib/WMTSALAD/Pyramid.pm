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
#
################################################################################

=begin nd
File: Pyramid.pm

Class: WMTSALAD::Pyramid

Using:
    (start code)
    use WMTSALAD::Pyramid;

    my pyramid = WMTSALAD::Pyramid->new( {
        
    } );

    $pyramid->write();    
    (end code)

Attributes:
    
   
=cut

################################################################################

package WMTSALAD::Pyramid;

use strict;
use warnings;

use File::Spec;

use Log::Log4perl qw(:easy);
use Data::Dumper;
use COMMON::Config;
use COMMON::CheckUtils;
use WMTSALAD::DataSource;
use WMTSALAD::PyrSource;
use WMTSALAD::WmsSource;
use BE4::TileMatrixSet;
use BE4::TileMatrix;
use BE4::Pixel;
use BE4::NoData;

use parent qw(Exporter);

our %EXPORT_TAGS = ( 'all' => [ qw() ] );
our @EXPORT_OK   = ( @{$EXPORT_TAGS{'all'}} );
our @EXPORT      = qw();

################################################################################
# Constantes
use constant TRUE  => 1;
use constant FALSE => 0;

my %IMAGE_SPECS = (
        image_format => [
            "TIFF_RAW_INT8",
            "TIFF_JPG_INT8",
            "TIFF_PNG_INT8",
            "TIFF_LZW_INT8",
            "TIFF_RAW_FLOAT32",
            "TIFF_LZW_FLOAT32",
            "TIFF_ZIP_INT8",
            "TIFF_ZIP_FLOAT32",
            "TIFF_PKB_INT8",
            "TIFF_PKB_FLOAT32"
        ],
        mask_format => [
            "TIFF_ZIP_INT8"
        ],
        metadata_type => [
            "INT32_DB_LZW"
        ],
        interpolation => [
            "lanczos",
            "nn",
            "bicubic",
            "linear"
        ],
        photometric => [
            "gray",
            "rgb",
            "mask"
        ],
        sampleformat => [
            "int",
            "uint",
            "float"
        ],
    );

my %DEFAULT = (
        noDataValue => {
            1 => "255",
            2 => "255,0",
            3 => "255,255,255",
            4 => "255,255,255,0",
        },
        compression => "RAW",
    );

################################################################################

BEGIN {}
INIT {}
END {}

####################################################################################################
#                                        Group: Constructors                                       #
####################################################################################################


=begin nd

Constructor: new

<WMTSALAD::WmsSource's> constructor.

Using:
    (start code)
    my pyramid = WMTSALAD::Pyramid->new( {
        
    } )
    (end code)

Parameters:
    params - hash reference, containing the following properties :
        {
            
        }

Returns:
    The newly created Pyramid object. 'undef' in case of failure.
    
=cut
sub new {
    my $this = shift;
    my $propertiesFile = shift;
    my $datasourcesFile = shift;

    my $class= ref($this) || $this;

    # IMPORTANT : if modification, think to update natural documentation (just above)
    # see config/pyramids/pyramid.xsd to get the list of parameters, as used by Rok4.
    my $self = {
        tileMatrixSet => undef,
        format => undef,
        channels => undef,
        noData => undef,
        interpolation => undef,
        photometric => undef,
        persistent => undef,
        dir_depth => undef,
        dir_image => undef,
        dir_nodata => undef,
        dir_mask => undef,
        dir_metadata => undef,
        pyr_name => undef,
        pyr_data_path => undef,
        pyr_desc_path => undef,

        image_width => undef,
        image_height => undef,

        datasources => {},
    };

    bless($self, $class);

    if (!$self->_init($propertiesFile,$datasourcesFile)) {
        ERROR(sprintf "Pyramid initialization failed.");
        return undef;
    }

    return $self;
}

sub _init {
    my $self = shift;
    my $propertiesFile = shift;
    my $datasourcesFile = shift;

    return FALSE if (!$self->_loadProperties($propertiesFile));
    return FALSE if (!$self->_loadDatasources($datasourcesFile));

    return TRUE;
}


####################################################################################################
#                              Group: Configuration files parsing                                  #
####################################################################################################

sub _loadProperties {
    my $self = shift;
    my $file = shift;

    if ((!defined $file) || ($file eq '')) {
        ERROR("Undefined properties configuration file.");
        return FALSE;
    } elsif ((! -e $file) || (! -r $file) || (-d $file)) {
        ERROR(sprintf "The properties configuration file does not exist, is not readable, or is a directory : %s", $file);
        return FALSE;
    }


    my $cfg = COMMON::Config->new({
        '-filepath' => $file,
        '-format' => 'INI',
        });
    my %fileContent = $cfg->getConfig();
    my $refFileContent = \%fileContent;
    my $sampleformat; # Value to pass to BE4::Pixel

    return FALSE if(! $self->_checkProperties($cfg));

    # Tile Matrix Set    
    my $TMS = BE4::TileMatrixSet->new(File::Spec->catfile($refFileContent->{pyramid}->{tms_path},$refFileContent->{pyramid}->{tms_name}));
    $self->{tileMatrixSet} = $TMS ;

    # Image format
    my $format = "TIFF_";
    if (( $cfg->isProperty({section => 'pyramid', property => 'compression'})) && ($refFileContent->{pyramid}->{compression} =~ m/^(jpg|png|lzw|zip|pkb)$/i)) {
        $format.= uc($refFileContent->{pyramid}->{compression})."_";
    } else {
        WARN(sprintf "Unrecognized or undefined compression type : '%s'. Setting to '%s'", $refFileContent->{pyramid}->{compression},$DEFAULT{compression});
        $format .= $DEFAULT{compression}."_";
    }
    if ($refFileContent->{pyramid}->{sampleformat} eq "int") {
        $sampleformat = "uint";
    } else {
        $sampleformat = $refFileContent->{pyramid}->{sampleformat};
    }
    $format .= uc($refFileContent->{pyramid}->{sampleformat}).$refFileContent->{pyramid}->{bitspersample} ;
    if ($self->isImageFormat($format)) {
        $self->{format} = $format;
    } else {
        ERROR(sprintf "Unrecognized image format : '%s'. Known image formats are : %s", $format, Dumper($IMAGE_SPECS{image_format}));
        return FALSE;
    }

    # Channels number 
    $self->{channels} = $refFileContent->{pyramid}->{samplesperpixel};

    # Pyramid's name. Some people say it's useful to name the resulting .pyr file.
    my $pyr_name = $refFileContent->{pyramid}->{pyr_name};
    $pyr_name =~ s/\.(pyr)$//i;
    $self->{pyr_name} = $pyr_name;

    # Persistence (default value : FALSE)
    my $persistent = $refFileContent->{pyramid}->{persistent};
    if (defined $persistent) {
        if ($persistent=~ m/\A(1|t|true)\z/i) {
            $self->{persistent} = TRUE;
        } elsif ($persistent =~ m/\A(0|f|false)\z/i) {
            $self->{persistent} = FALSE;
        }
    } else {
        INFO ("The undefined 'persistent' parameter is now set to 'false'.");
        $self->{persistent} = FALSE;
    }

    # Path depth
    $self->{dir_depth} = $refFileContent->{pyramid}->{dir_depth};

    # Data paths
    $self->{pyr_data_path} = $refFileContent->{pyramid}->{pyr_data_path};

    # Masks directory (optionnal)
    if ($cfg->isProperty({section => 'pyramid', property => 'dir_mask'})) {
        $self->{dir_mask} = $refFileContent->{pyramid}->{dir_mask};
    }

    # Images directory
    $self->{dir_image} = $refFileContent->{pyramid}->{dir_image};

    # Metadata directory (optionnal)
    if ($cfg->isProperty({section => 'pyramid', property => 'dir_metadata'})) {
        $self->{dir_metadata} = $refFileContent->{pyramid}->{dir_metadata};
    }

    # Nodata Directory
    $self->{dir_nodata} = $refFileContent->{pyramid}->{dir_nodata};

    # Descriptor's path
    $self->{pyr_desc_path} = $refFileContent->{pyramid}->{pyr_desc_path};

    # Photometric (optionnal)
    if ($cfg->isProperty({section => 'pyramid', property => 'photometric'})) {
        $self->{photometric} = $refFileContent->{pyramid}->{photometric};
    }

    # Nodata (default value : white, transparent if alpha channel available)
    my $pixel = BE4::Pixel->new({
        photometric => $self->{photometric},
        sampleformat => $sampleformat,
        bitspersample => $refFileContent->{pyramid}->{bitspersample},
        samplesperpixel => $self->{channels},
    });
    my $noDataValue;
    if ($cfg->isProperty({section => 'pyramid', property => 'color'})) {
        $noDataValue = $refFileContent->{pyramid}->{color};
    } else {
        my $chans = $self->{channels};
        $noDataValue = $DEFAULT{noDataValue}->{$chans};
    }
    $self->{noData} = BE4::NoData->new({ pixel => $pixel, value => $noDataValue });
    if (! defined $self->{noData}) {
        ERROR("Failed NoData initialization. Check 'color' value.");
        return FALSE;
    }

    # Interpolation (optionnal)
    if ($cfg->isProperty({section => 'pyramid', property => 'interpolation'})) {
        $self->{interpolation} = $refFileContent->{pyramid}->{interpolation};
    }

    # Image dimensions (number of tiles)
    $self->{image_width} = $refFileContent->{pyramid}->{image_width};
    $self->{image_height} = $refFileContent->{pyramid}->{image_height};

    return TRUE;
}


sub _loadDatasources {
    my $self = shift;
    my $file = shift;


    if ((!defined $file) || ($file eq '')) {
        ERROR("Undefined datasources configuration file.");
        return FALSE;
    } elsif ((! -e $file) || (! -r $file) || (-d $file)) {
        ERROR(sprintf "The datasources configuration file does not exist, is not readable, or is a directory : %s", $file);
        return FALSE;
    }

    my $cfg = COMMON::Config->new({
        '-filepath' => $file,
        '-format' => 'INI',
        });

    return FALSE if (!$self->_checkDatasources($cfg));

    foreach my $section ($cfg->getSections()) {
        my @orders = sort {$a <=> $b} ($cfg->getSubSections($section));
        for (my $lv = $cfg->getProperty({section => $section, property => 'lv_top'}); $lv <= $cfg->getProperty({section => $section, property => 'lv_bottom'}); $lv++) {
            $self->{datasources}->{$lv} = [];

            for (my $order = 0; $order < scalar (@orders); $order++) {
                my $source = {
                    level => $lv,
                    order => $order,
                };

                foreach my $param ($cfg->getProperties($section, $orders[$order])) {
                    $source->{$param} = $cfg->getProperty({section => $section, subsection => $orders[$order], property => $param});
                }

                if ($cfg->isProperty({section => $section, subsection => $orders[$order], property => 'file'})) {
                    push $self->{datasources}->{$lv}, WMTSALAD::PyrSource->new($source);
                } elsif ($cfg->isProperty({section => $section, subsection => $orders[$order], property => 'wms_url'})) {
                    push $self->{datasources}->{$lv}, WMTSALAD::WmsSource->new($source);
                }

            }
        }
    }


    return TRUE;
}


####################################################################################################
#                                        Group: Tests                                              #
####################################################################################################

sub isImageFormat {
    my $self = shift;
    my $string = shift;

    foreach my $imgFormat (@{$IMAGE_SPECS{image_format}}) {
        return TRUE if ($imgFormat eq $string);
    }

    return FALSE;
}

sub isMaskFormat {
    my $self = shift;
    my $string = shift;

    foreach my $validString (@{$IMAGE_SPECS{mask_format}}) {
        return TRUE if ($validString eq $string);
    }

    return FALSE;
}

sub isMetadataType {
    my $self = shift;
    my $string = shift;

    foreach my $validString (@{$IMAGE_SPECS{metadata_type}}) {
        return TRUE if ($validString eq $string);
    }

    return FALSE;
}

sub isInterpolation {
    my $self = shift;
    my $string = shift;

    foreach my $validString (@{$IMAGE_SPECS{interpolation}}) {
        return TRUE if ($validString eq $string);
    }

    return FALSE;
}

sub isPhotometric {
    my $self = shift;
    my $string = shift;

    foreach my $validString (@{$IMAGE_SPECS{photometric}}) {
        return TRUE if ($validString eq $string);
    }

    return FALSE;
}

sub isSampleFormat {
    my $self = shift;
    my $string = shift;

    foreach my $validString (@{$IMAGE_SPECS{sampleformat}}) {
        return TRUE if ($validString eq $string);
    }

    return FALSE;
}


sub _checkProperties {
    my $self = shift;
    my $propCfg = shift;

    if (!$propCfg->isSection('pyramid')) {
        ERROR(sprintf "Section '[ pyramid ]' is absent from properties configuration file.");
        return FALSE;
    }

    my $tms_path = $propCfg->getProperty({section => 'pyramid', property => 'tms_path'});
    if (!defined $tms_path) {
        ERROR("Undefined tile matrix system path.");
        return FALSE;
    }

    my $tms_name = $propCfg->getProperty({section => 'pyramid', property => 'tms_name'});
    if (!defined $tms_name) {
        ERROR("Undefined tile matrix system name.");
        return FALSE;
    }

    my $sampleformat = $propCfg->getProperty({section => 'pyramid', property => 'sampleformat'});
    if (!defined $sampleformat) {
        ERROR(sprintf "Undefined sampleformat");
        return FALSE;
    } elsif (!$self->isSampleFormat($sampleformat)) {
        ERROR(sprintf "Invalid sampleformat : '%s'. Valid formats are : %s", $sampleformat, Dumper($IMAGE_SPECS{sampleformat}));
        return FALSE;
    }

    my $bitspersample = $propCfg->getProperty({section => 'pyramid', property => 'bitspersample'});
    if (!defined $bitspersample) {
        ERROR(sprintf "Undefined bitspersample");
        return FALSE;
    }

    my $samplesperpixel = $propCfg->getProperty({section => 'pyramid', property => 'samplesperpixel'});
    if (!defined $samplesperpixel) {
        ERROR(sprintf "Undefined samples per pixel value.");
        return FALSE;
    } elsif (!COMMON::CheckUtils::isStrictPositiveInt($samplesperpixel)) {
        ERROR(sprintf "Samples per pixel value must be a strictly positive integer : %s.", $samplesperpixel);
        return FALSE;
    }

    my $pyr_name = $propCfg->getProperty({section => 'pyramid', property => 'pyr_name'});
    if (!defined $pyr_name) {
        ERROR ("The parameter 'pyr_name' is required!");
        return FALSE;
    }

    my $persistent = $propCfg->getProperty({section => 'pyramid', property => 'persistent'});
    if ((defined $persistent) && (! ($persistent =~ m/\A([01tf]|true|false)\z/i)))  {
        ERROR(sprintf "Invalid 'persistent' parameter value : '%s'. Must be a boolean value (format : number, case insensitive letter, case insensitive word).", $persistent);
        return FALSE;
    }

    my $dir_depth = $propCfg->getProperty({section => 'pyramid', property => 'dir_depth'});
    if (!defined $dir_depth) {
        ERROR("Undefined directory path depth.");
        return FALSE;
    } elsif (! COMMON::CheckUtils::isStrictPositiveInt($dir_depth)) {
        ERROR(sprintf "Directory path depth value must be a strictly positive integer : %s.", $dir_depth);
        return FALSE;
    }

    my $pyr_data_path = $propCfg->getProperty({section => 'pyramid', property => 'pyr_data_path'});
    if (! defined $pyr_data_path) {
        ERROR ("The parameter 'pyr_data_path' is required!");
        return FALSE;
    }

    my $dir_image = $propCfg->getProperty({section => 'pyramid', property => 'dir_image'});
    if (! defined $dir_image) {
        ERROR ("The parameter 'dir_image' is required!");
        return FALSE;
    }

    my $dir_nodata = $propCfg->getProperty({section => 'pyramid', property => 'dir_nodata'});
    if (! defined $dir_nodata) {
        ERROR ("The parameter 'dir_nodata' is required!");
        return FALSE;
    }

    my $pyr_desc_path = $propCfg->getProperty({section => 'pyramid', property => 'pyr_desc_path'});
    if (! defined $pyr_desc_path) {
        ERROR ("The parameter 'pyr_desc_path' is required!");
        return FALSE;
    }

    my $photometric = $propCfg->getProperty({section => 'pyramid', property => 'photometric'});
    if ((defined $photometric) && (! $self->isPhotometric($photometric))) {
        ERROR(sprintf "invalid photometric value : '%s'. Allowed values are : %s",$photometric,Dumper($IMAGE_SPECS{photometric}));
        return FALSE;
    }

    my $interpolation = $propCfg->getProperty({section => 'pyramid', property => 'interpolation'});
    if ((defined $interpolation) && (! $self->isInterpolation($interpolation))) {
        ERROR(sprintf "Invalid interpolation value : '%s'. Allowed values are : %s",$interpolation,Dumper($IMAGE_SPECS{interpolation}));
        return FALSE;
    }

    my $image_width = $propCfg->getProperty({section => 'pyramid', property => 'image_width'});
    if (!defined $image_width) {
        ERROR("Undefined widthwise number of tiles ('image_width').");
        return FALSE;
    } elsif (! COMMON::CheckUtils::isStrictPositiveInt($image_width)) {
        ERROR(sprintf "Widthwise number of tiles ('image_width') must be a strictly positive integer : %s.", $image_width);
        return FALSE;
    }

    my $image_height = $propCfg->getProperty({section => 'pyramid', property => 'image_height'});
    if (!defined $image_height) {
        ERROR("Undefined heightwise number of tiles ('image_height').");
        return FALSE;
    } elsif (! COMMON::CheckUtils::isStrictPositiveInt($image_height)) {
        ERROR(sprintf "Heightwise number of tiles ('image_height') must be a strictly positive integer : %s.", $image_height);
        return FALSE;
    }

    return TRUE;
}

sub _checkDatasources {
    my $self = shift;
    my $srcCfg = shift;

    my %ranges;

    foreach my $section ($srcCfg->getSections()) {
        if ((!defined $srcCfg->getProperty({section => $section, property => 'lv_bottom'})) || (!defined $srcCfg->getProperty({section => $section, property => 'lv_top'}))) {
            ERROR(sprintf "Levels range ('lv_bottom', 'lv_top') properties missing in section '%s'", $section);
            return FALSE;
        } elsif ((!COMMON::CheckUtils::isPositiveInt($srcCfg->getProperty({section => $section, property => 'lv_bottom'}))) || (!COMMON::CheckUtils::isPositiveInt($srcCfg->getProperty({section => $section, property => 'lv_top'})))) {
            ERROR(sprintf "'lv_top' (%s) and 'lv_bottom' (%s) must be positive integers (section '%s').", $srcCfg->getProperty({section => $section, property => 'lv_top'}), $srcCfg->getProperty({section => $section, property => 'lv_bottom'}), $section);
            return FALSE;
        }
        $ranges{$srcCfg->getProperty({section => $section, property => 'lv_bottom'})} = $srcCfg->getProperty({section => $section, property => 'lv_top'});

        foreach my $subsection ($srcCfg->getSubSections($section)) {
            if (!COMMON::CheckUtils::isPositiveInt($subsection)) {
                ERROR(sprintf "In section '%s', subsection name '%s' is not a positive integer. It cannot qualify to define the source order/priority.", $section, $subsection);
                return FALSE;
            }
            if($srcCfg->isProperty({section => $section, subsection => $subsection, property => 'transparent'})
             && (! ($srcCfg->getProperty({section => $section, subsection => $subsection, property => 'transparent'}) =~ m/\A([01TF]|TRUE|FALSE)\z/i))) {
                ERROR(sprintf "Invalid value for 'transparent' property : '%s' (must be boolean)", $srcCfg->getProperty({section => $section, subsection => $subsection, property => 'transparent'}));
                return FALSE;
            }
            if($srcCfg->isProperty({section => $section, subsection => $subsection, property => 'file'})
             && (! -f $srcCfg->getProperty({section => $section, subsection => $subsection, property => 'file'}) )) {
                ERROR(sprintf "Source pyramid descriptor does not exist : '%s'", $srcCfg->getProperty({section => $section, subsection => $subsection, property => 'file'}));
                return FALSE;
            }
        }
    }
    my @bottomLevels = sort (keys %ranges);
    for (my $i = 0; $i < ((scalar @bottomLevels)-1); $i++) {
        if ($ranges{$bottomLevels[$i]} <= $bottomLevels[$i+1]) {
            ERROR("Invalid datasources configuration : overlap of level ranges.");
            return FALSE;
        }
    }

    return TRUE;
}



####################################################################################################
#                                        Group: Output                                             #
####################################################################################################

sub dumpPyrHash {
    my $self = shift;
    
    my $pyr_dump = "\n  pyr_name => ".$self->{pyr_name};
    $pyr_dump .= "\n  pyr_desc_path => ".$self->{pyr_desc_path};
    $pyr_dump .= "\n  pyr_data_path => ".$self->{pyr_data_path};
    $pyr_dump .= "\n  dir_depth => ".$self->{dir_depth};
    $pyr_dump .= "\n  dir_image => ".$self->{dir_image};
    $pyr_dump .= "\n  dir_nodata => ".$self->{dir_nodata};
    if (exists $self->{dir_mask} && defined $self->{dir_mask}) {$pyr_dump .= "\n  dir_mask => ".$self->{dir_mask};}
    if (exists $self->{dir_metadata} && defined $self->{dir_metadata}) {$pyr_dump .= "\n  dir_metadata => ".$self->{dir_metadata};}
    $pyr_dump .= "\n  persistent => ".$self->{persistent};
    $pyr_dump .= "\n  image_width => ".$self->{image_width};
    $pyr_dump .= "\n  image_height => ".$self->{image_height};
    $pyr_dump .= "\n  format => ".$self->{format};
    $pyr_dump .= "\n  channels => ".$self->{channels};
    if (exists $self->{photometric} && defined $self->{photometric}) {$pyr_dump .= "\n  photometric => ".$self->{photometric};}
    $pyr_dump .= "\n  noDataValue => ".$self->{noData}->getValue();
    if (exists $self->{interpolation} && defined $self->{interpolation}) {$pyr_dump .= "\n  interpolation => ".$self->{interpolation};}
    $pyr_dump .= "\n  tileMatrixSet->{PATHFILENAME} => ".$self->{tileMatrixSet}->{PATHFILENAME};
    my $ds_dump = Dumper($self->{datasources});
    $ds_dump =~ s/^\$VAR1 = //;
    $pyr_dump .= "\n  datasources => ".$ds_dump;

    return $pyr_dump;
}

1;
