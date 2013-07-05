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
File: ImageSource.pm

Class: BE4::ImageSource

Define a data source, with georeferenced image directory.

Using:
    (start code)
    use BE4::ImageSource;

    # ImageSource object creation
    my $objImageSource = BE4::XXX->new({
        path_image => "/home/ign/DATA",
        path_metadata=> "/home/ign/METADATA",
    });
    (end code)

Attributes:
    PATHIMG - string - Path to images directory.
    PATHMTD - string - Path to metadata directory. NOT IMPLEMENTED.
    images - <GeoImage> array - Georeferenced images' ensemble, found in PATHIMG and subdirectories
    srs - string - SRS of the georeferenced images
    bestResX - double - Best X resolution among all images.
    bestResY - double - Best Y resolution among all images.
    pixel - <Pixel> - Pixel components of all images, have to be same for each one.

Limitations:
    Constraint on the input format of images :
        TIFF (extensions .tif, .TIF, .tiff and .TIFF)
        PNG (extensions .png, .PNG)
=cut

################################################################################

package BE4::ImageSource;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
use List::Util qw(min max);

use BE4::GeoImage;
use BE4::Pixel;

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

ImageSource constructor. Bless an instance.

Parameters (hash):
    path_image - string - Path to images' directory, to analyze.
    srs - string - SRS of the georeferenced images

See also:
    <_init>, <computeImageSource>
=cut
sub new {
    my $this = shift;
    my $params = shift;

    my $class= ref($this) || $this;
    # IMPORTANT : if modification, think to update natural documentation (just above)
    my $self = {
        PATHIMG => undef,
        PATHMTD => undef,
        #
        images  => [],
        #
        bestResX => undef,
        bestResY => undef,
        #
        pixel => undef,
    };

    bless($self, $class);

    TRACE;

    # init. class
    return undef if (! $self->_init($params));

    return undef if (! $self->computeImageSource());

    return $self;
}

=begin nd
Function: _init

Checks and stores informations.

Parameters (hash):
    path_image - string - Path to images' directory, to analyze.
    srs - string - SRS of the georeferenced images
=cut
sub _init {

    my $self   = shift;
    my $params = shift;

    TRACE;
    
    return FALSE if (! defined $params);
    if (! exists($params->{srs}) || ! defined ($params->{srs})) {
        ERROR ("We have to provide a SRS to create an ImageSource object");
        return FALSE;
    }
    
    # init. params    
    $self->{PATHIMG} = $params->{path_image} if (exists($params->{path_image})); 
    $self->{PATHMTD} = $params->{path_metadata} if (exists($params->{path_metadata}));
    $self->{srs} = $params->{srs};
    
    if (! defined ($self->{PATHIMG}) || ! -d $self->{PATHIMG}) {
        ERROR (sprintf "Directory image ('%s') doesn't exist !",$self->{PATHIMG});
        return FALSE;
    }
    
    if (defined ($self->{PATHMTD}) && ! -d $self->{PATHMTD}) {
        ERROR ("Directory metadata doesn't exist !");
        return FALSE;
    }

    return TRUE;

}

####################################################################################################
#                                 Group: Images treatments                                         #
####################################################################################################

=begin nd
Function: computeImageSource

Detects all TIFF files in *PATHIMG* and subdirectories and creates a corresponding <GeoImage> object. Determines data's components and check them.

See also:
    <getListImages>, <GeoImage::computeInfo>
=cut
sub computeImageSource {
    my $self = shift;

    TRACE;

    my $lstGeoImages = $self->{images};

    my $search = $self->getListImages($self->{PATHIMG});
    if (! defined $search) {
        ERROR ("Can not load data source !");
        return FALSE;
    }

    my @listGeoImagePath = @{$search->{images}};
    if (! @listGeoImagePath) {
        ERROR ("Can not load data source !");
        return FALSE;
    }

    my $badRefCtrl = 0;
    my $pixel = undef;
    my $bestResX = undef;
    my $bestResY = undef;

    foreach my $filepath (@listGeoImagePath) {

        my $objGeoImage = BE4::GeoImage->new($filepath);

        if (! defined $objGeoImage) {
            ERROR ("Can not load image source ('$filepath') !");
            return FALSE;
        }

        # images reading and analysis
        my @imageInfo = $objGeoImage->computeInfo();
        #  @imageInfo = [ bitspersample , photometric , sampleformat , samplesperpixel ]
        if (! @imageInfo) {
            ERROR ("Can not read image info ('$filepath') !");
            return FALSE;
        }

        if (! defined $pixel) {
            # we have read the first image, components are empty. This first image will be the reference.
            if ($imageInfo[0] == 1) {
                WARN ("Bitspersample value is 1 ! This data have not to be used for generations (only to calculate data limits)");
                # Pixel class wouldn't accept bitspersample = 1. we change artificially value for 8
                $imageInfo[0] = 8;
            }
            $pixel = BE4::Pixel->new({
                bitspersample => $imageInfo[0],
                photometric => $imageInfo[1],
                sampleformat => $imageInfo[2],
                samplesperpixel => $imageInfo[3]
            });
            if (! defined $pixel) {
                ERROR ("Can not create Pixel object for DataSource !");
                return FALSE;
            }
        } else {
            if ($imageInfo[0] == 1) {
                # bitspersample in the Pixel object is 1. we change artificially current value for 8
                $imageInfo[0] = 8;
            }
            # we have already values. We must have the same components for all images
            if (! ($pixel->getBitsPerSample eq $imageInfo[0] && $pixel->getPhotometric eq $imageInfo[1] &&
                    $pixel->getSampleFormat eq $imageInfo[2] && $pixel->getSamplesPerPixel eq $imageInfo[3])) {
                ERROR ("All images must have same components. This image ('$filepath') is different !");
                return FALSE;
            }
        }

        if ($objGeoImage->getXmin() == 0  && $objGeoImage->getYmax == 0){
            $badRefCtrl++;
        }
        if ($badRefCtrl>1){
            ERROR ("More than one image are at 0,0 position. Probably lost of georef file (tfw,...)");
            return FALSE;
        }

        my $xRes = $objGeoImage->getXres();
        my $yRes = $objGeoImage->getYres();

        $bestResX = $xRes if (! defined $bestResX || $xRes < $bestResX);
        $bestResY = $yRes if (! defined $bestResY || $yRes < $bestResY);

        push @$lstGeoImages, $objGeoImage;
        $objGeoImage->setImageSource($self);
    }

    $self->{pixel} = $pixel;
    $self->{bestResX} = $bestResX;
    $self->{bestResY} = $bestResY;

    if (!defined $lstGeoImages || ! scalar @$lstGeoImages) {
        ERROR (sprintf "Can not found image source in '%s' !",$self->{PATHIMG});
        return FALSE;
    }

    return TRUE;
}

