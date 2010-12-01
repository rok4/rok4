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
	'$path_tms_param',
);
$| = 1;
our($opt_l,$opt_p);

my $rep_log = $rep_logs_param;
my $programme_reproj = $programme_reproj_param;
my $path_tms = $path_tms_param;
# TODO eventuellement changer la def en fonction des specs
my $srs_wgs84g = "IGNF:WGS84G";
################################################################################
my $verif_programme_reproj = `which $programme_reproj`;
if ($verif_programme_reproj eq ""){
	print "[MAJ_CONF_SERVEUR] Le programme $programme_reproj est introuvable.\n";
	exit;
}
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

#action 2 :mettre a jour le lay
&ecrit_log("Mise a jour de $lay");
my $bool_maj = &maj_pyr_et_bounding_box($lay, $fichier_pyr);

if($bool_maj == 1){
	print "[MAJ_CONF_SERVEUR] Fichier $lay mis a jour avec pyramide et rectangle englobant.\n";
}else{
	print "[MAJ_CONF_SERVEUR] Des erreurs se sont produites, consulter $log.\n";
	# on revient en arriere : $lay n'a pas encore ete modifie
	unlink($lay_ancien);
	
}
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
sub maj_pyr_et_bounding_box{
	
	my $xml_lay = $_[0];
	my $nom_fichier_pyr = $_[1];
	
	my $bool_ok = 0;
	
	#0. mise a jour du hcemin de la pyramide dans le lay
	my $parser_lay = XML::LibXML->new();
	my $doc_lay = $parser_lay->parse_file("$xml_lay");
	
	my $absolu_pyramide = abs_path($nom_fichier_pyr);
	$doc_lay->find("//pyramidList/pyramid/text()")->get_node(0)->setData($absolu_pyramide);
	
	#1.recuperation de la resolution la plus grande dans le lay
	
	my $res_max = $doc_lay->find("//maxRes")->get_node(0)->textContent;
	
	#2. recuperation du TMS dans le pyr
	my $parser_pyr = XML::LibXML->new();
	my $doc_pyr = $parser_pyr->parse_file("$nom_fichier_pyr");
	
	my $tms = $doc_pyr->find("//tileMatrixSet")->get_node(0)->textContent;
	my $tms_complet = $path_tms."/".$tms.".tms";
	
	#3. recuperation du level, de la proj, des origines et tailles des tuiles en resolution max dans le TMS
	my $parser_tms = XML::LibXML->new();
	my $doc_tms = $parser_tms->parse_file("$tms_complet");
	
	my $proj = $doc_tms->find("//crs")->get_node(0)->textContent;
	
	my $tile_matrix = $doc_tms->find("//tileMatrix[resolution = '$res_max']")->get_node(0);
	
	my $level = $tile_matrix->find("./id")->get_node(0)->textContent;
	# floor / ceil / int servent a caster en chiffre pouique les espaces sont possibles
	my $origine_x = floor($tile_matrix->find("./topLeftCornerX")->get_node(0)->textContent);
	my $origine_y = ceil($tile_matrix->find("./topLeftCornerY")->get_node(0)->textContent);
	my $taille_tuile_pix_x = int($tile_matrix->find("./tileWidth")->get_node(0)->textContent);
	my $taille_tuile_pix_y = int($tile_matrix->find("./tileHeight")->get_node(0)->textContent);
	
	#4. recuperation des limites TMS dans le pyr sur le level concerne
	my $tms_limits = $doc_pyr->find("//level[tileMatrix = '$level']/TMSLimits")->get_node(0);
	my $min_tile_x = $tms_limits->find("./minTileRow")->get_node(0)->textContent;
	my $max_tile_x = $tms_limits->find("./maxTileRow")->get_node(0)->textContent;
	my $min_tile_y = $tms_limits->find("./minTileCol")->get_node(0)->textContent;
	my $max_tile_y = $tms_limits->find("./maxTileCol")->get_node(0)->textContent;
	
	#5. determination de la bbox en proj $proj
	my $x_min_bbox = ($min_tile_x * $res_max * $taille_tuile_pix_x) + $origine_x;
	my $x_max_bbox = ($max_tile_x * $res_max * $taille_tuile_pix_x) + $origine_x;
	my $y_min_bbox = $origine_y - ($max_tile_y * $res_max * $taille_tuile_pix_y);
	my $y_max_bbox = $origine_y - ($min_tile_y * $res_max * $taille_tuile_pix_y);
	
	#6. transformation en WGS84G pour l'info EX_GeographicBoundingBox au degre pres
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
	# teste si les coordonnees ne sont pas hors champ
	if(!($top_left_corner_x_g ne "*" && $top_right_corner_x_g ne "*" && $bottom_left_corner_x_g ne "*" && $bottom_right_corner_x_g ne "*" && $top_left_corner_y_g ne "*" && $top_right_corner_y_g ne "*" && $bottom_left_corner_y_g ne "*" && $bottom_right_corner_y_g ne "*" )){
		print "[MAJ_CONF_SERVEUR] ERREUR a la reprojection de $proj en $srs_wgs84g, coordonnees potentiellement hors champ.\n";
		&ecrit_log("ERREUR a la reprojection de $proj en $srs_wgs84g, coordonnees potentiellement hors champ.");
		return $bool_ok;
	}
	
	my $x_min_g = floor(min($top_left_corner_x_g, $top_right_corner_x_g, $bottom_left_corner_x_g, $bottom_right_corner_x_g));
	my $x_max_g = ceil(max($top_left_corner_x_g, $top_right_corner_x_g, $bottom_left_corner_x_g, $bottom_right_corner_x_g));
	my $y_min_g = floor(min($top_left_corner_y_g, $top_right_corner_y_g, $bottom_left_corner_y_g, $bottom_right_corner_y_g));
	my $y_max_g = ceil(max($top_left_corner_y_g, $top_right_corner_y_g, $bottom_left_corner_y_g, $bottom_right_corner_y_g));
	
	#7. mise a jour dans le layer
	my $geographic_bbox = $doc_lay->find("//EX_GeographicBoundingBox")->get_node(0);
	
	my $node_x_min_g = $geographic_bbox->find("./westBoundLongitude/text()")->get_node(0);
	$node_x_min_g->setData("$x_min_g");
	my $node_x_max_g = $geographic_bbox->find("./eastBoundLongitude/text()")->get_node(0);
	$node_x_max_g->setData("$x_max_g");
	my $node_y_min_g = $geographic_bbox->find("./southBoundLatitude/text()")->get_node(0);
	$node_y_min_g->setData("$y_min_g");
	my $node_y_max_g = $geographic_bbox->find("./northBoundLatitude/text()")->get_node(0);
	$node_y_max_g->setData("$y_max_g");
	
	#8. recuperation du srs voulu dans la balise boundingBox du layer
	my $bounding_box = $doc_lay->find("//boundingBox")->get_node(0);
	my $srs_layer = $bounding_box->find("./\@CRS")->get_node(0)->textContent;
	
	#9. transformation en srs du layer a l'unite pres
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
		$bounding_box->setAttribute("minx", $x_min_layer);
		$bounding_box->setAttribute("maxx", $x_max_layer);
		$bounding_box->setAttribute("miny", $y_min_layer);
		$bounding_box->setAttribute("maxy", $y_max_layer);		
	}else{
		&ecrit_log("ERREUR a l'execution de la transfo : les coordonnnes ne doivent pas etre dans la plage autorisee.");
		print "[MAJ_CONF_SERVEUR] ERREUR a l'execution de la transfo : les coordonnnes ne doivent pas etre dans la plage autorisee.\n";
		return $bool_ok;
	}
		
	#11. remplacement dans le fichier
	open LAY,">$xml_lay" or die "[MAJ_CONF_SERVEUR] Impossible d'ouvrir le fichier $xml_lay.";
	print LAY $doc_lay->toString;
	close LAY;
	
	$bool_ok = 1;
	return $bool_ok;
}