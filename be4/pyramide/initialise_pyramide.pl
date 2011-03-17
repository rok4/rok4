#!/usr/bin/perl -w 

use strict;
use Getopt::Std;
use XML::Simple;
use XML::LibXML;
use File::Basename;
use Cwd;
use Cwd 'abs_path';
use cache(
	'cree_repertoires_recursifs',
	'$rep_logs_param',
	'lecture_repertoires_pyramide',
	'cherche_pyramide_recente_lay',
);

#### CONSTANTES
# repertoire ou mettre le log
my $rep_log = $rep_logs_param;

################################################################################

# pas de bufferisation des sorties ecran
$| = 1;

#### VARIABLES GLOBALES

# valeur des parametres de la ligne de commande
our($opt_l,$opt_p);

# chemin vers le fichier de layer concerne par la pyramide
my $lay_ancien;
# chemin vers le fichier XML de la pyramide (.pyr)
my $fichier_pyr;

# chemin vers le fichier XML de la pyramide precedente (.pyr)
my $ancien_pyr;
# association entre les repertoires de l'ancien cache et du nouveau sous forme de reference
my $ref_rep_ancien_nouveau;
################################################################################

################ MAIN

# verification des parametres et initialisation des variables globales
my $bool_init_ok = &init();
if($bool_init_ok == 0){
	exit;
}

# action 1 : determiner la pyramide la plus recente
$ancien_pyr = &extrait_fichier_pyramide($lay_ancien);

# action 2 : mise a jour des indices de tuiles limites dans les niveaux de la pyramide
&mise_a_jour_tms_limites_nouveau_pyr($ancien_pyr, $fichier_pyr);

# action 3 : acceder au cache et faire le lien entre anciennes et nouvelles dalles
$ref_rep_ancien_nouveau = &correspondance_ancien_nouveau($ancien_pyr, $fichier_pyr);

# action 4 : lien-symboliquer les dalles de l'ancien cache vers le nouveau
&liens_ancienne_pyramide_nouvelle_pyramide($ref_rep_ancien_nouveau);

&fin();
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
	
	my $rep_actuel = cwd();
	
	chdir "$rep_fin" or die colored ("[INITIALISE_PYRAMIDE] Impossible d'aller dans le repertoire $rep_fin",'white on_red');
	
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
			
			my $new_nom = "$rep_ini/$fic";
			my $dernier_new_nom = $new_nom;
			
			# on rebondit sur les differents liens
			# un lien symbolique pointe alors toujours sur un fichier physique et non pas sur un autre lien symbolique
			# boucle jusqu'a ce qu'on tombe sur un vrai fichier
			while(defined $new_nom){
				$dernier_new_nom = File::Spec->rel2abs($new_nom, dirname($dernier_new_nom));
				$new_nom = readlink($new_nom);
			}
			
			# chemin relatif du fichier pointe par rapport au repertoire du lien a creer
# 			my $chemin_lien_relatif = File::Spec->abs2rel($dernier_new_nom, $rep_fin);
			
			my $chemin_lien_relatif = File::Spec->abs2rel($dernier_new_nom, $rep_fin);
			# creation du lien symbolique (equivalent a la commande linux ln -l)
			my $return = symlink("$chemin_lien_relatif","$rep_fin/$fic");
			if ($return != 1){
				&ecrit_log("ERREUR a la creation du lien symbolique $rep_ini/$fic -> $rep_fin/$fic.");
			}else{
				$nb_liens += 1;
			}
