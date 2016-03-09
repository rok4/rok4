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
use BE4::TileMatrixSet;
use BE4::TileMatrix;

use parent qw(Exporter);

our %EXPORT_TAGS = ( 'all' => [ qw() ] );
our @EXPORT_OK   = ( @{$EXPORT_TAGS{'all'}} );
our @EXPORT      = qw();

################################################################################
# Constantes
use constant TRUE  => 1;
use constant FALSE => 0;

my %IMAGE_SPECS = (
        format => [
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
        noDataValue => undef,
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

    # Tile Matrix Set
    my $TMS = BE4::TileMatrixSet->new(File::Spec->catfile($refFileContent->{pyramid}->{tms_path},$refFileContent->{pyramid}->{tms_name}));
    $self->{tileMatrixSet} = $TMS ;

    # Image format
    my $format = "TIFF_";
    if ($refFileContent->{pyramid}->{compression} =~ m/^jpg|png|lzw|zip|pkb$/i) {
        $format.= uc($refFileContent->{pyramid}->{compression})."_";
    } else {
        $format .= "RAW_";
    };
    $format .= uc($refFileContent->{pyramid}->{sampleformat}).$refFileContent->{pyramid}->{bitspersample} ;
    if ($self->isImageFormat($format)) {
        $self->{format} = $format;
    } else {
        ERROR(sprintf "Unrecognized image format : %s.", $format);
        return FALSE;
    }

    # Channels number 
    if (COMMON::CheckUtils::isStrictPositiveInt($refFileContent->{pyramid}->{samplesperpixel})) {
        $self->{channels} = $refFileContent->{pyramid}->{samplesperpixel};
    } else {
        ERROR(sprintf "Samples per pixel value must be a strictly positive integer : %s.", $refFileContent->{pyramid}->{samplesperpixel});
        return FALSE;
    }

    # Pyramid's name. Some people say it's useful to name the resulting .pyr file.
    if ((!exists $refFileContent->{pyramid}->{pyr_name}) || (!defined $refFileContent->{pyramid}->{pyr_name})) {
        ERROR ("The parameter 'pyr_name' is required!");
        return FALSE;
    }
    my $pyr_name = $refFileContent->{pyramid}->{pyr_name};
    $pyr_name =~ s/\.(pyr|PYR)$//;
    $self->{pyr_name} = $pyr_name;

    # Persistence
    if ((exists $refFileContent->{pyramid}->{persistent}) && (defined $refFileContent->{pyramid}->{persistent})) {
        if ((uc $refFileContent->{pyramid}->{persistent} eq "TRUE") || (uc $refFileContent->{pyramid}->{persistent} eq "T") || ($refFileContent->{pyramid}->{persistent} == TRUE)) {
            $self->{persistent} = TRUE;
        } elsif ((uc $refFileContent->{pyramid}->{persistent} eq "FALSE") || (uc $refFileContent->{pyramid}->{persistent} eq "F") || ($refFileContent->{pyramid}->{persistent} == FALSE)) {
            $self->{persistent} = FALSE;
        } else {
            ERROR("The 'persitent' parameter must be a boolean value (format : number, case insensitive letter, case insensitive word).");
            return FALSE;
        }
    } else {
        INFO ("The undefined 'persistent' parameter is now set to 'false'.");
        $self->{persistent} = FALSE;
    }


    # Path depth
    if (COMMON::CheckUtils::isStrictPositiveInt($refFileContent->{pyramid}->{dir_depth})) {
        $self->{dir_depth} = $refFileContent->{pyramid}->{dir_depth};
    } else {
        ERROR(sprintf "Directory path depth value must be a strictly positive integer : %s.", $refFileContent->{pyramid}->{dir_depth});
        return FALSE;
    }

    # Data paths
    if ((exists $refFileContent->{pyramid}->{pyr_data_path}) && (defined $refFileContent->{pyramid}->{pyr_data_path})) {
        $self->{pyr_data_path} = $refFileContent->{pyramid}->{pyr_data_path};
    } else {
        ERROR ("The parameter 'pyr_data_path' is required!");
        return FALSE;
    }

    # Masks directory (optionnal)
    if ((exists $refFileContent->{pyramid}->{dir_mask}) && (defined $refFileContent->{pyramid}->{dir_mask})) {
        $self->{dir_mask} = $refFileContent->{pyramid}->{dir_mask};
    }

    # Images directory
    if ((exists $refFileContent->{pyramid}->{dir_image}) && (defined $refFileContent->{pyramid}->{dir_image})) {
        $self->{dir_image} = $refFileContent->{pyramid}->{dir_image};
    } else {
        ERROR ("The parameter 'dir_image' is required!");
        return FALSE;
    }

    # Metadata directory

    # Nodata Directory

    # Descriptor's path

    # Nodata value

    # Interpolation

    # Photometric

    # Image dimensions




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

    return TRUE;
}


####################################################################################################
#                                        Group: Tests                                              #
####################################################################################################

sub isImageFormat {
    my $self = shift;
    my $string = shift;

    foreach my $imgFormat (@{$IMAGE_SPECS{format}}) {
        return TRUE if ($imgFormat eq $string);
    }

    return FALSE;
}


sub _checkProperties {

}

sub _checkDatasources {

}



####################################################################################################
#                                        Group: Output                                             #
####################################################################################################



1;
