#/usr/bin/perl

use strict;
use warnings;
use utf8;

use File::Basename;
use Data::Dumper;
use Log::Log4perl qw(:easy);

##############################################################################################

Log::Log4perl->easy_init({
    level => "INFO",
    layout => '%m %n',
    utf8  => 1
});

##############################################################################################

my $perl_directory = $ARGV[0];
if (! -d $perl_directory) {
    ERROR("$perl_directory is not a directory");
    exot(1);
}

my $libraries = {};
my @pms = `find $perl_directory -name "*.pm"`;
foreach my $pm (@pms) {
    chomp($pm);
    open(IN, "<$pm") or die "Cannot open '$pm' to read in it";
    
    my $error = 0;
    my $package = undef;
    my $attributes_documented = {};
    my $documented = 0;
    my $attributes_used = {};
    my $functions = {};
    while (my $line = <IN>) {
        chomp($line);
        if ($line =~ m/^package (\S+);$/) {
            if (defined $package) {
                ERROR("Package defined twice in $pm");
                next;
            }
            $package = $1;
        }
        if ($line =~ m/^Attributes:/) {
            # On va a priori lire le typage et les explications des attributs. On vérifiera après que tous ceux utilisés sont bien présents ici
            if ($documented) {
                WARN("In $pm we find two section Attributes:");
                $error = 1;
                last;
            }
            while($line = <IN>) {
                chomp($line);
                if ($line =~ m/^\s*$/) {
                    next;
                }
                elsif ($line =~ m/^\s+(\S+) - ([^-]+) - /) {
                    $attributes_documented->{$1} = $2;
                }
                elsif ($line =~ m/^[^\|\s]/) {
                    last;
                }
            }
        }
        if ($line =~ m/\$this->\{([^\}]+)\}/) {
            if ($1 !~ m/^\$/) {
                $attributes_used->{$1} = 1;
            }
        }
        if ($line =~ m/^sub (\S+) /) {
            $functions->{$1} = 1;
        }
    }

    if ($error) {
        ERROR("Error in $pm");
        next;
    }

    if (! defined $package) {
        ERROR("No package name defined in $pm");
        next;
    }


    
    close(IN);

    INFO("$package");

    foreach my $att (keys %{$attributes_used}) {
        if (! exists $attributes_documented->{$att}) {
            ERROR("    $att not documented");
        } else {
            $attributes_used->{$att} = $attributes_documented->{$att};
        }
    }

    foreach my $att (keys %{$attributes_documented}) {
        if (! exists $attributes_used->{$att}) {
            ERROR("    $att documented but not used");
        }
    }

    $package =~ m/([^:])+::(\S)+/;
    $libraries->{$package} = {
        file => $pm,
        namespace => $1,
        class => $2,
        attributes => $attributes_used
    };
}

INFO(Dumper($libraries));