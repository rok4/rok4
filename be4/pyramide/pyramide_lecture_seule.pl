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

# repertoire ou mettre le log
my $rep_log = $rep_logs_param;
# liste des repertoires de donnees de la pyramide
my @repertoires;
################################################################################

################ MAIN
my $time = time();
# nom du fichier de log
my $log = $rep_log."/log_pyramide_lecture_seule_$time.log";

open LOG, ">>$log" or die colored ("[PYRAMIDE_LECTURE_SEULE] Impossible de creer le fichier $log.", 'white on_red');
&ecrit_log("commande : @ARGV");

# recuperation des parametres de la commande
getopts("p:");

# sortie si le parametre obligatoire n'est pas present
if ( ! defined ($opt_p ) ){
	print colored ("[PYRAMIDE_LECTURE_SEULE] Nombre d'arguments incorrect.", 'white on_red');
	print "\n\n";
	&usage();
	&ecrit_log("ERREUR Nombre d'arguments incorrect.");
	print colored ("[PYRAMIDE_LECTURE_SEULE] Veuillez specifier un parametre -p.", 'white on_red');
	print "\n";
	exit;
}

# chemin vers le fichier XML de pyramide
my $fichier_pyr = $opt_p;
# sortie si le fichier XMl de pyramide n'existe pas (et n'est pas un fichier)
if (! (-e $fichier_pyr && -f $fichier_pyr)){
	print colored ("[PYRAMIDE_LECTURE_SEULE] Le fichier $fichier_pyr n'existe pas.", 'white on_red');
	print "\n";
	&ecrit_log("ERREUR Le fichier $fichier_pyr n'existe pas.");
	exit;
}

&ecrit_log("Recherche des repertoires de la pyramide de $fichier_pyr.");
# action 1 : trouver tous les repertoires image et masques de mtd
my ($ref_hash_rep_images, $ref_hash_rep_mtd) = &lecture_repertoires_pyramide($fichier_pyr);
# association entre id du level et repertoire image
my %level_rep_img = %{$ref_hash_rep_images};
# association entre id du level et repertoire des masques de mtd
my %level_rep_mtd = %{$ref_hash_rep_mtd};
# on met dans le liste des repertoires touts les repertoires d'images et tous les repertoires de masques de mtd (values des 2 hashes)
push (@repertoires, values %level_rep_img, values %level_rep_mtd);

# action 2 : mettre tous les fichiers en lecture seule 0444
&ecrit_log("Mise en lecture seule des fichiers");
# nombre de fichiers mis en lecture seule (initialise a 0)
my $nombre_fichiers = 0;
# nombre de repertoires mis en lecture seule (initialise a 0)
my $nombre_repertoires = 0;
# boucle sur tous les repertoires a mettre en lecture seule
foreach my $rep(@repertoires){
	# mise en lecture seule de ce repertoire
	my ($nombre_fichiers_temp, $nombre_repertoires_temp) = &lecture_seule($rep);
	# actualisation des nombres
	$nombre_fichiers += $nombre_fichiers_temp;
	$nombre_repertoires += $nombre_repertoires_temp
}
print colored ("[PYRAMIDE_LECTURE_SEULE] $nombre_fichiers fichiers et $nombre_repertoires repertoires mis en lecture seule.", 'green');
print "\n";
close LOG;

### FIN MAIN

######## FONCTIONS
################################################################################
sub usage{
	my $bool_ok = 0;
	
	print colored ("\nUsage : \npyramide_lecture_seule.pl -p path/fichier_pyramide.pyr\n",'black on_white');
	print "\n\n";
	
	$bool_ok = 1;
	return $bool_ok;
}
################################################################################
# ecrit un message dans le fichier de log
sub ecrit_log{
	
	# parametre : chaine de caracteres contenant le message a inscrire dans le log
	my $message = $_[0];
	
	my $bool_ok = 0;
	
	# recuperation du nom de la machine sur Linux
    chomp(my $machine_utilisee = `hostname`);
	
	# pour dater le message
	my $T = localtime();
	# exriture dans le log
	printf LOG "$machine_utilisee %s %s\n", $T, $message;
	
	$bool_ok = 1;
	return $bool_ok;
}
################################################################################
# mise en lecture seule recursivement de tous les fichiers et repertoires du repertoire parent en parametre
sub lecture_seule{
	
	# parametre : repertoire a mettre en lecture seule
	my $repertoire = $_[0];
	
	# nombre de fichiers mis en lecture seule
	my $nb_fic = 0;
	# nombre de repertoires mis en lecture seule
	my $nb_rep = 0;
	
	# ouverture du repertoire et stockage de la liste des fichiers dans un tableau
	opendir REP, "$repertoire" or die colored ("[PYRAMIDE_LECTURE_SEULE] Impossible d'ouvrir le repertoire $repertoire.", 'white on_red');
	my @fichiers = readdir REP;
	closedir REP;
	
	# boucle sur les fichiers du repertoire
	foreach my $fichier(@fichiers){
		# on elimine . et .. sinon on tourne a l'infini
		next if ($fichier =~ /^\.\.?$/);
		# on elimine les liens symboliques (qui s'ils sont restes liens symboliques ne sont qu'en lecture seule deja)
		# si c'est un fichier
		if (-f "$repertoire/$fichier"){
			# equivalent a la commande linux chmod
			chmod 0444, "$repertoire/$fichier";
			# incrementation du nombre de fichiers mis en lecture seule
			$nb_fic++;
		# si le fichier est en realite un repeetoire
		}elsif(-d "$repertoire/$fichier"){
			# on met en lecture seule ce sous-repertoire
			my ($nb_fic_temp, $nb_rep_temp) = &lecture_seule("$repertoire/$fichier");
			# actualisation des nombres
			$nb_fic += $nb_fic_temp;
			$nb_rep += $nb_rep_temp;
		}   
	}
	
	# on passe le repertoire lui-meme en lecture seule
	# 0555 et pas 0444 comme pour les fichiers car on doit pouvoir encore l'executer pour rentrer dedans!!
	chmod(0555, $repertoire);
	# incrementation du nombre de repertoires mis en lecture seule
	$nb_rep += 1;
	
	return ($nb_fic, $nb_rep);
}