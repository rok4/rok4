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
use Digest::SHA;
use File::Map qw(map_file);
use HTTP::Request;
use HTTP::Response;
use LWP::UserAgent;
use File::Basename;

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

my $UA = LWP::UserAgent->new();

### S3

my $S3_ENDPOINT_NOPROTOCOL = $ENV{ROK4_S3_URL};
if (defined $S3_ENDPOINT_NOPROTOCOL) {
    $S3_ENDPOINT_NOPROTOCOL =~ s/^https?:\/\///;
    $S3_ENDPOINT_NOPROTOCOL =~ s/:[0-9]+$//;
}

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
            my ($poolName, @rest) = split("/", $toPath);
            my $objectName = join("", @rest);

            if (! defined $poolName || ! defined $objectName) {
                ERROR("CEPH path is not valid (<poolName>/<objectName>) : $toPath");
                return FALSE;
            }

            `rados -p $poolName put $objectName $fromPath`;
            if ($?) {
                ERROR("Cannot upload file '$fromPath' to CEPH object $objectName (pool $poolName): $!");
                return FALSE;
            }

            return TRUE;
        }
        elsif ($toType eq "S3") {
            my ($bucketName, @rest) = split("/", $toPath);
            my $objectName = join("", @rest);

            if (! defined $bucketName || ! defined $objectName) {
                ERROR("S3 path is not valid (<bucketName>/<objectName>) : $toPath");
                return FALSE;
            }

            my $body;

            map_file $body, $fromPath;

            my $resource = "/$bucketName/$objectName";
            my $contentType="application/octet-stream";
            my $dateValue=`TZ=GMT date -R`;
            chomp($dateValue);
            my $stringToSign="PUT\n\n$contentType\n$dateValue\n$resource";

            my $signature = Digest::SHA::hmac_sha1_base64($stringToSign, $ENV{ROK4_S3_SECRETKEY});
            while (length($signature) % 4) {
                $signature .= '=';
            }

            # set custom HTTP request header fields
            my $request = HTTP::Request->new(PUT => $ENV{ROK4_S3_URL}.$resource);
            $request->content($body);
            $request->header('Host' => $S3_ENDPOINT_NOPROTOCOL);
            $request->header('Date' => $dateValue);
            $request->header('Content-Type' => $contentType);
            $request->header('Authorization' => sprintf ("AWS %s:$signature", $ENV{ROK4_S3_KEY}));
             
            my $response = $UA->request($request);
            if ($response->is_success) {
                return TRUE;
            }
            else {
                ERROR("Cannot upload file '$fromPath' to S3 object $objectName (bucket $bucketName)");
                ERROR("HTTP code: ", $response->code);
                ERROR("HTTP message: ", $response->message);
                ERROR("HTTP decoded content : ", $response->decoded_content);
                return FALSE;
            }
        }
    }
    elsif ($fromType eq "CEPH") {
        if ($toType eq "FILE") {
            my ($poolName, @rest) = split("/", $fromPath);
            my $objectName = join("", @rest);

            if (! defined $poolName || ! defined $objectName) {
                ERROR("CEPH path is not valid (<poolName>/<objectName>) : $fromPath");
                return FALSE;
            }

            # create folder
            my $dir = File::Basename::dirname($toPath);
            `mkdir -p $dir`;
            if ($?) {
                ERROR("Cannot create directory '$dir' : $!");
                return FALSE;
            }

            `rados -p $poolName get $objectName $toPath`;
            if ($?) {
                ERROR("Cannot download CEPH object $objectName (pool $poolName) into file '$fromPath': $!");
                return FALSE;
            }

            return TRUE;
        }
        elsif ($toType eq "CEPH") {
            my ($fromPool, @from) = split("/", $fromPath);
            my $fromObjectName = join("", @from);

            if (! defined $fromPool || ! defined $fromObjectName) {
                ERROR("CEPH path is not valid (<poolName>/<objectName>) : $fromPath");
                return FALSE;
            }

            my ($toPool, @to) = split("/", $toPath);
            my $toObjectName = join("", @to);

            if (! defined $toPool || ! defined $toObjectName) {
                ERROR("CEPH path is not valid (<poolName>/<objectName>) : $toPath");
                return FALSE;
            }

            if ($toPool ne $fromPool) {
                ERROR("CEPH copy is not possible for different pool: $fromPath -> X $toPath");
                return FALSE;
            }

            `rados -p $toPool cp $fromObjectName $toObjectName`;
            if ($?) {
                ERROR("Cannot copy CEPH object $fromObjectName -> $toObjectName (pool $fromPool): $!");
                return FALSE;
            }

            return TRUE;
        }
        elsif ($toType eq "S3") {
            return FALSE;
        }
    }
    elsif ($fromType eq "S3") {
        if ($toType eq "FILE") {

            my ($bucketName, @rest) = split("/", $fromPath);
            my $objectName = join("", @rest);

            if (! defined $bucketName || ! defined $objectName) {
                ERROR("S3 path is not valid (<bucketName>/<objectName>) : $fromPath");
                return FALSE;
            }

            my $context = "/$bucketName/$objectName";
            my $content_type = "application/octet-stream";
            my $date_gmt = `TZ=GMT date -R`;
            chomp($date_gmt);
            my $string_to_sign="GET\n\n$content_type\n$date_gmt\n$context";

            my $signature = Digest::SHA::hmac_sha1_base64($string_to_sign, $ENV{ROK4_S3_SECRETKEY});
            while (length($signature) % 4) {
                $signature .= '=';
            }

            my $request = HTTP::Request->new(GET => $ENV{ROK4_S3_URL}.$context);

            $request->header('Host' => $S3_ENDPOINT_NOPROTOCOL);
            $request->header('Date' => $date_gmt);
            $request->header('Content-Type' => $content_type);
            $request->header('Authorization' => sprintf ("AWS %s:$signature", $ENV{ROK4_S3_KEY}));
             
            # create folder
            my $dir = File::Basename::dirname($toPath);
            `mkdir -p $dir`;
            if ($?) {
                ERROR("Cannot create directory '$dir' : $!");
                return FALSE;
            }

            my $response = $UA->request($request, $toPath);
            if ($response->is_success) {
                return TRUE;
            } else {
                ERROR("Cannot download S3 object '$fromPath' to file $toPath");
                ERROR("HTTP code: ", $response->code);
                ERROR("HTTP message: ", $response->message);
                ERROR("HTTP decoded content : ", $response->decoded_content);
                return FALSE;
            }
        }
        elsif ($toType eq "CEPH") {
            return FALSE;
        }
        elsif ($toType eq "S3") {

            my ($fromBucket, @from) = split("/", $fromPath);
            my $fromObjectName = join("", @from);

            if (! defined $fromBucket || ! defined $fromObjectName) {
                ERROR("S3 path is not valid (<bucketName>/<objectName>) : $fromPath");
                return FALSE;
            }

            my ($toBucket, @to) = split("/", $toPath);
            my $toObjectName = join("", @to);

            if (! defined $toBucket || ! defined $toObjectName) {
                ERROR("CEPH path is not valid (<bucketName>/<objectName>) : $toPath");
                return FALSE;
            }

            my $context = "/$toBucket/$toObjectName";
            my $content_type = "application/octet-stream";
            my $date_gmt = `TZ=GMT date -R`;
            chomp($date_gmt);
            my $string_to_sign="PUT\n\n$content_type\n$date_gmt\n$context";

            my $signature = Digest::SHA::hmac_sha1_base64($string_to_sign, $ENV{ROK4_S3_SECRETKEY});
            while (length($signature) % 4) {
                $signature .= '=';
            }

            my $request = HTTP::Request->new(PUT => $ENV{ROK4_S3_URL}.$context);

            $request->header('Host' => $S3_ENDPOINT_NOPROTOCOL);
            $request->header('Date' => $date_gmt);
            $request->header('Content-Type' => $content_type);
            $request->header('x-amz-copy-source' => "/$fromBucket/$fromObjectName");
            $request->header('Authorization' => sprintf ("AWS %s:$signature", $ENV{ROK4_S3_KEY}));

            my $response = $UA->request($request, $toPath);
            if ($response->is_success) {
                return TRUE;
            } else {
                ERROR("Cannot copy S3 object '$fromPath' to S3 object '$toPath'");
                ERROR("HTTP code: ", $response->code);
                ERROR("HTTP message: ", $response->message);
                ERROR("HTTP decoded content : ", $response->decoded_content);
                return FALSE;
            }
        }
    }

    return FALSE;
}

