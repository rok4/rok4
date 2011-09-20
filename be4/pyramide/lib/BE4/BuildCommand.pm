package BE4::BuildCommand;

use strict;
use warnings;

use Log::Log4perl qw(:easy);

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
# global (inject code shell)
my $STATUS = sprintf "if [ $? != 0 ] ; then echo %s : Error in $(( %d - 1)) >&2 ;
                      exit 1; fi\n", __PACKAGE__, __LINE__;

################################################################################
# Preloaded methods go here.
BEGIN {}
INIT {}
END {}

################################################################################
# static function

sub tiffcp {
  my $clazz= shift;
  return undef if (!$clazz->isa(__PACKAGE__));
  
  my $command = undef;
  
  ERROR ("usage: tiffcp [options] input... output\n
    where options are :\n
    \t -a		append to output instead of overwriting\n
    \t -o offset	set initial directory offset\n
    \t -p contig	pack samples contiguously (e.g. RGBRGB...)\n
    \t -p separate	store samples separately (e.g. RRR...GGG...BBB...)\n
    \t -s		write output in strips\n
    \t -t		write output in tiles\n
    \t -i		ignore read errors\n
    \t -b file[,#]	bias (dark) monochrome image to be subtracted from all others\n
    \t -,=%		use % rather than , to separate image #'s (per Note below)\n
    \t -r #		make each strip have no more than # rows\n
    \t -w #		set output tile width (pixels)\n
    \t -l #		set output tile length (pixels)\n
    \t -f lsb2msb	force lsb-to-msb FillOrder for output\n
    \t -f msb2lsb	force msb-to-lsb FillOrder for output\n
    \t -c lzw[:opts]	compress output with Lempel-Ziv & Welch encoding\n
    \t -c zip[:opts]	compress output with deflate encoding\n
    \t -c jpeg[:opts]	compress output with JPEG encoding\n
    \t -c jbig	compress output with ISO JBIG encoding\n
    \t -c packbits	compress output with packbits encoding\n
    \t -c g3[:opts]	compress output with CCITT Group 3 encoding\n
    \t -c g4		compress output with CCITT Group 4 encoding\n
    \t -c none	use no compression algorithm on output\n
");
  
  return $command;
}
sub mergeNtiff {
  my $clazz= shift;
  return undef if (!$clazz->isa(__PACKAGE__));
  
  my $command = undef;
  
  ERROR ("Usage : mergeNtiff -f (list of datasource) -a (sampleformat) -i (interpolation) -n (color NoData) -t (type) -s (sampleperpixel) -b (bitspersample) -p (photometric)");
  
  return $command;
}
sub merge4tiff {
  my $clazz= shift;
  return undef if (!$clazz->isa(__PACKAGE__));
  
  my $command = undef;
  
  ERROR ("Usage : merge4tiff [options] image1 image2 image3 image4 imageOut\n
  options:\n
  \t -g gamma (float)\n
  \t -n nodata alti (float)\n
  \t -c compression (none|zip|packbits|jpeg|lzw) default is none\n
  \t -r rowsperstrip (int)");
  
  return $command;
}
sub tiff2tile {
  my $clazz= shift;
  return undef if (!$clazz->isa(__PACKAGE__));
  
  my $command = undef;
  
  ERROR ("usage : tiff2tile input_file -c [none/png/jpeg] -p [gray/rgb] -t [sizex] [sizey] -b [8/32] -a [uint/float] output_file");
  
  return $command;
}
sub tiff2gray {
  my $clazz= shift;
  return undef if (!$clazz->isa(__PACKAGE__));
  
  my $command = undef;
  
  ERROR ("???");
  
  return $command;
}

1;
__END__

# Below is stub documentation for your module. You'd better edit it!

=head1 NAME

=head1 SYNOPSIS

=head1 DESCRIPTION

=head2 EXPORT

None by default.

=head1 SEE ALSO

=head1 AUTHOR

Bazonnais Jean Philippe, E<lt>jpbazonnais@E<gt>

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2011 by Bazonnais Jean Philippe

This library is free software; you can redistribute it and/or modify
it under the same terms as Perl itself, either Perl version 5.10.1 or,
at your option, any later version of Perl 5 you may have available.

=cut
