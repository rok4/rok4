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
File: Array.pm

Class: COMMON::Array

Provides functions to compute the max value or the min index of an array. Do not instanciate.

Using:
    (start code)
    my @array = (1,3,5,9);
    my $minIndex = COMMON::Array::minArrayIndex(@array); # return 0
    my $maxArrayValue = COMMON::Array::maxArrayValue($array); # return 9
    (end code)
=cut

################################################################################

package COMMON::Array;

use strict;
use warnings;

use Data::Dumper;

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

####################################################################################################
#                               Group: Index returning functions                                   #
####################################################################################################

=begin nd
Function: minArrayIndex

Parameters (list):
    first - integer - Indice from which minimum is looked for.
    array - numbers array -

Returns:
    An integer, indice of the smaller element in a array, begining with the element 'first'
=cut
sub minArrayIndex {
    my $first = shift;
    my @array = @_;

    my $min = undef;
    my $minIndex = undef;

    for (my $i = $first; $i < scalar @array; $i++){
        if (! defined $minIndex || $min > $array[$i]) {
            $min = $array[$i];
            $minIndex = $i;
        }
    }

    return $minIndex;
}

####################################################################################################
#                               Group: Value returning functions                                   #
####################################################################################################

=begin nd
Function: maxArrayValue

Parameters:
    first - integer - Indice from which maximum is looked for.
    array - numbers (floats or integers) array

Returns:
    The greater value in a array, begining with the element 'first'
=cut
sub maxArrayValue {
    my $first = shift;
    my @array = @_;

    my $max = undef;

    for (my $i = $first; $i < scalar @array; $i++){
        if (! defined $max || $max < $array[$i]) {
            $max = $array[$i];
        }
    }

    return $max;
}


####################################################################################################
#                               Group: Boolean returning functions                                 #
####################################################################################################

=begin nd
Function: isInArray

Parameters:
    value - scalar - Value to search in the array
    array - array - Array where to search value

Returns:
    The index of the first occurence of the value, undef if not in the array
=cut
sub isInArray {
    my $value = shift;
    my @array = @_;

    for (my $i = 0; $i < scalar(@array), $i++) {
        if ($array[$i] eq $value) {
            return $i;
        }
    }

    return undef;
}

1;
__END__