####################################################################################################
#                               Group: Redis methods                                               #
####################################################################################################

sub redisValueFromKey {
    my $key = shift;

    my $redisValue = `redis-cli -h $ENV{ROK4_REDIS_HOST} -p $ENV{ROK4_REDIS_PORT} -a $ENV{ROK4_REDIS_PASSWD} GET $key`;
    chomp($redisValue);

    if ($redisValue ne "") {
        return $redisValue;
    } else {
        return undef;
    }
}

sub addKeyValue {
    my $key = shift;
    my $value = shift;

    my $ret = `redis-cli -h $ENV{ROK4_REDIS_HOST} -p $ENV{ROK4_REDIS_PORT} -a $ENV{ROK4_REDIS_PASSWD} SET $key $value`;
    chomp($ret);
    if ($ret ne "OK") {
        ERROR("REDIS: $ret");
        return FALSE;
    }

    return TRUE;
}

sub redisKeyExists {
    my $key = shift;

    my $redisPresent = `redis-cli -h $ENV{ROK4_REDIS_HOST} -p $ENV{ROK4_REDIS_PORT} -a $ENV{ROK4_REDIS_PASSWD} EXISTS $key | cut -d' ' -f1`;
    chomp($redisPresent);

    return ($redisPresent eq "1");
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
        ERROR("Not implemented");
        return FALSE;
    }
    elsif ($type eq "S3") {
        # On interroge la base REDIS puis le S3

        my ($bucketName, @rest) = split("/", $path);
        my $objectName = join("", @rest);

        if (! defined $bucketName || ! defined $objectName) {
            ERROR("S3 path is not valid (<bucketName>/<objectName>) : $path");
            return FALSE;
        }

        if (redisKeyExists($objectName)) {
            return TRUE;
        }

        my $resource = "/$bucketName/$objectName";
        my $contentType="application/octet-stream";
        my $dateValue=`TZ=GMT date -R`;
        chomp($dateValue);
        my $stringToSign="HEAD\n\n$contentType\n$dateValue\n$resource";

        my $signature = Digest::SHA::hmac_sha1_base64($stringToSign, $ENV{ROK4_S3_SECRETKEY});
        while (length($signature) % 4) {
            $signature .= '=';
        }

        # set custom HTTP request header fields
        my $request = HTTP::Request->new(HEAD => $ENV{ROK4_S3_URL}.$resource);
        $request->header('Host' => $S3_ENDPOINT_NOPROTOCOL);
        $request->header('Date' => $dateValue);
        $request->header('Content-Type' => $contentType);
        $request->header('Authorization' => sprintf ("AWS %s:$signature", $ENV{ROK4_S3_KEY}));
         
        my $response = $UA->request($request);
        if ($response->is_success) {
            return TRUE;
        } else {
            return FALSE;
        }
    }

    return FALSE;
}