# 			print "$rep_ini/$fic\n -> \n$rep_fin/$fic = $chemin_lien_relatif\n";
# 			exit;
			next;
		}elsif(-d "$rep_ini/$fic"){
			# si le fichier est en realite un repertoire, on descend en prenant soin de verifier que le rep
			# du nouveau cache existe (creation le cas echeant)
			if( !(-e "$rep_fin/$fic" && -d "$rep_fin/$fic") ){
				&ecrit_log("Creation du repertoire $rep_fin/$fic.");
				mkdir "$rep_fin/$fic", 0775 or die "[INITIALISE_PYRAMIDE] Impossible de creer le repertoire $rep_fin/$fic.";
			}
			$nb_liens += &cree_liens_syboliques_recursifs("$rep_ini/$fic", "$rep_fin/$fic");
			next;
		}
		
	}
	
	chdir "$rep_actuel" or die colored ("[INITIALISE_PYRAMIDE] Impossible d'aller dans le repertoire $rep_actuel",'white on_red');
	
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
	
	# boucle sur les balises <level> du .pyr
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
			print "[INITIALISE_PYRAMIDE] Le niveau $level_pyr n'a pas ete trouve dans $xml_pyr.\nLe TMS est-il bien en accord avec $xml_pyr ??\n";
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
################################################################################
# initialise le traitement : verification des parametres et initialisation des variables globales
sub init{
	my $time = time();
	# nom du log
	my $log = $rep_log."/log_initialise_pyramide_$time.log";
	
	# creation du log
	open LOG, ">>$log" or die "[INITIALISE_PYRAMIDE] Impossible de creer le fichier $log.";
	&ecrit_log("commande : @ARGV");
	
	# recuperation des parametres de la ligne de commande
	getopts("l:p:");
	
	my $bool_getopt_ok = 1;
	# sortie si tous les parametres obligatoires ne sont pas presents
	if ( ! defined ($opt_l and $opt_p ) ){
		$bool_getopt_ok = 0;
		print "[INITIALISE_PYRAMIDE] Nombre d'arguments incorrect.\n\n";
		&ecrit_log("ERREUR Nombre d'arguments incorrect.");
		if(! defined $opt_l){
			print "[INITIALISE_PYRAMIDE] Veuillez specifier un parametre -l.\n";
		}
		if(! defined $opt_p){
			print "[INITIALISE_PYRAMIDE] Veuillez specifier un parametre -p.\n";
		}
	}
	
	# on sort de la fonction si deja les parametres ne sont pas tous presents
	if ($bool_getopt_ok == 0){
		&usage();
		return $bool_getopt_ok;
	}
	
	# chemin vers le fichier de layer concerne par la pyramide
	$lay_ancien = $opt_l;
	# chemin vers le fichier XML de la pyramide (.pyr)
	$fichier_pyr = $opt_p;
	
	# verification des parametres
	my $bool_param_ok = 1;
	# sortie si le fichier de layer n'existe pas
	if (! (-e $lay_ancien && -f $lay_ancien)){
		print "[INITIALISE_PYRAMIDE] Le fichier $lay_ancien n'existe pas.\n";
		&ecrit_log("ERREUR Le fichier $lay_ancien n'existe pas.");
		$bool_param_ok = 0;
	}
	# sortie si le fichier XMl de pyramide n'existe pas
	if (! (-e $fichier_pyr && -f $fichier_pyr)){
		print "[INITIALISE_PYRAMIDE] Le fichier $fichier_pyr n'existe pas.\n";
		&ecrit_log("ERREUR Le fichier $fichier_pyr n'existe pas.");
		$bool_param_ok = 0;
	}
	
	# on retourne :
	# un booleen : 1 pour OK , 0 sinon
	return $bool_param_ok;
}
################################################################################
sub extrait_fichier_pyramide{
	
	# chemin vers le fichier de layer
	my $fichier_layer = $_[0];
	
	&ecrit_log("Lecture du fichier $fichier_layer.");
	# chemin vers le fichier XML de pyramide
	my $ancien_xml_pyr = &cherche_pyramide_recente_lay($fichier_layer);
	# test si le chemin est en absolu (commence par un /) ou en relatif
	# (dans ce cas on ajoute le repertoire du lay car le .pyr est en relatif par rapport au lay)
	if( $ancien_xml_pyr !~ /^\//){
		$ancien_xml_pyr = dirname($fichier_layer)."/".$ancien_xml_pyr;
	}
	&ecrit_log("Pyramide la plus recente du layer : $ancien_xml_pyr.");
	
	# test si ce fichier existe
	if ( !(-e $ancien_xml_pyr && -f $ancien_xml_pyr )){
		&ecrit_log("ERREUR Le fichier $ancien_xml_pyr n'existe pas.");
		print "[INITIALISE_PYRAMIDE] Le fichier $ancien_xml_pyr n'existe pas.\n";
		exit;
	}
	
	# on retourne :
	# chemin vers le XML de la pyramide precedente
	return $ancien_xml_pyr;
}
################################################################################
sub mise_a_jour_tms_limites_nouveau_pyr{

	# chemin vers le XML de la pyramide precedente
	my $xml_pyr_precedente = $_[0];
	# chemin vers le XML de la pyramide actuelle
	my $xml_pyr_actuelle = $_[1];
	
	&ecrit_log("Mise a jour des limites dans le TMS avec $xml_pyr_precedente.");
	# recuperation d'une reference vers le limites du TMS
	# (sous forme de hash associant l'id du level a une reference vers un tableau du type (x_min, x_max, y_min, y_max) )
	my $ref_tms_limits = &lecture_limites_tms_pyramide($xml_pyr_precedente);
	# mise a jour du fichier .pyr avec les limites du TMS
	&maj_limites_tms($xml_pyr_actuelle, $ref_tms_limits);
	
	
}
################################################################################
sub fin{
	
	&ecrit_log("Traitement termine.");
	# fermeture du handler du fichier de log
	close LOG;
	# sortie du programme
	exit;
}
################################################################################
sub liens_ancienne_pyramide_nouvelle_pyramide{
	
	# association entre les repertoires de l'ancien cache et du nouveau sous forme de reference
	my $ref_hash_ancien_nouveau = $_[0];
	# recuperation sous forme de vrai hash
	my %hash_ancien_nouveau = %{$ref_hash_ancien_nouveau};
	
	&ecrit_log("Creation des liens symboliques entre ancien et nouveau cache.");
	# nombre de liens symboliques crees entre la nouvelle pyramide et les donnes de l'ancienne pyramide
	my $nombre = 0;
	# boucle sur tous les couples cle-valeur du hash %rep_ancien_nouveau
	while( my ($repertoire_ancien_cache, $repertoire_nouveau_cache) = each %hash_ancien_nouveau ){
		# on cree d'abord les repertoires s'il n'existent pas (car on ne pourrait alors pas creer les liens symboliques)
		&ecrit_log("Creation des repertoires manquants.");
		&cree_repertoires_recursifs($repertoire_nouveau_cache);
		# creation des liens symboliques vers les fichiers de l'ancienne pyramide
		$nombre += &cree_liens_syboliques_recursifs($repertoire_ancien_cache, $repertoire_nouveau_cache);
	}
	&ecrit_log("$nombre dalles de l'ancien cache copiees dans le nouveau.");
	
}
################################################################################
sub correspondance_ancien_nouveau{
	
	# chemin vers le XML de la pyramide precedente
	my $xml_precedent = $_[0];
	# chemin vers le XML de la pyramide actuelle
	my $xml_actuel = $_[1];
	
	&ecrit_log("Lecture des repertoires de la pyramide $xml_precedent.");
	# association entre id du level et repertoire des images pour l'ancienne pyramide sous forme de reference
	# association entre id du level et repertoire des masques de mtd pour l'ancienne pyramide sous forme de reference
	my ($ref_hash_images_ancien, $ref_hash_mtd_ancien) = &lecture_repertoires_pyramide($xml_precedent);
	
	&ecrit_log("Lecture des repertoires de la pyramide $xml_actuel.");
	# association entre id du level et repertoire des images pour la nouvelle pyramide sous forme de reference
	# assosciation entre id du level et repertoire des masques de mtd pour la nouvelle pyramide sous forme de reference
	my ($ref_hash_images_nouveau, $ref_hash_mtd_nouveau) = &lecture_repertoires_pyramide($xml_actuel);
	
	# correspondance entre les repertoires image de l'ancien cache et du nouveau sous forme de reference
	my $ref_ancien_nouveau_image = &cree_correspondance($ref_hash_images_nouveau, $ref_hash_images_ancien);
	# correspondance entre les repertoires mtd de l'ancien cache et du nouveau sous forme de reference
	my $ref_ancien_nouveau_mtd = &cree_correspondance($ref_hash_mtd_nouveau, $ref_hash_mtd_ancien);
	
	# correspondance entre les repertoires de l'ancien cache et du nouveau
	my %hash_correspondance = (%{$ref_ancien_nouveau_image},%{$ref_ancien_nouveau_mtd});
	
	# on retourne :
	# reference a un hash de correspondance entre les repertoires de l'ancien cache et du nouveau
	return \%hash_correspondance;
}
################################################################################
sub cree_correspondance{
	
	# association entre id du level et repertoire pour la nouvelle pyramide sous forme de reference
	my $ref_hash_nouveau = $_[0];
	# recuperation sous forme de hash
	my %hash_nouveau = %{$ref_hash_nouveau};
	# association entre id du level et repertoire pour l'ancienne pyramide sous forme de reference
	my $ref_hash_ancien = $_[1];
	# recuperation sous forme de hash
	my %hash_ancien = %{$ref_hash_ancien};
	
	# correspondance entre les repertoires de l'ancien cache et du nouveau
	my %hash_correspondance_temp;
	
	# boucle sur tous les couples cle-valeur du hash %hash_nouveau
	while ( my($level,$rep) = each %hash_nouveau ){
		# si le level existe dans l'ancienne pyramide on cree la correspondance
		# entre rep image de l'ancienne pyramide et rep image de la nouvelle pyramide (pour le level donne)
		if(defined $hash_ancien{"$level"}){
			my $rep_ancien = $hash_ancien{"$level"};
			$hash_correspondance_temp{$rep_ancien} = $rep;
		}
	}
	
	# on retourne :
	# reference a un hash de correspondance entre les repertoires de l'ancien cache et du nouveau
	return \%hash_correspondance_temp;
}