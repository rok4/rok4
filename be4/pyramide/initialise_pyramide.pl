#!/usr/bin/perl -w 

use strict;
use Getopt::Std;
use XML::Simple;
use XML::LibXML;
use File::Basename;
use Cwd 'abs_path';
use cache(
	'cree_repertoires_recursifs',
	'$rep_logs_param',
	'lecture_repertoires_pyramide',
);
$| = 1;
our($opt_l,$opt_p);

# correspondance entre les repertoires de l'ancien cache et du nouveau
my %rep_ancien_nouveau;
my $rep_log = $rep_logs_param;
################################################################################

################ MAIN
my $time = time();
my $log = $rep_log."/log_initialise_pyramide_$time.log";

open LOG, ">>$log" or die "[INITIALISE_PYRAMIDE] Impossible de creer le fichier $log.";
&ecrit_log("commande : @ARGV");

getopts("l:p:");

if ( ! defined ($opt_l and $opt_p ) ){
	print "[INITIALISE_PYRAMIDE] Nombre d'arguments incorrect.\n\n";
	&usage();
	&ecrit_log("ERREUR Nombre d'arguments incorrect.");
	if(! defined $opt_l){
		print "[INITIALISE_PYRAMIDE] Veuillez specifier un parametre -l.\n";
	}
	if(! defined $opt_p){
		print "[INITIALISE_PYRAMIDE] Veuillez specifier un parametre -p.\n";
	}
	exit;
}
my $lay_ancien = $opt_l;
my $fichier_pyr = $opt_p;

# verification des parametres
if (! (-e $lay_ancien && -f $lay_ancien)){
	print "[INITIALISE_PYRAMIDE] Le fichier $lay_ancien n'existe pas.\n";
	&ecrit_log("ERREUR Le fichier $lay_ancien n'existe pas.");
	exit;
}
if (! (-e $fichier_pyr && -f $fichier_pyr)){
	print "[INITIALISE_PYRAMIDE] Le fichier $fichier_pyr n'existe pas.\n";
	&ecrit_log("ERREUR Le fichier $fichier_pyr n'existe pas.");
	exit;
}

# action 1 : determiner la pyramide la plus recente
&ecrit_log("Lecture de la configuration.");
my $ancien_pyr = &lecture_lay($lay_ancien);
# test si le chemin est en absolu ou en relatif (dans ce cas on ajoute le repertoire du lay)
if( $ancien_pyr !~ /^\//){
	$ancien_pyr = dirname($lay_ancien)."/".$ancien_pyr;
}
&ecrit_log("Pyramide la plus recente : $ancien_pyr.");

# action 2 : mise a jour des indices de tuiles limites dans les niveaux de la pyramide
&ecrit_log("Mise a jour des limites dans le TMS avec $ancien_pyr.");
if ( -e $ancien_pyr && -f $ancien_pyr ){
	my $ref_tms_limits = &lecture_limites_tms_pyramide($ancien_pyr);
	&maj_limites_tms($fichier_pyr, $ref_tms_limits)
}else{
	&ecrit_log("ERREUR Le fichier $ancien_pyr n'existe pas.");
	print "[INITIALISE_PYRAMIDE] Le fichier $ancien_pyr n'existe pas.\n";
	exit;
}

# action 3 : acceder au cache et faire le lien entre anciennes et nouvelles dalles
&ecrit_log("Lecture des repertoires de la pyramide $ancien_pyr.");
my ($ref_hash_images_ancien, $ref_hash_mtd_ancien) = &lecture_repertoires_pyramide($ancien_pyr);
my %level_rep_img_ancien = %{$ref_hash_images_ancien};
my %level_rep_mtd_ancien = %{$ref_hash_mtd_ancien};
&ecrit_log("Lecture des repertoires de la pyramide $fichier_pyr.");
my ($ref_hash_images_nouveau, $ref_hash_mtd_nouveau) = &lecture_repertoires_pyramide($fichier_pyr);
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

# action 4 : lien-symboliquer les dalles de l'ancien cache vers le nouveau
&ecrit_log("Creation des liens symboliques entre ancien et nouveau cache.");
my $nombre = 0;
while( my ($repertoire_ancien_cache, $repertoire_nouveau_cache) = each %rep_ancien_nouveau ){
	# on cree les repertoires s'il n'existent pas
	&ecrit_log("Creation des repertoires manquants.");
	&cree_repertoires_recursifs($repertoire_nouveau_cache);
	$nombre += &cree_liens_syboliques_recursifs($repertoire_ancien_cache, $repertoire_nouveau_cache);
}

&ecrit_log("$nombre dalles de l'ancien cache copiees dans le nouveau.");
&ecrit_log("Traitement termine.");

close LOG;
################################################################################