####################################################################################################
#                               Group: Content methods                                             #
####################################################################################################

=begin nd
Function: getSize

Return size of file/object
=cut
sub getSize {
    my $type = shift;
    my $path = shift;

    if ($type eq "FILE") {
        return -s $path;
    }
    elsif ($type eq "CEPH") {
        ERROR("Not implemented");
        return undef;
    }
    elsif ($type eq "S3") {

        my ($bucketName, @rest) = split("/", $path);
        my $objectName = join("", @rest);

        if (! defined $bucketName || ! defined $objectName) {
            ERROR("S3 path is not valid (<bucketName>/<objectName>) : $path");
            return FALSE;
        }

        my $resource = "/$bucketName/$objectName";
        my $contentType="application/octet-stream";
        my $dateValue=`TZ=GMT date -R`;
        chomp($dateValue);
        my $stringToSign="HEAD\n\n$contentType\n$dateValue\n$resource";

        my $signature = Digest::SHA::hmac_sha1_base64($stringToSign, $ENV{ROK4_S3_SECRETKEY});
        while (length($signature) % 4) {
            $signature .= '=';
        }

        # set custom HTTP request header fields
        my $request = HTTP::Request->new(HEAD => $ENV{ROK4_S3_URL}.$resource);
        $request->header('Host' => $S3_ENDPOINT_NOPROTOCOL);
        $request->header('Date' => $dateValue);
        $request->header('Content-Type' => $contentType);
        $request->header('Authorization' => sprintf ("AWS %s:$signature", $ENV{ROK4_S3_KEY}));
         
        my $response = $UA->request($request);
        if ($response->is_success) {
            return $response->header("Content-Length");
        }
        else {
            ERROR("HTTP code: ", $response->code);
            ERROR("HTTP message: ", $response->message);
            ERROR("HTTP decoded content : ", $response->decoded_content);
            return undef;
        }
    }

    return undef;
}

####################################################################################################
#                               Group: Link methods                                                #
####################################################################################################

=begin nd
Function: symLink

