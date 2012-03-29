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

package BE4::DataSource;

# use strict;
use warnings;

use Log::Log4perl qw(:easy);

use List::Util qw(min max);

# My module
use BE4::ImageSource;

require Exporter;
use AutoLoader qw(AUTOLOAD);

our @ISA = qw(Exporter);

our %EXPORT_TAGS = ( 'all' => [ qw() ] );
our @EXPORT_OK   = ( @{$EXPORT_TAGS{'all'}} );
our @EXPORT      = qw();

################################################################################
# version
our $VERSION = '0.0.1';

################################################################################
# constantes
use constant TRUE  => 1;
use constant FALSE => 0;

################################################################################
# Preloaded methods go here.
BEGIN {}
INIT {}
END {}

# Group: constructor
#

################################################################################
# constructor
sub new {
  my $this = shift;

  my $class= ref($this) || $this;
  my $self = {
    PATHIMG => undef, # path to images
    PATHMTD => undef, # path to metadata
    SRS     => undef, # 
    #
    images  => [],    # list of images sources
    #
    resolution => undef,
    #
    pixel => undef, #Pixel object
  };

  bless($self, $class);
  
  TRACE;
  
  # init. class
  return undef if (! $self->_init(@_));
  
  return $self;
}

################################################################################
# privates init.
sub _init {
    my $self   = shift;
    my $params = shift;

    TRACE;
    
    return FALSE if (! defined $params);
    
    # init. params    
    $self->{PATHIMG}=$params->{path_image}    if (exists($params->{path_image})); 
    $self->{PATHMTD}=$params->{path_metadata} if (exists($params->{path_metadata}));
    $self->{SRS}=uc($params->{srs})               if (exists($params->{srs}));
    
    if (defined ($self->{PATHIMG}) && ! -d $self->{PATHIMG}) {
        ERROR ("Directory image doesn't exist !");
        return FALSE;
    }
    
    if (defined ($self->{PATHMTD}) && ! -d $self->{PATHMTD}) {
        ERROR ("Directory metadata doesn't exist !");
        return FALSE;
    }
    
    if (! defined ($self->{SRS})) {
        ERROR ("SRS undefined !");
        return FALSE;
    }
    
    return TRUE;
}

#
# Group: public method
#

################################################################################
# method: computeImageSource
#   Load all image in a list of object BE4::ImageSource, determine the medium
#   resolution of data, check components.

sub computeImageSource {
    my $self = shift;

    TRACE;

    my %resDict;

    my $lstImagesSources = $self->{images}; # it's a ref !

    my $badRefCtrl = 0;

    my $search = $self->getListImages($self->{PATHIMG});
    if (! defined $search) {
        ERROR ("Can not load data source !");
        return FALSE;
    }

    my @listSourcePath = @{$search->{images}};
    if (! @listSourcePath) {
        ERROR ("Can not load data source !");
        return FALSE;
    }

    my $pixel = undef;

    foreach my $filepath (@listSourcePath) {

        my $objImageSource = BE4::ImageSource->new($filepath);

        if (! defined $objImageSource) {
            ERROR ("Can not load image source ('$filepath') !");
            return FALSE;
        }

        # images reading and analysis
        my @imageInfo = $objImageSource->computeInfo();
        #  @imageInfo = [ bitspersample , photometric , sampleformat , samplesperpixel ]
        if (! @imageInfo) {
            ERROR ("Can not read image info ('$filepath') !");
            return FALSE;
        }

        if (! defined $pixel) {
            # we have read the first image, components are empty. This first image will be the reference.
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
            # we have already values. We must have the same components for all images
            if (! ($pixel->{bitspersample} eq $imageInfo[0] && $pixel->{photometric} eq $imageInfo[1] &&
                    $pixel->{sampleformat} eq $imageInfo[2] && $pixel->{samplesperpixel} eq $imageInfo[3])) {
                ERROR ("All images must have same components. This image ('$filepath') is different !");
                return FALSE;
            }
        }

        if ($objImageSource->getXmin() == 0  && $objImageSource->getYmax == 0){
            $badRefCtrl++;
        }
        if ($badRefCtrl>1){
            ERROR ("More than one image are at 0,0 position. Probably lost of georef file (tfw,...)");
            return FALSE;
        }

        # FIXME :
        #  - resolution resx == resy ?
        #  - unique resolution for all image !
        my $xRes = $objImageSource->getXres();
        $resDict{$xRes} = 1;
        $self->{resolution} = $xRes;
        #
        push @$lstImagesSources, $objImageSource;
    }

    $self->{pixel} = $pixel;

    if (!defined $lstImagesSources || ! scalar @$lstImagesSources) {
        ERROR ("Can not found image source in '$self->{PATHIMG}' !");
        return FALSE;
    }

    # NV 2012-01-02 : Je ne pense pas que des resolution multiples posent problème à mergeNtiff.
    #                 Je commente donc le bloc suivant qui ne permet pas de traiter les veilles bd-parcellaire.
    # if (keys (%resDict) != 1) {
    #   ERROR ("The resolution of image source is not unique !");
    #   return FALSE;
    # }

    return TRUE;
}
################################################################################
# method: exportImageSource
#   Export all informations of image in a file
#
#   Format :
#     filename, xmin, ymax, xmax, ymin, xres, yres
#
#   Parameter:
#    file - filepath of the export
#
sub exportImageSource {
  my $self = shift;
  my $file = shift; # pathfilename !
  
  TRACE;

  my $lstImagesSources = $self->{images};
  
  if (! open (FILE, ">", $file)) {
    ERROR ("Can not create file ('$file') !");
    return FALSE;
  }
  
  foreach my $objImage (@$lstImagesSources) {
    # image xmin ymax xmax ymin resx resy
    printf FILE "%s\t %s\t %s\t %s\t %s\t %s\t %s\n",
            # FIXME : File::Spec->catfile($objImage->{filepath}, $objImage->{filename}) ?
            $objImage->{filename},
            $objImage->{xmin},
            $objImage->{ymax},
            $objImage->{xmax},
            $objImage->{ymin},
            $objImage->{xres},
            $objImage->{yres};
  }
  
  close FILE;
  
  return TRUE;
}
################################################################################
# method: computeBbox
#   Bbox of data source
#
sub computeBbox {
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

  # FIXME : format bbox (Upper Left, Lower Right) ?
  push @bbox, ($xmin,$ymax,$xmax,$ymin);
  
  return @bbox;
}


