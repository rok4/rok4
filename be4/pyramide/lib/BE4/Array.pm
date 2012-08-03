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

# version
my $VERSION = "0.0.1";

# method: minArrayIndex
#  Renvoie l'indice de l'élément le plus petit du tableau
#-------------------------------------------------------------------------------
sub minArrayIndex {
    my @array = @_;
    my $min = undef;
    my $minIndex = undef;

    for (my $i = 0; $i < scalar @array; $i++){
        if (! defined $minIndex || $min > $array[$i]) {
            $min = $array[$i];
            $minIndex = $i;
        }
    }

    return $minIndex;
}
# method: maxArrayIndex
#  Renvoie l'indice de l'élément le plus grand du tableau
#-------------------------------------------------------------------------------
sub maxArrayIndex {
    my @array = @_;
    my $max = undef;
    my $maxIndex = undef;

    for (my $i = 0; $i < scalar @array; $i++){
        if (! defined $maxIndex || $max < $array[$i]) {
            $max = $array[$i];
            $maxIndex = $i;
        }
    }

    return $maxIndex;
}

# method: maxArrayValue
#  Renvoie la valeur maximale du tableau
#-------------------------------------------------------------------------------
sub maxArrayValue {
  my @array = @_;
  my $max = undef;
  
  for (my $i = 0; $i < scalar @array; $i++){
    if (! defined $max || $max < $array[$i]) {
      $max = $array[$i];
    }
  }
  return $max;
}

# method: minArrayValue
#  Renvoie la valeur minimale du tableau
#-------------------------------------------------------------------------------
sub minArrayValue {
  my @array = @_;
  my $min = undef;

  for (my $i = 0; $i < scalar @array; $i++){
    if (! defined $min || $min > $array[$i]) {
      $min = $array[$i];
    }
  }

  return $min;
}

# method: sumArray
#  Renvoie la somme des élément du tableau tableau
#-------------------------------------------------------------------------------
sub sumArray {
    my $self = shift;
    my @array = @_;

    TRACE;

    my $sum = 0;

    for (my $i = 0; $i < scalar @array; $i++){
        $sum += $array[$i];
    }

    return $sum;
}

# method: statArray
#  fonction de test renvoyant des statistiques sur le tableau donné : moyenne
#  et écart type.
#-------------------------------------------------------------------------------
sub statArray {
    my $self = shift;
    my @array = @_;

    TRACE;

    my $moyenne = 0;

    for (my $i = 0; $i < scalar @array; $i++){
        $moyenne += $array[$i];
    }

    $moyenne /= scalar @array;
    my $variance = 0;

    for (my $i = 0; $i < scalar @array; $i++){
        $variance += ($array[$i]-$moyenne) * ($array[$i]-$moyenne);
    }
    $variance /= scalar @array;
    my $ecarttype = sqrt($variance);
    #print "Moyenne : $moyenne, écart type : $ecarttype\n";
    return ($moyenne,$ecarttype);
}


1;
__END__