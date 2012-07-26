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

package BE4::ImageSource;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
use List::Util qw(min max);

use BE4::GeoImage;

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

################################################################################
=begin nd
Group: variable

variable: $self
    * PATHIMG => undef, # path to images
    * PATHMTD => undef, # path to metadata, not implemented
    * images  => [], # list of images sources
    * bestResX => undef,
    * bestResY => undef,
    * pixel => undef, # Pixel object
=cut

####################################################################################################
#                                       CONSTRUCTOR METHODS                                        #
####################################################################################################

# Group: constructor

sub new {
  my $this = shift;

  my $class= ref($this) || $this;
  # IMPORTANT : if modification, think to update natural documentation (just above) and pod documentation (bottom)
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
  return undef if (! $self->_init(@_));

  return undef if (! $self->computeImageSource());
  
  return $self;
}

sub _init {

    my $self   = shift;
    my $imagesParams = shift;

    TRACE;
    
    return FALSE if (! defined $imagesParams);
    
    # init. params    
    $self->{PATHIMG} = $imagesParams->{path_image} if (exists($imagesParams->{path_image})); 
    $self->{PATHMTD} = $imagesParams->{path_metadata} if (exists($imagesParams->{path_metadata}));
    
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
#                                        IMAGES TREATMENTS                                         #
####################################################################################################

# Group: images treatments

#
=begin nd
    method: computeImageSource
    Load all images in a list of object BE4::GeoImage, determine the components of data and check them.
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
            if (! ($pixel->{bitspersample} eq $imageInfo[0] && $pixel->{photometric} eq $imageInfo[1] &&
                    $pixel->{sampleformat} eq $imageInfo[2] && $pixel->{samplesperpixel} eq $imageInfo[3])) {
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
    }

    $self->{pixel} = $pixel;
    $self->{bestResX} = $bestResX;
    $self->{bestResY} = $bestResY;

    if (!defined $lstGeoImages || ! scalar @$lstGeoImages) {
        ERROR ("Can not found image source in '$self->{PATHIMG}' !");
        return FALSE;
    }

    return TRUE;
}

#
=begin nd
method: getListImages

Get the list of all path data image (image tiff only !). Recursive to browse directories.
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
    
    if ( -d File::Spec->catdir($directory, $entry)) {
      TRACE(sprintf "DIR:%s\n",$entry);      
      # recursif
      $newsearch = $self->getListImages(File::Spec->catdir($directory, $entry));
      push @{$search->{images}}, $_  foreach(@{$newsearch->{images}});
    }

    next if ($entry!~/.*\.(tif|TIF|tiff|TIFF)$/);
    
    push @{$search->{images}}, File::Spec->catfile($directory, $entry);
  }
  
  return $search;
}

#
=begin nd
method: computeBBox

Calculate extrem limits of images, in the source SRS.
=cut
sub computeBBox {
    my $self = shift;

    TRACE;

    my $lstImagesSources = $self->{images};

    my @bbox;

    my $xmin = $lstImagesSources->[0]->{xmin};
    my $xmax = $lstImagesSources->[0]->{xmax};
    my $ymin = $lstImagesSources->[0]->{ymin};
    my $ymax = $lstImagesSources->[0]->{ymax};

    foreach my $objImage (@$lstImagesSources) {
        $xmin = min($xmin, $objImage->{xmin});
        $xmax = max($xmax, $objImage->{xmax});
        $ymin = min($ymin, $objImage->{ymin});
        $ymax = max($ymax, $objImage->{ymax});
    }

    push @bbox, ($xmin,$ymin,$xmax,$ymax);

    return @bbox;
}

####################################################################################################
#                                       GETTERS / SETTERS                                          #
####################################################################################################

# Group: getters - setters
 
sub hasImages {
  my $self = shift;
  
  return FALSE if (! defined ($self->{PATHIMG}));
  return TRUE;
}

sub getResolution {
  my $self = shift;
  return $self->{resolution};  
}

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
#                                        EXPORT METHOD                                             #
####################################################################################################

# Group: export method

#
=begin nd
method: exportImageSource

Export all informations of image in a file.

Parameter:
    file - filepath of the export
    
Return:
    A string : filename, xmin, ymax, xmax, ymin, xres, yres
=cut
sub exportImageSource {
  my $self = shift;
  my $file = shift; # pathfilename !
  
  TRACE;

  my $lstGeoImages = $self->{images};
  
  if (! open (FILE, ">", $file)) {
    ERROR ("Can not create file ('$file') !");
    return FALSE;
  }
  
  foreach my $objGeoImage (@$lstGeoImages) {
    # image xmin ymax xmax ymin resx resy
    printf FILE "%s\t %s\t %s\t %s\t %s\t %s\t %s\n",
            # FIXME : File::Spec->catfile($objImage->{filepath}, $objImage->{filename}) ?
            $objGeoImage->{filename},
            $objGeoImage->{xmin},
            $objGeoImage->{ymax},
            $objGeoImage->{xmax},
            $objGeoImage->{ymin},
            $objGeoImage->{xres},
            $objGeoImage->{yres};
  }
  
  close FILE;
  
  return TRUE;
}

1;
__END__

=head1 NAME

BE4::ImageSource - information about a source made up of georeferenced images

=head1 SYNOPSIS

    use BE4::ImageSource;
  
    # ImageSource object creation
    my $objImageSource = BE4::XXX->new({
        path_image => "/home/ign/DATA",
        path_metadata=> "/home/ign/METADATA",
    });

=head1 DESCRIPTION

=head2 ATTRIBUTES

=over 4

=item PATHIMG

Directory which contains images

=item PATHMTD

Directory which contains metadata (not yet implemented)

=item images

Array of GeoImage objects

=item bestResX, bestResY

=item pixel

Pixel object

=back

=head1 SEE ALSO

=head2 POD documentation

=begin html

<ul>
<li><A HREF="./lib-BE4-GeoImage.html">BE4::GeoImage</A></li>
<li><A HREF="./lib-BE4-Pixel.html">BE4::Pixel</A></li>
</ul>

=end html

=head2 NaturalDocs

=begin html

<A HREF="../Natural/Html/index.html">Index</A>

=end html

=head1 AUTHOR

Satabin Théo, E<lt>theo.satabin@ign.frE<gt>

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2011 by Satabin Théo

This library is free software; you can redistribute it and/or modify it under the same terms as Perl itself, either Perl version 5.10.1 or, at your option, any later version of Perl 5 you may have available.

=cut
