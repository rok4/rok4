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
File: PyrSource.pm

Class: WMTSALAD::PyrSource

(see WMTSALAD_PyrSource.png)

Using:
    (start code)
    use WMTSALAD::PyrSource;

    my $pyrSource = WMTSALAD::PyrSource->new( {
        file => "/path/to/source_pyramid.pyr",
        style => "normal",
        transparent => "true",
    } );

    $pyrSource->writeInXml();    
    (end code)

Attributes:
    file - string - Path to the source pyramid's descriptor file
    style - string - The style to apply to source images when streaming them (default : normal)
    transparent - boolean - Another style parameter, whose name is explicit (default : false)
    
=cut

################################################################################

package WMTSALAD::PyrSource;

use strict;
use warnings;

use Log::Log4perl qw(:easy);
use Data::Dumper;
use XML::LibXML;

require Exporter;
use AutoLoader qw(AUTOLOAD);

our @ISA = qw(Exporter);

our %EXPORT_TAGS = ( 'all' => [ qw() ] );
our @EXPORT_OK   = ( @{$EXPORT_TAGS{'all'}} );
our @EXPORT      = qw();

################################################################################
# Constants
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

<WMTSALAD::PyrSource> constructor.

Using:
    (start code)
    my $pyrSource = WMTSALAD::PyrSource->new( {
        file => "/path/to/source_pyramid.pyr",
        style => "normal",
        transparent => "true"
    } );
    (end code)

Parameters:
    params - hash reference, containing the following properties :
        {
            file - string - Path to the source pyramid's descriptor file
            style - string - The style to apply to source images when streaming them (default : normal)
            transparent - boolean - Another style parameter, whose name is explicit (default : false)
        }

Returns:
    The newly created PyrSource object. 'undef' in case of failure.
    
=cut
sub new {
    my $class = shift;
    my $params = shift;

    $class = ref($class) || $class;
    # IMPORTANT : if modification, think to update natural documentation (just above)
    my $this = {
        file => undef,
        style => undef,
        transparent => undef
    };

    bless($this, $class);

    if (! $this->_init($params)) {
        ERROR("Could not load pyramid source.");
        return undef;
    }

    return $this;
}

=begin nd

Function: _init

<WMTSALAD::PyrSource> initializer. Checks parameters passed to 'new', 
then load them in the new PyrSource object.

Using:
    (start code)
    my loadSucces = pyrSource->_init( {
        file => "/path/to/source_pyramid.pyr",
        style => "normal",
        transparent => true,
    } )
    (end code)

Parameters:
    params - hash reference, containing the following properties :
        {
            file - string - Path to the source pyramid's descriptor file
            style - string - The style to apply to source images when streaming them (default : normal)
            transparent - boolean - Another style parameter, whose name is explicit (default : false)
        }

Returns:
    1 (TRUE) in case of success, 0 (FALSE) in case of failure.
    
=cut
sub _init {
    my $this = shift;
    my $params = shift;

    if (!exists $params->{file} || !defined $params->{file}) {
        ERROR("A pyramid descriptor's file path must be passed to load a pyramid source.");
        return FALSE;
    }
    if (! -e $params->{file}) {
        ERROR("Thé pyramid descriptor's does not exists: ".$params->{file});
        return FALSE;
    }


    $this->{file} = $params->{file};
    if (exists $params->{style} && defined $params->{style} && $params->{style} ne '') {
        $this->{style} = $params->{style};
    } else {
        $this->{style} = "normal";
    }
    if (exists $params->{transparent} && defined $params->{transparent} && $params->{transparent} ne '') {
        if ($params->{transparent} =~ m/\A(1|t|true)\z/i) {
            $this->{transparent} = "true";
        } elsif ($params->{transparent} =~ m/\A(0|f|false)\z/i) {
            $this->{transparent} = "false";
        } else {
            WARN(sprintf "Unhandled value for 'transparent' attribute : %s. Ignoring it.", $params->{transparent});
        }
    } else {
        $this->{transparent} = "false";
    }

    return TRUE;
}


####################################################################################################
#                                        Group: Output                                             #
####################################################################################################

=begin nd

Function: writeInXml

Writes the 'basedPyramid' element node in the pyramid descriptor file. This function needs to know where to write (see parameters).

Using:
    (start code)
    $pyrSource->writeInXml(xmlDocument, sourcesNode);
    (end code)

Parameters:
    xmlDocument - <XML::LibXML::Document> - The xml document where the 'basedPyramid' node will be written. (i.e. the interface for the descriptor file)
    sourcesNode - <XML::LibXML::Element> - The parent node where the 'basedPyramid' element will be nested. (ex: the 'sources' element node)

Returns:
    1 (TRUE) if success. 0 (FALSE) if an error occured.

=cut
sub writeInXml {
    my $this = shift;
    my $xmlDoc = shift;
    my $sourcesNode = shift;

    my $basedPyramidEl = $xmlDoc->createElement("basedPyramid");
    $sourcesNode->appendChild($basedPyramidEl);
    $basedPyramidEl->appendTextChild("file", $this->{file});
    $basedPyramidEl->appendTextChild("style", $this->{style});
    $basedPyramidEl->appendTextChild("transparent", $this->{transparent});

    return TRUE;
}


1;
__END__
