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

package BE4::Array;

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

################################################################################

####################################################################################################
#                                      Index Returning Functions                                   #
####################################################################################################

# Group: Index Returning Functions

#
=begin nd
method: minArrayIndex

Parameters:
    first - integer, indice from which minimum is looked for.
    array - numbers (floats or integers) array

Returns:
    Index of the smaller element in a array, begining with the element 'first'
=cut
sub minArrayIndex {
    my $self = shift;
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
#                                      Value Returning Functions                                   #
####################################################################################################

# Group: Value Returning Functions

#
=begin nd
method: maxArrayValue

Parameters:
    first - integer, indice from which maximum is looked for.
    array - numbers (floats or integers) array

Returns:
    The greater value in a array, begining with the element 'first'
=cut
sub maxArrayValue {
    my $self = shift;
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

1;
__END__

=head1 NAME

BE4::Array - Array Manipulation tools

=head1 SYNOPSIS

    use BE4::Array;
    my @array = (1,3,5,9);
    my $minIndex = BE4::Arrray->minArrayIndex(@array); # return 0
    my $maxArrayValue = BE4::Array->maxArrayValue($array); # return 9
    
=head1 DESCRIPTION

Provide functios to compute the max value or the min index of an array

=head1 SEE ALSO

=head2 NaturalDocs

=begin html

<A HREF="../Natural/Html/index.html">Index</A>

=end html

=head1 AUTHORS

Chevereau Simon, E<lt>simon.chevereau@ign.frE<gt>
Satabin Théo, E<lt>theo.satabin@ign.frE<gt>

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2011 by Satabin Théo

This library is free software; you can redistribute it and/or modify it under the same terms as Perl itself, either Perl version 5.10.1 or, at your option, any later version of Perl 5 you may have available.

=cut