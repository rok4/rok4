#!/usr/bin/perl -w

use strict;
use cache(
	'$base_param',
	'%base10_base_param',
	'$color_no_data_param',
	'$dalle_no_data_param',
	'%produit_res_utiles_param',
	'$programme_ss_ech_param',
	'cree_repertoires_recursifs',
	'$programme_format_pivot_param',
# 	'%produit_format_param',
	'$path_tms_param',
	'lecture_tile_matrix_set',
	'$dalle_no_data_mtd_param',
	'$programme_dalles_base_param',
	'$programme_copie_image_param',
	'$rep_logs_param',
	'$programme_reproj_param',
);
use Term::ANSIColor;
use Getopt::Std;
#use File::Copy;
use XML::Simple;
use File::Basename;
# pas de bufferisation des sorties
$| = 1;
our ($opt_p, $opt_f, $opt_x, $opt_m, $opt_s, $opt_d, $opt_r, $opt_n, $opt_t);
my $base = $base_param;
my %base10_base = %base10_base_param;

my $color_no_data = $color_no_data_param;
my $dalle_no_data = $dalle_no_data_param;
my $dalle_no_data_mtd = $dalle_no_data_mtd_param;
my %produit_res_utiles = %produit_res_utiles_param;
my $programme_ss_ech = $programme_ss_ech_param;
my $programme_format_pivot = $programme_format_pivot_param;
my $programme_dalles_base = $programme_dalles_base_param;
my $programme_copie_image = $programme_copie_image_param;

# my %produit_format = %produit_format_param;
my $path_tms = $path_tms_param;
my $rep_log = $rep_logs_param;
my $programme_reproj = $programme_reproj_param;
################################################################################

### HELP lignes GDAL ### XXXX

# verification de l'existence de fichiers et repertoires annexes
if(!(-e $dalle_no_data && -f $dalle_no_data)){
	print colored ("[CREE_DALLAGE_BASE] Le fichier $dalle_no_data est introuvable.", 'white on_red');
	print "\n";
	exit;
}
if(!(-e $dalle_no_data_mtd && -f $dalle_no_data_mtd)){
	print colored ("[CREE_DALLAGE_BASE] Le fichier $dalle_no_data_mtd est introuvable.", 'white on_red');
	print "\n";
	exit;
}
if(!(-e $path_tms && -d $path_tms)){
	print colored ("[CREE_DALLAGE_BASE] Le repertoire $path_tms est introuvable.", 'white on_red');
	print "\n";
	exit;
}
#verification de la presence des programmes $programme_ss_ech $programme_format_pivot $programme_dalles_base $programme_reproj
my $verif_programme_dalle_base = `which $programme_dalles_base`;
if ($verif_programme_dalle_base eq ""){
	print colored ("[CREE_DALLAGE_BASE] Le programme $programme_dalles_base est introuvable.", 'white on_red');
	print "\n";
	exit;
}
my $verif_programme_ss_ech = `which $programme_ss_ech`;
if ($verif_programme_ss_ech eq ""){
	print colored ("[CREE_DALLAGE_BASE] Le programme $programme_ss_ech est introuvable.", 'white on_red');
	print "\n";
	exit;
}
my $verif_programme_pivot = `which $programme_format_pivot`;
if ($verif_programme_pivot eq ""){
	print colored ("[CREE_DALLAGE_BASE] Le programme $programme_format_pivot est introuvable.", 'white on_red');
	print "\n";
	exit;
}
my $verif_programme_reproj = `which $programme_reproj`;
if ($verif_programme_reproj eq ""){
	print colored ("[CREE_DALLAGE_BASE] Le programme $programme_reproj est introuvable.", 'white on_red');
	print "\n";
	exit;
}
############ MAIN
my $time = time();
my $log = $rep_log."/log_cree_dallage_base_$time.log";

open LOG, ">>$log" or die colored ("[CREE_DALLAGE_BASE] Impossible de creer le fichier $log.", 'white on_red');

&ecrit_log("commande : @ARGV");

#### recuperation des parametres
getopts("p:f:x:m:s:d:r:n:t:");

if ( ! defined ($opt_p and $opt_f and $opt_x and $opt_s and $opt_d and $opt_r and $opt_n and $opt_t) ){
	print colored ("[CREE_DALLAGE_BASE] Nombre d'arguments incorrect.", 'white on_red');
	print "\n\n";
	&ecrit_log("ERREUR : Nombre d'arguments incorrect.");
	if(! defined $opt_p){
		print colored ("[CREE_DALLAGE_BASE] Veuillez specifier un parametre -p.", 'white on_red');
		print "\n";
	}
	if(! defined $opt_f){
		print colored ("[CREE_DALLAGE_BASE] Veuillez specifier un parametre -f.", 'white on_red');
		print "\n";
	}
	if(! defined $opt_x){
		print colored ("[CREE_DALLAGE_BASE] Veuillez specifier un parametre -x.", 'white on_red');
		print "\n";
	}
	if(! defined $opt_s){
		print colored ("[CREE_DALLAGE_BASE] Veuillez specifier un parametre -s.", 'white on_red');
		print "\n";
	}
	if(! defined $opt_d){
		print colored ("[CREE_DALLAGE_BASE] Veuillez specifier un parametre -d.", 'white on_red');
		print "\n";
	}
	if(! defined $opt_r){
		print colored ("[CREE_DALLAGE_BASE] Veuillez specifier un parametre -r.", 'white on_red');
		print "\n";
	}
	if(! defined $opt_n){
		print colored ("[CREE_DALLAGE_BASE] Veuillez specifier un parametre -n.", 'white on_red');
		print "\n";
	}
	if(! defined $opt_t){
		print colored ("[CREE_DALLAGE_BASE] Veuillez specifier un parametre -t.", 'white on_red');
		print "\n";
	}
	&usage();
	exit;
}

my $produit = $opt_p;
my $ss_produit;
my $fichier_dalle_source = $opt_f;
my $fichier_mtd_source;
if (defined $opt_m){
	$fichier_mtd_source = $opt_m;
}
my $fichier_pyramide = $opt_x;
my $systeme_source = "IGNF:".$opt_s;
my $pourcentage_dilatation = $opt_d;
my $dilatation_reproj = $opt_r;
my $nom_script = $opt_n;
my $taille_dalle_pix = $opt_t;
my ($taille_image_pix_x, $taille_image_pix_y) = ($taille_dalle_pix, $taille_dalle_pix);
# verifications des parametres
if ($produit !~ /^ortho|parcellaire|scan(?:25|50|100|dep|reg|1000)|franceraster$/i){
	print colored ("[CREE_DALLAGE_BASE] Produit mal specifie.", 'white on_red');
	print "\n";
	&ecrit_log("ERREUR Produit mal specifie.");
	exit;
}else{
	if($produit =~ /^scan(25|50|100|dep|reg|1000)$/i){
		$produit = "scan";
		$ss_produit = "scan".lc($1);
	}else{
		$produit = lc($produit);
		$ss_produit = $produit;
	}
		
}
if (! (-e $fichier_dalle_source && -f $fichier_dalle_source)){
	print colored ("[CREE_DALLAGE_BASE] Le fichier $fichier_dalle_source n'existe pas.", 'white on_red');
	print "\n";
	&ecrit_log("ERREUR : Le fichier $fichier_dalle_source n'existe pas.");
	exit;
}
if (defined $fichier_mtd_source && ( ! (-e $fichier_mtd_source && -f $fichier_mtd_source))){
	print colored ("[CREE_DALLAGE_BASE] Le fichier $fichier_mtd_source n'existe pas.", 'white on_red');
	print "\n";
	&ecrit_log("ERREUR : Le fichier $fichier_mtd_source n'existe pas.");
	exit;
}
if (! (-e $fichier_pyramide && -f $fichier_pyramide)){
	print colored ("[CREE_DALLAGE_BASE] Le fichier $fichier_pyramide n'existe pas.", 'white on_red');
	print "\n";
	&ecrit_log("ERREUR : Le fichier $fichier_pyramide n'existe pas.");
	exit;
}
if ($pourcentage_dilatation !~ /^\d{1,3}$/ || $pourcentage_dilatation > 100 ){
	print colored ("[CREE_DALLAGE_BASE] Le pourcentage de dilatation -d $pourcentage_dilatation est incorrect.", 'white on_red');
	print "\n";
	&ecrit_log("ERREUR : Le pourcentage de dilatation -d $pourcentage_dilatation est incorrect.");
	exit;
}
if ($dilatation_reproj !~ /^\d{1,3}$/ || $dilatation_reproj > 100 ){
	print colored ("[CREE_DALLAGE_BASE] Le pourcentage de dilatation -r $dilatation_reproj est incorrect.", 'white on_red');
	print "\n";
	&ecrit_log("ERREUR : Le pourcentage de dilatation -r $dilatation_reproj est incorrect.");
	exit;
}
if ($taille_dalle_pix !~ /^\d+$/){
	print colored ("[CREE_DALLAGE_BASE] La taille des dalles en pixels -t $taille_dalle_pix est incorrecte.", 'white on_red');
	print "\n";
	&ecrit_log("ERREUR :La taille des dalles en pixels -t $taille_dalle_pix est incorrecte.");
	exit;
}
########### traitement
# creation d'un rep temporaire pour les calculs intermediaires
my $rep_temp = "cree_dallage_base_".$time;
if (! (-e $rep_temp && -d $rep_temp)){
	mkdir "$rep_temp", 0775 or die colored ("[CREE_DALLAGE_BASE] Impossible de creer le repertoire $rep_temp.", 'white on_red');
}

# action 0 : determiner les resolutions utiles
my $ref_niv_utiles = $produit_res_utiles{$ss_produit};
my ($res_min_produit, $res_max_produit) = @{$ref_niv_utiles};

# action 1 : recuperer les infos de la pyramide
&ecrit_log("Lecture de la pyramide $fichier_pyramide.");
print "[CREE_DALLAGE_BASE] Lecture de la pyramide $fichier_pyramide.\n";
my ($ref_niveau_ordre_croissant, $ref_rep_images, $ref_rep_mtd, $ref_res, $ref_taille_m_x, $ref_taille_m_y, $ref_origine_x, $ref_origine_y, $ref_profondeur, $ref_taille_tuile_x, $ref_taille_tuile_y, $systeme_target, $format_imgs_pyramide) = &lecture_pyramide($fichier_pyramide);
my @niveaux_ranges = @{$ref_niveau_ordre_croissant};
my %niveau_repertoire_image = %{$ref_rep_images};
my %niveau_repertoire_mtd = %{$ref_rep_mtd};
my %niveau_res = %{$ref_res};
my %niveau_taille_m_x = %{$ref_taille_m_x};
my %niveau_taille_m_y = %{$ref_taille_m_y};
my %niveau_origine_x = %{$ref_origine_x};
my %niveau_origine_y = %{$ref_origine_y};
my %niveau_profondeur = %{$ref_profondeur};
my %niveau_taille_tuile_x = %{$ref_taille_tuile_x};
my %niveau_taille_tuile_y = %{$ref_taille_tuile_y};
my $level_min = $niveaux_ranges[0];
my $res_min = $niveau_res{"$level_min"};
my $level_max = $niveaux_ranges[@niveaux_ranges - 1];
my $res_max = $niveau_res{"$level_max"};

