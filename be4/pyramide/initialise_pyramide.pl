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
	'cherche_pyramide_recente_lay',
);
$| = 1;
our($opt_l,$opt_p);

# association entre les repertoires de l'ancien cache et du nouveau
my %rep_ancien_nouveau;
# repertoire ou mettre le log
my $rep_log = $rep_logs_param;
################################################################################

################ MAIN
my $time = time();
# nom du log
my $log = $rep_log."/log_initialise_pyramide_$time.log";

# creation du log
open LOG, ">>$log" or die "[INITIALISE_PYRAMIDE] Impossible de creer le fichier $log.";
&ecrit_log("commande : @ARGV");

# recuperation des parametres de la ligne de commande
getopts("l:p:");

# sortie si tous les parametres obligatoires ne sont pas presents
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

# chemin vers le fichier de layer concerne par la pyramide
my $lay_ancien = $opt_l;
# chemin vers le fichier XML de la pyramide (.pyr)
my $fichier_pyr = $opt_p;

# verification des parametres
# sortie si le fichier de layer n'existe pas
if (! (-e $lay_ancien && -f $lay_ancien)){
	print "[INITIALISE_PYRAMIDE] Le fichier $lay_ancien n'existe pas.\n";
	&ecrit_log("ERREUR Le fichier $lay_ancien n'existe pas.");
	exit;
}
# sortie si le fichier XMl de pyramide n'existe pas
if (! (-e $fichier_pyr && -f $fichier_pyr)){
	print "[INITIALISE_PYRAMIDE] Le fichier $fichier_pyr n'existe pas.\n";
	&ecrit_log("ERREUR Le fichier $fichier_pyr n'existe pas.");
	exit;
}

# action 1 : determiner la pyramide la plus recente
&ecrit_log("Lecture de la configuration.");
my $ancien_pyr = &cherche_pyramide_recente_lay($lay_ancien);
# test si le chemin est en absolu (commence par un /) ou en relatif
# (dans ce cas on ajoute le repertoire du lay car le .pyr est en relatif par rapport au lay)
if( $ancien_pyr !~ /^\//){
	$ancien_pyr = dirname($lay_ancien)."/".$ancien_pyr;
}
&ecrit_log("Pyramide la plus recente : $ancien_pyr.");

# action 2 : mise a jour des indices de tuiles limites dans les niveaux de la pyramide
&ecrit_log("Mise a jour des limites dans le TMS avec $ancien_pyr.");
# sortie si l'ancien fichier de pyramide (extrait du lay) n'existe pas
if ( -e $ancien_pyr && -f $ancien_pyr ){
	# recuperation d'une reference vers le limites du TMS
	# (sous forme de hash associant l'id du level a une reference vers un tableau du type (x_min, x_max, y_min, y_max) )
	my $ref_tms_limits = &lecture_limites_tms_pyramide($ancien_pyr);
	# mise a jour du fichier .pyr avec les limites du TMS
	&maj_limites_tms($fichier_pyr, $ref_tms_limits)
}else{
	&ecrit_log("ERREUR Le fichier $ancien_pyr n'existe pas.");
	print "[INITIALISE_PYRAMIDE] Le fichier $ancien_pyr n'existe pas.\n";
	exit;
}

# action 3 : acceder au cache et faire le lien entre anciennes et nouvelles dalles
&ecrit_log("Lecture des repertoires de la pyramide $ancien_pyr.");
my ($ref_hash_images_ancien, $ref_hash_mtd_ancien) = &lecture_repertoires_pyramide($ancien_pyr);
# association entre id du level et repertoire des images pour l'ancienne pyramide
my %level_rep_img_ancien = %{$ref_hash_images_ancien};
# association entre id du level et repertoire des masques de mtd pour l'ancienne pyramide
my %level_rep_mtd_ancien = %{$ref_hash_mtd_ancien};
&ecrit_log("Lecture des repertoires de la pyramide $fichier_pyr.");
my ($ref_hash_images_nouveau, $ref_hash_mtd_nouveau) = &lecture_repertoires_pyramide($fichier_pyr);
# association entre id du level et repertoire des images pour la nouvelle pyramide
my %level_rep_img_nouveau = %{$ref_hash_images_nouveau};
# assosciation entre id du level et repertoire des masques de mtd pour la nouvelle pyramide
my %level_rep_mtd_nouveau = %{$ref_hash_mtd_nouveau};