################################################################################
# method: getListImages
#   Get the list of all path data image (image tiff only !)
#   
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
    
    ALWAYS(sprintf "Nouvelle image : %s",File::Spec->catfile($directory, $entry)); #TEST#
    push @{$search->{images}}, File::Spec->catfile($directory, $entry);
  }
  
  return $search;
}

################################################################################
# method: hasImages
#   
sub hasImages {
  my $self = shift;
  
  return FALSE if (! defined ($self->{PATHIMG}));
  return TRUE;
}

#
# Group: get/set
#
sub getResolution {
  my $self = shift;
  return $self->{resolution};  
}
################################################################################
# method: getImages
#   Get the list of all object data image (BE4::ImageSource)
#
sub getImages {
  my $self = shift;
  # copy !
  my @images;
  foreach (@{$self->{images}}) {
    push @images, $_;
  }
  return @images; 
}
sub getSRS {
  my $self = shift;
  
  return $self->{SRS};
}
1;
__END__

=pod

=head1 NAME

  BE4::DataSource - Managing data sources

=head1 SYNOPSIS

  use BE4::DataSource;
  
  my $objImplData = BE4::DataSource->new(path_image => $path,
                                         path_metadata => $path,
                                         srs => 'IGNF:LAMB93');
  
  $objImplData->computeImageSource();
  $objImplData->exportImageSource($fileout);
  
  my @images = $objImplData->getListImages(); # path images tif !
  my @bbox   = $objImplData->computeBBox();   # (xmin,ymax,xmax,ymin) !
  my $srs    = $objImplData->getSRS();        # IGNF:LAMB93 !
  my $res    = $objImplData->getResolution(); # 0.50 cm !

=head1 DESCRIPTION

  This class allows to manage the data source :
   - list of image
   - get SRS
   - get Resolution
   - list of object BE4::ImageSource (get image info)
   - get bbox of data source
   - export data source in a file
   - ...

  The SRS must be in the proj4 format.
  
=head2 EXPORT

None by default.

=head1 LIMITATION & BUGS

* Does not support data source multiple !

* Select of data image only in the format tiff !

* Does not implement the managing of metadata !

=head1 SEE ALSO

  eg BE4::ImageSource

=head1 AUTHOR

Bazonnais Jean Philippe, E<lt>jpbazonnais@E<gt>

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2011 by Bazonnais Jean Philippe

This library is free software; you can redistribute it and/or modify
it under the same terms as Perl itself, either Perl version 5.10.1 or,
at your option, any later version of Perl 5 you may have available.

=cut
