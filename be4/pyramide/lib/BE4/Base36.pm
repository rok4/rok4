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
File: Base36.pm

Class: BE4::Base36

Base 36 converting tools. Do not instanciate.

Using:
    (start code)
    use BE4::Base36;
    my ($i) = BE4::Base36::b36PathToIndices("E21") ; # returns 18217
    my ($i,$j) = BE4::Base36::b36PathToIndices("3E/42/01") ; # returns [4032,18217]
    my $b36 = BE4::Base36::encodeB10toB36(32674) ; # returns "P7M"
    my $b36Path = BE4::Base36::indicesToB36Path(4032, 18217, 2) ; # returns "3E/42/01"
    (end code)
=cut

################################################################################

package BE4::Base36;

use strict;
use warnings;

use Data::Dumper;
use List::Util qw(min max);

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
use constant B36STRING => "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

####################################################################################################
#                               Group: B36 returning functions                                     #
####################################################################################################

=begin nd
Function: encodeB10toB36

Convert a base-10 number into base-36 (string).

Parameters (list):
    number - integer - The base-10 integer to convert.
    
Examples:
    - BE4::Base36::encodeB10toB36(32674) returns "P7M".
    - BE4::Base36::encodeB10toB36(156) returns "4C".
    - BE4::Base36::encodeB10toB36(0) returns "0".
=cut
sub encodeB10toB36 {
    my $number = shift; # in base 10 !
    
    return "0" if ($number == 0);
    
    my $b36 = ""; # in base 36 !
    while ( $number ) {
        my $v = $number % 36;
        $b36 .= substr(B36STRING, $v, 1);
        $number = int $number / 36;
    }

    return reverse($b36);
}

=begin nd
Function: indicesToB36Path

Convert base-10 indices into a base-36 path (string). If the base-36 indices are (ABCD,1234), the base-36 path (length 3) is "A1B2/C3/D4".

Parameters (list):
    i,j - integers - The base-10 indices to convert into a path.
    pathlength - integer - Number of subdirectories + the file.
    
Examples:
    BE4::Base36::indicesToB36Path(4032, 18217, 3) returns "3E/42/01".
    
See also:
    <encodeB10toB36>
=cut
sub indicesToB36Path {
    my $i = shift ;
    my $j = shift ;
    my $pathlength = shift ;
    
    my $xb36 = BE4::Base36::encodeB10toB36($i);
    my $yb36 = BE4::Base36::encodeB10toB36($j);
    
    my $maxLength = max($pathlength, length($xb36), length($yb36));
    
    if (length ($yb36) < $maxLength) {
        $yb36 = "0"x($maxLength - length ($yb36)).$yb36;
    }
    
    if (length ($xb36) < $maxLength) {
        $xb36 = "0"x($maxLength - length ($xb36)).$xb36;
    }
    
    my $B36Path = "";
    my $nbSlash = $pathlength - 1;
    
    # coordinates are interlaced and (pathlength - 1) "/" are added : X1X2..Xn and Y1Y2..Yn -> X1Y1X2Y2/../XnYn
    while(length ($xb36) > 0) {
        $B36Path = chop($yb36).$B36Path;
        $B36Path = chop($xb36).$B36Path;
        
        if ($nbSlash > 0) {        
            $B36Path = '/'.$B36Path;
            $nbSlash--;
        }
    }
        
    return $B36Path ;
}

####################################################################################################
#                               Group: B10 returning functions                                     #
####################################################################################################

=begin nd
Function: encodeB36toB10

Convert a base-36 number into base-10 (int).

Parameters (list):
    b36 - string - The base-36 integer to convert.
    
Example:
    BE4::Base36::encodeB10toB36("F4S6") returns 706038.
=cut
sub encodeB36toB10 {
    my $b36  = shift; # idx in base 36 !
    
    $b36 = reverse(uc($b36));
    
    my $number = 0;
    
    for (my $i = 0; $i < length($b36); $i++) {
        my $c = index(B36STRING, substr($b36, $i, 1));
        $number += $c * (36 ** $i);
    }
    
    return $number;
}

=begin nd
Function: b36PathToIndices

Converts a base-36 path into base-10 indices (x,y). If the base-36 path is "A1/B2/C3", the base-36 indices are (ABC,123).

Parameters (list):
    path - string - The base-36 path to convert into 2 base-10 indices

Returns:
    An integer list, (col,row).
    
Example:
    BE4::Base36::b36PathToIndices("3E/42/01") returns (4032,18217).
        
See also:
    <encodeB36toB10>
=cut
sub b36PathToIndices {
    my $path = shift;
    
    my $xB36 = "";
    my $yB36 = "";
    
    # We remove slashes
    $path =~ s/\///g;
    
    # Coordinates are interlaced, we separate them : X1Y1X2Y2..XnYn -> X1X2..Xn and Y1Y2..Yn
    while (length($path) > 0) {
        $yB36 = chop($path).$yB36;
        $xB36 = chop($path).$xB36;
    }
    
    my $x = BE4::Base36::encodeB36toB10($xB36);
    my $y = BE4::Base36::encodeB36toB10($yB36);
    
    return ($x, $y);
}


1;
__END__
