#!/usr/bin/perl

# S'obtient avec :
# ack -h "^use [A-Z]" ./ | grep -v -E "JOINCACHE|COMMON|BE4|WMTSALAD|EXPYR|FOURALAMO|PYR2PYR"| sed -r "s#;.+#;#" | sed "s# ;#;#" | sort | uniq

use strict;
use warnings;

use AutoLoader qw(AUTOLOAD);
use Cwd;
use Cwd qw(realpath cwd);
use Data::Dumper;
use DBD::Pg;
use DBI;
use Devel::Size;
use Devel::Size qw(size total_size);
use Digest::SHA;
use ExtUtils::MakeMaker;
use File::Basename;
use File::Copy;
use File::Find::Rule;
use File::Map qw(map_file);
use File::Path;
use File::Path qw(make_path);
use File::Spec;
use FindBin qw($Bin);
use Geo::GDAL;
use Geo::OGR;
use Geo::OSR;
use Getopt::Long;
use HTTP::Request;
use HTTP::Request::Common;
use HTTP::Response;
use JSON::Parse;
use JSON::Parse qw(assert_valid_json parse_json);
use List::Util qw(min max);
use Log::Log4perl qw(:easy);
use LWP::UserAgent;
use Math::BigFloat;
use POSIX qw(locale_h);
use Scalar::Util qw/reftype/;
use Term::ProgressBar;
use Test::More;
use Tie::File;
use XML::LibXML;
