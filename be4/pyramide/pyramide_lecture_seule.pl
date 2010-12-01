#!/usr/bin/perl -w

use strict;
use Term::ANSIColor;
use Getopt::Std;
use cache(
	'$rep_logs_param',
	'lecture_repertoires_pyramide',
);
$| = 1;
our($opt_p);

my $rep_log = $rep_logs_param;
my @repertoires;
################################################################################

################ MAIN
my $time = time();
my $log = $rep_log."/log_pyramide_lecture_seule_$time.log";

open LOG, ">>$log" or die colored ("[PYRAMIDE_LECTURE_SEULE] Impossible de creer le fichier $log.", 'white on_red');
&ecrit_log("commande : @ARGV");

getopts("l:p:");

if ( ! defined ($opt_p ) ){
	print colored ("[PYRAMIDE_LECTURE_SEULE] Nombre d'arguments incorrect.", 'white on_red');
	print "\n\n";
	&usage();
	&ecrit_log("ERREUR Nombre d'arguments incorrect.");
	print colored ("[PYRAMIDE_LECTURE_SEULE] Veuillez specifier un parametre -p.", 'white on_red');
	print "\n";
	exit;
}

my $fichier_pyr = $opt_p;
if (! (-e $fichier_pyr && -f $fichier_pyr)){
	print colored ("[PYRAMIDE_LECTURE_SEULE] Le fichier $fichier_pyr n'existe pas.", 'white on_red');
	print "\n";
	&ecrit_log("ERREUR Le fichier $fichier_pyr n'existe pas.");
	exit;
}

&ecrit_log("Recherche des repertoires de la pyramide de $fichier_pyr.");
# action 1 : trouver tous les repertoires image
my ($ref_hash_rep_images, $ref_hash_rep_mtd) = &lecture_repertoires_pyramide($fichier_pyr);
my %level_rep_img = %{$ref_hash_rep_images};
my %level_rep_mtd = %{$ref_hash_rep_mtd};
push (@repertoires, values %level_rep_img, values %level_rep_mtd);

# action 2 : mettre tous les fichiers en lecture seule 0444
&ecrit_log("Mise en lecture seule des fichiers");
my $nombre = 0;
foreach my $rep(@repertoires){
	$nombre += &lecture_seule($rep);
}
print colored ("[PYRAMIDE_LECTURE_SEULE] $nombre fichiers mis en lecture seule.", 'green');
print "\n";
close LOG;
################################################################################

######## FONCTIONS
sub usage{
	my $bool_ok = 0;
	
	print colored ("\nUsage : \npyramide_lecture_seule.pl -p path/fichier_pyramide.pyr\n",'black on_white');
	print "\n\n";
	
	$bool_ok = 1;
	return $bool_ok;
}
################################################################################
sub ecrit_log{
	
	my $message = $_[0];
	
	my $bool_ok = 0;
	
	# machine sur Linux
    chomp(my $machine_utilisee = `hostname`);
	
	# largement inspire par P.PONS et gen_cache.pl
	my $T = localtime();
	printf LOG "$machine_utilisee %s %s\n", $T, $message;
	
	$bool_ok = 1;
	return $bool_ok;
}
################################################################################
sub lecture_seule{
	
	my $repertoire = $_[0];
	
	my $nb = 0;
	
	opendir REP, "$repertoire" or die colored ("[PYRAMIDE_LECTURE_SEULE] Impossible d'ouvrir le repertoire $repertoire.", 'white on_red');
	my @fichiers = readdir REP;
	closedir REP;
	
	foreach my $fichier(@fichiers){
		next if ($fichier =~ /^\.\.?$/);
		# on elimine les liens symboliques (qui s'ils sont restes liens symboliques ne sont qu'en lecture seule deja)
		if (-f "$repertoire/$fichier"){
			chmod 0444, "$repertoire/$fichier";
			$nb++;
		}elsif(-d "$repertoire/$fichier"){
			$nb += &lecture_seule("$repertoire/$fichier");
		}
	}
	
	return $nb;
}