# images
# boucle sur tous les couples cle-valeur du hash %level_rep_img_nouveau
while ( my($level,$rep) = each %level_rep_img_nouveau ){
	# si le level existe dans l'ancienne pyramide on cree la correspondance
	# entre rep image de l'ancienne pyramide et rep image de la nouvelle pyramide (pour el level donne)
	if(defined $level_rep_img_ancien{"$level"}){
		my $rep_ancien = $level_rep_img_ancien{"$level"};
		$rep_ancien_nouveau{$rep_ancien} = $rep;
	}
}
# idem pour les masques de mtd
while ( my($level,$rep) = each %level_rep_mtd_nouveau ){
	if(defined $level_rep_mtd_ancien{"$level"}){
		my $rep_ancien = $level_rep_mtd_ancien{"$level"};
		$rep_ancien_nouveau{$rep_ancien} = $rep;
	}
}

# action 4 : lien-symboliquer les dalles de l'ancien cache vers le nouveau
&ecrit_log("Creation des liens symboliques entre ancien et nouveau cache.");
# nombre de liens symboliques crees entre la nouvelle pyramide et les donnes de l'ancienne pyramide
my $nombre = 0;
# boucle sur tous les couples cle-valeur du hash %rep_ancien_nouveau
while( my ($repertoire_ancien_cache, $repertoire_nouveau_cache) = each %rep_ancien_nouveau ){
	# on cree d'abord les repertoires s'il n'existent pas (car on ne pourrait alors pas creer les liens symboliques)
	&ecrit_log("Creation des repertoires manquants.");
	&cree_repertoires_recursifs($repertoire_nouveau_cache);
	# creation des liens symboliques vers les fichiers de l'ancienne pyramide
	$nombre += &cree_liens_syboliques_recursifs($repertoire_ancien_cache, $repertoire_nouveau_cache);
}

&ecrit_log("$nombre dalles de l'ancien cache copiees dans le nouveau.");
&ecrit_log("Traitement termine.");

