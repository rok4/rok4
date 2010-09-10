#!/usr/bin/perl -w 

use strict;
use Term::ANSIColor;
use Getopt::Std;
use XML::Simple;
use Cwd 'abs_path';
use cache(
	'cree_repertoires_recursifs',
);

our($opt_l,$opt_p);

# correspondance entre les repertoires de l'ancien cache et du nouveau
my %rep_ancien_nouveau;

################################################################################

################ MAIN
my $time = time();
my $log = "log_initialise_pyramide_$time.log";

open LOG, ">>$log" or die colored ("[INITIALISE_PYRAMIDE] Impossible de creer le fichier $log.", 'white on_red');
&ecrit_log("commande : @ARGV");

getopts("l:p:");

if ( ! defined ($opt_l and $opt_p ) ){
	print colored ("[INITIALISE_PYRAMIDE] Nombre d'arguments incorrect.", 'white on_red');
	print "\n\n";
	&usage();
	&ecrit_log("ERREUR Nombre d'arguments incorrect.");
	if(! defined $opt_l){
		print colored ("[INITIALISE_PYRAMIDE] Veuillez specifier un parametre -l.", 'white on_red');
		print "\n";
	}
	if(! defined $opt_p){
		print colored ("[INITIALISE_PYRAMIDE] Veuillez specifier un parametre -p.", 'white on_red');
		print "\n";
	}
	exit;
}
my $lay_ancien = $opt_l;
my $fichier_pyr = $opt_p;

# verification des parametres
if (! (-e $lay_ancien && -f $lay_ancien)){
	print colored ("[INITIALISE_PYRAMIDE] Le fichier $lay_ancien n'existe pas.", 'white on_red');
	print "\n";
	&ecrit_log("ERREUR Le fichier $lay_ancien n'existe pas.");
	exit;
}
if (! (-e $fichier_pyr && -f $fichier_pyr)){
	print colored ("[INITIALISE_PYRAMIDE] Le fichier $fichier_pyr n'existe pas.", 'white on_red');
	print "\n";
	&ecrit_log("ERREUR Le fichier $fichier_pyr n'existe pas.");
	exit;
}

# action 1 : determiner la pyramide la plus recente
&ecrit_log("Lecture de la configuration.");
print "[INITIALISE_PYRAMIDE] Lecture de la configuration.\n";
my $ancien_pyr = &lecture_lay($lay_ancien);
&ecrit_log("Pyramide la plus recente : $ancien_pyr.");
print "[INITIALISE_PYRAMIDE] Pyramide la plus recente : $ancien_pyr.\n";

# action 2 : acceder au cache et faire le lien entre anciennes et nouvelles dalles
my %level_rep_img_ancien;
my %level_rep_mtd_ancien;
if ( -e $ancien_pyr && -f $ancien_pyr ){
	&ecrit_log("Lecture de la pyramide $ancien_pyr.");
	print "[INITIALISE_PYRAMIDE] Lecture de la pyramide $ancien_pyr.\n";
	my ($ref_hash_images_ancien, $ref_hash_mtd_ancien) = &lecture_pyramide($ancien_pyr);
	%level_rep_img_ancien = %{$ref_hash_images_ancien};
	%level_rep_mtd_ancien = %{$ref_hash_mtd_ancien};
}else{
	&ecrit_log("ERREUR Le fichier $ancien_pyr n'existe pas.");
	print colored ("[INITIALISE_PYRAMIDE] Le fichier $ancien_pyr n'existe pas.", 'white on_red');
	print "\n";
	exit;
}
&ecrit_log("Lecture de la pyramide $fichier_pyr.");
print "[INITIALISE_PYRAMIDE] Lecture de la pyramide $fichier_pyr.\n";
my ($ref_hash_images_nouveau, $ref_hash_mtd_nouveau) = &lecture_pyramide($fichier_pyr);
my %level_rep_img_nouveau = %{$ref_hash_images_nouveau};
my %level_rep_mtd_nouveau = %{$ref_hash_mtd_nouveau};

# images
while ( my($level,$rep) = each %level_rep_img_nouveau ){
	if(defined $level_rep_img_ancien{"$level"}){
		my $rep_ancien = $level_rep_img_ancien{"$level"};
		$rep_ancien_nouveau{$rep_ancien} = $rep;
	}
}
# mtd
while ( my($level,$rep) = each %level_rep_mtd_nouveau ){
	if(defined $level_rep_mtd_ancien{"$level"}){
		my $rep_ancien = $level_rep_mtd_ancien{"$level"};
		$rep_ancien_nouveau{$rep_ancien} = $rep;
	}
}

# action 3 : lien-symboliquer les dalles de l'ancien cache vers le nouveau
&ecrit_log("Creation des liens symboliques entre ancien et nouveau cache.");
print "[INITIALISE_PYRAMIDE] Creation des liens symboliques entre ancien et nouveau cache.\n";
my $nombre = 0;
while( my ($repertoire_ancien_cache, $repertoire_nouveau_cache) = each %rep_ancien_nouveau ){
	# on cree les repertoires s'il n'existent pas
	&ecrit_log("Creation des repertoires manquants.");
	print "[INITIALISE_PYRAMIDE] Creation des repertoires manquants.\n";
	&cree_repertoires_recursifs($repertoire_nouveau_cache);
	$nombre += &cree_liens_syboliques_recursifs($repertoire_ancien_cache, $repertoire_nouveau_cache);
}