######## FONCTIONS
sub usage{
	my $bool_ok = 0;
	
	print "\nUsage : \ninitialise_pyramide.pl -l path/fichier_layer.lay -p path/fichier_pyramide.pyr\n";
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
	
	if($ref eq "ARRAY"){
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
    chomp(my $machine_utilisee = `hostname`);
	
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
		print "[INITIALISE_PYRAMIDE] Le repertoire de l'ancienne pyramide $rep_ini n'existe pas.\n";
		return $nb_liens;
	}
	
	opendir REP, $rep_ini or die "[INITIALISE_PYRAMIDE] Impossible d'ouvrir le repertoire $rep_ini.";
	my @fichiers = readdir REP;
	closedir REP;
	foreach my $fic(@fichiers){
		next if ($fic =~ /^\.\.?$/);
		# si fichier => lien symbolique
		if( -f "$rep_ini/$fic" || -l "$rep_ini/$fic"){
			
			# on rebondit sur les differents liens
			my $new_nom = "$rep_ini/$fic";
			my $dernier_new_nom;
			while(defined $new_nom){
				$dernier_new_nom = $new_nom;
				$new_nom = readlink($new_nom);
			}

			my $return = symlink("$dernier_new_nom","$rep_fin/$fic");
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
				mkdir "$rep_fin/$fic", 0775 or die "[INITIALISE_PYRAMIDE] Impossible de creer le repertoire $rep_fin/$fic.";
			}
			$nb_liens += &cree_liens_syboliques_recursifs("$rep_ini/$fic", "$rep_fin/$fic");
			next;
		}
		
	}
	
	return $nb_liens;
	
}
################################################################################
sub lecture_limites_tms_pyramide{
	
	my $fichier_pyramide = $_[0];
	
	my %level_tms_limits;
	
	my $xml_fictif2 = new XML::Simple(KeyAttr=>[]);

	# lire le fichier XML
	my $data = $xml_fictif2->XMLin("$fichier_pyramide");
	
	foreach my $level (@{$data->{level}}){
		my $id = $level->{tileMatrix};
		my $tms_limits = $level->{TMSLimits};
		my $tile_min_x = $tms_limits->{minTileRow};
		my $tile_min_y = $tms_limits->{minTileCol};
		my $tile_max_x = $tms_limits->{maxTileRow};
		my $tile_max_y = $tms_limits->{maxTileCol};
		
		my @limits = ($tile_min_x, $tile_max_x, $tile_min_y, $tile_max_y);
		
		$level_tms_limits{"$id"} = \@limits;
		
	}
	
	
	return \%level_tms_limits;
}
################################################################################
sub maj_limites_tms{
	
	my $xml_pyr = $_[0];
	my $ref_limites_tms = $_[1];
	
	my %hash_level_ref_limites = %{$ref_limites_tms};
	
	my $bool_ok = 0;
	
	# utilisation de xml::libxml car xml::simple deteriore l'ordre des noeuds
	my $parser = XML::LibXML->new();
	my $doc = $parser->parse_file("$xml_pyr");
	
	# remplacement dans la structure memorisee
	foreach my $level_pyr(keys %hash_level_ref_limites){
		# on sait qu'il n'y a qu'un TMSLimits, qu'un minTileRow...
		my $node_limits = $doc->find("//level[tileMatrix = '$level_pyr']/TMSLimits")->get_node(0);
		my $node_min_x_src = $node_limits->find("./minTileRow/text()")->get_node(0);
		my $node_max_x_src = $node_limits->find("./maxTileRow/text()")->get_node(0);
		my $node_min_y_src = $node_limits->find("./minTileCol/text()")->get_node(0);
		my $node_max_y_src = $node_limits->find("./maxTileCol/text()")->get_node(0);
		
 		my ($tile_min_x_ancien, $tile_max_x_ancien, $tile_min_y_ancien, $tile_max_y_ancien) = @{$hash_level_ref_limites{"$level_pyr"}};
		# mis e a jour de l'info
		if($node_min_x_src->textContent > $tile_min_x_ancien ){
			$node_min_x_src->setData("$tile_min_x_ancien");
		}
		if($node_max_x_src->textContent < $tile_max_x_ancien ){
			$node_max_x_src->setData("$tile_max_x_ancien");
		}
		if($node_min_y_src->textContent > $tile_min_y_ancien ){
			$node_min_y_src->setData("$tile_min_y_ancien");
		}
		if($node_max_y_src->textContent < $tile_max_y_ancien ){
			$node_max_y_src->setData("$tile_max_y_ancien");
		}
		
	}
	
	# remplacement dans le fichier
	open PYR,">$xml_pyr" or die "[INITIALISE_PYRAMIDE] Impossible d'ouvrir le fichier $xml_pyr.";
	print PYR $doc->toString;
	close PYR;
	
	$bool_ok = 1;
	return $bool_ok;
}
