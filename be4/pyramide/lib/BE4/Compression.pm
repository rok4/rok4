package BE4::Compression;

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
my %COMPRESS;

################################################################################
# Preloaded methods go here.
BEGIN {}
INIT {
  # TODO :
  #  put in a configuration file !
  
  # FIXME
  #  i'm prefer to note the code for the type raw :
  #    raw      => "TIFF_RAW_INT8",
  #    floatraw => "TIFF_RAW_FLOAT32"
  
  %COMPRESS = (
    # type => code !
    raw      => "TIFF_INT8",    # or none !
    jpg      => "TIFF_JPG_INT8",
    png      => "TIFF_PNG_INT8",
    floatraw => "TIFF_FLOAT32",
  );
  
}
END {}

################################################################################
# Group: variable
#

#
# variable: $self
#
#    * type
#    * code
#

################################################################################
# Group: constructor
#
sub new {
  my $this = shift;
  my $type = shift;

  my $class= ref($this) || $this;
  my $self = {
    type => undef, # ie raw
    code => undef, # ie TIFF_INT8
  };

  bless($self, $class);
  
  TRACE;
  
  # exception :
  $type = 'raw' if (!defined ($type) || $type eq 'none');
  
  return undef if (! $self->isTypeExist($type));
  
  $self->{type} = $type;
  $self->{code} = $COMPRESS{$type};
  
  return $self;
}

################################################################################
# Group: public methods
#
sub isTypeExist {
  my $self = shift;
  my $type = shift;
  
  TRACE;
  
  foreach (keys %COMPRESS) {
    return TRUE if $type eq $_;
  }
  ERROR(sprintf "the type of compression '%s' doesn't exist ?", $type);
  return FALSE;
}
################################################################################
# Group: static method
#
sub listCompressionType {
  my $clazz= shift;
  return undef if (!$clazz->isa(__PACKAGE__));
  
  foreach (keys %COMPRESS) {
    print "$_\n";
  }
}
sub listCompressionCode {
  my $clazz= shift;
  return undef if (!$clazz->isa(__PACKAGE__));
  
  foreach (values %COMPRESS) {
    print "$_\n";
  }
}
sub listCompression {
  my $clazz= shift;
  return undef if (!$clazz->isa(__PACKAGE__));
  
  while (my ($k,$v) = each (%COMPRESS)) {
    print "$k => $v\n";
  }
}
sub getCompressionType {
  my $clazz= shift;
  return undef if (!$clazz->isa(__PACKAGE__));
  
  my $code = shift;
  
  foreach (keys %COMPRESS) {
    return $_ if ($COMPRESS{$_} eq $code);
  }
  ERROR(sprintf "No find type for the code '%s' !", $code);
  return undef;
}
sub getCompressionCode {
  my $clazz= shift;
  return undef if (!$clazz->isa(__PACKAGE__));
  
  my $type = shift;
  
  $type = 'raw' if ($type eq 'none');
  
  foreach (keys %COMPRESS) {
    return $COMPRESS{$_} if ($_ eq $type);
  }
  ERROR(sprintf "No find code for the type '%s' !", $type);
  return undef;
}
sub decodeCompression {
  my $clazz= shift;
  return undef if (!$clazz->isa(__PACKAGE__));
  
  my $code = shift;
  
  foreach (keys %COMPRESS) {
    if ($code eq $COMPRESS{$_}) {
      my @value = split(/_/, $code);
      if (scalar @value == 2) {
        $value[1] =~ m/(\w+)(\d+)/;
        my $u = lc $1;
        my $p = lc $2;
        $u = 'uint' if ($u eq 'int');
        return (lc $value[0], 'raw', $u, $p);
      }
      elsif (scalar @value == 3) {
        $value[2] =~ m/(\w+)(\d+)/;
        my $u = lc $1;
        my $p = lc $2;
        $u = 'uint' if ($u eq 'int');
        return (lc $value[0], lc $value[1], $u, $p);
      }
      else {
        return undef;
      }
    }
  }
  ERROR(sprintf "Can not decode the code '%s' !", $code);
  return undef;
  # ie 'tiff', 'raw', 'uint' , '8'
  # ie 'tiff', 'png', 'uint' , '8'
  # ie 'tiff', 'raw', 'float', '32'
  # ie 'tiff', 'jpg', 'uint' , '8'
  
}
################################################################################
# Group: get
#
sub getType {
  my $self = shift;
  return $self->{type};
}

sub getCode {
  my $self = shift;
  return $self->{code};
}

1;
__END__

# Below is stub documentation for your module. You'd better edit it!

=head1 NAME

 BE4::Compression - mapping between code and compression

=head1 SYNOPSIS

  use BE4::Compression;
  
  my $objC = BE4::Compression->new("raw");
  
  my $code = $objC->getCode(); # "TIFF_INT8" !
  my $type = $objC->getType(); # "raw" !

  # mode static 
  my @info = BE4::Compression->decodeCompression("TIFF_INT8");  #  ie 'tiff', 'raw', 'uint' , '8' !
  my $code = BE4::Compression->getCompressionCode("none");      # TIFF_INT8 !
  my $type = BE4::Compression->getCompressionType("TIFF_INT8"); # raw !

=head1 DESCRIPTION

  [ type / code ]
 
  "raw"      = "TIFF_INT8"     # or none !
  "jpg"      = "TIFF_JPG_INT8"
  "png"      = "TIFF_PNG_INT8"
  "floatraw" = "TIFF_FLOAT32"
 
  'type' is use for the program 'tiff2tile'.
  'code' is written in the pyramid file, and it is useful for 'Rok4' !

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