print colored ("[INITIALISE_PYRAMIDE] $nombre dalles de l'ancien cache copiees dans le nouveau.\n",'green');
&ecrit_log("Traitement termine.");

close LOG;
################################################################################

######## FONCTIONS
sub usage{
	my $bool_ok = 0;
	
	print colored ("\nUsage : \ninitialise_pyramide.pl -l path/fichier_layer.lay -p path/fichier_pyramide.pyr\n",'black on_white');
	print "\n\n";
	
	$bool_ok = 1;
	return $bool_ok;
}
################################################################################
sub lecture_lay{

	my $xml_lay = $_[0];
	my $path_fichier_pyramide_recente = "";
	
	my $xml_fictif = new XML::Simple(KeyAttr=>[]);

	# lire le fichier XML
	my $data = $xml_fictif->XMLin("$xml_lay");
	my $liste_pyramides = $data->{pyramidList};
	
	# pour tester quel type d'objet est reference (ou pas)
	my $ref = ref($liste_pyramides->{pyramid});
	
	if($ref eq "ARRAY" ){
		$path_fichier_pyramide_recente = $liste_pyramides->{pyramid}->[0];
	}else{
		$path_fichier_pyramide_recente = $liste_pyramides->{pyramid};
	}
	
	
	return $path_fichier_pyramide_recente;
}
################################################################################
sub ecrit_log{
	
	my $message = $_[0];
	
	my $bool_ok = 0;
	
	# machine sur Linux
    my $machine_utilisee = $ENV{'SYSMAC'};
	
	# largement inspire par P.PONS et gen_cache.pl
	my $T = localtime();
	printf LOG "$machine_utilisee %s %s\n", $T, $message;
	
	$bool_ok = 1;
	return $bool_ok;
}
################################################################################

sub cree_liens_syboliques_recursifs{
	
	my $rep_ini = $_[0];
	my $rep_fin = $_[1];
	
	my $nb_liens = 0;
	
	if( !(-e $rep_ini && -d $rep_ini) ){
		&ecrit_log("ERREUR Le repertoire $rep_ini n'existe pas.");
		print colored ("[INITIALISE_PYRAMIDE] Le repertoire de l'ancienne pyramide $rep_ini n'existe pas.", 'white on_red');
		print "\n";
		return $nb_liens;
	}
	
	opendir REP, $rep_ini or die colored ("[INITIALISE_PYRAMIDE] Impossible d'ouvrir le repertoire $rep_ini", 'white on_red');
	my @fichiers = readdir REP;
	closedir REP;
	foreach my $fic(@fichiers){
		next if ($fic =~ /^\.\.?$/);
		# si fichier => lien symbolique
		if( -f "$rep_ini/$fic" || -l "$rep_ini/$fic"){
			my $return = symlink("$rep_ini/$fic","$rep_fin/$fic");
			if ($return != 1){
				&ecrit_log("ERREUR a la creation du lien symbolique $rep_ini/$fic -> $rep_fin/$fic.");
			}else{
				$nb_liens += 1;
			}
			next;
		}elsif(-d "$rep_ini/$fic"){
			# si repertoire, on descend en prenant soin de verifier que le rep du nouveau cache existe
			if( !(-e "$rep_fin/$fic" && -d "$rep_fin/$fic") ){
				&ecrit_log("Creation du repertoire $rep_fin/$fic.");
				mkdir "$rep_fin/$fic", 0775 or die colored ("[INITIALISE_PYRAMIDE] Impossible de creer le repertoire $rep_fin/$fic.", 'white on_red');
			}
			$nb_liens += &cree_liens_syboliques_recursifs("$rep_ini/$fic", "$rep_fin/$fic");
			next;
		}
		
	}
	
	return $nb_liens;
	
}
################################################################################
sub lecture_pyramide{
	
	my $xml_pyramide = $_[0];
	
	my (%id_rep_images, %id_rep_mtd);
	
	my @refs_rep_levels;
	
	my $xml_fictif = new XML::Simple(KeyAttr=>[]);

	# lire le fichier XML
	my $data = $xml_fictif->XMLin("$xml_pyramide");
	
	foreach my $level (@{$data->{level}}){
		my $id = $level->{tileMatrix};
		# oblige car abs_path ne marche pas toujours
		my $rep1 = $level->{baseDir};
		if (substr($rep1, 0, 1) eq "/" ){
			$id_rep_images{"$id"} = $rep1;
		}else{
			$id_rep_images{"$id"} = abs_path($rep1);
		}
		my $metadata = $level->{metadata};
		my $rep2 = $metadata->{baseDir};
		if (substr($rep2, 0, 1) eq "/" ){
			$id_rep_mtd{"$id"} = $rep2;
		}else{
			$id_rep_mtd{"$id"} = abs_path($rep2);
		}
		
	}
	
	push(@refs_rep_levels, \%id_rep_images, \%id_rep_mtd);
	
	return @refs_rep_levels;
	
}
