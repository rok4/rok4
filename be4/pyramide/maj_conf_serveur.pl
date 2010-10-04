#!/usr/bin/perl -w

use strict;
use Term::ANSIColor;
use Getopt::Std;
use File::Copy;
use Cwd 'abs_path';
use cache(
	'$rep_logs_param',
);
$| = 1;
our($opt_l,$opt_p);

my $rep_log = $rep_logs_param;
################################################################################

################ MAIN
my $time = time();
my $log = $rep_log."/log_maj_conf_serveur_$time.log";

open LOG, ">>$log" or die colored ("[MAJ_CONF_SERVEUR] Impossible de creer le fichier $log.", 'white on_red');
&ecrit_log("commande : @ARGV");

getopts("l:p:");

if ( ! defined ($opt_l and $opt_p ) ){
	print colored ("[MAJ_CONF_SERVEUR] Nombre d'arguments incorrect.", 'white on_red');
	print "\n\n";
	&usage();
	&ecrit_log("ERREUR Nombre d'arguments incorrect.");
	if(! defined $opt_l){
		print colored ("[MAJ_CONF_SERVEUR] Veuillez specifier un parametre -l.", 'white on_red');
		print "\n";
	}
	if(! defined $opt_p){
		print colored ("[MAJ_CONF_SERVEUR] Veuillez specifier un parametre -p.", 'white on_red');
		print "\n";
	}
	exit;
}
my $lay = $opt_l;
my $fichier_pyr = $opt_p;

# verification des parametres
if (! (-e $lay && -f $lay)){
	print colored ("[MAJ_CONF_SERVEUR] Le fichier $lay n'existe pas.", 'white on_red');
	print "\n";
	&ecrit_log("ERREUR Le fichier $lay n'existe pas.");
	exit;
}
if (! (-e $fichier_pyr && -f $fichier_pyr)){
	print colored ("[MAJ_CONF_SERVEUR] Le fichier $fichier_pyr n'existe pas.", 'white on_red');
	print "\n";
	&ecrit_log("ERREUR Le fichier $fichier_pyr n'existe pas.");
	exit;
}

# action 1 : copie de l'ancien lay pour rollback eventuel
my $lay_ancien = $lay.".old";
&ecrit_log("Copie de $lay vers $lay_ancien.");
my $return = copy($lay, $lay_ancien);
if ($return == 0){
	&ecrit_log("ERREUR a la copie de $lay vers $lay_ancien");
}

# action 2 : supprimer le lay pour le recreer ensuite
&ecrit_log("Suppression de $lay.");
my $suppr = unlink($lay);
if($suppr != 1){
	&ecrit_log("ERREUR a la destruction de $lay.");
}

# action 3 : lire le fichier lay ancien
&ecrit_log("Lecture de $lay_ancien.");
open LAY_ANCIEN, "<$lay_ancien" or die colored ("[MAJ_CONF_SERVEUR] Impossible de lire le fichier $lay_ancien.", 'white on_red');
my @lignes_lay_ancien = <LAY_ANCIEN>;
close LAY_ANCIEN;

# action 3 : reeecrire le lay mis a jour
&ecrit_log("Creation et mise a jour de $lay.");
my $absolu_pyramide = abs_path($fichier_pyr);
open LAY_NOUVEAU, ">$lay" or die colored ("[MAJ_CONF_SERVEUR] Impossible de creer le fichier $lay.", 'white on_red');
my $bool_maj = 0;
foreach my $ligne_ancien(@lignes_lay_ancien){
	chomp(my $ligne_nouveau = $ligne_ancien);
	my $bool_print = 1;
	if($ligne_ancien =~ /^\s*<pyramid>(.*)<\/pyramid>\s*/){
		# si on l'a deja mis a jour, on ne recopie pas les autres pyramides du layer
		if($bool_maj == 1){
			$bool_print = 0;
		}else{
			$ligne_nouveau =~ s/$1/$absolu_pyramide/;
			$bool_maj = 1;
		}
		
	}
	if($bool_print == 1){
		print LAY_NOUVEAU "$ligne_nouveau\n";
	}
	
}
close LAY_NOUVEAU;
print colored ("[MAJ_CONF_SERVEUR] Fichier $lay mis a jour avec $absolu_pyramide.", 'green');
print "\n";
close LOG;
################################################################################

######## FONCTIONS
sub usage{
	my $bool_ok = 0;
	
	print colored ("\nUsage : \nmaj_conf_serveur.pl -l path/fichier_layer.lay -p path/fichier_pyramide.pyr\n",'black on_white');
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