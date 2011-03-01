#!/usr/bin/perl -w

use strict;
use Term::ANSIColor;
use Getopt::Std;
use File::Copy;
use XML::LibXML;
use POSIX qw(ceil floor);
use List::Util qw( min max );
use Cwd 'abs_path';
use cache(
	'$rep_logs_param',
	'reproj_point',
	'$programme_reproj_param',
	'extrait_tms_from_pyr',
	'$srs_wgs84g_param',
);
$| = 1;
our($opt_l,$opt_p);

# repertoire ou mettre le log
my $rep_log = $rep_logs_param;
# nom du programme de reprojection de coordonnees
my $programme_reproj = $programme_reproj_param;
# systeme de coordonnees de l'emprise dans le layer
my $srs_wgs84g = $srs_wgs84g_param;
################################################################################
# verifiaction de l'existence du programe de reprojection
my $verif_programme_reproj = `which $programme_reproj`;
# sortie si le programme de reprojection n'existe pas
if ($verif_programme_reproj eq ""){
	print "[MAJ_CONF_SERVEUR] Le programme $programme_reproj est introuvable.\n";
	exit;
}
################ MAIN
my $time = time();
# nom du fichier de log
my $log = $rep_log."/log_maj_conf_serveur_$time.log";

# creation du fichier de log
open LOG, ">>$log" or die colored ("[MAJ_CONF_SERVEUR] Impossible de creer le fichier $log.", 'white on_red');
&ecrit_log("commande : @ARGV");

# recuperation des parametres de la ligne de commande
getopts("l:p:");

# sortie si les parametres obligatoires ne sont pas presents
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

# chemin vers le fichier XML de layer
my $lay = $opt_l;
# chemin vers le fichier XML de pyramide
my $fichier_pyr = $opt_p;

# verification des parametres
# sortie si le fichier XML de layer n'existe pas
if (! (-e $lay && -f $lay)){
	print colored ("[MAJ_CONF_SERVEUR] Le fichier $lay n'existe pas.", 'white on_red');
	print "\n";
	&ecrit_log("ERREUR Le fichier $lay n'existe pas.");
	exit;
}
# sortie si le fichier XML de pyramide n'existe pas
if (! (-e $fichier_pyr && -f $fichier_pyr)){
	print colored ("[MAJ_CONF_SERVEUR] Le fichier $fichier_pyr n'existe pas.", 'white on_red');
	print "\n";
	&ecrit_log("ERREUR Le fichier $fichier_pyr n'existe pas.");
	exit;
}

# action 1 : copie de l'ancien lay pour rollback eventuel
# nom du layer renomme
my $lay_ancien = $lay.".old";
&ecrit_log("Copie de $lay vers $lay_ancien.");
# copie du layer actuel en layer renomme (pour un rollback eventuel : on pourra revenir a l'etat precedent)
my $return = copy($lay, $lay_ancien);
if ($return == 0){
	&ecrit_log("ERREUR a la copie de $lay vers $lay_ancien");
}

#action 2 :mettre a jour le lay
&ecrit_log("Mise a jour de $lay");
# mise a jour des information des le XML de layer
my $bool_maj = &maj_pyr_et_bounding_box($lay, $fichier_pyr);

