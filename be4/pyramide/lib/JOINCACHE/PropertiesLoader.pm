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
File: PropertiesLoader.pm

Class: JOINCACHE::PropertiesLoader

Reads a configuration file for joinCache, respect the IniFiles format, but consider order. Possible sections are limited :
    - logger
    - pyramid
    - bboxes
    - composition
    - process

Using:
    (start code)
    use JOINCACHE::PropertiesLoader;

    my $objPropLoader = JOINCACHE::PropertiesLoader->new("/home/IGN/properties.txt");
    (end code)

Attributes:
    configurationPath - string - Configuration file path.
    
    logger - hash - Can be null
    pyramid - hash - Final pyramid's parameters
    bboxes - hash - Defines identifiants with associated bounding boxes (as string)
    composition - hash - Defines source pyramids for each level, extent, and order
|       level_id => priority => {
|               bbox => bbox_id,
|               pyr => descriptor_path,
|       }
    sourceByLevel - integer hash - Precises the number of source pyramids for each level (to define priorities).
    sourcePyramids - string hash - Key is the descriptor's path. Just undefined values, to list used pyramids.
    process - hash - Generation parameters
=cut

################################################################################

package JOINCACHE::PropertiesLoader;

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

=begin nd
Variable: PROPLOADER

Define allowed sections
=cut
my %PROPLOADER;

################################################################################

BEGIN {}
INIT {
    
    %PROPLOADER = (
        sections => ['pyramid','process','composition','logger','bboxes'],
    );
    
}
END {}

####################################################################################################
#                                        Group: Constructors                                       #
####################################################################################################

# Function: new
sub new {
    my $this = shift;
    my $filepath = shift;

    my $class= ref($this) || $this;
    # IMPORTANT : if modification, think to update natural documentation (just above)
    my $self = {
        configurationPath => undef,
        
        logger => undef,
        pyramid => undef,
        bboxes => undef,
        composition => undef,
        process => undef,
    };

    bless($self, $class);

    TRACE;

    # Load parameters
    return undef if (! $self->_load($filepath));

    return $self;
}

=begin nd
Function: _load

Read line by line (order is important), no library is used.

Parameters (list):
    filepath - string - Configuration file path, to read

See Also:
    <isConfSection>, <readCompositionLine>
=cut
sub _load {
    my $self = shift;
    my $filepath = shift;

    TRACE;

    if (! open CFGF, "<", $filepath ){
        ERROR(sprintf "Cannot open configurations' file %s.",$filepath);
        return FALSE;
    }

    my $currentSection = undef;

    while( defined( my $l = <CFGF> ) ) {
        chomp $l;
        $l =~ s/\s+//g; # we remove all spaces
        $l =~ s/;\S*//; # we remove comments

        next if ($l eq '');

        if ($l =~ m/^\[(\w*)\]$/) {
            $l =~ s/[\[\]]//g;

            if (! $self->isConfSection($l)) {
                ERROR ("Invalid section's name");
                return FALSE;
            }
            $currentSection = $l;
            next;
        }

        if (! defined $currentSection) {
            ERROR (sprintf "A property must always be in a section (%s)",$l);
            return FALSE;
        }

        my @prop = split(/=/,$l,-1);
        if (scalar @prop != 2 || $prop[0] eq '' || $prop[1] eq '') {
            ERROR (sprintf "A line is invalid (%s). Must be prop = val",$l);
            return FALSE;
        }

        if ($currentSection ne 'composition') {
            if (exists $self->{$currentSection}->{$prop[0]}) {
                ERROR (sprintf "A property is defined twice in the configuration : section %s, parameter %s", $currentSection,$prop[0]);
                return FALSE;
            }
            $self->{$currentSection}->{$prop[0]} = $prop[1];
        } else {
            if (! $self->readCompositionLine($prop[0],$prop[1])) {
                ERROR (sprintf "Cannot read a composition line !");
                return FALSE;
            }
        }

    }

    close CFGF;

    return TRUE;
}

=begin nd
Function: readComposition

Reads a *composition* section line. Determine sources by level and calculate priorities.

Parameters (list):
    prop - string - Composition's name: levelId.bboxId
    val - string - Composition's value: pyrPath1,pyrPath2,pyrPath3
=cut
sub readCompositionLine {
    my $self = shift;
    my $prop = shift;
    my $val = shift;

    TRACE;

    my ($levelId,$bboxId) = split(/\./,$prop,-1);

    if ($levelId eq '' || $bboxId eq '') {
        ERROR (sprintf "Cannot define a level id and a bbox id (%s). Must be levelId.bboxId",$prop);
        return FALSE;
    }

    my @pyrs = split(/,/,$val,-1);

    foreach my $pyr (@pyrs) {
        if ($pyr eq '') {
            ERROR (sprintf "Invalid list of pyramids (%s). Must be /path/pyr1.pyr,/path/pyr2.pyr",$val);
            return FALSE;
        }
        if (! -f $pyr) {
            ERROR (sprintf "A referenced pyramid's file doesn't exist : %s",$pyr);
            return FALSE;
        }

        my $priority = 1;
        if (exists $self->{sourceByLevel}->{$levelId}) {
            $self->{sourceByLevel}->{$levelId} += 1;
            $priority = $self->{sourceByLevel}->{$levelId};
        } else {
            $self->{sourceByLevel}->{$levelId} = 1;
        }

        $self->{composition}->{$levelId}->{$priority} = {
            bbox => $bboxId,
            pyr => $pyr,
        };

        if (! exists $self->{sourcePyramids}->{$pyr}) {
            # we have a new source pyramid, but not yet information about
            $self->{sourcePyramids}->{$pyr} = undef;
        }

    }

    return TRUE;
}


####################################################################################################
#                                      Group: Tester                                               #
####################################################################################################

=begin nd
Function: isConfSection

Check section's name. Possible values: 'pyramid','process','composition','logger','bboxes'.

Parameters (list):
    section - string - section's name
=cut
sub isConfSection {
    my $self = shift;
    my $section = shift;

    TRACE;

    return FALSE if (! defined $section);

    foreach (@{$PROPLOADER{sections}}) {
        return TRUE if ($section eq $_);
    }
    ERROR (sprintf "Unknown 'section' (%s) !",$section);
    return FALSE;
}

####################################################################################################
#                                Group: Getters - Setters                                          #
####################################################################################################

# Function: getSourcePyramids
sub getSourcePyramids {
    my $self = shift;
    return $self->{sourcePyramids};
}

# Function: getPyramidSection
sub getPyramidSection {
    my $self = shift;
    return $self->{pyramid};
}

# Function: getLoggerSection
sub getLoggerSection {
    my $self = shift;
    return $self->{logger};
}

# Function: getCompositionSection
sub getCompositionSection {
    my $self = shift;
    return $self->{composition};
}

# Function: getBboxesSection
sub getBboxesSection {
    my $self = shift;
    return $self->{bboxes};
}

# Function: getProcessSection
sub getProcessSection {
    my $self = shift;
    return $self->{process};
}

1;
__END__