# TODO voir si les elses sont corrects
my $res_min_utile;
my $res_max_utile;
if($res_min_produit <= $res_min){
	$res_min_utile = $res_min;
}else{
	$res_min_utile = $res_min_produit;
}
if($res_max_produit >= $res_max){
	$res_max_utile = $res_max;
}else{
	$res_max_utile = $res_max_produit;
}

my $x_min_niveau_max = $niveau_origine_x{"$level_max"};
my $y_max_niveau_max = $niveau_origine_y{"$level_max"};
my $x_max_niveau_max = $x_min_niveau_max + $niveau_taille_m_x{"$level_max"};
my $y_min_niveau_max = $y_max_niveau_max - $niveau_taille_m_y{"$level_max"};

# determination de la compression finale
my $compress;
my $bool_perte = 0;
if($format_imgs_pyramide =~ /jpg/i){
	$compress = "jpeg";
	$bool_perte = 1;
}elsif($format_imgs_pyramide =~ /png/i){
	$compress = "png";
}else{
	$compress = "none";
}

# determination d'une reprojection
my $bool_reprojection = 0;
if($systeme_source ne $systeme_target){
	$bool_reprojection = 1;
}

# action 2 : lire le fichier de dalles source et des mtd
# => bbox des donnees, hashes des xmin, xmax, ymin, ymax, resx, resy
&ecrit_log("Lecture du fichier d'images source $fichier_dalle_source.");
print "[CREE_DALLAGE_BASE] Lecture du fichier d'images source $fichier_dalle_source.\n";
my ($reference_hash_x_min, $reference_hash_x_max, $reference_hash_y_min, $reference_hash_y_max, $reference_hash_res_x, $reference_hash_res_y, $x_min_bbox, $x_max_bbox, $y_min_bbox, $y_max_bbox) = &lecture_fichier_dalles_source($fichier_dalle_source, 1);
my %source_x_min = %{$reference_hash_x_min};
my %source_x_max = %{$reference_hash_x_max};
my %source_y_min = %{$reference_hash_y_min};
my %source_y_max = %{$reference_hash_y_max};
my %source_res_x = %{$reference_hash_res_x};
my %source_res_y = %{$reference_hash_res_y};
my @dalles_source = keys %source_x_min; # ou un autre
# mtd seulement si definies
my ($ref_hash_x_min, $ref_hash_x_max, $ref_hash_y_min, $ref_hash_y_max, $ref_hash_res_x, $ref_hash_res_y);
my %mtd_source_x_min;
my %mtd_source_x_max;
my %mtd_source_y_min;
my %mtd_source_y_max;
my %mtd_source_res_x;
my %mtd_source_res_y ;
my @mtd_source;
if (defined $fichier_mtd_source){
	&ecrit_log("Lecture du fichier de mtd source $fichier_mtd_source.");
	print "[CREE_DALLAGE_BASE] Lecture du fichier de mtd source $fichier_mtd_source.\n";
	($ref_hash_x_min, $ref_hash_x_max, $ref_hash_y_min, $ref_hash_y_max, $ref_hash_res_x, $ref_hash_res_y) = &lecture_fichier_dalles_source($fichier_mtd_source, 0);
	%mtd_source_x_min = %{$ref_hash_x_min};
	%mtd_source_x_max = %{$ref_hash_x_max};
	%mtd_source_y_min = %{$ref_hash_y_min};
	%mtd_source_y_max = %{$ref_hash_y_max};
	%mtd_source_res_x = %{$ref_hash_res_x};
	%mtd_source_res_y = %{$ref_hash_res_y};
	@mtd_source = keys %mtd_source_x_min;
}

# action 3 : infos de la dalle cache la plus haute : xmin, xmax, ymin, ymax, resx, resy 
# (au moins au plus haut niveau de la pyramide) (pas forcement existante)
&ecrit_log("Determination des infos de la dalle la plus haute recouvrant les donnees.");
print "[CREE_DALLAGE_BASE] Determination des infos de la dalle la plus haute recouvrant les donnees.\n";
my ($x_min_dalle0, $x_max_dalle0, $y_min_dalle0, $y_max_dalle0, $res_dalle0, $niveau_dalle0, $indice_niveau0) =  &trouve_infos_pyramide($x_min_bbox, $x_max_bbox, $y_min_bbox, $y_max_bbox, $x_min_niveau_max, $x_max_niveau_max, $y_min_niveau_max, $y_max_niveau_max, $res_max, $level_max, $systeme_source, $systeme_target);

# action 4 : creer arbre : cree_arbre_dalles_cache
# XXXX pour GDAL
# XXXX my %cache_arbre_niveau;
my %cache_arbre_x_min;
my %cache_arbre_x_max;
my %cache_arbre_y_min;
my %cache_arbre_y_max;
my %cache_arbre_res;
# correspondance entre index de la dalle cache dans l'arbre et liste des dalles source recouvrantes
my %index_arbre_liste_dalle;
my %index_arbre_liste_mtd;
# seules les cles sont utilisees
my %dalles_cache_plus_bas;
my %dalle_arbre_niveau;
my %dalle_arbre_indice_niveau;
# initialisation pour la dalle0
$dalle_arbre_niveau{"0"} = $niveau_dalle0;
$dalle_arbre_indice_niveau{"0"} = @niveaux_ranges - 1;
#recursivite descendante et mise en memoire pour chacune des dalles cache
&ecrit_log("Calcul de l'arbre image.");
print "[CREE_DALLAGE_BASE] Calcul de l'arbre image.\n";
my $nombre_dalles_cache = cree_arbre_dalles_cache(\@dalles_source, "image", $pourcentage_dilatation);
&ecrit_log("$nombre_dalles_cache images definies.");
print "[CREE_DALLAGE_BASE] $nombre_dalles_cache images definies.\n";
if (defined $fichier_mtd_source){
	&ecrit_log("Calcul de l'arbre mtd.");
	print "[CREE_DALLAGE_BASE] Calcul de l'arbre mtd.\n";
	my $nombre_mtd_cache = cree_arbre_dalles_cache(\@mtd_source, "mtd", $pourcentage_dilatation);
	&ecrit_log("$nombre_mtd_cache images definies.");
	print "[CREE_DALLAGE_BASE] $nombre_mtd_cache images definies.\n";
}


# transformation des index dans l'arbre en nom des dalles cache
my @liste_dalles_cache_index_arbre = keys %index_arbre_liste_dalle;
my @liste_total_dalles_cache;
my @liste_total_dalles_cache_mtd;
my %dalle_cache_min_liste_dalle;
my %mtd_cache_min_liste_mtd;
my %dalle_cache_dessous;
my %niveau_ref_dalles_inf;
my %niveau_ref_mtd_inf;
my %nom_dalle_index_base;
&ecrit_log("Passage des donnees arbre aux donnees cache.");
print "[CREE_DALLAGE_BASE] Passage des donnees arbre aux donnees cache.\n";
&arbre2cache(\@liste_dalles_cache_index_arbre);

##### ATTENTION; action 5 gelée : plus besoin d'intialiser la pyramide avec du vide
# on fera simplement reference a la dalle no_data plus tard

# 	# action 5 : completer la pyramide initiale
# 	# images
# 	&ecrit_log("Completement des dalles absentes de la pyramide initiale.");
# 	print "[CREE_DALLAGE_BASE] Completement des dalles absentes de la pyramide initiale.\n";
# 	my $nombre_ajoutees = &complete_pyramide_initiale(\@liste_total_dalles_cache, "image");
# 	&ecrit_log("$nombre_ajoutees images ajoutees.");
# 	print "[CREE_DALLAGE_BASE] $nombre_ajoutees images ajoutees.\n";
# 	# mtd
# 	if (defined $fichier_mtd_source){
# 		my $nombre_ajoutees_mtd = &complete_pyramide_initiale(\@liste_total_dalles_cache_mtd, "mtd");
# 		&ecrit_log("$nombre_ajoutees_mtd mtd ajoutees.");
# 		print "[CREE_DALLAGE_BASE] $nombre_ajoutees_mtd mtd ajoutees.\n";
# 	}

# les actions 6 et 7 sont envoyees dans le script
open SCRIPT, ">$nom_script" or die colored ("[CREE_DALLAGE_BASE] Impossible de creer le fichier $nom_script.", 'white on_red');

#action 6 : calculer le niveau minimum : en WMS si compression avec perte ou reprojection
&ecrit_log("Calcul des images du niveau le plus bas.");
print "[CREE_DALLAGE_BASE] Calcul des images du niveau le plus bas.\n";
my $rep_fichiers_img = dirname($fichier_dalle_source);
my $nombre_dalles_minimum_calc = &calcule_niveau_minimum(\%dalle_cache_min_liste_dalle, $rep_fichiers_img, "image");
&ecrit_log("$nombre_dalles_minimum_calc images du plus bas niveau calculees.");
print "[CREE_DALLAGE_BASE] $nombre_dalles_minimum_calc images du plus bas niveau calculees.\n";
if (defined $fichier_mtd_source){
	&ecrit_log("Calcul des mtd du niveau le plus bas.");
	print "[CREE_DALLAGE_BASE] Calcul des mtd du niveau le plus bas.\n";
	my $rep_fichiers_mtd = dirname($fichier_mtd_source);
	my $nombre_mtd_minimum_calc = &calcule_niveau_minimum(\%mtd_cache_min_liste_mtd, $rep_fichiers_mtd, "mtd");
	&ecrit_log("$nombre_mtd_minimum_calc mtd du plus bas niveau calculees.");
	print "[CREE_DALLAGE_BASE] $nombre_mtd_minimum_calc mtd du plus bas niveau calculees.\n";
}

# action 7 : calculer les niveaux inferieurs
&ecrit_log("Calcul des images des niveaux inferieurs.");
print "[CREE_DALLAGE_BASE] Calcul des images des niveaux inferieurs.\n";
my $nombre_dalles_niveaux_inf = &calcule_niveaux_inferieurs(\%niveau_ref_dalles_inf, "MOYENNE", \%dalle_cache_min_liste_dalle, "image");
&ecrit_log("$nombre_dalles_niveaux_inf images calculees.");
print "[CREE_DALLAGE_BASE] $nombre_dalles_niveaux_inf images calculees.\n";
if (defined $fichier_mtd_source){
	&ecrit_log("Calcul des mtd des niveaux inferieurs.");
	print "[CREE_DALLAGE_BASE] Calcul des mtd des niveaux inferieurs.\n";
	my $nombre_mtd_niveaux_inf = &calcule_niveaux_inferieurs(\%niveau_ref_mtd_inf, "PPV", \%mtd_cache_min_liste_mtd, "mtd");
	&ecrit_log("$nombre_mtd_niveaux_inf images calculees.");
	print "[CREE_DALLAGE_BASE] $nombre_mtd_niveaux_inf images calculees.\n";
}

close SCRIPT;