if($bool_maj == 1){
	print "[MAJ_CONF_SERVEUR] Fichier $lay mis a jour avec pyramide et rectangle englobant.\n";
}else{
	print "[MAJ_CONF_SERVEUR] Des erreurs se sont produites, consulter $log.\n";
	# on revient en arriere : $lay n'a pas encore ete modifie
	# suppression de la copie faite precedemment
	unlink($lay_ancien);
}
# fermeture du hadler du fichier de log
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
# insere un message dans le fihcier de log
sub ecrit_log{
	
	# parametre : chaine de caractere a inserer dans le log
	my $message = $_[0];
	
	my $bool_ok = 0;
	
	# recuperation du nom de la machine sur Linux
    chomp(my $machine_utilisee = `hostname`);
	
	# pour dater le message
	my $T = localtime();
	# inscription dans le fichier de log
	printf LOG "$machine_utilisee %s %s\n", $T, $message;
	
	$bool_ok = 1;
	return $bool_ok;
}
################################################################################
# parsage du fichier XML de layer pour mettre a jour des informations
sub maj_pyr_et_bounding_box{
	
	# parametre : chemin vers le fichier XML de layer 
	my $xml_lay = $_[0];
	# parametre : chemin vers le fichier XML de pyramide
	my $nom_fichier_pyr = $_[1];
	
	my $bool_ok = 0;
	
	#0. mise a jour du chemin de la pyramide dans le lay
	# stockage du contenu du XML de layer dans une variable (utilisation du module XML::LibXML)
	my $parser_lay = XML::LibXML->new();
	my $doc_lay = $parser_lay->parse_file("$xml_lay");
	
	my $absolu_pyramide = abs_path($nom_fichier_pyr);
	# mise a jour du contenu de la balise <pyramidList><pyramid> du layer
	$doc_lay->find("//pyramidList/pyramid/text()")->get_node(0)->setData($absolu_pyramide);
	
	#1. recuperation du TMS dans le pyr
	my $tms_complet = &extrait_tms_from_pyr($nom_fichier_pyr);
	
	#2. recuperation du level, de la proj, des origines et tailles des tuiles en resolution min dans le TMS
	# stockage du contenu du fichier XML de TMS dans une variable (utilisation du module XML::LibXML)
	my $parser_tms = XML::LibXML->new();
	my $doc_tms = $parser_tms->parse_file("$tms_complet");
	
	# resolution min du TMS
	# liste des contenus des balises <tileMatrix><resolution> du TMS
	my @nodes_resolution = $doc_tms->findnodes("//tileMatrix/resolution")->get_nodelist();
	my $res_min = 99999999999;
	# boucles sur les resolutions trouvees pour en extraire la plus petite
	foreach my $resolution(@nodes_resolution){
		if($resolution->textContent < $res_min){
			$res_min = $resolution->textContent;
		}
	}
	
	# contenu de la balise <crs> du TMS
	my $proj = $doc_tms->find("//crs")->get_node(0)->textContent;
	
	# contenu de la balise <tileMatrix> du TMS qui a la plus petite resolution : cette balise TM devient le contexte XPath
	my $tile_matrix = $doc_tms->find("//tileMatrix[resolution = '$res_min']")->get_node(0);
	
	# contenu de la balise <id>
	my $level = $tile_matrix->find("./id")->get_node(0)->textContent;
	# floor / ceil / int servent a caster en chiffre puisque les espaces sont possibles
	# contenu de la balise <topLeftCornerX>
	my $origine_x = floor($tile_matrix->find("./topLeftCornerX")->get_node(0)->textContent);
	# contenu de la balise <topLeftCornerY>
	my $origine_y = ceil($tile_matrix->find("./topLeftCornerY")->get_node(0)->textContent);
	# contenu de la balise <tileWidth>
	my $taille_tuile_pix_x = int($tile_matrix->find("./tileWidth")->get_node(0)->textContent);
	# contenu de la balise <tileHeight>
	my $taille_tuile_pix_y = int($tile_matrix->find("./tileHeight")->get_node(0)->textContent);
	
	#3. recuperation des limites TMS dans le pyr sur le level concerne
	# stockage du contenu du XML de pyramide dans une variable (utilisation du module XML::LibXML)
	my $parser_pyr = XML::LibXML->new();
 	my $doc_pyr = $parser_pyr->parse_file("$nom_fichier_pyr");
	
	# contenu de la balise <level><TMSLimits> du XML de pyramide dont la balise <tileMatrix> a pour valeur le nom du level a resolutino minimum du TMS
	# cette balise devient le contexte XPath
	my $tms_limits = $doc_pyr->find("//level[tileMatrix = '$level']/TMSLimits")->get_node(0);
	# contenu de la balise <minTileRow>
	my $min_tile_x = $tms_limits->find("./minTileRow")->get_node(0)->textContent;
	# contenu de la balise <maxTileRow>
	my $max_tile_x = $tms_limits->find("./maxTileRow")->get_node(0)->textContent;
	# contenu de la balise <minTileCol>
	my $min_tile_y = $tms_limits->find("./minTileCol")->get_node(0)->textContent;
	# contenu de la balise <maxTileCol>
	my $max_tile_y = $tms_limits->find("./maxTileCol")->get_node(0)->textContent;
	
	#4. determination de la bbox en proj $proj
	my $x_min_bbox = ($min_tile_x * $res_min * $taille_tuile_pix_x) + $origine_x;
	my $x_max_bbox = ($max_tile_x * $res_min * $taille_tuile_pix_x) + $origine_x;
	my $y_min_bbox = $origine_y - ($max_tile_y * $res_min * $taille_tuile_pix_y);
	my $y_max_bbox = $origine_y - ($min_tile_y * $res_min * $taille_tuile_pix_y);
	
	#5. transformation en WGS84G pour l'info EX_GeographicBoundingBox au degre pres
	my ($top_left_corner_x_g, $top_left_corner_y_g) = &reproj_point($x_min_bbox, $y_max_bbox, $proj, $srs_wgs84g);
	my ($top_right_corner_x_g, $top_right_corner_y_g) = &reproj_point($x_max_bbox, $y_max_bbox, $proj, $srs_wgs84g);
	my ($bottom_left_corner_x_g, $bottom_left_corner_y_g) = &reproj_point($x_min_bbox, $y_min_bbox, $proj, $srs_wgs84g);
	my ($bottom_right_corner_x_g, $bottom_right_corner_y_g) = &reproj_point($x_max_bbox, $y_min_bbox, $proj, $srs_wgs84g);
	
	# on ne teste que les x car reproj est implemente comme ca, si erreur x et y en erreur
	if(!($top_left_corner_x_g ne "erreur" && $top_right_corner_x_g ne "erreur" && $bottom_left_corner_x_g ne "erreur" && $bottom_right_corner_x_g ne "erreur")){
		print "[MAJ_CONF_SERVEUR] ERREUR a la reprojection de $proj en $srs_wgs84g.\n";
		&ecrit_log("ERREUR a la reprojection de $proj en $srs_wgs84g.");
		return $bool_ok;
	}
	# teste si les coordonnees ne sont pas hors champ : le programme de reprojection renvoie * dans ce cas la
	if(!($top_left_corner_x_g ne "*" && $top_right_corner_x_g ne "*" && $bottom_left_corner_x_g ne "*" && $bottom_right_corner_x_g ne "*" && $top_left_corner_y_g ne "*" && $top_right_corner_y_g ne "*" && $bottom_left_corner_y_g ne "*" && $bottom_right_corner_y_g ne "*" )){
		print "[MAJ_CONF_SERVEUR] ERREUR a la reprojection de $proj en $srs_wgs84g, coordonnees potentiellement hors champ.\n";
		&ecrit_log("ERREUR a la reprojection de $proj en $srs_wgs84g, coordonnees potentiellement hors champ.");
		return $bool_ok;
	}
	
	# arrondi au degre pres des 4 valeurs
	my $x_min_g = floor(min($top_left_corner_x_g, $top_right_corner_x_g, $bottom_left_corner_x_g, $bottom_right_corner_x_g));
	my $x_max_g = ceil(max($top_left_corner_x_g, $top_right_corner_x_g, $bottom_left_corner_x_g, $bottom_right_corner_x_g));
	my $y_min_g = floor(min($top_left_corner_y_g, $top_right_corner_y_g, $bottom_left_corner_y_g, $bottom_right_corner_y_g));
	my $y_max_g = ceil(max($top_left_corner_y_g, $top_right_corner_y_g, $bottom_left_corner_y_g, $bottom_right_corner_y_g));
	
	#6. mise a jour dans le layer
	# mise a jour du contenu de la balise <EX_GeographicBoundingBox> du layer : cette balise devient le contexte XPath
	my $geographic_bbox = $doc_lay->find("//EX_GeographicBoundingBox")->get_node(0);
	
	# mise a jour du contenu de la balise <westBoundLongitude>
	my $node_x_min_g = $geographic_bbox->find("./westBoundLongitude/text()")->get_node(0);
	$node_x_min_g->setData("$x_min_g");
	# mise a jour du contenu de la balise <eastBoundLongitude>
	my $node_x_max_g = $geographic_bbox->find("./eastBoundLongitude/text()")->get_node(0);
	$node_x_max_g->setData("$x_max_g");
	# mise a jour du contenu de la balise <southBoundLatitude>
	my $node_y_min_g = $geographic_bbox->find("./southBoundLatitude/text()")->get_node(0);
	$node_y_min_g->setData("$y_min_g");
	# mise a jour du contenu de la balise <northBoundLatitude>
	my $node_y_max_g = $geographic_bbox->find("./northBoundLatitude/text()")->get_node(0);
	$node_y_max_g->setData("$y_max_g");
	
	#7. recuperation du srs voulu dans la balise boundingBox du layer
	# contenu de la balise <boundingBox> du layer : devient le contexte XPath
	my $bounding_box = $doc_lay->find("//boundingBox")->get_node(0);
	# contenu de l'attribut CRS de la balise
	my $srs_layer = $bounding_box->find("./\@CRS")->get_node(0)->textContent;
	
	#8. transformation des coins de la BBOX des donness en srs du layer a l'unite pres
	my ($top_left_corner_x_layer, $top_left_corner_y_layer) = &reproj_point($x_min_bbox, $y_max_bbox, $proj, $srs_layer);
	my ($top_right_corner_x_layer, $top_right_corner_y_layer) = &reproj_point($x_max_bbox, $y_max_bbox, $proj, $srs_layer);
	my ($bottom_left_corner_x_layer, $bottom_left_corner_y_layer) = &reproj_point($x_min_bbox, $y_min_bbox, $proj, $srs_layer);
	my ($bottom_right_corner_x_layer, $bottom_right_corner_y_layer) = &reproj_point($x_max_bbox, $y_min_bbox, $proj, $srs_layer);
	
	# on ne teste que les x car reproj est implemente comme ca, si erreur x et y en erreur
	if(!($top_left_corner_x_layer ne "erreur" && $top_right_corner_x_layer ne "erreur" && $bottom_left_corner_x_layer ne "erreur" && $bottom_right_corner_x_layer ne "erreur")){
		print "[MAJ_CONF_SERVEUR] ERREUR a la reprojection de $proj en $srs_layer.\n";
		&ecrit_log("ERREUR a la reprojection de $proj en $srs_layer.");
		return $bool_ok;
	}
	
	# on verifie que la transfo est dans les cordes de le projection sinon cs2cs renvoie *
	if($top_left_corner_x_layer ne "*" && $top_left_corner_y_layer ne "*" && $top_right_corner_x_layer ne "*" && $top_right_corner_y_layer ne "*" && $bottom_left_corner_x_layer ne "*" && $bottom_left_corner_y_layer ne "*" && $bottom_right_corner_x_layer ne "*" && $bottom_right_corner_y_layer ne "*"){
		my $x_min_layer = floor(min($top_left_corner_x_layer, $top_right_corner_x_layer, $bottom_left_corner_x_layer, $bottom_right_corner_x_layer));
		my $x_max_layer = ceil(max($top_left_corner_x_layer, $top_right_corner_x_layer, $bottom_left_corner_x_layer, $bottom_right_corner_x_layer));
		my $y_min_layer = floor(min($top_left_corner_y_layer, $top_right_corner_y_layer, $bottom_left_corner_y_layer, $bottom_right_corner_y_layer));
		my $y_max_layer = ceil(max($top_left_corner_y_layer, $top_right_corner_y_layer, $bottom_left_corner_y_layer, $bottom_right_corner_y_layer));
		
		#10. mise a jour dans le layer	
		# mise a jour du contenu des attributs minx maxx miny maxy de la balise <boundingBox> du layer	
		$bounding_box->setAttribute("minx", $x_min_layer);
		$bounding_box->setAttribute("maxx", $x_max_layer);
		$bounding_box->setAttribute("miny", $y_min_layer);
		$bounding_box->setAttribute("maxy", $y_max_layer);		
	}else{
		&ecrit_log("ERREUR a l'execution de la transfo : les coordonnnes ne doivent pas etre dans la plage autorisee.");
		print "[MAJ_CONF_SERVEUR] ERREUR a l'execution de la transfo : les coordonnnes ne doivent pas etre dans la plage autorisee.\n";
		return $bool_ok;
	}
		
	#9. remplacement dans le fichier
	# transformation de la variable ou sont stockees les valeurs mise a jour en fichier reel (ecrasement du lay eractuel)
	open LAY,">$xml_lay" or die "[MAJ_CONF_SERVEUR] Impossible d'ouvrir le fichier $xml_lay.";
	print LAY $doc_lay->toString;
	close LAY;
	
	$bool_ok = 1;
	
	return $bool_ok;
	
}