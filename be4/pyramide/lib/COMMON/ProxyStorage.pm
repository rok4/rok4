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
File: ProxyStorage.pm

Class: COMMON::ProxyStorage

Proxy to manipulate different storage types : object or file, we can copy, reference or remove from one to another

Using:
    (start code)
    use COMMON::ProxyStorage;
    (end code)
=cut

################################################################################

package COMMON::ProxyStorage;

use strict;
use warnings;

use Data::Dumper;
use Net::Amazon::S3;
use File::Basename;

use COMMON::Array;

require Exporter;
use AutoLoader qw(AUTOLOAD);

our @ISA = qw(Exporter);

our %EXPORT_TAGS = ( 'all' => [ qw() ] );
our @EXPORT_OK   = ( @{$EXPORT_TAGS{'all'}} );
our @EXPORT      = qw();

use Log::Log4perl qw(:easy);

################################################################################
# Constantes
use constant TRUE  => 1;
use constant FALSE => 0;

my @STORAGETYPES = ("FILE", "CEPH", "S3");

####################################################################################################
#                               Group: Copy methods                                                #
####################################################################################################

=begin nd
Function: copy

Return TRUE if success FALSE otherwise
=cut
sub copy {
    my $fromType = shift;
    my $fromPath = shift;
    my $toType = shift;
    my $toPath = shift;

    if ($fromType eq "FILE") {
        if ($toType eq "FILE") {

            # create folder
            my $dir = File::Basename::dirname($toPath);
            `mkdir -p $dir`;
            if ($?) {
                ERROR("Cannot create directory '$dir' : $!");
                return FALSE;
            }
        
            `cp $fromPath $toPath`;
            if ($?) {
                ERROR("Cannot copy from file '$fromPath' to file '$toPath' : $!");
                return FALSE;
            }
            return TRUE;
        }
        elsif ($toType eq "CEPH") {
            return FALSE;
        }
        elsif ($toType eq "S3") {
            return FALSE;
        }
    }
    elsif ($fromType eq "CEPH") {
        if ($toType eq "FILE") {
            return FALSE;
        }
        elsif ($toType eq "CEPH") {
            return FALSE;
        }
        elsif ($toType eq "S3") {
            return FALSE;
        }
    }
    elsif ($fromType eq "S3") {
        if ($toType eq "FILE") {
            return FALSE;
        }
        elsif ($toType eq "CEPH") {
            return FALSE;
        }
        elsif ($toType eq "S3") {
            return FALSE;
        }
    }

    return FALSE;
}

####################################################################################################
#                               Group: Test methods                                                #
####################################################################################################

=begin nd
Function: whatIs

Return string DIRECTORY, LINK, or REAL
=cut
sub whatIs {
    my $type = shift;
    my $path = shift;

    if ($type eq "FILE") {
        if (-d $path) {
            return "DIRECTORY";
        }
        if (-l $path) {
            return "LINK";
        }
        if (-f $path) {
            return "REAL";
        }
    }
    elsif ($type eq "CEPH") {
        return undef;
    }
    elsif ($type eq "S3") {
        return undef;
    }

    return undef;
}

=begin nd
Function: isPresent

Return TRUE or FALSE
=cut
sub isPresent {
    my $type = shift;
    my $path = shift;

    if ($type eq "FILE") {
        if (-f $path) {
            return TRUE;
        }
        if (-d $path) {
            return TRUE;
        }

        return FALSE;
    }
    elsif ($type eq "CEPH") {
        return FALSE;
    }
    elsif ($type eq "S3") {
        return FALSE;
    }

    return FALSE;
}

####################################################################################################
#                               Group: Link methods                                                #
####################################################################################################

=begin nd
Function: symLink

Return TRUE if success FALSE otherwise
=cut
sub symLink {
    my $targetType = shift;
    my $targetPath = shift;
    my $toType = shift;
    my $toPath = shift;

    if ($targetType eq "FILE") {
        if ($toType eq "FILE") {

            # create folder
            my $dir = File::Basename::dirname($toPath);
            `mkdir -p $dir`;
            if ($?) {
                ERROR("Cannot create directory '$dir' : $!");
                return FALSE;
            }

            my $relTargetPath = File::Spec->abs2rel($targetPath, $dir);

            `ln -s $relTargetPath $toPath`;
            if ($?) {
                ERROR("Cannot link (symbolic) file $targetPath from file $toPath : $!");
                return FALSE;
            }
            return TRUE;
        }
        elsif ($toType eq "CEPH") {
            return FALSE;
        }
        elsif ($toType eq "S3") {
            return FALSE;
        }
    }
    elsif ($targetType eq "CEPH") {
        if ($toType eq "FILE") {
            return FALSE;
        }
        elsif ($toType eq "CEPH") {
            return FALSE;
        }
        elsif ($toType eq "S3") {
            return FALSE;
        }
    }
    elsif ($targetType eq "S3") {
        if ($toType eq "FILE") {
            return FALSE;
        }
        elsif ($toType eq "CEPH") {
            return FALSE;
        }
        elsif ($toType eq "S3") {
            return FALSE;
        }
    }

    return FALSE;
}

=begin nd
Function: hardLink

Return TRUE if success FALSE otherwise
=cut
sub hardLink {
    my $targetType = shift;
    my $targetPath = shift;
    my $toType = shift;
    my $toPath = shift;

    if ($targetType eq "FILE") {
        if ($toType eq "FILE") {

            # create folder
            my $dir = File::Basename::dirname($toPath);
            `mkdir -p $dir`;
            if ($?) {
                ERROR("Cannot create directory '$dir' : $!");
                return FALSE;
            }

            `ln $targetPath $toPath`;
            if ($?) {
                ERROR("Cannot link (hard) file $targetPath from file $toPath : $!");
                return FALSE;
            }
            return TRUE;
        }
        elsif ($toType eq "CEPH") {
            return FALSE;
        }
        elsif ($toType eq "S3") {
            return FALSE;
        }
    }
    elsif ($targetType eq "CEPH") {
        if ($toType eq "FILE") {
            return FALSE;
        }
        elsif ($toType eq "CEPH") {
            return FALSE;
        }
        elsif ($toType eq "S3") {
            return FALSE;
        }
    }
    elsif ($targetType eq "S3") {
        if ($toType eq "FILE") {
            return FALSE;
        }
        elsif ($toType eq "CEPH") {
            return FALSE;
        }
        elsif ($toType eq "S3") {
            return FALSE;
        }
    }

    return FALSE;
}

1;
__END__
