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
File: Node.pm

Class: JOINCACHE::Node

Descibe a node

Using:
    (start code)
    use JOINCACHE::Node

    my $node = BE4::Node->new(51, 756, "12", 2);
    (end code)

Attributes:
    i - integer - Column
    j - integer - Row
    level - string - Level's identifiant
    pyramidName - string - Relative path of this node in the pyramid (generated from i,j). Example : "00/12/L5.tif"
    code - string - Commands to execute to generate this node (to write in a script)
    script - <Script> - Script in which the node will be generated
    sources - hash array - Source images from which this node is generated. One image source :
|               img - string - Absolute path to the image
|               msk - string - Absolute path to the associated mask (optionnal)
|               sourcePyramid - <JOINCACHE::SourcePyramid> - Pyramid which image belong to
=cut

################################################################################

package JOINCACHE::Node;

use strict;
use warnings;

use Log::Log4perl qw(:easy);

use File::Spec ;
use Data::Dumper ;
use COMMON::Base36 ;

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

BEGIN {}
INIT {}
END {}

####################################################################################################
#                                        Group: Constructors                                       #
####################################################################################################

=begin nd
Constructor: new

Node constructor. Bless an instance.

Parameters (list):
    i - integer - Node's column
    j - integer - Node's row
    level - string - Node's level ID
    dirDepth - integer - Depth, to determine the base-36 path

See also:
    <_init>
=cut
sub new {
    my $this = shift;
    my @params = @_;
    
    my $class= ref($this) || $this;
    # IMPORTANT : if modification, think to update natural documentation (just above)
    my $self = {
        i => undef,
        j => undef,
        level => undef,
        pyramidName => undef,
        code => '',
        script => undef,
        sources => [],
    };
    
    bless($self, $class);
    
    TRACE;
    
    # init. class
    return undef if (! $self->_init(@params));
    
    return $self;
}

=begin nd
Function: _init

Check and store node's attributes values. Initialize weights to 0. Calculate the pyramid's relative path, from indices, thanks to <Base36::indicesToB36Path>.

Parameters (list):
    i - integer - Node's column
    j - integer - Node's row
    level - string - Node's level ID
    dirDepth - integer - Depth, to determine the base-36 path
=cut
sub _init {
    my $self = shift;
    my $i = shift;
    my $j = shift;
    my $level = shift;
    my $dirDepth = shift;
    
    TRACE;
    
    # mandatory parameters !
    if (! defined $i) {
        ERROR("Node's column is undefined !");
        return FALSE;
    }
    if (! defined $j) {
        ERROR("Node's row is undefined !");
        return FALSE;
    }
    if (! defined $level) {
        ERROR("Node's level is undefined !");
        return FALSE;
    }
    if (! defined $dirDepth) {
        ERROR("dirDepth is undefined !");
        return FALSE;
    }
    
    # init. params
    $self->{i} = $i;
    $self->{j} = $j;
    $self->{level} = $level;
    $self->{code} = '';
    
    $self->{pyramidName} = COMMON::Base36::indicesToB36Path( $i, $j, $dirDepth+1).".tif";
    
    return TRUE;
}

####################################################################################################
#                                Group: Getters - Setters                                          #
####################################################################################################

# Function: getColumn
sub getColumn {
    my $self = shift;
    return $self->{i};
}

# Function: getRow
sub getRow {
    my $self = shift;
    return $self->{j};
}

# Function: getLevel
sub getLevel {
    my $self = shift;
    return $self->{level};
}

# Function: getPyramidName
sub getPyramidName {
    my $self = shift;
    return $self->{pyramidName};
}

# Function: getScript
sub getScript {
    my $self = shift;
    return $self->{script}
}

=begin nd
Function: writeInScript

Write own code in the associated script.

Parameters (list):
    additionnalText - string - Optionnal, can be undefined, text to add after the own code.
=cut
sub writeInScript {
    my $self = shift;
    my $additionnalText = shift;

    my $text = $self->{code};
    $text .= $additionnalText if (defined $additionnalText);

    $self->{script}->write($text);
}

=begin nd
Function: setScript

Parameters (list):
    script - <Script> - Script to set.
