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

package BE4::Format;

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
# Global
my %COMPRESSION;
my %SAMPLEFORMAT;

################################################################################
# Preloaded methods go here.
BEGIN {}
INIT {

  
    %COMPRESSION = (
        raw      => "RAW",
#       floatraw => "RAW",  use raw instead
        jpg      => "JPG",
        png      => "PNG",
        lzw      => "LZW"
    );

    %SAMPLEFORMAT = (
        uint      => "INT",
        float     => "FLOAT"
    );
  
}
END {}

################################################################################
# Group: variable
#

#
# variable: $self
#
#    * compression
#    * sampleformat
#    * bitspersample
#    * code
#

################################################################################
# Group: constructor
#
sub new {
    my $this = shift;
    my $compression = shift;
    my $sampleformat = shift;
    my $bitspersample = shift;

    my $class= ref($this) || $this;
    my $self = {
        compression => undef, # ie raw
        sampleformat => undef, # ie uint
        bitspersample => undef, # ie 8
        code => undef, # ie TIFF_RAW_INT8
    };

    bless($self, $class);

    TRACE;
    
#   to remove when compression type 'floatraw' will be remove
    if ($compression eq 'floatraw') {
        WARN("'floatraw' is a deprecated compression type, use 'raw' instead");
        $compression = 'raw';
    }
  
    # exception :
    $compression = 'raw' if (!defined ($compression) || $compression eq 'none');
    $sampleformat = 'uint' if (!defined ($sampleformat));
    $bitspersample = 8 if (!defined ($bitspersample));

    return undef if (! $self->isCompressionExist($compression));
    return undef if (! $self->isSampleFormat($sampleformat));

    $self->{compression} = $compression;
    $self->{sampleformat} = $sampleformat;
    $self->{bitspersample} = $bitspersample;

    # codes handled by rok4 are :
    #     - TIFF_INT8 (deprecated, use TIFF_RAW_INT8 instead)
    #     - TIFF_RAW_INT8
    #     - TIFF_JPG_INT8
    #     - TIFF_LZW_INT8
    #     - TIFF_PNG_INT8

    #     - TIFF_FLOAT32 (deprecated, use TIFF_RAW_FLOAT32 instead)
    #     - TIFF_RAW_FLOAT32
    #     - TIFF_LZW_FLOAT32

    # code : TIFF_[COMPRESSION]_[SAMPLEFORMAT][BITSPERSAMPLE]
    $self->{code} = 'TIFF_'.$COMPRESSION{$compression}.'_'.$SAMPLEFORMAT{$sampleformat}.$bitspersample;

    return $self;
}

################################################################################
# Group: public methods
#
sub isCompressionExist {
    my $self = shift;
    my $compression = shift;

    TRACE;
    
    #   to remove when compression type 'floatraw' will be remove
    if ($compression eq 'floatraw') {
        WARN("'floatraw' is a deprecated compression type, use 'raw' instead");
        $compression = 'raw';
    }

    foreach (keys %COMPRESSION) {
    return TRUE if $compression eq $_;
    }
    ERROR(sprintf "the type of compression '%s' doesn't exist ?", $compression);
    return FALSE;
}

sub isSampleFormat {
    my $self = shift;
    my $sampleformat = shift;

    TRACE;

    foreach (keys %SAMPLEFORMAT) {
    return TRUE if $sampleformat eq $_;
    }
    ERROR(sprintf "the sample format '%s' doesn't exist ?", $sampleformat);
    return FALSE;
}
################################################################################
# Group: static method
#

sub listCompressionType {
    my $class = shift;
    return undef if (!$class->isa(__PACKAGE__));

    foreach (keys %COMPRESSION) {
        print "$_\n";
    }
}

sub listCompressionCode {
    my $class = shift;
    return undef if (!$class->isa(__PACKAGE__));

    foreach (values %COMPRESSION) {
        print "$_\n";
    }
}

sub listCompression {
    my $class = shift;
    return undef if (!$class->isa(__PACKAGE__));

    while (my ($k,$v) = each (%COMPRESSION)) {
        print "$k => $v\n";
    }
}

sub getCompressionType {
    my $class = shift;
    return undef if (!$class->isa(__PACKAGE__));

    my $code = shift;

    foreach (keys %COMPRESSION) {
        return $_ if ($COMPRESSION{$_} eq $code);
    }
    ERROR(sprintf "No find type for the code '%s' !", $code);
    return undef;
}

