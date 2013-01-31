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
File: ClassName.pm

Class: PACK::ClassName

What is this class for ? Just test. Adds A and B.

See this beautiful image.

(see image.png)

Using:
    (start code)
    use PACK::ClassName;

    # ClassName object creation

    # Example of constructor call
    my $objClassName = PACK::ClassName->new(5,7);
    (end code)

Attributes:
    A - integer - An integer 
    B - integer - An other integer
    C - integer - A + B
=cut

################################################################################

package PACK::ClassName;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
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

# Constant GLOBALHASH
# It used to ...
my %GLOBALHASH;

################################################################################

BEGIN {}
INIT {

    %GLOBALHASH = {
        key1 => "value1",
        key2 => "value2",
    };

}
END {}

####################################################################################################
#                                        Group: Constructors                                       #
####################################################################################################

=begin nd
Constructor: new

ClassName constructor. Bless an instance.

Parameters (list):
    A - integer - Value to store in A
    B - integer - Value to store in B

See also:
    <_init>, <_load>
=cut
sub new {
    my $this = shift;
    my $A = shift;
    my $B = shift;

    my $class= ref($this) || $this;

    my $self = {
        A => undef,
        B => undef,
        C => undef,
    };

    bless($self, $class);

    TRACE;

    # init. parameters
    return undef if (! $self->_init($A,$B));

    # load parameters
    return undef if (! $self->_load($A,$B));

    return $self;
}

=begin nd
Constructor: new

Checks parameters presence.

Parameters (list):
    A - integer - Value to store in A
    B - integer - Value to store in B

=cut
sub _init {
    my $self   = shift;
    my $A = shift;
    my $B = shift;

    TRACE;

    if (! defined $A) {
        ERROR("Parameter 'A' is not defined");
        return FALSE,
    }

    if (! defined $B) {
        ERROR("Parameter 'B' is not defined");
        return FALSE,
    }
    
    return TRUE;
}

=begin nd
Constructor: new

Store and use attributes' values.

Parameters (list):
    A - integer - Value to store in A
    B - integer - Value to store in B
=cut
sub _load {
    my $self = shift;
    my $A = shift;
    my $B = shift;

    TRACE;

    $self->{A} = int($A);
    $self->{B} = int($B);

    $self->{C} = $self->{A} + $self->{B};

    return TRUE;
  
}

####################################################################################################
#                      Group: Name of the group, present in naturalDoc                             #
####################################################################################################

=begin nd
method: printCharCtimes

Repeats the provided character C times and print it. If C is negative, we print the empty string. If provided parameter is a string, we repeat just the first cgaracter.

Parameters (hash):
    char - string - Character to repeat C times

Returns:
    TRUE if success, FALSE if failure
=cut
sub printCharCtimes { 
    my $self = shift;
    my $char = shift;

    if (! defined $char) {
        ERROR("The character to repeat is not defined");
        return FALSE;
    }

    if (length($char) > 1) {
        WARN("A string has been provided, we keep the first character");
        $char = substr($char,0,1);
    }

    my $repeat = $self->getC();

    if ($repeat < 0) {
        WARN("C is negative.");
        $repeat = 0;
    }

    print "$char"x($repeat)."\n";

    return TRUE;    
}


####################################################################################################
#                                Group: Getters - Setters                                          #
####################################################################################################

# Function: getC
sub getC { 
    my $self  = shift;
    return $self->{C};    
}

####################################################################################################
#                                Group: Export methods                                             #
####################################################################################################

=begin nd
Function: exportForDebug

Returns all ClassName's informations. Useful for debug.

Example:
    (start code)
    (end code)
=cut
sub exportForDebug {
    my $self = shift ;
    
    my $export = "";
    
    $export .= "\nObject PACK::ClassName :\n";
    $export .= sprintf "\t A : %s\n", $self->{A};
    $export .= sprintf "\t B : %s\n", $self->{B};
    $export .= sprintf "\t C : %s\n", $self->{C};
    
    return $export;
}

1;
__END__