# suppression du repertoire temporaire
opendir TEMP, "$rep_temp" or die colored ("[CREE_DALLAGE_BASE] Impossible d'ouvir le repertoire $rep_temp.", 'white on_red');
my @suppr = readdir TEMP;
closedir TEMP;
foreach my $temp(@suppr){
	next if ($temp =~ /^\.\.?$/);
	unlink("$rep_temp/$temp");
	
}
my $bool_suppr = rmdir("$rep_temp");
if($bool_suppr == 0){
	&ecrit_log("ERREUR a la suppression du repertoire $rep_temp.");
}

&ecrit_log("Traitement termine.");
print colored ("[CREE_DALLAGE_BASE] Traitement termine.\n", 'green');
close LOG;

# lecture du log pour voir si erreurs
open CHECK, "<$log" or die colored ("[CREE_DALLAGE_BASE] Impossible d'ouvrir le fichier $log.", 'white on_red');
my @lignes_log = <CHECK>;
close CHECK;
my $nb_erreur = 0;
foreach my $ligne(@lignes_log){
	if ($ligne =~ /err(?:eur|or)|unable|not found/i){
		$nb_erreur += 1;
	}
}
if ($nb_erreur != 0){
	print colored ("[CREE_DALLAGE_BASE] $nb_erreur erreurs se sont produites. Consulter le fichier $log.", 'white on_red');
	print "\n";
}else{
	print colored ("[CREE_DALLAGE_BASE] $nb_erreur erreurs se sont produites. Consulter le fichier $log.", 'green');
	print "\n";
}

# on l'ecrit dans le log
open LOG, ">>$log" or die colored ("[CREE_DALLAGE_BASE] Impossible d'ouvrir le fichier $log en ajout.", 'white on_red');
&ecrit_log("FIN : $nb_erreur erreurs se sont produites.");
close LOG;
################### FIN

################################################################################

