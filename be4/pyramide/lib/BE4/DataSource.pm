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
#
# Group: variable
#

#
# variable: $self
#
#    * PATHIMG => undef, # path to images
#    * PATHMTD => undef, # path to metadata
#    * SRS     => undef, # ie proj4 !
#    * images  => [],    # list of object images sources (BE4::ImageSource)

#
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
    SRS     => undef, # ie proj4 !
    #
    images  => [],    # list of images sources
    #
    resolution => undef,
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
    $self->{SRS}=$params->{srs}               if (exists($params->{srs}));
    
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
#   Load all image in a list of object BE4::ImageSource, and determmine the medium
#   resolution of data.

sub computeImageSource {
  my $self = shift;
    
  TRACE;
  
  my %Res;
  
  my $lstImagesSources = $self->{images}; # it's a ref !
  
  foreach my $filepath ($self->getListImages()) {
    
    my $objImageSource = BE4::ImageSource->new($filepath);
    
    if (! defined $objImageSource) {
      ERROR ("Can not load image source ('$filepath') !");
      return FALSE;
    }
    
    if (! $objImageSource->computeInfo()) {
      ERROR ("Can not read image info ('$filepath') !");
      return FALSE;
    }
    # FIXME :
    #  - resolution resx == resy ?
    #  - unique resolution for all image !
    my $key = $objImageSource->getXres();
    $Res{$key} = 1;
    $self->{resolution} = $key;
    #
    push @$lstImagesSources, $objImageSource;
  }
  
  if (!defined $lstImagesSources || ! scalar @$lstImagesSources) {
    ERROR ("Can not found image source in '$self->{PATHIMG}' !");
    return FALSE;
  }
  
  if (keys (%Res) != 1) {
    ERROR ("The resolution of image source is not unique !");
    return FALSE;
  }
  
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
  my $self = shift;
  
  TRACE;
  
  my $lstImagesSources = ();
  
  my $pathdir = $self->{PATHIMG};
  
  if (! opendir DIR, $pathdir) {
    ERROR ("Can not open directory source ('$pathdir') !");
    return undef;
  }
  
  foreach my $entry (readdir DIR) {
    next if ($entry=~m/^\.{1,2}$/);
    next if (! -f File::Spec->catdir($pathdir,$entry));
    
    # FIXME : type of data product (tif by default !)
    # but implemented too in Class ImageSource !
    next if ($entry!~/.*\.(tif|TIF|tiff|TIFF)$/);
    
    push @$lstImagesSources, File::Spec->catdir($pathdir,$entry);
  }
  
  closedir(DIR);
  
  return @$lstImagesSources;
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