# fermeture du handler du fichier de log
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
# inscrit un message dans le log
sub ecrit_log{
	
	# parametre : chaine de caracteres a inserer dans le fichier de log
	my $message = $_[0];
	
	my $bool_ok = 0;
	
	# recuperation du nom de la machine sur Linux
    chomp(my $machine_utilisee = `hostname`);
	
	# pour dater le log
	my $T = localtime();
	printf LOG "$machine_utilisee %s %s\n", $T, $message;
	
	$bool_ok = 1;
	return $bool_ok;
}
################################################################################
# cree des liens symboliques recursivement entre fichiers des repertoires parametres
sub cree_liens_syboliques_recursifs{
	
	# parametre : chemin vers le repertoire de fichiers de l'ancienne pyramide
	my $rep_ini = $_[0];
	# parametre : chemin vers le repertoire de fichiers de la nouvelle pyramide
	my $rep_fin = $_[1];
	
	# nombre de liens symboliques crees
	my $nb_liens = 0;
	
	# on sort de suite de la fonction si le repertoire initial n'existe pas
	if( !(-e $rep_ini && -d $rep_ini) ){
		&ecrit_log("ERREUR Le repertoire $rep_ini n'existe pas.");
		print "[INITIALISE_PYRAMIDE] Le repertoire de l'ancienne pyramide $rep_ini n'existe pas.\n";
		return $nb_liens;
	}
	
	# ouverture du repertoire initial et stockage de la liste de ses fichiers dans un tableau
	opendir REP, $rep_ini or die "[INITIALISE_PYRAMIDE] Impossible d'ouvrir le repertoire $rep_ini.";
	my @fichiers = readdir REP;
	closedir REP;
	# boucle sur les fichiers du repertoire
	foreach my $fic(@fichiers){
		# on saute . et .. sinon on boucle a l'infini
		next if ($fic =~ /^\.\.?$/);
		# si fichier est un fichier ou un deja un lien symbolique
		if( -f "$rep_ini/$fic" || -l "$rep_ini/$fic"){
			
			# on rebondit sur les differents liens
			# un lien symbolique pointe alors toujours sur un fichier physique et non pas sur un autre lien symbolique
			my $new_nom = "$rep_ini/$fic";
			my $dernier_new_nom;
			# boucle jusqu'a ce qu'on tombre sur un vrai fichier
			while(defined $new_nom){
				$dernier_new_nom = $new_nom;
				$new_nom = readlink($new_nom);
			}
			
			# creatio du lien symbolique (equivalent a la commande linux ln -l)
			my $return = symlink("$dernier_new_nom","$rep_fin/$fic");
			if ($return != 1){
				&ecrit_log("ERREUR a la creation du lien symbolique $rep_ini/$fic -> $rep_fin/$fic.");
			}else{
				$nb_liens += 1;
			}
			next;
		}elsif(-d "$rep_ini/$fic"){
			# si le fichier est en realite un reperttoire, on descend en prenant soin de verifier que le rep
			# du nouveau cache existe (creation el ca echeant)
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
# parsage du XML de pyramide pour en extraire ses limites dans le TMS
sub lecture_limites_tms_pyramide{
	
	# parametre : chemin vers le fichier XMl de pyramide (.pyr)
	my $fichier_pyramide = $_[0];
	
	# association entre id d'un level et reference vers un tableau contenant
	# les limites du level dans le TMS sous la forme (x_min, x_max, y_min, y_max)
	my %level_tms_limits;
	
	# stockage du contenu du XMl de pyramide dans une varaible (utilisation du module XML::Simple)
	my $xml_fictif2 = new XML::Simple(KeyAttr=>[]);

	# lire le fichier XML
	my $data = $xml_fictif2->XMLin("$fichier_pyramide");
	
	# boucle sur les balises <level> dy .pyr
	foreach my $level (@{$data->{level}}){
		# contenu de la balise <tileMatrix>
		my $id = $level->{tileMatrix};
		# contenu de la balise <TMSLimits>
		my $tms_limits = $level->{TMSLimits};
		# contenu de la balise <minTileRow> de la balise <TMSLimits>
		my $tile_min_x = $tms_limits->{minTileRow};
		# contenu de la balise <minTileCol> de la balise <TMSLimits>
		my $tile_min_y = $tms_limits->{minTileCol};
		# contenu de la balise <maxTileRow> de la balise <TMSLimits>
		my $tile_max_x = $tms_limits->{maxTileRow};
		# contenu de la balise <maxTileCol> de la balise <TMSLimits>
		my $tile_max_y = $tms_limits->{maxTileCol};
		
		# creation d'un tableau contenant les limites
		my @limits = ($tile_min_x, $tile_max_x, $tile_min_y, $tile_max_y);
		
		# association a l'id du level en question
		$level_tms_limits{"$id"} = \@limits;
		
	}
	
	return \%level_tms_limits;
}
################################################################################
# parsage du XML de pyramide pour y mettre a jour les informations sur ses limites dans le TMS
sub maj_limites_tms{
	
	# parametre : chemin vers le XML de pyramide (.pyr)
	my $xml_pyr = $_[0];
	# parametre : reference vers un hash associant a un id de level une reference vers un tableau contenant les limites du level dans le TMS
	my $ref_limites_tms = $_[1];
	
	# recuperation du parametre sous forme de hash
	my %hash_level_ref_limites = %{$ref_limites_tms};
	
	my $bool_ok = 0;
	
	# utilisation de xml::libxml car xml::simple deteriore l'ordre des noeuds
	# stockage du contenu du XMl de pyramide dans une variable
	my $parser = XML::LibXML->new();
	my $doc = $parser->parse_file("$xml_pyr");
	
	# remplacement dans la structure memorisee
	# boucle sur les levels
	foreach my $level_pyr(keys %hash_level_ref_limites){
		# on sait qu'il n'y a qu'un TMSLimits, qu'un minTileRow...
		# contenu de la balise <level><TMSLimits> dont le conteu de la balise <tileMatrix> est le level
		# cela devient le contexte XPath
		my $node_limits = $doc->find("//level[tileMatrix = '$level_pyr']/TMSLimits")->get_node(0);
		if( ! defined $node_limits){
			print "[INITIALISE_PYRAMIDE] Le niveau $level_pyr n'a pas ete trouve dans $xml_pyr.\nLe TMS est-il bien en accord avec $xml_pyr ??.\n";
			&ecrit_log("Le niveau $level_pyr n'a pas ete trouve dans $xml_pyr.");
			exit;
		}
		# contenu de la balise <minTileRow>
		my $node_min_x_src = $node_limits->find("./minTileRow/text()")->get_node(0);
		# contenu de la balise <maxTileRow>
		my $node_max_x_src = $node_limits->find("./maxTileRow/text()")->get_node(0);
		# contenu de la balise <minTileCol>
		my $node_min_y_src = $node_limits->find("./minTileCol/text()")->get_node(0);
		# contenu de la balise <maxTileCol>
		my $node_max_y_src = $node_limits->find("./maxTileCol/text()")->get_node(0);
		
		# recuperation des anciennes limites
 		my ($tile_min_x_ancien, $tile_max_x_ancien, $tile_min_y_ancien, $tile_max_y_ancien) = @{$hash_level_ref_limites{"$level_pyr"}};
		# mise a jour de l'info (si elle n'etait pas correcte)
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
	# transformation de la variable contenant le XML de pyramide mis a jour en fichier (ecrasement du fichier existant ici) 
	print PYR $doc->toString;
	close PYR;
	
	$bool_ok = 1;
	return $bool_ok;
}