########## FONCTIONS
sub usage{
	my $bool_ok = 0;
	
	print colored ("\nUsage : \ncree_dallage_base.pl -p produit -f path/fichier_dalles_source [-m path/fichier_mtd_source] -s systeme_coordonnees_source -x path/fichier_pyramide.pyr -d %_dilatation_dalles_base -r %_dilatation_reproj -n path/nom_script -t taille_dalles_pixels\n",'black on_white');
	print "\nproduit :\n";
 	print "\tortho\n\tparcellaire\n\tscan[25|50|100|dep|reg|1000]\n\tfranceraster\n";
 	print "\nsysteme_coordonnees_source :\n";
	print "\tcode RIG des images source : LAMB93 LAMBE ...\n";
	print "\npourcentage_dilatation : 0 a 100\n";
	print "%_dilatation_dalles_base : dilatation des dalles du cache pour trouver les dalles source en recouvrement\n";
	print "%_dilatation_reproj : dilatation des dalles du cache reprojetees pour parer a la deformation de la reprojection\n";
	print "\n\n";
	
	$bool_ok = 1;
	return $bool_ok;
}
################################################################################
sub nom_dalle_cache{
	
	my $x_min_dalle = $_[0];
	my $y_max_dalle = $_[1];
	my $origine_x = $_[2];
	my $origine_y = $_[3];
	my $taille_x = $_[4];
	my $taille_y = $_[5];
	my $rep_niveau = $_[6];
	my $prof_niveau = $_[7];
	
	my ($index_x_dalle, $index_y_dalle, $longueur_index) = xy2index_base($base, $x_min_dalle, $y_max_dalle, $origine_x, $origine_y, $taille_x, $taille_y);
	
	# on code les index x et y sur au moins la profondeur + 1
	if ($longueur_index < $prof_niveau + 1){
		$index_x_dalle = &formate_zero_base($index_x_dalle, $prof_niveau + 1);
		$index_y_dalle = &formate_zero_base($index_y_dalle, $prof_niveau + 1);
		$longueur_index = $prof_niveau + 1;
	}
	
	# la profondeur du chemin est de $prof_niveau => n + 1 = $prof_niveau => n = $prof_niveau - 1
	# $longueur_index = m + n => m = $longueur_index - $n
	my $n = $prof_niveau;
	my $m = $longueur_index - $n;
	
	my $index_base = "";
	
	my $nom_first_ss_rep = "";
	
	for (my $i = 0 ; $i < $m ; $i++){
		my $chiffre_x = substr($index_x_dalle, $i, 1);
		my $chiffre_y = substr($index_y_dalle, $i, 1);
		$nom_first_ss_rep .= $chiffre_x.$chiffre_y;
		$index_base .= $chiffre_x.$chiffre_y;
	}
	
	my $nom_reps_suiv = "";
	for (my $j = $m ; $j < $m + $n - 1 ; $j++){
		my $chiffre_x2 = substr($index_x_dalle, $j, 1);
		my $chiffre_y2 = substr($index_y_dalle, $j, 1);
		$nom_reps_suiv .= $chiffre_x2.$chiffre_y2."/";
		$index_base .= $chiffre_x2.$chiffre_y2;
	}
	
	my $dernier_chiffre_x = substr($index_x_dalle, $m + $n - 1, 1);
	my $dernier_chiffre_y = substr($index_y_dalle, $m + $n - 1, 1);
	my $nom_image = $dernier_chiffre_x.$dernier_chiffre_y.".tif";
	$index_base .= $dernier_chiffre_x.$dernier_chiffre_y;
	
	my $nom_dalle = $rep_niveau."/".$nom_first_ss_rep."/".$nom_reps_suiv.$nom_image;
	
	#remplissage association entre nom_dalle et index (pour les noms de fichiers)
	$nom_dalle_index_base{$nom_dalle} = $index_base;
	
	return $nom_dalle;
}
################################################################################
sub xy2index_base{
	
	my $base_util = $_[0];
	my $x = $_[1];
	my $y = $_[2];
	my $x_origin = $_[3];
	my $y_origin = $_[4];
	my $taille_x_m = $_[5];
	my $taille_y_m = $_[6];
	
	my ($index_x_10,$index_y_10) = xy2index10($x, $y, $x_origin, $y_origin, $taille_x_m, $taille_y_m);
	
	my $index_x = nb2base($base, $index_x_10);
	my $index_y = nb2base($base, $index_y_10);
	
	# le nombre de chiffres en base X doit etre egal pour x et y
	my $longueur = length($index_x);
	if (length($index_y) > $longueur){
		$longueur = length($index_y);
	}
	
	$index_x = &formate_zero_base($index_x, $longueur);
	$index_y = &formate_zero_base($index_y, $longueur);

	my @indexes;
	
	push(@indexes, $index_x, $index_y, $longueur);
	
	return @indexes;
}
################################################################################
sub xy2index10{
	
	my $x_m = $_[0];
	my $y_m = $_[1];
	my $origin_x = $_[2];
	my $origin_y = $_[3];
	my $taille_x_metre = $_[4];
	my $taille_y_metre = $_[5];
	
	my $x_10 = ($x_m - $origin_x) / $taille_x_metre;
	my $y_10 = ($origin_y - $y_m) / $taille_y_metre;
	
	my @index10;
	push (@index10, $x_10, $y_10);
	
	return @index10;
}
################################################################################
sub nb2base{

	my $base_utilisee = $_[0];
	my $nb_a_convertir = $_[1];
	my $nb_base = "";
	
	# reste de la division euclidienne %
	while($nb_a_convertir > $base_utilisee - 1){
		my $reste = $nb_a_convertir % $base_utilisee;
		my $reste_base = $base10_base{$reste};
		$nb_base = $reste_base.$nb_base;
		$nb_a_convertir = ($nb_a_convertir - $reste) / $base_utilisee;
	}
	
	my $reste_fin_base = $base10_base{$nb_a_convertir};
	$nb_base = $reste_fin_base.$nb_base;
	
	return $nb_base;
}
################################################################################
sub formate_zero_base{
	
	my $variable = $_[0];
	my $nb = $_[1];
	
	my $diff = $nb - length($variable);
	
	if ($diff > 0 ){
		for(my $compteur = 0 ; $compteur < $diff ; $compteur++){
			$variable = $base10_base{0}.$variable;
		}
	}
	
	return $variable;
  
}
################################################################################
# sub complete_pyramide_initiale{
# 
# 	my $ref_list_dal_cach = $_[0];
# 	my @liste_dalles = @{$ref_list_dal_cach};
# 	my $type_dal = $_[1];
# 	
# 	my $nombre_dalles_ajoutees = 0;
# 	
# 	foreach my $dalle_cache(@liste_dalles){
# 		if (!(-e $dalle_cache)){
# 			my $dalle_a_copier;
# 			if($type_dal eq "image"){
# 				$dalle_a_copier = $dalle_no_data;
# 			}elsif($type_dal eq "mtd"){
# 				$dalle_a_copier = $dalle_no_data_mtd;
# 			}else{
# 				print colored ("[CREE_DALLAGE_BASE] Probleme de programmation : type $type_dal incorrect.", 'white on_red');
# 				print "\n";
# 				exit;
# 			}
# 			ecrit_log("Creation des eventuels repertoires manquants.");
# 			&cree_repertoires_recursifs(dirname($dalle_cache));
# 			my $return = copy($dalle_a_copier, $dalle_cache);
# 			if ($return == 0){
# 				&ecrit_log("ERREUR a la copie de $dalle_a_copier vers $dalle_cache");
# 			}else{
# 				print ".";
# 				&ecrit_log("Copie de $dalle_a_copier vers $dalle_cache");
# 				$nombre_dalles_ajoutees += 1;
# # XXXX				# calcul d'un TFW pour GDAL
# # XXXX				if($cache_arbre_niveau{$dalle_cache} eq "$level_min"){
# # XXXX					my $string_tfw = &cree_string_tfw($cache_arbre_x_min{$dalle_cache}, $cache_arbre_y_max{$dalle_cache}, $cache_arbre_res{$dalle_cache}, 0);
# # XXXX					my $fichier_tfw = &cree_fichier_georef(basename($dalle_cache), dirname($dalle_cache), "tfw", $string_tfw);
# # XXXX					# test de l'existence du fichier
# # XXXX					if (! (-e $fichier_tfw && -f $fichier_tfw) ){
# # XXXX						print colored ("[CREE_DALLAGE_BASE] Erreur a la creation de $fichier_tfw.", 'white on_red');
# # XXXX						print "\n";
# # XXXX						&ecrit_log("ERREUR a la creation de $fichier_tfw.");
# # XXXX						
# # XXXX					}
# # XXXX				}
# 				
# 			}
# 			
# 		}
# 	}
# 	print "\n";
# 	
# 	return $nombre_dalles_ajoutees;
# }
################################################################################
sub indice_arbre2xy{
	
	my $indice_arbre = $_[0];
	my $x_min_origin = $_[1];
	my $x_max_origin = $_[2];
	my $y_min_origin = $_[3];
	my $y_max_origin = $_[4];
	
	# initialisation a la dalle 0
	my ($x_min_cache, $x_max_cache, $y_min_cache, $y_max_cache) = ($x_min_origin, $x_max_origin, $y_min_origin, $y_max_origin);
	
	# la dalle 0 a ses propres coordonnees!!
	if($indice_arbre ne "0"){
		my @chiffres_indice = split //, $indice_arbre;
		foreach my $chiffre (@chiffres_indice){
			my $taille_x_diff = ($x_max_cache - $x_min_cache) / 2;
			my $taille_y_diff = ($y_max_cache - $y_min_cache) / 2;
			# 4 cas possibles, on ajoute ou retranche selon les cas
			if($chiffre eq "1"){
				$x_max_cache -= $taille_x_diff;
				$y_min_cache += $taille_y_diff;
			}elsif($chiffre eq "2"){
				$x_min_cache += $taille_x_diff;
				$y_min_cache += $taille_y_diff;
			}elsif($chiffre eq "3"){
				$x_min_cache += $taille_x_diff;
				$y_max_cache -= $taille_y_diff;
			}elsif($chiffre eq "4"){
				$x_max_cache -= $taille_x_diff;
				$y_max_cache -= $taille_y_diff;
			}else{
				print colored ("[CREE_DALLAGE_BASE] Probleme pour obtenir les coordonnees a partir de l'indice de l'arbre : indice $indice_arbre non correct.", 'white on_red');
				print "\n";
				&ecrit_log("ERREUR a l'appel de la fonction indice_arbre2xy : $indice_arbre $x_min_origin $x_max_origin $y_min_origin $y_max_origin");
				exit;
			}
		}
	}
	
	return ($x_min_cache, $x_max_cache, $y_min_cache, $y_max_cache);
	
}
################################################################################
sub lecture_fichier_dalles_source{
	
	my $fichier_a_lire = $_[0];
	my $bool_bbox = $_[1];
	
	my %hash_x_min;
	my %hash_x_max;
	my %hash_y_min;
	my %hash_y_max;
	
	my %hash_res_x;
	my %hash_res_y;
	
	# bbox du chantier
	my $xmin = 99999999999;
	my $xmax = 0;
	my $ymin = 99999999999;
	my $ymax = 0;
	
	my @infos;
	
	open SOURCE, "<$fichier_a_lire" or die colored ("[CREE_DALLAGE_BASE] Impossible d'ouvrir le fichier $fichier_a_lire.", 'white on_red');
	my @lignes = <SOURCE>;
	close SOURCE;
	
	foreach my $ligne(@lignes){
		chomp($ligne);
		my @infos_dalle = split /\t/, $ligne;
		# remplissage des infos
		if(! (defined $infos_dalle[0] && defined $infos_dalle[1] && defined $infos_dalle[2] && defined $infos_dalle[3] && defined $infos_dalle[4] && defined $infos_dalle[5] && defined $infos_dalle[6]) ){
			&ecrit_log("ERREUR de formatage du fichier $fichier_a_lire");
		}
		$hash_x_min{"$infos_dalle[0]"} = $infos_dalle[1];
		$hash_x_max{"$infos_dalle[0]"} = $infos_dalle[3];
		$hash_y_min{"$infos_dalle[0]"} = $infos_dalle[4];
		$hash_y_max{"$infos_dalle[0]"} = $infos_dalle[2];
		$hash_res_x{"$infos_dalle[0]"} = $infos_dalle[5];
		$hash_res_y{"$infos_dalle[0]"} = $infos_dalle[6];
		if($bool_bbox == 1){
			if($infos_dalle[1] < $xmin){
				$xmin = $infos_dalle[1];
			}
			if($infos_dalle[3] > $xmax){
				$xmax = $infos_dalle[3];
			}
			if($infos_dalle[4] < $ymin){
				$ymin = $infos_dalle[4];
			}
			if($infos_dalle[2] > $ymax){
				$ymax = $infos_dalle[2];
			}
		}
		
	}
	push(@infos, \%hash_x_min, \%hash_x_max, \%hash_y_min, \%hash_y_max, \%hash_res_x, \%hash_res_y);
	if($bool_bbox == 1){
		push(@infos, $xmin, $xmax, $ymin, $ymax);
	}
	return @infos;
}
################################################################################
sub intersects{

	my $x_min_1 = $_[0];
	my $x_max_1 = $_[1];
	my $y_min_1 = $_[2];
	my $y_max_1 = $_[3];
	my $x_min_2 = $_[4];
	my $x_max_2 = $_[5];
	my $y_min_2 = $_[6];
	my $y_max_2 = $_[7];
	
	my $bool_intersects = 0;
	
	# un des 4 points de la dalle 1 est a l'interieur de la dalle 2
	# ou un des 4 points de la dalle 2 est dans la dalle 1
	# capture dans gen_cache.pl de P.PONS
	if ( ($x_min_1 < $x_max_2) && ($x_max_1 > $x_min_2) && ($y_min_1 < $y_max_2) && ($y_max_1 > $y_min_2) ){
		$bool_intersects = 1;
	}
	
	return $bool_intersects;

}
################################################################################
sub trouve_infos_pyramide{
	
	my $x_min_chantier = $_[0];
	my $x_max_chantier = $_[1];
	my $y_min_chantier = $_[2];
	my $y_max_chantier = $_[3];
	my $x_min_niveau_maxi = $_[4];
	my $x_max_niveau_maxi = $_[5];
	my $y_min_niveau_maxi = $_[6];
	my $y_max_niveau_maxi = $_[7];
	my $res_maxi = $_[8];
	my $niveau_max = $_[9];
	my $syst_dalles = $_[10];
	my $syst_pyramide = $_[11];
	
	# les calculs sont fait dans l'espace dalles
	# la remontee de niveaux se fait dans l'espace cache
	
	# initialisation a la resolution la plus grande de la pyramide
	my ($niveau_dalle_haut, $res_haut) = ($niveau_max, $res_maxi);
	my ($x_min_haut_proj_cache, $x_max_haut_proj_cache, $y_min_haut_proj_cache, $y_max_haut_proj_cache) = ($x_min_niveau_maxi, $x_max_niveau_maxi, $y_min_niveau_maxi, $y_max_niveau_maxi);
	
	my ($x_min_haut_proj_dalles, $x_max_haut_proj_dalles, $y_min_haut_proj_dalles, $y_max_haut_proj_dalles) = ($x_min_haut_proj_cache, $x_max_haut_proj_cache, $y_min_haut_proj_cache, $y_max_haut_proj_cache);
	# si on a une difference de proj, on doit reprojeter les dalles cache (fictives) en proj des images source
	if($bool_reprojection){
		($x_min_haut_proj_dalles, $x_max_haut_proj_dalles, $y_min_haut_proj_dalles, $y_max_haut_proj_dalles) = &reproj_rectangle($x_min_haut_proj_cache, $x_max_haut_proj_cache, $y_min_haut_proj_cache, $y_max_haut_proj_cache, $syst_pyramide, $syst_dalles, $dilatation_reproj);
	}
	
	my @infos_dalle_haut;
	
	# les donnees source ne peuvent pas etre au nord ou a l'ouest de l'origine
	# l'origine a ete calculee pour
	if (($x_min_haut_proj_dalles > $x_min_chantier) || ($y_max_haut_proj_dalles < $y_max_chantier)){
		print colored ("[CREE_DALLAGE_BASE] Probleme dans la definition de la pyramide : les donnees source n'y sont pas incluses.", 'white on_red');
		print "\n";
		&ecrit_log("ERREUR dans la definition de la pyramide : les donnees source n'y sont pas incluses.");
		exit;
	}
		
	# jusqu'a ce que la dalle couvre la box des donnees source
	# hypothese : toutes les dalles ont la meme taille pixel
	while( ! (intersects($x_min_haut_proj_dalles, $x_max_haut_proj_dalles, $y_min_haut_proj_dalles, $y_max_haut_proj_dalles, $x_min_chantier, $x_max_chantier, $y_min_chantier, $y_max_chantier)) ){
		$res_haut *= 2;
		$x_max_haut_proj_cache = $x_min_haut_proj_cache + $res_haut * $taille_image_pix_x;
		$y_min_haut_proj_cache = $y_max_haut_proj_cache - $res_haut * $taille_image_pix_y;
		
		# on recalcule tout le rectangle dans le cas d'une reproj
		if($bool_reprojection){
			($x_min_haut_proj_dalles, $x_max_haut_proj_dalles, $y_min_haut_proj_dalles, $y_max_haut_proj_dalles) = &reproj_rectangle($x_min_haut_proj_cache, $x_max_haut_proj_cache, $y_min_haut_proj_cache, $y_max_haut_proj_cache, $syst_pyramide, $syst_dalles, $dilatation_reproj)
		}else{
			$x_max_haut_proj_dalles = $x_max_haut_proj_cache;
			$y_min_haut_proj_dalles = $y_min_haut_proj_cache;
		}
		$niveau_dalle_haut = "$res_haut";
		# mise a jour des infos des variables globales
		$niveau_res{"$niveau_dalle_haut"} = $res_haut;
		push(@niveaux_ranges, "$niveau_dalle_haut");
	}
	
	# si besoin on repasse dans le systeme cache, la dalle haut est agmentee d'un niveau a cause des deformations 
	if($bool_reprojection){
		$res_haut *= 2;
		$x_max_haut_proj_cache = $x_min_haut_proj_cache + $res_haut * $taille_image_pix_x;
		$y_min_haut_proj_cache = $y_max_haut_proj_cache - $res_haut * $taille_image_pix_y;
		$niveau_dalle_haut = "$res_haut";
		$niveau_res{"$niveau_dalle_haut"} = $res_haut;
		push(@niveaux_ranges, "$niveau_dalle_haut");
	}
	
	my $indice_dalle_haut = @niveaux_ranges - 1;
	
	
	
	push(@infos_dalle_haut, $x_min_haut_proj_cache, $x_max_haut_proj_cache, $y_min_haut_proj_cache, $y_max_haut_proj_cache, $res_haut, $niveau_dalle_haut, $indice_dalle_haut);
	
	return @infos_dalle_haut;
	
}
################################################################################
sub definit_bloc_dalle{
	
	my $id_dalle = $_[0];
	my $x_min_dalle_cache = $_[1];
	my $x_max_dalle_cache = $_[2];
	my $y_min_dalle_cache = $_[3];
	my $y_max_dalle_cache = $_[4];
	my $res_dalle = $_[5];
	my $niveau_dalle = $_[6];
	my $indice_niveau = $_[7];
	my $ref_dalles = $_[8];
	my $type = $_[9];
	my $pcent_dilat = $_[10];
	
	my @dalles_initiales = @{$ref_dalles};
	
	my $nombre_dalles_traitees = 0;
	
	# pour les reproj
	my ($x_min_dalle_cache_proj_dalles, $x_max_dalle_cache_proj_dalles, $y_min_dalle_cache_proj_dalles, $y_max_dalle_cache_proj_dalles) = ($x_min_dalle_cache, $x_max_dalle_cache, $y_min_dalle_cache, $y_max_dalle_cache);
	# sortie si la dalle n'est pas dans la bbox des dalles source
	if ($bool_reprojection){
		($x_min_dalle_cache_proj_dalles, $x_max_dalle_cache_proj_dalles, $y_min_dalle_cache_proj_dalles, $y_max_dalle_cache_proj_dalles) = &reproj_rectangle($x_min_dalle_cache, $x_max_dalle_cache, $y_min_dalle_cache, $y_max_dalle_cache, $systeme_source, $systeme_target, $dilatation_reproj);
	}
	my $interieur_ok = 0;
	if ( intersects($x_min_dalle_cache_proj_dalles, $x_max_dalle_cache_proj_dalles, $y_min_dalle_cache_proj_dalles, $y_max_dalle_cache_proj_dalles, $x_min_bbox, $x_max_bbox, $y_min_bbox, $y_max_bbox) ){
		$interieur_ok = 1;
	}
	if($interieur_ok == 0){
		return $nombre_dalles_traitees;
	}
	
	# filtrage des dalles sources qui recouvrent
	my @dalles_recouvrantes;
	
	# on dilate la dalle cache pour le test d'intersection
	my $x_min_dilate = $x_min_dalle_cache - (($x_max_dalle_cache - $x_min_dalle_cache) * ($pcent_dilat / 100));
	my $x_max_dilate = $x_max_dalle_cache + (($x_max_dalle_cache - $x_min_dalle_cache) * ($pcent_dilat / 100));
	my $y_min_dilate = $y_min_dalle_cache - (($y_max_dalle_cache - $y_min_dalle_cache) * ($pcent_dilat / 100));
	my $y_max_dilate = $y_max_dalle_cache + (($y_max_dalle_cache - $y_min_dalle_cache) * ($pcent_dilat / 100));
	
	# pour les reproj
	my ($x_min_dilate_proj_dalles, $x_max_dilate_proj_dalles, $y_min_dilate_proj_dalles, $y_max_dilate_proj_dalles) = ($x_min_dilate, $x_max_dilate, $y_min_dilate, $y_max_dilate);
	if ($bool_reprojection){
		($x_min_dilate_proj_dalles, $x_max_dilate_proj_dalles, $y_min_dilate_proj_dalles, $y_max_dilate_proj_dalles) = &reproj_rectangle($x_min_dilate, $x_max_dilate, $y_min_dilate, $y_max_dilate, $systeme_source, $systeme_target, $dilatation_reproj);
	}
	
	foreach my $source(@dalles_initiales){
		if( intersects($x_min_dilate_proj_dalles, $x_max_dilate_proj_dalles, $y_min_dilate_proj_dalles, $y_max_dilate_proj_dalles, $source_x_min{$source}, $source_x_max{$source}, $source_y_min{$source}, $source_y_max{$source}) ){
			push(@dalles_recouvrantes, $source);
		}
	}
	
	# sortie si pas de dalles en recouvrement
	if(@dalles_recouvrantes == 0){
		return $nombre_dalles_traitees;
	}else{
		$nombre_dalles_traitees += 1;
	}
	
	# on tronconne direct en ne mettant que les dalles interessantes :
	# dans les niveaux utiles
	# TODO remplacer $res_max et $res_min par les vraies des niveaux utiles
	if( ($res_dalle <= $res_max_utile) && ($res_dalle >= $res_min_utile) ){
		if ($type eq "mtd"){
			$index_arbre_liste_mtd{$id_dalle} = \@dalles_recouvrantes;
		}elsif($type eq "image"){
			$index_arbre_liste_dalle{$id_dalle} = \@dalles_recouvrantes;
		}else{
			print colored ("[CREE_DALLAGE_BASE] Probleme de programmation : type $type incorrect.", 'white on_red');
			print "\n";
			exit;
		}
		
	}
	
	# on regarde si la dalle cache est dans la resolution_min
	if($res_dalle == $res_min){
		# la dalle cache est une dalle a faire a partir de dalles source
		$dalles_cache_plus_bas{$id_dalle} = "toto";
		return $nombre_dalles_traitees;
	}else{
		# la dalle est a faire a partir de dalles cache
		# on fait les 4 en dessous
		
		# si c'est la dalle 0 on enleve le chiffre pour retomber sur nos pattes
		if($id_dalle eq "0"){
			$id_dalle = "";
		}
		my $indice_dessous = $indice_niveau - 1;
		my $niveau_inferieur = $niveaux_ranges[$indice_dessous];
		my $res_dessous = $niveau_res{"$niveau_inferieur"};
		
		my $x_inter = $x_min_dalle_cache + $res_dessous * $taille_image_pix_x;
		my $y_inter = $y_max_dalle_cache - $res_dessous * $taille_image_pix_y;
		
		$nombre_dalles_traitees += definit_bloc_dalle($id_dalle."1", $x_min_dalle_cache, $x_inter, $y_inter, $y_max_dalle_cache, $res_dessous, $niveau_inferieur, $indice_dessous, \@dalles_recouvrantes, $type, $pcent_dilat);
	    $nombre_dalles_traitees += definit_bloc_dalle($id_dalle."2", $x_inter, $x_max_dalle_cache, $y_inter, $y_max_dalle_cache, $res_dessous, $niveau_inferieur, $indice_dessous, \@dalles_recouvrantes, $type, $pcent_dilat);
	    $nombre_dalles_traitees += definit_bloc_dalle($id_dalle."3", $x_inter, $x_max_dalle_cache, $y_min_dalle_cache, $y_inter, $res_dessous, $niveau_inferieur, $indice_dessous, \@dalles_recouvrantes, $type, $pcent_dilat);
	    $nombre_dalles_traitees += definit_bloc_dalle($id_dalle."4", $x_min_dalle_cache, $x_inter, $y_min_dalle_cache, $y_inter, $res_dessous, $niveau_inferieur, $indice_dessous, \@dalles_recouvrantes, $type, $pcent_dilat);
		
		# on remplit les niveaux des dalles d'en dessous
		$dalle_arbre_niveau{$id_dalle."1"} = $niveau_inferieur;
		$dalle_arbre_niveau{$id_dalle."2"} = $niveau_inferieur;
		$dalle_arbre_niveau{$id_dalle."3"} = $niveau_inferieur;
		$dalle_arbre_niveau{$id_dalle."4"} = $niveau_inferieur;
		
		$dalle_arbre_indice_niveau{$id_dalle."1"} = $indice_dessous;
		$dalle_arbre_indice_niveau{$id_dalle."2"} = $indice_dessous;
		$dalle_arbre_indice_niveau{$id_dalle."3"} = $indice_dessous;
		$dalle_arbre_indice_niveau{$id_dalle."4"} = $indice_dessous;
		
	}
	
	return $nombre_dalles_traitees;
}
################################################################################
sub calcule_niveau_minimum {
	
	my $ref_hash_list_dal_source = $_[0];
	my $rep_enregistr = $_[1];
	my $type = $_[2];
	my %hash_dalle_cache_dalles_source = %{$ref_hash_list_dal_source};
	
	my $nb_dal = 0;
	
	while ( my($dalle_cache, $ref_dalles_source) = each %hash_dalle_cache_dalles_source ){
		if(defined $ref_dalles_source ){
			&ecrit_log("Calcul de $dalle_cache.");
			my @liste_dalles_source = @{$ref_dalles_source};
			print ".";
			
			# creation le cas echeant des repertoires parents
			my $rep_parent_dalle = dirname($dalle_cache);
			&ecrit_log("Creation des eventuels repertoires manquants de $rep_parent_dalle.");
			&cree_repertoires_recursifs($rep_parent_dalle);
			
			my $nom_dalle = $nom_dalle_index_base{$dalle_cache};
			
			my $nom_dalle_temp = "$rep_temp/$nom_dalle.tif";
			# mise en format travail dalle_cache pour image initiale
			# destruction de la dalle_cache temporaire si elle existe
			print SCRIPT "if [ -r \"$nom_dalle_temp\" ] ; then rm -f $nom_dalle_temp ; fi\n";
# 			# >> pour le script
# 			if(-e $nom_dalle_temp && -f $nom_dalle_temp){
# 				my $suppr = unlink($nom_dalle_temp);
# 				if($suppr != 1){
# 					&ecrit_log("ERREUR a la destruction de $nom_dalle_temp.");
# 				}
# 			}
			
			# test si on a une dalle (normalement un lien vers une pyramide precedente qui est en format pyramide)
			if (-e $dalle_cache ){
				print SCRIPT "$programme_copie_image -s -r $taille_dalle_pix $dalle_cache $nom_dalle_temp 2>&1\n";
# 				# >> pour le script
# 				&ecrit_log("Copie raw de $dalle_cache.");
# 				&ecrit_log("Execution de $programme_copie_image -s -r $taille_dalle_pix $dalle_cache $nom_dalle_temp");
# 				system("$programme_copie_image -s -r $taille_dalle_pix $dalle_cache $nom_dalle_temp >>$log 2>&1");
# 				if(!(-e $nom_dalle_temp && -f $nom_dalle_temp)){
# 					&ecrit_log("ERREUR a la copie raw de $dalle_cache.");
# 				}
				# suppression du lien symbolique preexistant sinon la creation va abimer les anciennes pyramides
				print SCRIPT "if [ -L \"$dalle_cache\" ] ; then rm -f $dalle_cache ; fi\n";
# 				# >> pour le script
# 				if(-l $dalle_cache){
# 					&ecrit_log("Suppression du lien symbolique $dalle_cache.");
# 					unlink($dalle_cache);
# 					if (-l $dalle_cache){
# 						&ecrit_log("ERREUR a la suppression du lien symbolique $dalle_cache.");
# 					}
# 				}
			}else{
				# sinon on fait reference a la dalle no_data
				if($type eq "image"){
					$nom_dalle_temp = $dalle_no_data;
				}elsif($type eq "mtd"){
					$nom_dalle_temp = $dalle_no_data_mtd;
				}else{
					print colored ("[CREE_DALLAGE_BASE] Probleme de programmation : type $type incorrect.", 'white on_red');
					print "\n";
					exit;
				}
			}
			
			
			my $nom_fichier = $rep_enregistr."/".$nom_dalle."_".$type;
			
			my $res_x_max_source = 0;
			my $res_y_max_source = 0;
			
			open FIC, ">$nom_fichier" or die colored ("[CREE_DALLAGE_BASE] Impossible de creer le fichier $nom_fichier.", 'white on_red');
			# dalle cache
			print FIC "$dalle_cache\t$cache_arbre_x_min{$dalle_cache}\t$cache_arbre_y_max{$dalle_cache}\t$cache_arbre_x_max{$dalle_cache}\t$cache_arbre_y_min{$dalle_cache}\t$cache_arbre_res{$dalle_cache}\t$cache_arbre_res{$dalle_cache}\n";
			# dalles source a la suite
			
			# d'abord la dalle cache temporaire
			# (puisque $programme_dalles_base ne supporte pas de ne pas avoir de donnees)
			
			print FIC "$nom_dalle_temp\t$cache_arbre_x_min{$dalle_cache}\t$cache_arbre_y_max{$dalle_cache}\t$cache_arbre_x_max{$dalle_cache}\t$cache_arbre_y_min{$dalle_cache}\t$cache_arbre_res{$dalle_cache}\t$cache_arbre_res{$dalle_cache}\n";
			
# XXXX			# TODO supprimer la string des dalles source pour GDAL
# XXXX			# my $gdal_source = "";
			foreach my $src(@liste_dalles_source){
# XXXX				# $gdal_source .= "$src ";
				if($source_res_x{$src} > $res_x_max_source){
					$res_x_max_source = $source_res_x{$src};
				}
				if($source_res_y{$src} > $res_y_max_source){
					$res_y_max_source = $source_res_y{$src};
				}
				print FIC "$src\t$source_x_min{$src}\t$source_y_max{$src}\t$source_x_max{$src}\t$source_y_min{$src}\t$source_res_x{$src}\t$source_res_y{$src}\n";
			}
			
			# ATTENTION, intuition feminine : l'execution dans certains environnement fait qu'il se melange les pinceaux
			#et cree des fichiers vides, en le faisant dormir, ca a l'air de reparer
			sleep(1);
			
			close FIC;
			# definition de l'interpolateur
			my $interpolateur = "bicubique";
# XXXX			# TODO a supprimer le interpol_gdal
# XXXX			# my $interpol_gdal = "near";
			
			if($type eq "mtd"){
				$interpolateur = "ppv";
			}elsif(($cache_arbre_res{$dalle_cache} / $res_x_max_source < 2) && ($cache_arbre_res{$dalle_cache} / $res_y_max_source)){
				$interpolateur = "lanczos";
			}
			# definition de la couleur no_data
			my $no_data;
			if($type eq "mtd"){
				$no_data = "0";
			}elsif( $type eq "image"){
				$no_data = $color_no_data;
# XXXX				# TODO transformer en RGB le hexa ??
# XXXX				$no_data = "\"255 255 255 \"";
			}else{
				print colored ("[CREE_DALLAGE_BASE] Probleme de programmation : type $type incorrect.", 'white on_red');
				print "\n";
				exit;
			}
			
# XXXX			# desctruction de la dalle temporaire si elle existe
# XXXX			if(-e "$rep_temp/temp.tif" && -f "$rep_temp/temp.tif"){
# XXXX				my $suppr = unlink("$rep_temp/temp.tif");
# XXXX				if($suppr != 1){
# XXXX					&ecrit_log("ERREUR a la destruction de $rep_temp/temp.tif.");
# XXXX				}
# XXXX			}
# XXXX			
			
# XXXX			
# XXXX			# copie du tfw en plus pour GDAL
# XXXX			my $nom_tfw = substr($dalle_cache, 0, length($dalle_cache) - 4).".tfw";
# XXXX			my $return2 = copy($nom_tfw, "$rep_temp/cache.tfw");
# XXXX			if ($return2 == 0){
# XXXX				&ecrit_log("ERREUR a la copie de $nom_tfw vers $rep_temp/cache.tfw");
# XXXX			}
# XXXX			
# XXXX			# lecture du lien de la dalle cache pour GDAL qui comprend rien 
# XXXX			my $fichier_pointe = readlink("$dalle_cache");
# XXXX			if ( ! defined $fichier_pointe){
# XXXX				$fichier_pointe = "$dalle_cache";
# XXXX			}
# XXXX			
# XXXX			system("gdalwarp --config GDAL_CACHEMAX 512 -of GTiff -co PROFILE=BASELINE -te $cache_arbre_x_min{$dalle_cache} $cache_arbre_y_min{$dalle_cache} $cache_arbre_x_max{$dalle_cache} $cache_arbre_y_max{$dalle_cache} -tr $cache_arbre_res{$dalle_cache} $cache_arbre_res{$dalle_cache} -r $interpol_gdal -dstnodata $no_data $fichier_pointe $gdal_source $rep_temp/temp.tif >>$log 2>&1");
# XXXX			system("gdalwarp --config GDAL_CACHEMAX 512 -of GTiff -co PROFILE=BASELINE -te $cache_arbre_x_min{$dalle_cache} $cache_arbre_y_min{$dalle_cache} $cache_arbre_x_max{$dalle_cache} $cache_arbre_y_max{$dalle_cache} -tr $cache_arbre_res{$dalle_cache} $cache_arbre_res{$dalle_cache} -r $interpol_gdal -dstnodata $no_data $rep_temp/cache.tif $gdal_source $rep_temp/temp.tif >>$log 2>&1");
			
			# pour le programme
			my $type_dalles_base = $type;
			if($type_dalles_base eq "image"){
				$type_dalles_base = "img";
			}
			# TODO nombre de canaux, nombre de bits, couleur en parametre
			# TODO supprimer no_data qui ne sert a rien
			print SCRIPT "$programme_dalles_base -f $nom_fichier -i $interpolateur -n $no_data -t $type_dalles_base -s 3 -b 8 -p rgb 2>&1\n";
# 			# >> pour le script
# 			&ecrit_log("Execution de : $programme_dalles_base -f $nom_fichier -i $interpolateur -n $no_data -t $type_dalles_base -s 3 -b 8 -p rgb");
# 			system("$programme_dalles_base -f $nom_fichier -i $interpolateur -n $no_data -t $type_dalles_base -s 3 -b 8 -p rgb >>$log 2>&1");
			
# XXXX			# TODO voir si sans GDAL, la suppression est necessaire
# XXXX			# suppression de la dalle existante (blanche ou lien) avant remplacement
# XXXX			if(-e $dalle_cache && -f $dalle_cache){
# XXXX				my $suppr = unlink("$dalle_cache");
# XXXX				if($suppr != 1){
# XXXX					&ecrit_log("ERREUR a la destruction de $dalle_cache.");
# XXXX				}
# XXXX			}
# XXXX			my $bool_success = move ("$rep_temp/temp.tif","$dalle_cache");
# XXXX			if($bool_success == 0){
# XXXX				&ecrit_log("ERREUR au renommage $dalle_cache.");
# XXXX			}
			
			$nb_dal += 1;
		}
	}
	print "\n";
	return $nb_dal;
	
}
################################################################################
sub calcule_niveaux_inferieurs{
	
	my $ref_dalles_a_calc = $_[0];
	my $interpol = $_[1];
	my $ref_niveau_bas = $_[2];
	my $type_dalle = $_[3];
	
	my %hash_niveau_liste = %{$ref_dalles_a_calc};
	
	my $nb_calc = 0;
	
	for (my $i = 1 ; $i < @niveaux_ranges ; $i++){
		my $niveau_inf = $niveaux_ranges[$i];
		if(defined $hash_niveau_liste{"$niveau_inf"}){
			&ecrit_log("Calcul niveau $niveau_inf.");
			print "[CREE_DALLAGE_BASE] Calcul niveau $niveau_inf\n";
			my @dalles_a_calc = @{$hash_niveau_liste{"$niveau_inf"}};
			foreach my $dal(@dalles_a_calc){
				if (defined $dalle_cache_dessous{$dal}){
					&ecrit_log("Calcul de $dal.");
					my @list_cache_dessous = @{$dalle_cache_dessous{$dal}};
					print ".";
					
					# creation le cas echeant des repertoires parents
					my $rep_parent_dalle_niveau_inf = dirname($dal);
					&ecrit_log("Creation des eventuels repertoires manquants de $rep_parent_dalle_niveau_inf.");
					&cree_repertoires_recursifs($rep_parent_dalle_niveau_inf);
					
					my $string_dessous = "";
					foreach my $dalle_dessous(@list_cache_dessous){
						my $fichier_pointe;
						# si la dalle n'existe pas on met la dalle no_data
						if(!(-e $dalle_dessous)){
							if($type_dalle eq "image"){
								$fichier_pointe = $dalle_no_data;
							}elsif($type_dalle eq "mtd"){
								$fichier_pointe = $dalle_no_data_mtd;
							}else{
								print colored ("[CREE_DALLAGE_BASE] Probleme de programmation : type $type_dalle incorrect.", 'white on_red');
								print "\n";
								exit;
							}
							
						}else{
# 							# lecture du lien de la dalle cache pour test 
# 							$fichier_pointe = readlink("$dalle_dessous");
# 							if ( ! defined $fichier_pointe){
# 								$fichier_pointe = "$dalle_dessous";
#							}
							
							my $nom_dalle_cache = basename($dalle_dessous);
							
							# mise en format travail de la dalle dalle_cache
							# desctruction de la dalle_cache temporaire si elle existe
							print SCRIPT "if [ -r \"$rep_temp/$nom_dalle_cache\" ] ; then rm -f $rep_temp/$nom_dalle_cache ; fi\n";
# 							# >> pour le script
# 							if(-e "$rep_temp/$nom_dalle_cache" && -f "$rep_temp/$nom_dalle_cache"){
# 								my $suppr = unlink("$rep_temp/$nom_dalle_cache");
# 								if($suppr != 1){
# 									&ecrit_log("ERREUR a la destruction de $rep_temp/$nom_dalle_cache.");
# 								}
# 							}
							print SCRIPT "$programme_copie_image -s -r $taille_dalle_pix $dalle_dessous $rep_temp/$nom_dalle_cache 2>&1\n";
# 							# >> pour le script
# 							&ecrit_log("Copie raw de $dalle_dessous.");
# 							&ecrit_log("Execution de : $programme_copie_image -s -r $taille_dalle_pix $dalle_dessous $rep_temp/$nom_dalle_cache");
# 							system("$programme_copie_image -s -r $taille_dalle_pix $dalle_dessous $rep_temp/$nom_dalle_cache >>$log 2>&1");
# 							if(!(-e "$rep_temp/$nom_dalle_cache" && -f "$rep_temp/$nom_dalle_cache")){
# 								&ecrit_log("ERREUR a la copie raw de $dalle_dessous.");
# 							}
							$fichier_pointe = "$rep_temp/$nom_dalle_cache";
						}
						$string_dessous .= " $fichier_pointe";
					}
					# TODO ajouter l'interpolation dans la ligne de commande -i $interpol!!
					print SCRIPT "$programme_ss_ech $string_dessous $dal 2>&1\n";
# 		 			# >> pour le script
# 					&ecrit_log("Execution de : $programme_ss_ech $string_dessous $dal");
# 					system("$programme_ss_ech $string_dessous $dal >>$log 2>&1");
					# ou my $return = `$programme_ss_ech -i $interpol $string_dessous $dal`;
					$nb_calc += 1; 
				}
				
			}
			print "\n";
			
			# Passage en format pivot du niveaux dessous
			
			my $niveau_termine = $niveaux_ranges[$i - 1];
			my @dalles_change_format;
			# si c'est le niveau le plus bas on n'a pas le hash
			if($niveau_termine eq "$level_min" && defined $ref_niveau_bas){
				@dalles_change_format = keys %{$ref_niveau_bas};
			}elsif(defined $hash_niveau_liste{"$niveau_termine"}){
				@dalles_change_format = @{$hash_niveau_liste{"$niveau_termine"}};
			}
			&ecrit_log("Passage en format final niveau $niveau_termine.");
			print "[CREE_DALLAGE_BASE] Passage en format final niveau $niveau_termine\n";
			&passage_pivot($niveau_taille_tuile_x{"$niveau_termine"}, $niveau_taille_tuile_y{"$niveau_termine"}, \@dalles_change_format);
		}
	}
	
	# Passage en format pivot du dernier niveau
	my $dernier_niveau = "$level_max";
	&ecrit_log("Passage en format final niveau $dernier_niveau.");
	print "[CREE_DALLAGE_BASE] Passage en format final niveau $dernier_niveau\n";
	my @dalles_change_format_haut;
	if(defined $hash_niveau_liste{"$dernier_niveau"}){
		@dalles_change_format_haut = @{$hash_niveau_liste{"$dernier_niveau"}};
	} 
	&passage_pivot($niveau_taille_tuile_x{"$dernier_niveau"}, $niveau_taille_tuile_y{"$dernier_niveau"}, \@dalles_change_format_haut);
	
	return $nb_calc;

}
################################################################################

