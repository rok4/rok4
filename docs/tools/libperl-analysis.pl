#/usr/bin/perl
# 

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

## Paramètres

my $perl_directory = $ARGV[0];
if (! -d $perl_directory) {
    ERROR("$perl_directory is not a directory");
    exit(1);
}

my $output_directory = $ARGV[1];
if (! defined $output_directory) {
    ERROR("Output directory is not defined");
    exit(1);
}

##############################################################################################

# Création des dossiers

`mkdir -p $output_directory`;

##############################################################################################

my $binaries = {};
my @pls = `find $perl_directory -name "*.pl.in"`;
foreach my $pl (@pls) {
    chomp($pl);
    open(IN, "<$pl") or die "Cannot open '$pl' to read in it";
    
    my $bin = File::Basename::basename($pl);
    $bin =~ s/.pl.in//;
    my $functions = [];
    my $called_constructors = {};

    while (my $line = <IN>) {
        chomp($line);

        if ($line =~ m/([A-Za-z0-9::]+)->new/) {
            $called_constructors->{$1} = 1;
        }
        if ($line =~ m/^sub ([^\s{]+) ?/) {
            push(@{$functions}, $1);
        }
    }
    
    close(IN);

    INFO("Binary $bin");

    $binaries->{$bin} = {
        file => $pl,
        functions => $functions,
        called_constructors => $called_constructors
    };
}

##############################################################################################

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
    my $functions = [];
    my $called_constructors = {};
    # my $called_class_methods = {};
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
        if (defined $package && $line =~ m/\$this->\{([^\}]+)\}/) {
            if ($1 !~ m/^\$/) {
                $attributes_used->{$1} = 1;
            }
        }
        if (defined $package && $line =~ m/([A-Za-z0-9::]+)->new/) {
            $called_constructors->{$1} = 1;
        }
        # if ($line =~ m/([A-Za-z0-9]+::[A-Za-z0-9]+)::([A-Za-z0-9_-]+)/) {
        #     push(@{$called_class_methods->{$1}}, $2);
        # }
        if (defined $package && $line =~ m/^sub ([^\s{]+) ?/) {
            push(@{$functions}, $1);
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
        attributes => $attributes_used,
        functions => $functions,
        called_constructors => $called_constructors,
        caller_constructors => {}
        # called_class_methods => $called_class_methods,
        # caller_class_methods => []
    };
}

##############################################################################################

# On refait une passe sur les libs et bins pour ajouter les appelants de son constructeur et des méthodes de classe

while (my ($package, $lib) = each (%{$libraries})) {
    foreach my $called (keys %{$lib->{called_constructors}}) {
        if (exists $libraries->{$called}) {
            $libraries->{$called}->{caller_constructors}->{libs}->{$package} = 1;
        }
    }
}

while (my ($bin, $binary) = each (%{$binaries})) {
    foreach my $called (keys %{$binary->{called_constructors}}) {
        if (exists $libraries->{$called}) {
            $libraries->{$called}->{caller_constructors}->{bins}->{$bin} = 1;
        }
    }
}

##############################################################################################

# Pour chaque lib on dessine le diagrame de classe
while (my ($package, $lib) = each (%{$libraries})) {
    INFO("Diagram for $package");
    my $attributes = "";
    while (my ($att, $type) = each(%{$lib->{attributes}})) {
        $type =~ s/</\\</g;
        $type =~ s/>/\\>/g;
        $attributes .= "+ $att: $type\\l";
    }
    my $functions = "";
    foreach my $fun (@{$lib->{functions}}) {
        if ($fun =~ m/^_/) {next;}
        $functions .= "+ $fun(...)\\l";
    }

    my $basename = $package;
    $basename =~ s/::/_/;
    my $dot_file = "tmp.dot";
    my $png_file = "$output_directory/$basename.png";

    my $file = $dot_file;
    open(OUT, ">$file") or die "Cannot open '$file' to write in it";
    
    print OUT "digraph $basename {\n";
    print OUT "    node[shape=record,style=filled,fillcolor=gray95]\n";
    print OUT "    edge[]\n";
    print OUT "    ${basename} [label = \"{$package|$attributes|$functions}\"]\n";

    foreach my $caller (keys %{$lib->{caller_constructors}->{libs}}) {
        my $bn = $caller;
        $bn =~ s/::/_/g;
        print OUT "    ${bn} [label = \"{$caller}\"]\n";
        print OUT "    ${bn} -> ${basename}\n";
    }

    foreach my $caller (keys %{$lib->{caller_constructors}->{bins}}) {
        my $bn = $caller;
        $bn =~ s/-/_/g;
        $bn =~ s/^(\d)/a$1/;
        print OUT "    ${bn} [shape=ellipse, label = \"$caller\"]\n";
        print OUT "    ${bn} -> ${basename}\n";
    }

    print OUT "}\n";

    close(OUT);

    `dot -Tpng $dot_file -o $png_file`;
}