Return the real file/object linked if success, undef otherwise
=cut
sub symLink {
    my $targetType = shift;
    my $targetPath = shift;
    my $toType = shift;
    my $toPath = shift;

    if ($targetType eq "FILE" && $toType eq "FILE") {
        # create folder
        my $dir = File::Basename::dirname($toPath);
        `mkdir -p $dir`;
        if ($?) {
            ERROR("Cannot create directory '$dir' : $!");
            return undef;
        }

        my $relativeTargetPath;
        my $realTargetPath;
        if (-f $targetPath && ! -l $targetPath) {
            $relativeTargetPath = File::Spec->abs2rel($targetPath,$dir);
            $realTargetPath = $targetPath;
        }
        elsif (-f $targetPath && -l $targetPath) {
            $realTargetPath = File::Spec::Link->full_resolve( File::Spec::Link->linked($targetPath) );
            $relativeTargetPath = File::Spec->abs2rel($realTargetPath, $dir);
        } else {
            ERROR(sprintf "The tile '%s' (to symlink) is not a file or a link in '%s' !", basename($targetPath), dirname($targetPath));
            return undef;
        }

        my $result = eval { symlink ($relativeTargetPath, $toPath); };
        if (! $result) {
            ERROR (sprintf "The tile '%s' can not be linked by '%s' (%s) ?", $targetPath, $toPath, $!);
            return undef;
        }

        return $realTargetPath;
    }
    elsif ($targetType eq "CEPH" && $toType eq "CEPH") {
        my ($tPoolName, @rest) = split("/", $targetPath);
        my $realTarget = join("", @rest);

        # On vérifie que la dalle CEPH à lier n'est pas un alias, auquel cas on référence le vrai objet (pour éviter des alias en cascade)
        my $value = redisValueFromKey($realTarget);
        if (defined $value) {
            # On a une valeur qui est le vrai nom de l'objet (réellement stocké sur S3)
            $realTarget = $value;
        }

        # On retire le bucket du nom de l'alias à créer
        (my $toPoolName, @rest) = split("/", $toPath);
        $toPath = join("", @rest);

        if ($tPoolName ne $toPoolName) {
            ERROR("CEPH link (redis key-value) is not possible between different pool: $toPath -> X $targetPath");
            return FALSE;
        }

        # On ajoute la paire clé - valeur dans redis : $toPath => $realTarget
        if (! addKeyValue($toPath, $realTarget)) {
            ERROR("Cannot symlink (add a key/value in redis) object $realTarget with alias $toPath : $!");
            return undef;
        }

        return "$tPoolName/$realTarget";
    }
    elsif ($targetType eq "S3" && $toType eq "S3") {
        my ($tBucketName, @rest) = split("/", $targetPath);
        my $realTarget = join("", @rest);

        # On vérifie que la dalle S3 à lier n'est pas un alias, auquel cas on référence le vrai objet (pour éviter des alias en cascade)

        my $value = redisValueFromKey($realTarget);
        if (defined $value) {
            # On a une valeur qui est le vrai nom de l'objet (réellement stocké sur S3)
            $realTarget = $value;
        }

        # On retire le bucket du nom de l'alias à créer
        (my $toBucketName, @rest) = split("/", $toPath);
        $toPath = join("", @rest);

        if ($tBucketName ne $toBucketName) {
            ERROR("S3 link (redis key-value) is not possible between different pool: $toPath -> X $targetPath");
            return FALSE;
        }

        # On ajoute la paire clé - valeur dans redis : $toPath => $realTarget
        if (! addKeyValue($toPath, $realTarget)) {
            ERROR("Cannot symlink (add a key/value in redis) object $realTarget with alias $toPath : $!");
            return undef;
        }

        return "$tBucketName/$realTarget";
    }

    ERROR("Symlink can only be done between two file/path using the same storage type (and not $toType -> $targetType)");
    return undef;
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

    if ($targetType eq "FILE" && $toType eq "FILE") {

        # create folder
        my $dir = File::Basename::dirname($toPath);
        `mkdir -p $dir`;
        if ($?) {
            ERROR("Cannot create directory '$dir' : $!");
            return FALSE;
        }

        my $realTarget;
        if (-f $targetPath && ! -l $targetPath) {
            $realTarget = $targetPath;
        }
        elsif (-f $targetPath && -l $targetPath) {
            $realTarget = File::Spec::Link->full_resolve( File::Spec::Link->linked($targetPath) );
        } else {
            ERROR(sprintf "The file (to hardlink) '%s' is not a file or a link in '%s' !", basename($targetPath), dirname($targetPath));
            return FALSE;
        }

        `ln $targetPath $toPath`;
        if ($?) {
            ERROR("Cannot link (hard) file $targetPath from file $toPath : $!");
            return FALSE;
        }

        return TRUE;
    }

    ERROR("Symlink can only be done between two files (and not $toType -> $targetType)");
    return FALSE;
}

1;
__END__