sub arbre2cache{
	
	my @liste_toutes_dalles = @{$_[0]};
	
	my $bool_ok = 0;
	
	foreach my $dalle_arbre(@liste_toutes_dalles){
		#transformer en x_y
		my ($x_min_dalle_arbre, $x_max_dalle_arbre, $y_min_dalle_arbre, $y_max_dalle_arbre) = &indice_arbre2xy("$dalle_arbre", $x_min_dalle0, $x_max_dalle0, $y_min_dalle0, $y_max_dalle0);
		my $niveau_dalle = $dalle_arbre_niveau{"$dalle_arbre"};
		my $indice_niveau_dalle = $dalle_arbre_indice_niveau{"$dalle_arbre"};
		my $x_m_origine = $niveau_origine_x{"$niveau_dalle"};
		my $y_m_origine = $niveau_origine_y{"$niveau_dalle"};
		my $taille_dalle_x = $niveau_taille_m_x{"$niveau_dalle"};
		my $taille_dalle_y = $niveau_taille_m_y{"$niveau_dalle"};
		my $rep_niveau_dalle = $niveau_repertoire_image{"$niveau_dalle"};
		my $profondeur = $niveau_profondeur{"$niveau_dalle"};
		my $rep_niveau_mtd = $niveau_repertoire_mtd{"$niveau_dalle"};
		# transformer en nomenclature de pyramide
		my $new_nom_dalle = &nom_dalle_cache($x_min_dalle_arbre, $y_max_dalle_arbre, $x_m_origine, $y_m_origine, $taille_dalle_x, $taille_dalle_y, $rep_niveau_dalle, $profondeur);
		my $new_nom_mtd = &nom_dalle_cache($x_min_dalle_arbre, $y_max_dalle_arbre, $x_m_origine, $y_m_origine, $taille_dalle_x, $taille_dalle_y, $rep_niveau_mtd, $profondeur);
		push (@liste_total_dalles_cache, $new_nom_dalle);
		push (@liste_total_dalles_cache_mtd, $new_nom_mtd);
		
		# remplissage des infos de la dalle cache
# XXXX		# pour GDAL
# XXXX		$cache_arbre_niveau{$new_nom_dalle} = $niveau_dalle;
		$cache_arbre_x_min{$new_nom_dalle} = $x_min_dalle_arbre;
		$cache_arbre_x_max{$new_nom_dalle} = $x_max_dalle_arbre;
		$cache_arbre_y_min{$new_nom_dalle} = $y_min_dalle_arbre;
		$cache_arbre_y_max{$new_nom_dalle} = $y_max_dalle_arbre;
		$cache_arbre_res{$new_nom_dalle} = $niveau_res{"$niveau_dalle"};
		
# XXXX		# pour GDAL
# XXXX		$cache_arbre_niveau{$new_nom_mtd} = $niveau_dalle;
		$cache_arbre_x_min{$new_nom_mtd} = $cache_arbre_x_min{$new_nom_dalle};
		$cache_arbre_x_max{$new_nom_mtd} = $cache_arbre_x_max{$new_nom_dalle};
		$cache_arbre_y_min{$new_nom_mtd} = $cache_arbre_y_min{$new_nom_dalle};
		$cache_arbre_y_max{$new_nom_mtd} = $cache_arbre_y_max{$new_nom_dalle};
		$cache_arbre_res{$new_nom_mtd} = $cache_arbre_res{$new_nom_dalle};
		
		# si la dalle est dans le niveau le plus bas
		if(exists $dalles_cache_plus_bas{"$dalle_arbre"}){
			my $ref_liste_dalles = $index_arbre_liste_dalle{"$dalle_arbre"};
			$dalle_cache_min_liste_dalle{$new_nom_dalle} = $ref_liste_dalles;
			my $ref_liste_mtd = $index_arbre_liste_mtd{"$dalle_arbre"};
			$mtd_cache_min_liste_mtd{$new_nom_mtd} = $ref_liste_mtd;
		}else{
			my $indice_niveau_dessous = $indice_niveau_dalle - 1;
			my $niveau_dessous = $niveaux_ranges[$indice_niveau_dessous];
			my $rep_dessous =  $niveau_repertoire_image{"$niveau_dessous"};
			my $rep_dessous_mtd = $niveau_repertoire_mtd{"$niveau_dessous"};
			my $res_dessous = $niveau_res{"$niveau_dessous"};
			my $taille_x_dessous = $taille_dalle_x / 2;
			my $taille_y_dessous = $taille_dalle_y / 2;
			my $x_intermediaire = $x_min_dalle_arbre + $res_dessous * $taille_image_pix_x;
			my $y_intermediaire = $y_max_dalle_arbre - $res_dessous * $taille_image_pix_y;
			
			# meme principe que l'arbre
			my $dalle1 = &nom_dalle_cache($x_min_dalle_arbre, $y_max_dalle_arbre, $x_m_origine, $y_m_origine, $taille_x_dessous, $taille_y_dessous, $rep_dessous, $profondeur);
			my $dalle2 = &nom_dalle_cache($x_intermediaire, $y_max_dalle_arbre, $x_m_origine, $y_m_origine, $taille_x_dessous, $taille_y_dessous, $rep_dessous, $profondeur);
			my $dalle3 = &nom_dalle_cache($x_intermediaire, $y_intermediaire, $x_m_origine, $y_m_origine, $taille_x_dessous, $taille_y_dessous, $rep_dessous, $profondeur);
			my $dalle4 = &nom_dalle_cache($x_min_dalle_arbre, $y_intermediaire, $x_m_origine, $y_m_origine, $taille_x_dessous, $taille_y_dessous, $rep_dessous, $profondeur);
			# ATTENTION DEPEND DE L'ORDRE DANS LEQUEL TRAVAILLE $programme_ss_ech
			my @liste_cache_dalle = ($dalle1, $dalle2, $dalle4, $dalle3);
			$dalle_cache_dessous{$new_nom_dalle} = \@liste_cache_dalle;
			
			my $mtd1 = &nom_dalle_cache($x_min_dalle_arbre, $y_max_dalle_arbre, $x_m_origine, $y_m_origine, $taille_x_dessous, $taille_y_dessous, $rep_dessous_mtd, $profondeur);
			my $mtd2 = &nom_dalle_cache($x_intermediaire, $y_max_dalle_arbre, $x_m_origine, $y_m_origine, $taille_x_dessous, $taille_y_dessous, $rep_dessous_mtd, $profondeur);
			my $mtd3 = &nom_dalle_cache($x_intermediaire, $y_intermediaire, $x_m_origine, $y_m_origine, $taille_x_dessous, $taille_y_dessous, $rep_dessous_mtd, $profondeur);
			my $mtd4 = &nom_dalle_cache($x_min_dalle_arbre, $y_intermediaire, $x_m_origine, $y_m_origine, $taille_x_dessous, $taille_y_dessous, $rep_dessous_mtd, $profondeur);
			my @liste_cache_mtd = ($mtd1, $mtd2, $mtd4, $mtd3);
			$dalle_cache_dessous{$new_nom_mtd} = \@liste_cache_mtd;
			
			#remplissage de la correspondance entre niveau et liste de dalles inferieures (pour les calculer dans l'ordre)
			my @liste_prov_dalle;
			if (defined $niveau_ref_dalles_inf{"$niveau_dalle"}){
				@liste_prov_dalle = @{$niveau_ref_dalles_inf{"$niveau_dalle"}};
			} 
			push(@liste_prov_dalle, $new_nom_dalle);
			$niveau_ref_dalles_inf{"$niveau_dalle"} = \@liste_prov_dalle;
			my @liste_prov_mtd;
			if (defined $niveau_ref_mtd_inf{"$niveau_dalle"}){
				@liste_prov_mtd = @{$niveau_ref_mtd_inf{"$niveau_dalle"}};
			} 
			push(@liste_prov_mtd, $new_nom_mtd);
			$niveau_ref_mtd_inf{"$niveau_dalle"} = \@liste_prov_mtd;
		}
		
	}
	
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
sub cree_arbre_dalles_cache{
	
	my $ref_liste = $_[0];
	my $type = $_[1];
	my $pourcentage = $_[2];
	
	my $nombre_dalles = 0;
	$nombre_dalles += definit_bloc_dalle("0", $x_min_dalle0, $x_max_dalle0, $y_min_dalle0, $y_max_dalle0, $res_dalle0, $niveau_dalle0, $indice_niveau0, $ref_liste, $type, $pourcentage);
	
	return $nombre_dalles;
}
################################################################################
sub lecture_pyramide{
	
	my $xml_pyramide = $_[0];
	
	my (%id_rep_images, %id_rep_mtd, %id_res, %id_taille_m_x, %id_taille_m_y, %id_origine_x, %id_origine_y, %id_profondeur, %id_taille_pix_tuile_x, %id_taille_pix_tuile_y);
	
	my @refs_infos_levels;
	
	my $xml_fictif = new XML::Simple(KeyAttr=>[]);

	# lire le fichier XML
	my $data = $xml_fictif->XMLin("$xml_pyramide");
	
	my $nom_tms = $data->{tileMatrixSet};
	my $tms_complet = $path_tms."/".$nom_tms.".tms";
	
	my ($ref_inutile1, $ref_id_resolution, $ref_id_taille_pix_tuile_x, $ref_id_taille_pix_tuile_y, $ref_id_origine_x, $ref_id_origine_y, $srs) = &lecture_tile_matrix_set($tms_complet);
	my %tms_level_resolution = %{$ref_id_resolution};
	my %tms_level_taille_pix_tuile_x = %{$ref_id_taille_pix_tuile_x};
	my %tms_level_taille_pix_tuile_y = %{$ref_id_taille_pix_tuile_y};
	my %tms_level_origine_x = %{$ref_id_origine_x};
	my %tms_level_origine_y = %{$ref_id_origine_y};
	
	#fonction de tri des niveaux par resolution
	my @niveaux_croissants = sort { $tms_level_resolution{"$a"} <=> $tms_level_resolution{"$b"}} keys %tms_level_resolution;
	
	# liste des formats (cles uniquement)
	my %formats;
	my $format_images_pyramide;
	
	foreach my $level (@{$data->{level}}){
		my $id = $level->{tileMatrix};
		my $rep1 = $level->{baseDir};
		if (substr($rep1, 0, 1) eq "/" ){
			$id_rep_images{"$id"} = $rep1;
		}else{
			$id_rep_images{"$id"} = abs_path($rep1);
		}
		my $format_level = $level->{format};
		$formats{$format_level} = "toto";
		my $metadata = $level->{metadata};
		my $rep2 = $metadata->{baseDir};
		if (substr($rep2, 0, 1) eq "/" ){
			$id_rep_mtd{"$id"} = $rep2;
		}else{
			$id_rep_mtd{"$id"} = abs_path($rep2);
		}
		$id_profondeur{"$id"} = $level->{pathDepth};
		# lecture dans le  TMS associe
		$id_res{"$id"} = $tms_level_resolution{"$id"};
		$id_origine_x{"$id"} = $tms_level_origine_x{"$id"};
		$id_origine_y{"$id"} = $tms_level_origine_y{"$id"};
		$id_taille_pix_tuile_x{"$id"} = $tms_level_taille_pix_tuile_x{"$id"};
		$id_taille_pix_tuile_y{"$id"} = $tms_level_taille_pix_tuile_y{"$id"};
		$id_taille_m_x{"$id"} = $id_res{"$id"} * $id_taille_pix_tuile_x{"$id"} * $level->{tilesPerWidth};
		$id_taille_m_y{"$id"} = $id_res{"$id"} * $id_taille_pix_tuile_y{"$id"} * $level->{tilesPerHeight};
	}
	
	# verification de l'unicite des formats
	my @formats_temp = keys %formats;
	my $nb_formats_temp = @formats_temp;
	if ($nb_formats_temp == 0){
		&ecrit_log("ERREUR Pas de formats d'images trouves dans le fichier $xml_pyramide.");
		print colored ("[CREE_DALLAGE_BASE] Pas de formats d'images trouves dans le fichier $xml_pyramide.", 'white on_red');
		print "\n";
		exit;
	}else{
		if($nb_formats_temp > 1){
			&ecrit_log("WARNING Plusieurs formats d'images trouves dans le fichier $xml_pyramide => retenu : $formats_temp[0].");
			print colored ("[CREE_DALLAGE_BASE] WARNING Plusieurs formats d'images trouves dans le fichier $xml_pyramide => retenu : $formats_temp[0].", 'white on_red');
			print "\n";
		}
		$format_images_pyramide = $formats_temp[0];
	}
	
	push(@refs_infos_levels, \@niveaux_croissants, \%id_rep_images, \%id_rep_mtd, \%id_res, \%id_taille_m_x, \%id_taille_m_y, \%id_origine_x, \%id_origine_y, \%id_profondeur, \%id_taille_pix_tuile_x, \%id_taille_pix_tuile_y, $srs, $format_images_pyramide);
	
	return @refs_infos_levels;
	
}
################################################################################
sub passage_pivot{
	
	my $taille_pix_x_tuile = $_[0];
	my $taille_pix_y_tuile = $_[1];
	my $ref_dalles_travail = $_[2];
	my @dalles_travail = @{$ref_dalles_travail};
	
	foreach my $dal2(@dalles_travail){
		&ecrit_log("Passage en format pivot de $dal2.");
		print ".";
# 		# >> pour le script
		print SCRIPT "if [ -r \"$rep_temp/temp.tif\" ] ; then rm -f $rep_temp/temp.tif ; fi\n";
# 		if ( !(-e $dal2) ){
# 			print "[CREE_DALLAGE_BASE] La dalle $dal2 n'existe pas.\n";
# 			&ecrit_log("ERREUR : la dalle $dal2 n'existe pas (passage en pivot).");
# 			next;
# 		}
# 		if( -e "$rep_temp/temp.tif" && -f "$rep_temp/temp.tif"){
# 			&ecrit_log("Destruction de $rep_temp/temp.tif.");
# 			my $suppr2 = unlink("$rep_temp/temp.tif");
# 			if($suppr2 != 1){
# 				&ecrit_log("ERREUR a la destruction de $rep_temp/temp.tif.");
# 			}
# 		}
		
		# TODO introduire la couleur dans $programme_format_pivot
		print SCRIPT "$programme_format_pivot $dal2 -c $compress -t $taille_pix_x_tuile $taille_pix_y_tuile $rep_temp/temp.tif 2>&1\n";
# 		# >> pour le script
# 		&ecrit_log("Execution de : $programme_format_pivot $dal2 -c $compress -t $taille_pix_x_tuile $taille_pix_y_tuile $rep_temp/temp.tif");
# 		system("$programme_format_pivot $dal2 -c $compress -t $taille_pix_x_tuile $taille_pix_y_tuile $rep_temp/temp.tif >>$log 2>&1");
		#ou my $return = `$programme_format_pivot $dal2 -c $compress -t $taille_pix_x_tuile $taille_pix_y_tuile $rep_temp/temp.tif`;
		print SCRIPT "rm -f $dal2\n";
# 		# >> pour le script
# 		my $suppr = unlink("$dal2");
# 		if($suppr != 1){
# 			&ecrit_log("ERREUR a la destruction de $dal2.");
# 		}
		print SCRIPT "mv $rep_temp/temp.tif $dal2\n";
# 		# >> pour le script
# 		my $bool_success = move ("$rep_temp/temp.tif","$dal2");
# 		if($bool_success == 0){
# 			&ecrit_log("ERREUR au renommage $dal2.");
# 		}
	}
	print "\n";
	
}
################################################################################
sub reproj_point{

	my $x_point = $_[0];
	my $y_point = $_[1];
	my $srs_ini = $_[2];
	my $srs_fin = $_[3];
	
	my $x_reproj;
	my $y_reproj;
	
	my $result = `echo $x_point $y_point | $programme_reproj +init=$srs_ini +to +init=$srs_fin`;
	my @split2 = split /\s/, $result;
	if(defined $split2[0] && defined $split2[1]){
		$x_reproj = $split2[0];
		$y_reproj = $split2[1];
	}else{
		print colored ("[CREE_DALLAGE_BASE] Erreur a la reprojection de $x_point $y_point $srs_ini en $srs_fin.", 'white on_red');
		print "\n";
		&ecrit_log("ERREUR a la reprojection de $x_point $y_point $srs_ini en $srs_fin.");
	}
	
	return ($x_reproj, $y_reproj);
}
################################################################################
sub reproj_rectangle{

	my $x_min_poly = $_[0];
	my $x_max_poly = $_[1];
	my $y_min_poly = $_[2];
	my $y_max_poly = $_[3];
	my $srs_ini_poly = $_[4];
	my $srs_fin_poly = $_[5];
	my $dilat_securite = $_[6];
	
	my ($x_min_reproj, $x_max_reproj, $y_min_reproj, $y_max_reproj);
	
	# schema du rectangle
	# 12
	# 43
	my ($x1,$y1) = &reproj_point($x_min_poly, $y_max_poly, $srs_ini_poly, $srs_fin_poly);
	my ($x2,$y2) = &reproj_point($x_max_poly, $y_max_poly, $srs_ini_poly, $srs_fin_poly);
	my ($x3,$y3) = &reproj_point($x_max_poly, $y_min_poly, $srs_ini_poly, $srs_fin_poly);
	my ($x4,$y4) = &reproj_point($x_min_poly, $y_min_poly, $srs_ini_poly, $srs_fin_poly);
	
	# determination de la bbox resultat
	my $x_min_result = 99999999999;
	if($x1 < $x_min_result){
		$x_min_result = $x1;
	}
	if($x2 < $x_min_result){
		$x_min_result = $x2;
	}
	if($x3 < $x_min_result){
		$x_min_result = $x3;
	}
	if($x4 < $x_min_result){
		$x_min_result = $x4;
	}
	my $x_max_result = -99999999999;
	if($x1 > $x_max_result){
		$x_max_result = $x1;
	}
	if($x2 > $x_max_result){
		$x_max_result = $x2;
	}
	if($x3 > $x_max_result){
		$x_max_result = $x3;
	}
	if($x4 > $x_max_result){
		$x_max_result = $x4;
	}
	my $y_min_result = 99999999999;
	if($y1 < $y_min_result){
		$y_min_result = $y1;
	}
	if($y2 < $y_min_result){
		$y_min_result = $y2;
	}
	if($y3 < $y_min_result){
		$y_min_result = $y3;
	}
	if($y4 < $y_min_result){
		$y_min_result = $y4;
	}
	my $y_max_result = -99999999999;
	if($y1 > $y_max_result){
		$y_max_result = $y1;
	}
	if($y2 > $y_max_result){
		$y_max_result = $y2;
	}
	if($y3 > $y_max_result){
		$y_max_result = $y3;
	}
	if($y4 > $y_max_result){
		$y_max_result = $y4;
	}
	
	# dilatataion de la bbox resultat
	my $dilat_x = ($x_max_result - $x_min_result) * ($dilat_securite / 100);
	my $dilat_y = ($y_max_result - $y_min_result) * ($dilat_securite / 100);
	$x_min_reproj = $x_min_result - $dilat_x;
	$x_max_reproj = $x_max_result + $dilat_x;
	$y_min_reproj = $y_min_result - $dilat_y;
	$y_max_reproj = $y_max_result + $dilat_y;
	
	return ($x_min_reproj, $x_max_reproj, $y_min_reproj, $y_max_reproj);
}
################################################################################
# XXXXTODO a supprimer sans GDAL
# XXXXsub cree_string_tfw{
# XXXX	my $x_min_tfw = $_[0];
# XXXX	my $y_max_tfw = $_[1];
# XXXX	my $resolution_tfw = $_[2];
# XXXX	my $rotation_tfw = $_[3];
# XXXX	
# XXXX	my $tfw = '';
# XXXX	$tfw.= sprintf "%.2f\n%.2f\n%.2f\n%.2f\n", $resolution_tfw, $rotation_tfw, $rotation_tfw, $resolution_tfw * -1;
# XXXX	$tfw.= sprintf "%.2f\n", $x_min_tfw + 0.5 * $resolution_tfw;
# XXXX	$tfw.= sprintf "%.2f\n", $y_max_tfw - 0.5 * $resolution_tfw;
# XXXX	return $tfw;
# XXXX}
# XXXXsub cree_fichier_georef{
# XXXX	
# XXXX	my $nom_image_georef = $_[0];
# XXXX	my $repertoire_georef = $_[1];
# XXXX	my $type = $_[2];
# XXXX	my $contenu_fichier = $_[3];
# XXXX	
# XXXX	my $nom_ss_ext = substr($nom_image_georef,0,length($nom_image_georef)-4);
# XXXX	
# XXXX	my $nom_fichier_georef = $repertoire_georef."/".$nom_ss_ext.".".$type;
# XXXX	
# XXXX	if(!(-e $nom_fichier_georef && -f $nom_fichier_georef)){
# XXXX		open GEOREF, ">$nom_fichier_georef" or die colored ("[PARAM_DIFFUSION_RASTER] Impossible de creer le fichier $type de l'image $nom_image_georef.", 'white on_red');
# XXXX		print GEOREF $contenu_fichier;
# XXXX		close GEOREF;
# XXXX	}
# XXXX	
# XXXX	return $nom_fichier_georef;
# XXXX	
# XXXX}