=begin nd
Function: getListImages

Recursive method to browse a directory and list all TIFF file. Returns an hash containing the TIFF file path's array.
| {
|     images => [...],
| };

Parameters (list):
    directory - string - Path to directory, to browse.
=cut  
sub getListImages {
    my $self      = shift;
    my $directory = shift;

    TRACE();

    my $search = {
        images => [],
    };

    if (! opendir (DIR, $directory)) {
        ERROR("Can not open directory cache (%s) ?",$directory);
        return undef;
    }

    my $newsearch;

    foreach my $entry (readdir DIR) {
        next if ($entry =~ m/^\.{1,2}$/);

        # Si on a à faire à un dossier, on appelle récursivement la méthode pourle parcourir
        if ( -d File::Spec->catdir($directory, $entry)) {
            $newsearch = $self->getListImages(File::Spec->catdir($directory, $entry));
            push @{$search->{images}}, $_  foreach(@{$newsearch->{images}});
        }

        # Si le fichier n'a pas l'extension TIFF ou PNG, on ne le traite pas
        next if ( $entry !~ /.*\.(tif|TIF|tiff|TIFF)$/ && $entry !~ /.*\.(png|PNG)$/);

        # On a à faire à un fichier avec l'extension TIFF, on l'ajoute au tableau
        push @{$search->{images}}, File::Spec->catfile($directory, $entry);
    }

    return $search;
}

=begin nd
Function: computeBBox

Calculate extrem limits of images, in the source SRS.

Returns a double list : (xMin,yMin,xMax,yMax).
=cut
sub computeBBox {
    my $self = shift;

    TRACE;

    my $lstGeoImages = $self->{images};

    my ($xmin,$ymin,$xmax,$ymax) = $lstGeoImages->[0]->getBBox;

    foreach my $objImage (@$lstGeoImages) {
        $xmin = min($xmin, $objImage->getXmin);
        $xmax = max($xmax, $objImage->getXmax);
        $ymin = min($ymin, $objImage->getYmin);
        $ymax = max($ymax, $objImage->getYmax);
    }

    return ($xmin,$ymin,$xmax,$ymax);
}

####################################################################################################
#                                Group: Getters - Setters                                          #
####################################################################################################

# Function: getResolution
sub getResolution {
    my $self = shift;
    return $self->{resolution};  
}

# Function: getSRS
sub getSRS {
    my $self = shift;
    return $self->{srs};
}

# Function: getImages
sub getImages {
    my $self = shift;
    # copy !
    my @images;
    foreach (@{$self->{images}}) {
        push @images, $_;
    }
    return @images;
}

####################################################################################################
#                                Group: Export methods                                             #
####################################################################################################

=begin nd
Function: exportForDebug

Returns all image source's informations. Useful for debug.

Example:
    (start code)
    (end code)
=cut
sub exportForDebug {
    my $self = shift ;
    
    my $export = "";
    
    $export .= "\nObject BE4::ImageSource :\n";
    $export .= sprintf "\t Image directory : %s\n", $self->{PATHIMG};
    $export .= sprintf "\t Image number : %s\n", scalar @{$self->{images}};

    $export .= "\t Best resolution : \n";
    $export .= sprintf "\t\t- x : %s\n", $self->{bestResX};
    $export .= sprintf "\t\t- y : %s\n", $self->{bestResY};
    
    $export .= sprintf "\t Pixel : %s\n", $self->{pixel}->exportForDebug;
    
    return $export;
}

1;
__END__