=cut
sub setScript {
    my $self = shift;
    my $script = shift;

    if (! defined $script || ref ($script) ne "JOINCACHE::Script") {
        ERROR("We expect to have a JOINCACHE::Script object.");
    }

    $self->{script} = $script;
}

=begin nd
Function: getWorkBaseName

Returns the work image base name (no extension) : "level_col_row", or "level_col_row_suffix" if defined.

Parameters (list):
    prefix - string - Optionnal, suffix to add to the work name
=cut
sub getWorkBaseName {
    my $self = shift;
    my $suffix = shift;
    
    # si un prefixe est précisé
    return (sprintf "%s_%s_%s_%s", $self->{level}, $self->{i}, $self->{j}, $suffix) if (defined $suffix);
    # si pas de prefixe
    return (sprintf "%s_%s_%s", $self->{level}, $self->{i}, $self->{j});
}

=begin nd
Function: getWorkName

Returns the work image name : "level_col_row.tif", or "level_col_row_suffix.tif" if defined.

Parameters (list):
    prefix - string - Optionnal, suffix to add to the work name
=cut
sub getWorkName {
    my $self = shift;
    my $suffix = shift;
    
    return $self->getWorkBaseName($suffix).".tif";
}

# Function: getSources
sub getSources {
    my $self = shift;
    return $self->{sources};
}

=begin nd
Function: getSource

Parameters (list):
    ind - integer - Index of the wanted source image

Returns
    A source image, as an hash :
|               img - string - Absolute path to the image
|               msk - string - Absolute path to the associated mask (optionnal)
|               sourcePyramid - <JOINCACHE::SourcePyramid> - Pyramid which image belong to
=cut
sub getSource {
    my $self = shift;
    my $ind = shift;
    return $self->{sources}->[$ind];
}

# Function: getSourcesNumber
sub getSourcesNumber {
    my $self = shift;
    return scalar @{$self->{sources}};
}

=begin nd
Function: addSource

Parameters (list):
    image - hash reference - Source images to add
|               img - string - Absolute path to the image
|               msk - string - Absolute path to the associated mask (optionnal)
|               sourcePyramid - <JOINCACHE::SourcePyramid> - Pyramid which image belong to
=cut
sub addSource {
    my $self = shift;
    my $image = shift;
    
    push @{$self->{sources}}, $image;
    
    return TRUE;
}

=begin nd
Function: setCode

Parameters (list):
    code - string - Code to set.
=cut
sub setCode {
    my $self = shift;
    my $code = shift;
    $self->{code} = $code;
}

# Function: getScriptID
sub getScriptID {
    my $self = shift;
    return $self->{script}->getID;
}

####################################################################################################
#                                Group: Export methods                                             #
####################################################################################################

=begin nd
Function: exportForOntConf

Export attributes of the Node for overlayNtiff configuration file : /path/to/image.tif[ path/to/mask.tif]. Provided paths will be written as is, so can be relative or absolute (or use environment variables).

Parameters (list):
    imagePath - string - Path to the image, have to be defined
    maskPath - string - Path to the associated mask, can be undefined
=cut
sub exportForOntConf {
    my $self = shift;
    my $imagePath = shift;
    my $maskPath = shift;

    my $output = "$imagePath";
    if (defined $maskPath) {
        $output .= " $maskPath";
    }

    return $output."\n";
}

=begin nd
Function: exportForDebug

Returns all image's components. Useful for debug.

Example:
    (start code)
    (end code)
=cut
sub exportForDebug {
    my $self = shift ;
    
    my $output = "";
    
    $output .= sprintf "Object JOINCACHE::Node :\n";
    $output .= sprintf "\tLevel : %s\n",$self->{level};
    $output .= sprintf "\tColumn : %s\n",$self->getColumn();
    $output .= sprintf "\tRow : %s\n",$self->getRow();
    if (defined $self->getScript()) {
        $output .= sprintf "\tScript ID : %s\n",$self->getScriptID();
    } else {
        $output .= sprintf "\tScript undefined.\n";
    }
    $output .= sprintf "\t %s sources\n", scalar @{$self->{sources}};
    
    return $output;
}

1;
__END__