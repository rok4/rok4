#!/usr/bin/perl

# S'obtient avec :
# ack -h "^use [A-Z]" ./ | grep -v -E "JOINCACHE|COMMON|BE4|WMTSALAD|EXPYR|FOURALAMO"| sed -r "s#;.+#;#" | sed "s# ;#;#" | sort | uniq

use strict;
use warnings;
use AutoLoader qw(AUTOLOAD);

use Cwd;
use Data::Dumper;
use DBD::Pg;
use Digest::SHA;
use ExtUtils::MakeMaker;
use File::Basename;
use File::Copy;
use File::Find::Rule;
use File::Map qw(map_file);
use File::Path;
use File::Path qw(make_path);
use File::Spec;
use File::Spec;
use FindBin qw($Bin);
use Geo::GDAL;
use Geo::OGR;
use Geo::OSR;
use Getopt::Long;
use HTTP::Request;
use HTTP::Response;
use JSON::Parse;
use List::Util qw(min max);
use Log::Log4perl qw(:easy);
use LWP::UserAgent;
use Math::BigFloat;
use POSIX qw(locale_h);
use Scalar::Util qw/reftype/;
use Test::More;
use Tie::File;
use XML::LibXML;


# use Tk;
# use Tk::EntryCheck;
# use Tk::FileSelect;
# use Tk::LabFrame;
# use Tk::NoteBook;
# use Tk::ROText;
# use Tk::Table;