sub getCompressionCode {
    my $class = shift;
    return undef if (!$class->isa(__PACKAGE__));

    my $type = shift;

    $type = 'raw' if ($type eq 'none');
    
    if ($type eq 'floatraw') {
        WARN("'floatraw' is a deprecated compression type, use 'raw' instead");
        $type = 'raw';
    }

    foreach (keys %COMPRESSION) {
        return $COMPRESSION{$_} if ($_ eq $type);
    }
    ERROR(sprintf "No find code for the type '%s' !", $type);
    return undef;
}

sub decodeFormat {
    my $class = shift;
    return undef if (!$class->isa(__PACKAGE__));
    
    my $format = shift;
    
#   to remove when format 'TIFF_INT8' and 'TIFF_FLOAT32' will be remove
    if ($format eq 'TIFF_INT8') {
        WARN("'TIFF_INT8' is a deprecated format, use 'TIFF_RAW_INT8' instead");
        $format = 'TIFF_RAW_INT8';
    }
    if ($format eq 'TIFF_FLOAT32') {
        WARN("'TIFF_FLOAT32' is a deprecated format, use 'TIFF_RAW_FLOAT32' instead");
        $format = 'TIFF_RAW_FLOAT32';
    }
    
    my @value = split(/_/, $format);
    if (scalar @value != 3) {
        ERROR(sprintf "Compression code is not valid '%s' !", $format);
        return undef;
    }
  
    $value[2] =~ m/(\w+)(\d+)/;
    
    my $compression = '';
    my $sampleformat = '';
    my $bitspersample = $2;
    
    foreach (keys %SAMPLEFORMAT) {
        if ($1 eq $SAMPLEFORMAT{$_}) {
            $sampleformat = $_;
        }
    }
    if ($sampleformat eq '') {
        ERROR(sprintf "Can not decode the sample's format '%s' !", $1);
        return undef;
    }
    
    foreach (keys %COMPRESSION) {
        if ($value[1] eq $COMPRESSION{$_}) {
            $compression = $_;
        }
    }
    if ($compression eq '') {
        ERROR(sprintf "Can not decode the compression '%s' !", $value[1]);
        return undef;
    }
    
    return (lc $value[0], $compression, $sampleformat, $bitspersample);
  
    # ie 'tiff', 'raw', 'uint' , '8'
    # ie 'tiff', 'png', 'uint' , '8'
    # ie 'tiff', 'jpg', 'uint' , '8'
    # ie 'tiff', 'lzw', 'uint' , '8'
    # ie 'tiff', 'raw', 'float', '32'    
    # ie 'tiff', 'lzw', 'float', '32'
    
}
################################################################################
# Group: get
#
sub getCompression {
    my $self = shift;
    return $self->{compression};
}

sub getSampleFormat {
    my $self = shift;
    return $self->{sampleformat};
}

sub getBitsperSample {
    my $self = shift;
    return $self->{bitspersample};
}

sub getCode {
    my $self = shift;
    return $self->{code};
}

1;
__END__

# Below is stub documentation for your module. You'd better edit it!

=head1 NAME

 BE4::Format - mapping between compression/sampleformat/bitspersample and code

=head1 SYNOPSIS

  use BE4::Format;
  
  my $objC = BE4::Format->new("raw","uint",8);
  
  my $code = $objC->getCode(); # "TIFF_RAW_INT8" !
  my $type = $objC->getCompression(); # "raw" !
  my $type = $objC->getSampleFormat(); # "uint" !
  my $type = $objC->getBitsperSample(); # "8" !

  # mode static 
  my @info = BE4::Format->decodeFormat("TIFF_RAW_INT8");  #  ie 'tiff', 'raw', 'uint' , '8' !
  my $compressionCode = BE4::Format->getCompressionCode("none");      # RAW !
  my $compressionType = BE4::Format->getCompressionType("RAW"); # raw !

=head1 DESCRIPTION

  Format {
      compression       => raw/jpg/png/lzw (floatraw is deprecated, use raw instead)
      sampleformat      => uint/float
      bitspersample     => 8/32
      code              => TIFF_RAW_INT8 for example
  }
 
  'compression' is use for the program 'tiff2tile'.
  'code' is written in the pyramid file, and it is useful for 'Rok4' !

=head2 EXPORT

None by default.

=head1 SEE ALSO

=head1 AUTHOR

Satabin Théo, E<lt>tsatabin@E<gt>

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2011 by Satabin Théo

This library is free software; you can redistribute it and/or modify
it under the same terms as Perl itself, either Perl version 5.10.1 or,
at your option, any later version of Perl 5 you may have available.

=cut
