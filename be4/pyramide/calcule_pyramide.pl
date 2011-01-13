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
# 	'$path_tms_param',
	'lecture_tile_matrix_set',
	'$dalle_no_data_mtd_param',
	'$programme_dalles_base_param',
	'$programme_copie_image_param',
	'$rep_logs_param',
	'$programme_reproj_param',
	'reproj_point',
	'$nom_fichier_first_jobs_param',
	'$nom_fichier_last_jobs_param',
	'extrait_tms_from_pyr',
	'$version_wms_param',
);
use Getopt::Std;
use XML::Simple;
use File::Basename;
use List::Util qw( max );
use POSIX qw(ceil floor);

# pas de bufferisation des sorties
$| = 1;
our ($opt_p, $opt_f, $opt_x, $opt_m, $opt_s, $opt_d, $opt_r, $opt_n, $opt_t, $opt_j, $opt_k, $opt_l);
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
my $nom_fichier_first_jobs = $nom_fichier_first_jobs_param;
my $nom_fichier_last_jobs = $nom_fichier_last_jobs_param;
# my %produit_format = %produit_format_param;
# my $path_tms = $path_tms_param;
my $rep_log = $rep_logs_param;
my $programme_reproj = $programme_reproj_param;
my $version_wms = $version_wms_param;
################################################################################

# verification de l'existence de fichiers et repertoires annexes
if(!(-e $dalle_no_data && -f $dalle_no_data)){
	print "[CALCULE_PYRAMIDE] Le fichier $dalle_no_data est introuvable.\n";
	exit;
}
if(!(-e $dalle_no_data_mtd && -f $dalle_no_data_mtd)){
	print "[CALCULE_PYRAMIDE] Le fichier $dalle_no_data_mtd est introuvable.\n";
	exit;
}
# if(!(-e $path_tms && -d $path_tms)){
# 	print "[CALCULE_PYRAMIDE] Le repertoire $path_tms est introuvable.\n";
# 	exit;
# }
#verification de la presence des programmes $programme_ss_ech $programme_format_pivot $programme_dalles_base $programme_reproj
my $verif_programme_dalle_base = `which $programme_dalles_base`;
if ($verif_programme_dalle_base eq ""){
	print "[CALCULE_PYRAMIDE] Le programme $programme_dalles_base est introuvable.\n";
	exit;
}
my $verif_programme_ss_ech = `which $programme_ss_ech`;
if ($verif_programme_ss_ech eq ""){
	print "[CALCULE_PYRAMIDE] Le programme $programme_ss_ech est introuvable.\n";
	exit;
}
my $verif_programme_pivot = `which $programme_format_pivot`;
if ($verif_programme_pivot eq ""){
	print "[CALCULE_PYRAMIDE] Le programme $programme_format_pivot est introuvable\n.";
	exit;
}
my $verif_programme_reproj = `which $programme_reproj`;
if ($verif_programme_reproj eq ""){
	print "[CALCULE_PYRAMIDE] Le programme $programme_reproj est introuvable.\n";
	exit;
}
############ MAIN
my $time = time();
my $log = $rep_log."/log_calcule_pyramide_$time.log";

open LOG, ">>$log" or die "[CALCULE_PYRAMIDE] Impossible de creer le fichier $log.";

&ecrit_log("commande : @ARGV");

#### recuperation des parametres
getopts("p:f:x:m:s:d:r:n:t:j:k:l:");

if ( ! defined ($opt_p and $opt_f and $opt_x and $opt_s and $opt_d and $opt_r and $opt_n and $opt_t and $opt_j) ){
	print "[CALCULE_PYRAMIDE] Nombre d'arguments incorrect.\n\n";
	&ecrit_log("ERREUR : Nombre d'arguments incorrect.");
	if(! defined $opt_p){
		print "[CALCULE_PYRAMIDE] Veuillez specifier un parametre -p.\n";
	}
	if(! defined $opt_f){
		print "[CALCULE_PYRAMIDE] Veuillez specifier un parametre -f.\n";
	}
	if(! defined $opt_x){
		print "[CALCULE_PYRAMIDE] Veuillez specifier un parametre -x.\n";
	}
	if(! defined $opt_s){
		print "[CALCULE_PYRAMIDE] Veuillez specifier un parametre -s.\n";
	}
	if(! defined $opt_d){
		print "[CALCULE_PYRAMIDE] Veuillez specifier un parametre -d.\n";
	}
	if(! defined $opt_r){
		print "[CALCULE_PYRAMIDE] Veuillez specifier un parametre -r.\n";
	}
	if(! defined $opt_n){
		print "[CALCULE_PYRAMIDE] Veuillez specifier un parametre -n.\n";
	}
	if(! defined $opt_t){
		print "[CALCULE_PYRAMIDE] Veuillez specifier un parametre -t.\n";
	}
	if(! defined $opt_j){
		print "[CALCULE_PYRAMIDE] Veuillez specifier un parametre -j.\n";
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
my $nombre_jobs = $opt_j;
my $localisation_serveur_rok4;
if(defined $opt_k){
	$localisation_serveur_rok4 = $opt_k;
} 
my $nom_layer_pour_requetes_wms;
if(defined $opt_l){
	$nom_layer_pour_requetes_wms = $opt_l;
}
# verifications des parametres
if ($produit !~ /^ortho|parcellaire|scan(?:25|50|100|dep|reg|1000)|franceraster$/i){
	print "[CALCULE_PYRAMIDE] Produit mal specifie.\n";
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
	print "[CALCULE_PYRAMIDE] Le fichier $fichier_dalle_source n'existe pas.\n";
	&ecrit_log("ERREUR : Le fichier $fichier_dalle_source n'existe pas.");
	exit;
}
if (defined $fichier_mtd_source && ( ! (-e $fichier_mtd_source && -f $fichier_mtd_source))){
	print "[CALCULE_PYRAMIDE] Le fichier $fichier_mtd_source n'existe pas.\n";
	&ecrit_log("ERREUR : Le fichier $fichier_mtd_source n'existe pas.");
	exit;
}
if (! (-e $fichier_pyramide && -f $fichier_pyramide)){
	print "[CALCULE_PYRAMIDE] Le fichier $fichier_pyramide n'existe pas.\n";
	&ecrit_log("ERREUR : Le fichier $fichier_pyramide n'existe pas.");
	exit;
}
if ($pourcentage_dilatation !~ /^\d{1,3}$/ || $pourcentage_dilatation > 100 ){
	print "[CALCULE_PYRAMIDE] Le pourcentage de dilatation -d $pourcentage_dilatation est incorrect.\n";
	&ecrit_log("ERREUR : Le pourcentage de dilatation -d $pourcentage_dilatation est incorrect.");
	exit;
}
if ($dilatation_reproj !~ /^\d{1,3}$/ || $dilatation_reproj > 100 ){
	print "[CALCULE_PYRAMIDE] Le pourcentage de dilatation -r $dilatation_reproj est incorrect.\n";
	&ecrit_log("ERREUR : Le pourcentage de dilatation -r $dilatation_reproj est incorrect.");
	exit;
}
if ($taille_dalle_pix !~ /^\d+$/){
	print "[CALCULE_PYRAMIDE] La taille des dalles en pixels -t $taille_dalle_pix est incorrecte.\n";
	&ecrit_log("ERREUR : La taille des dalles en pixels -t $taille_dalle_pix est incorrecte.");
	exit;
}
if ($nombre_jobs !~ /^\d+$/ && $nombre_jobs > 0){
	print "[CALCULE_PYRAMIDE] Le nombre de batchs -j $nombre_jobs est incorrect.\n";
	exit;
}
########### traitement
# creation d'un rep temporaire pour les calculs intermediaires
my $rep_temp = "../tmp/CALCULE_PYRAMIDE_".$time;
mkdir $rep_temp, 0755 or die "[CALCULE_PYRAMIDE] Impossible de creer le repertoire $rep_temp.";
my $string_script_creation_rep_temp = "if [ ! -d \"$rep_temp\" ] ; then mkdir $rep_temp ; fi\n";
my $string_script_destruction_rep_temp = "rm -rf $rep_temp\n";

# action 0 : determiner les resolutions utiles
my $ref_niv_utiles = $produit_res_utiles{$ss_produit};
my ($res_min_produit, $res_max_produit) = @{$ref_niv_utiles};

# action 1 : recuperer les infos de la pyramide
&ecrit_log("Lecture de la pyramide $fichier_pyramide.");
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
my $indice_niveau_max = @niveaux_ranges - 1;
my $level_max = $niveaux_ranges[$indice_niveau_max];
my $res_max = $niveau_res{"$level_max"};

# liste des dalles liens (cles seulement)
my @reps_toutes_dalles_liens = (values %niveau_repertoire_image, values %niveau_repertoire_mtd);
my $ref_dalles_liens = &liste_liens(\@reps_toutes_dalles_liens);
my %dalles_liens = %{$ref_dalles_liens};

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

# on regarde que la localisation du serveur et du layer sont renseigne s'il y a perte et/ou reprojection
if(!($bool_perte == 0 && $bool_reprojection == 0)){
	my $bool_presence_param = 1;
	if(! defined $localisation_serveur_rok4){
		print "[CALCULE_PYRAMIDE] Veuillez specifier un parametre -k.\n";
		$bool_presence_param = 0;
	}
	if(! defined $nom_layer_pour_requetes_wms){
		print "[CALCULE_PYRAMIDE] Veuillez specifier un parametre -l.\n";
		$bool_presence_param = 0;
	}
	if($bool_presence_param == 0){
		&usage();
		exit;
	}
}

# action 2 : lire le fichier de dalles source et des mtd
# => bbox des donnees, hashes des xmin, xmax, ymin, ymax, resx, resy
&ecrit_log("Lecture du fichier d'images source $fichier_dalle_source.");
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
	($ref_hash_x_min, $ref_hash_x_max, $ref_hash_y_min, $ref_hash_y_max, $ref_hash_res_x, $ref_hash_res_y) = &lecture_fichier_dalles_source($fichier_mtd_source, 0);
	%mtd_source_x_min = %{$ref_hash_x_min};
	%mtd_source_x_max = %{$ref_hash_x_max};
	%mtd_source_y_min = %{$ref_hash_y_min};
	%mtd_source_y_max = %{$ref_hash_y_max};
	%mtd_source_res_x = %{$ref_hash_res_x};
	%mtd_source_res_y = %{$ref_hash_res_y};
	@mtd_source = keys %mtd_source_x_min;
}

# action 3 :determiner le niveau de travail (ou il y aura un nombre de jobs > $nombre_jobs) et le niveau max
my ($ref_liste_niveau_travail, $ref_travail_x_min, $ref_travail_x_max, $ref_travail_y_min, $ref_travail_y_max, $ref_liste_niveau_max, $ref_niv_max_x_min, $ref_niv_max_x_max, $ref_niv_max_y_min, $ref_niv_max_y_max, $res_travail, $indice_travail, $niveau_travail) = &infos_niveau_travail_niveau_max($x_min_bbox, $x_max_bbox, $y_min_bbox, $y_max_bbox, $x_min_niveau_max, $y_max_niveau_max, $res_max, $taille_image_pix_x, $taille_image_pix_y, $indice_niveau_max, $level_max, $systeme_source, $systeme_target);

my @liste_dalles_arbre_niveau_travail = @{$ref_liste_niveau_travail};
my %dalle_travail_x_min = %{$ref_travail_x_min};
my %dalle_travail_x_max = %{$ref_travail_x_max};
my %dalle_travail_y_min = %{$ref_travail_y_min};
my %dalle_travail_y_max = %{$ref_travail_y_max};

my @liste_dalles_arbre_niveau_max = @{$ref_liste_niveau_max};
my %dalle_niveau_max_x_min = %{$ref_niv_max_x_min};
my %dalle_niveau_max_x_max = %{$ref_niv_max_x_max};
my %dalle_niveau_max_y_min = %{$ref_niv_max_y_min};
my %dalle_niveau_max_y_max = %{$ref_niv_max_y_max};

# action 4 : creer arbre : cree_arbre_dalles_cache pour chaque dalle du niveau de travail

# liste des scripts en first ou last
my @scripts_first;
my @scripts_last;

# liste des dalles calculees (cles seulement)
my %dalles_calculees;

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
# pour la transfo vers cache
my @liste_dalles_cache_index_arbre;
my %dalle_cache_min_liste_dalle;
my %mtd_cache_min_liste_mtd;
my %dalle_cache_dessous;
my %niveau_ref_dalles_inf;
my %niveau_ref_mtd_inf;
my %nom_dalle_index_base;

foreach my $dalle_arbre_niveau_travail(@liste_dalles_arbre_niveau_travail){
	
	my $string_script_this = "";
	my $nombre_dalles_creees_script = 0;
	
	# remise a zero des variables d'arbre
	%cache_arbre_x_min = ();
	%cache_arbre_x_max = ();
	%cache_arbre_y_min = ();
	%cache_arbre_y_max = ();
	%cache_arbre_res = ();
	%index_arbre_liste_dalle = ();
	%index_arbre_liste_mtd = ();
	%dalles_cache_plus_bas = ();
	%dalle_arbre_niveau = ();
	%dalle_arbre_indice_niveau = ();
	
	@liste_dalles_cache_index_arbre = ();
	%dalle_cache_min_liste_dalle = ();
	%mtd_cache_min_liste_mtd = ();
	%dalle_cache_dessous = ();
	%niveau_ref_dalles_inf = ();
	%niveau_ref_mtd_inf = ();
	%nom_dalle_index_base = ();
	
	my $x_min_dalle0 = $dalle_travail_x_min{"$dalle_arbre_niveau_travail"};
	my $x_max_dalle0 = $dalle_travail_x_max{"$dalle_arbre_niveau_travail"};
	my $y_min_dalle0 = $dalle_travail_y_min{"$dalle_arbre_niveau_travail"};
	my $y_max_dalle0 = $dalle_travail_y_max{"$dalle_arbre_niveau_travail"};
	my $res_dalle0 = $res_travail;
	my $indice_niveau0 = $indice_travail;
	my $level_niveau0 = $niveau_travail;
	# initialisation pour la dalle0
	$dalle_arbre_niveau{"dalle0"} = $level_niveau0;
	$dalle_arbre_indice_niveau{"dalle0"} = $indice_niveau0;
	
	#recursivite descendante et mise en memoire pour chacune des dalles cache
	&ecrit_log("Calcul de l'arbre image issu de $dalle_arbre_niveau_travail.");
	my $nombre_dalles_cache = &cree_arbre_dalles_cache(\@dalles_source, "image", $pourcentage_dilatation, "dalle0", $x_min_dalle0, $x_max_dalle0, $y_min_dalle0, $y_max_dalle0, $res_dalle0, $indice_niveau0, $res_min, $res_dalle0);
	&ecrit_log("$nombre_dalles_cache image(s) definie(s).");
	if (defined $fichier_mtd_source){
		&ecrit_log("Calcul de l'arbre mtd issu de $dalle_arbre_niveau_travail.");
		my $nombre_mtd_cache = &cree_arbre_dalles_cache(\@mtd_source, "mtd", $pourcentage_dilatation, "dalle0", $x_min_dalle0, $x_max_dalle0, $y_min_dalle0, $y_max_dalle0, $res_dalle0, $indice_niveau0, $res_min, $res_dalle0);
		&ecrit_log("$nombre_mtd_cache image(s) definie(s).");
	}
	
	# transformation des index dans l'arbre en nom des dalles cache
	@liste_dalles_cache_index_arbre = keys %index_arbre_liste_dalle;
	&ecrit_log("Passage des donnees arbre issu de $dalle_arbre_niveau_travail aux donnees cache.");
	&arbre2cache(\@liste_dalles_cache_index_arbre, $x_min_dalle0, $x_max_dalle0, $y_min_dalle0, $y_max_dalle0);	
	
	#action 6 : calculer le niveau minimum : en WMS si compression avec perte ou reprojection
	&ecrit_log("Definition des images du niveau le plus bas.");
	my ($nombre_dalles_minimum_calc, $string_script_niveau_min_img) = &calcule_niveau_minimum(\%dalle_cache_min_liste_dalle, $rep_temp, "image");
	$string_script_this .= $string_script_niveau_min_img;
	$nombre_dalles_creees_script += $nombre_dalles_minimum_calc;
	&ecrit_log("$nombre_dalles_minimum_calc image(s) du plus bas niveau a calculer.");
	if (defined $fichier_mtd_source){
		&ecrit_log("Definition des mtd du niveau le plus bas.");
		my ($nombre_mtd_minimum_calc, $string_script_niveau_min_mtd) = &calcule_niveau_minimum(\%mtd_cache_min_liste_mtd, $rep_temp, "mtd");
		$string_script_this .= $string_script_niveau_min_mtd;
		$nombre_dalles_creees_script += $nombre_mtd_minimum_calc;
		&ecrit_log("$nombre_mtd_minimum_calc mtd du plus bas niveau a calculer.");
	}
	
	# action 7 : calculer les niveaux inferieurs de 1 jusqu'au niveau de travail
	&ecrit_log("Definition des images des niveaux inferieurs.");
	my ($nombre_dalles_niveaux_inf, $string_script_niveaux_inf_img) = &calcule_niveaux_inferieurs(\%niveau_ref_dalles_inf, "MOYENNE", \%dalle_cache_min_liste_dalle, "image", 1, $indice_niveau0);
	$string_script_this .= $string_script_niveaux_inf_img;
	$nombre_dalles_creees_script += $nombre_dalles_niveaux_inf;
	&ecrit_log("$nombre_dalles_niveaux_inf image(s) a calculer.");
	if (defined $fichier_mtd_source){
		&ecrit_log("Definition des mtd des niveaux inferieurs.");
		my ($nombre_mtd_niveaux_inf, $string_script_niveaux_inf_mtd) = &calcule_niveaux_inferieurs(\%niveau_ref_mtd_inf, "PPV", \%mtd_cache_min_liste_mtd, "mtd", 1, $indice_niveau0);
		$string_script_this .= $string_script_niveaux_inf_mtd;
		$nombre_dalles_creees_script += $nombre_mtd_niveaux_inf;
		&ecrit_log("$nombre_mtd_niveaux_inf image(s) a calculer.");
	}
	
	# on ne cree le script que si des dalles sont creees
	if ($string_script_this ne ""){
		my $nom_script_complet = "$nom_script"."_"."$dalle_arbre_niveau_travail";
		open SCRIPT, ">$nom_script_complet" or die "[CALCULE_PYRAMIDE] Impossible de creer le fichier $nom_script_complet.";
		# pour la repartition de torque
		my $temps = &cree_string_temps_torque($nombre_dalles_creees_script);
		print SCRIPT "$temps\n";
		print SCRIPT $string_script_creation_rep_temp;
		print SCRIPT $string_script_this;
		close SCRIPT;
		push(@scripts_first, $nom_script_complet);
	}
	
}

# action 8 : definition du job des images plus hautes que le niveau de travail : LE 17 EME JOB !!
my $string_script4 = "";
my $nombre_dalles_job17 = 0;
foreach my $dalle_arbre_niveau_max(@liste_dalles_arbre_niveau_max){
	
	# remise a zero des variables d'arbre
	%cache_arbre_x_min = ();
	%cache_arbre_x_max = ();
	%cache_arbre_y_min = ();
	%cache_arbre_y_max = ();
	%cache_arbre_res = ();
	%index_arbre_liste_dalle = ();
	%index_arbre_liste_mtd = ();
	%dalles_cache_plus_bas = ();
	%dalle_arbre_niveau = ();
	%dalle_arbre_indice_niveau = ();
	
	@liste_dalles_cache_index_arbre = ();
	%dalle_cache_min_liste_dalle = ();
	%mtd_cache_min_liste_mtd = ();
	%dalle_cache_dessous = ();
	%niveau_ref_dalles_inf = ();
	%niveau_ref_mtd_inf = ();
	%nom_dalle_index_base = ();
	
	my $x_min_dalle0 = $dalle_niveau_max_x_min{"$dalle_arbre_niveau_max"};
	my $x_max_dalle0 = $dalle_niveau_max_x_max{"$dalle_arbre_niveau_max"};
	my $y_min_dalle0 = $dalle_niveau_max_y_min{"$dalle_arbre_niveau_max"};
	my $y_max_dalle0 = $dalle_niveau_max_y_max{"$dalle_arbre_niveau_max"};
	my $res_dalle0 = $res_max;
	my $indice_niveau0 = $indice_niveau_max;
	my $level_niveau0 = $level_max;
	# initialisation pour la dalle0
	$dalle_arbre_niveau{"dalle0"} = $level_niveau0;
	$dalle_arbre_indice_niveau{"dalle0"} = $indice_niveau0;
	
	#recursivite descendante et mise en memoire pour chacune des dalles cache
	&ecrit_log("Calcul de l'arbre image issu de $dalle_arbre_niveau_max.");
	my $nombre_dalles_cache = &cree_arbre_dalles_cache(\@dalles_source, "image", $pourcentage_dilatation, "dalle0", $x_min_dalle0, $x_max_dalle0, $y_min_dalle0, $y_max_dalle0, $res_dalle0, $indice_niveau0, $res_travail, $res_dalle0);
	&ecrit_log("$nombre_dalles_cache image(s) definie(s).");
	if (defined $fichier_mtd_source){
		&ecrit_log("Calcul de l'arbre mtd issu de $dalle_arbre_niveau_max.");
		my $nombre_mtd_cache = &cree_arbre_dalles_cache(\@mtd_source, "mtd", $pourcentage_dilatation, "dalle0", $x_min_dalle0, $x_max_dalle0, $y_min_dalle0, $y_max_dalle0, $res_dalle0, $indice_niveau0, $res_travail, $res_dalle0);
		&ecrit_log("$nombre_mtd_cache image(s) definie(s).");
	}
	
	# transformation des index dans l'arbre en nom des dalles cache
	@liste_dalles_cache_index_arbre = keys %index_arbre_liste_dalle;
	&ecrit_log("Passage des donnees arbre issu de $dalle_arbre_niveau_max aux donnees cache.");
	&arbre2cache(\@liste_dalles_cache_index_arbre, $x_min_dalle0, $x_max_dalle0, $y_min_dalle0, $y_max_dalle0);	
	
	# action 7 : calculer les niveaux inferieurs de 1 jusqu'au niveau de travail
	&ecrit_log("Definition des images des niveaux inferieurs.");
	my ($nombre_dalles_niveaux_inf, $string_script_temp) = &calcule_niveaux_inferieurs(\%niveau_ref_dalles_inf, "MOYENNE", \%dalle_cache_min_liste_dalle, "image", $indice_travail, $indice_niveau0);
	$string_script4 .= $string_script_temp;
	&ecrit_log("$nombre_dalles_niveaux_inf image(s) a calculer.");
	$nombre_dalles_job17 += $nombre_dalles_niveaux_inf;
	if (defined $fichier_mtd_source){
		&ecrit_log("Definition des mtd des niveaux inferieurs.");
		my ($nombre_mtd_niveaux_inf, $string_script_temp2) = &calcule_niveaux_inferieurs(\%niveau_ref_mtd_inf, "PPV", \%mtd_cache_min_liste_mtd, "mtd", $indice_travail, $indice_niveau0);
		$string_script4 .= $string_script_temp2;
		&ecrit_log("$nombre_mtd_niveaux_inf image(s) a calculer.");
		$nombre_dalles_job17 += $nombre_mtd_niveaux_inf;
	}
	
}


# passage en pivot du dernier niveau
&ecrit_log("Passage en format final niveau $level_max.");
my @dalles_change_format_haut;
if(defined $niveau_ref_dalles_inf{"$level_max"}){
	@dalles_change_format_haut = @{$niveau_ref_dalles_inf{"$level_max"}};
}
$string_script4 .= &passage_pivot($niveau_taille_tuile_x{"$level_max"}, $niveau_taille_tuile_y{"$level_max"}, \@dalles_change_format_haut);


if($string_script4 ne ""){
	my $nom_script_job17 = "$nom_script"."_"."job17";
	open JOB17, ">$nom_script_job17" or die "[CALCULE_PYRAMIDE] Impossible de creer le fichier $nom_script_job17.";
	# pour la repartition de torque
	my $temps_17 = &cree_string_temps_torque($nombre_dalles_job17);
	print JOB17 "$temps_17\n";
	print JOB17 $string_script_creation_rep_temp;
	print JOB17 $string_script4;
	print JOB17 $string_script_destruction_rep_temp;
	close JOB17;
	push(@scripts_last, $nom_script_job17);
}

# on cree 2 fichiers avec les premier et derniers traitements
open FIRST, ">$nom_fichier_first_jobs" or die "[CALCULE_PYRAMIDE] Impossible de creer le fichier $nom_fichier_first_jobs.";
foreach my $fisrt_job(@scripts_first){
	print FIRST "$fisrt_job ";
}
print FIRST "\n";
close FIRST;
open LAST, ">$nom_fichier_last_jobs" or die "[CALCULE_PYRAMIDE] Impossible de creer le fichier $nom_fichier_last_jobs.";
foreach my $last_job(@scripts_last){
	print LAST "$last_job ";
}
print LAST "\n";
close LAST;

&ecrit_log("Traitement termine.");
close LOG;

# lecture du log pour voir si erreurs
open CHECK, "<$log" or die "[CALCULE_PYRAMIDE] Impossible d'ouvrir le fichier $log.";
my @lignes_log = <CHECK>;
close CHECK;
my $nb_erreur = 0;
foreach my $ligne(@lignes_log){
	if ($ligne =~ /err(?:eur|or)|unable|not found/i){
		$nb_erreur += 1;
	}
}
if($nb_erreur != 0){
	print "[CALCULE_PYRAMIDE] $nb_erreur erreurs se sont produites. Consulter le fichier $log.\n";
}

# on l'ecrit dans le log
open LOG, ">>$log" or die "[CALCULE_PYRAMIDE] Impossible d'ouvrir le fichier $log en ajout.";
&ecrit_log("FIN : $nb_erreur erreurs se sont produites.");
close LOG;
################### FIN

################################################################################

########## FONCTIONS
sub usage{
	my $bool_ok = 0;
	
	print "\nUsage : \ncalcule_pyramide.pl -p produit -f path/fichier_dalles_source [-m path/fichier_mtd_source] -s systeme_coordonnees_source -x path/fichier_pyramide.pyr -d %_dilatation_dalles_base -r %_dilatation_reproj -n path/prefixe_nom_script -t taille_dalles_pixels -j nombre_jobs_min [-k localisation_serveur_rok4 -l nom_layer_a_utiliser]\n";
	print "\nproduit :\n";
 	print "\tortho\n\tparcellaire\n\tscan[25|50|100|dep|reg|1000]\n\tfranceraster\n";
 	print "\nsysteme_coordonnees_source :\n";
	print "\tcode RIG des images source : LAMB93 LAMBE ...\n";
	print "\npourcentage_dilatation : 0 a 100\n";
	print "%_dilatation_dalles_base : dilatation des dalles du cache pour trouver les dalles source en recouvrement\n";
	print "%_dilatation_reproj : dilatation des dalles du cache reprojetees pour parer a la deformation de la reprojection\n";
	print "\nlocalisation du serveur rok4 : ex obernai.ign.fr/rok4/bin/rok4\n";
	print "\nlayer a utiliser : pour les requetes WMS : ex ORTHO_RAW\n";
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
# 				print "[CALCULE_PYRAMIDE] Probleme de programmation : type $type_dal incorrect.";
# 				exit;
# 			}
# 			ecrit_log("Creation des eventuels repertoires manquants.");
# 			&cree_repertoires_recursifs(dirname($dalle_cache));
# 			my $return = copy($dalle_a_copier, $dalle_cache);
# 			if ($return == 0){
# 				&ecrit_log("ERREUR a la copie de $dalle_a_copier vers $dalle_cache");
# 			}else{
# 				&ecrit_log("Copie de $dalle_a_copier vers $dalle_cache");
# 				$nombre_dalles_ajoutees += 1;
# 				
# 			}
# 			
# 		}
# 	}
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
	if($indice_arbre ne "dalle0"){
		my @chiffres_indice = split //, $indice_arbre;
		foreach my $chiffre (@chiffres_indice){
			my $taille_x_diff = ($x_max_cache - $x_min_cache) / 2;
			my $taille_y_diff = ($y_max_cache - $y_min_cache) / 2;
			# 4 cas possibles, on ajoute ou retranche selon les cas
			if($chiffre eq "0"){
				$x_max_cache -= $taille_x_diff;
				$y_min_cache += $taille_y_diff;
			}elsif($chiffre eq "1"){
				$x_min_cache += $taille_x_diff;
				$y_min_cache += $taille_y_diff;
			}elsif($chiffre eq "3"){
				$x_min_cache += $taille_x_diff;
				$y_max_cache -= $taille_y_diff;
			}elsif($chiffre eq "2"){
				$x_max_cache -= $taille_x_diff;
				$y_max_cache -= $taille_y_diff;
			}else{
				print "[CALCULE_PYRAMIDE] Probleme pour obtenir les coordonnees a partir de l'indice de l'arbre : indice $indice_arbre non correct.\n";
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
	
	open SOURCE, "<$fichier_a_lire" or die "[CALCULE_PYRAMIDE] Impossible d'ouvrir le fichier $fichier_a_lire.";
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
sub definit_bloc_dalle{
	
	my $id_dalle = $_[0];
	my $x_min_dalle_cache = $_[1];
	my $x_max_dalle_cache = $_[2];
	my $y_min_dalle_cache = $_[3];
	my $y_max_dalle_cache = $_[4];
	my $res_dalle = $_[5];
	my $indice_niveau = $_[6];
	my $ref_dalles = $_[7];
	my $type = $_[8];
	my $pcent_dilat = $_[9];
	my $res_min_calcul = $_[10];
	my $res_max_calcul = $_[11];
	
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
	if( ($res_dalle <= $res_max_calcul) && ($res_dalle >= $res_min_calcul) ){
		if ($type eq "mtd"){
			$index_arbre_liste_mtd{$id_dalle} = \@dalles_recouvrantes;
		}elsif($type eq "image"){
			$index_arbre_liste_dalle{$id_dalle} = \@dalles_recouvrantes;
		}else{
			print "[CALCULE_PYRAMIDE] Probleme de programmation : type $type incorrect.\n";
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
		if($id_dalle eq "dalle0"){
			$id_dalle = "";
		}
		my $indice_dessous = $indice_niveau - 1;
		my $niveau_inferieur = $niveaux_ranges[$indice_dessous];
		my $res_dessous = $niveau_res{"$niveau_inferieur"};
		
		my $x_inter = $x_min_dalle_cache + $res_dessous * $taille_image_pix_x;
		my $y_inter = $y_max_dalle_cache - $res_dessous * $taille_image_pix_y;
		
		$nombre_dalles_traitees += definit_bloc_dalle($id_dalle."0", $x_min_dalle_cache, $x_inter, $y_inter, $y_max_dalle_cache, $res_dessous, $indice_dessous, \@dalles_recouvrantes, $type, $pcent_dilat, $res_min_calcul, $res_max_calcul);
	    $nombre_dalles_traitees += definit_bloc_dalle($id_dalle."1", $x_inter, $x_max_dalle_cache, $y_inter, $y_max_dalle_cache, $res_dessous, $indice_dessous, \@dalles_recouvrantes, $type, $pcent_dilat, $res_min_calcul, $res_max_calcul);
	    $nombre_dalles_traitees += definit_bloc_dalle($id_dalle."3", $x_inter, $x_max_dalle_cache, $y_min_dalle_cache, $y_inter, $res_dessous, $indice_dessous, \@dalles_recouvrantes, $type, $pcent_dilat, $res_min_calcul, $res_max_calcul);
	    $nombre_dalles_traitees += definit_bloc_dalle($id_dalle."2", $x_min_dalle_cache, $x_inter, $y_min_dalle_cache, $y_inter, $res_dessous, $indice_dessous, \@dalles_recouvrantes, $type, $pcent_dilat, $res_min_calcul, $res_max_calcul);
		
		# on remplit les niveaux des dalles d'en dessous
		$dalle_arbre_niveau{$id_dalle."0"} = $niveau_inferieur;
		$dalle_arbre_niveau{$id_dalle."1"} = $niveau_inferieur;
		$dalle_arbre_niveau{$id_dalle."3"} = $niveau_inferieur;
		$dalle_arbre_niveau{$id_dalle."2"} = $niveau_inferieur;
		
		$dalle_arbre_indice_niveau{$id_dalle."0"} = $indice_dessous;
		$dalle_arbre_indice_niveau{$id_dalle."1"} = $indice_dessous;
		$dalle_arbre_indice_niveau{$id_dalle."3"} = $indice_dessous;
		$dalle_arbre_indice_niveau{$id_dalle."2"} = $indice_dessous;
		
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
	my $string_script = "";
	
	
	while ( my($dalle_cache, $ref_dalles_source) = each %hash_dalle_cache_dalles_source ){
		if(defined $ref_dalles_source ){
			&ecrit_log("Calcul de $dalle_cache.");
			my @liste_dalles_source = @{$ref_dalles_source};
			
			# creation le cas echeant des repertoires parents
			my $rep_parent_dalle = dirname($dalle_cache);
			&ecrit_log("Creation des eventuels repertoires manquants de $rep_parent_dalle.");
			&cree_repertoires_recursifs($rep_parent_dalle);
			
			my $nom_dalle = $nom_dalle_index_base{$dalle_cache};
			
			# suppression du lien symbolique preexistant sinon la creation va abimer les anciennes pyramides
			$string_script .= "if [ -L \"$dalle_cache\" ] ; then rm -f $dalle_cache ; fi\n";
			
			if($bool_reprojection == 0 && $bool_perte == 0){
				my $nom_dalle_temp = "$rep_temp/$nom_dalle.tif";
				# mise en format travail dalle_cache pour image initiale
				# destruction de la dalle_cache temporaire si elle existe
				$string_script .= "if [ -r \"$nom_dalle_temp\" ] ; then rm -f $nom_dalle_temp ; fi\n";
				
				# test si on a une dalle (normalement un lien vers une pyramide precedente qui est en format pyramide)
				#if (-e $dalle_cache ){
				# avec le script il faut faire autrement
				if (exists $dalles_liens{$dalle_cache} ){
					$string_script .= "$programme_copie_image -s -r $taille_dalle_pix $dalle_cache $nom_dalle_temp\n";
				}else{
					# sinon on fait reference a la dalle no_data
					if($type eq "image"){
						$nom_dalle_temp = $dalle_no_data;
					}elsif($type eq "mtd"){
						$nom_dalle_temp = $dalle_no_data_mtd;
					}else{
						print "[CALCULE_PYRAMIDE] Probleme de programmation : type $type incorrect.\n";
						exit;
					}
				}
				
				my $nom_fichier = $rep_enregistr."/".$nom_dalle."_".$type;
				
				my $res_x_max_source = 0;
				my $res_y_max_source = 0;
				
				open FIC, ">$nom_fichier" or die "[CALCULE_PYRAMIDE] Impossible de creer le fichier $nom_fichier.";
				# dalle cache
				print FIC "$dalle_cache\t$cache_arbre_x_min{$dalle_cache}\t$cache_arbre_y_max{$dalle_cache}\t$cache_arbre_x_max{$dalle_cache}\t$cache_arbre_y_min{$dalle_cache}\t$cache_arbre_res{$dalle_cache}\t$cache_arbre_res{$dalle_cache}\n";
				# dalles source a la suite
				
				# d'abord la dalle cache temporaire
				# (puisque $programme_dalles_base ne supporte pas de ne pas avoir de donnees)
				
				print FIC "$nom_dalle_temp\t$cache_arbre_x_min{$dalle_cache}\t$cache_arbre_y_max{$dalle_cache}\t$cache_arbre_x_max{$dalle_cache}\t$cache_arbre_y_min{$dalle_cache}\t$cache_arbre_res{$dalle_cache}\t$cache_arbre_res{$dalle_cache}\n";
				
				foreach my $src(@liste_dalles_source){
					if($source_res_x{$src} > $res_x_max_source){
						$res_x_max_source = $source_res_x{$src};
					}
					if($source_res_y{$src} > $res_y_max_source){
						$res_y_max_source = $source_res_y{$src};
					}
					# TODO passer par une image tiff2gray pour la bdparcel
					print FIC "$src\t$source_x_min{$src}\t$source_y_max{$src}\t$source_x_max{$src}\t$source_y_min{$src}\t$source_res_x{$src}\t$source_res_y{$src}\n";
				}
				
				# ATTENTION, intuition feminine : l'execution dans certains environnements fait qu'il se melange les pinceaux
				# et cree des fichiers vides, en le faisant dormir, ca a l'air de reparer
				# sleep(1);
				# moyen de le faire dormir moins que 1s : sleep ne fait que les entiers !!
				# ~15s pour 1500 images
				select(undef, undef, undef, 0.01);
				
				close FIC;
				# definition de l'interpolateur
				my $interpolateur = "bicubique";
				
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
	
				}else{
					print "[CALCULE_PYRAMIDE] Probleme de programmation : type $type incorrect.\n";
					exit;
				}
				
				# pour le programme
				my $type_dalles_base = $type;
				if($type_dalles_base eq "image"){
					$type_dalles_base = "img";
				}
				# TODO nombre de canaux, nombre de bits, couleur en parametre
				# TODO supprimer no_data qui ne sert a rien
				$string_script .= "$programme_dalles_base -f $nom_fichier -i $interpolateur -n $no_data -t $type_dalles_base -s 3 -b 8 -p rgb\n";
			}else{
				# on recupere directement dans un cache existant une image tiff en format de travail
				$string_script .= "wget --no-proxy -O $dalle_cache \"http://".$localisation_serveur_rok4."?LAYERS=".$nom_layer_pour_requetes_wms."&SERVICE=WMS&VERSION=".$version_wms."&REQUEST=GetMap&FORMAT=image/tiff&CRS=".$systeme_target."&BBOX=".$cache_arbre_x_min{$dalle_cache}.",".$cache_arbre_y_min{$dalle_cache}.",".$cache_arbre_x_max{$dalle_cache}.",".$cache_arbre_y_max{$dalle_cache}."&WIDTH=".$taille_image_pix_x."&HEIGHT=".$taille_image_pix_y."\"";
			}
			
			$dalles_calculees{$dalle_cache} = "toto";
			$nb_dal += 1;
			
		}
	}
	return ($nb_dal, $string_script);
	
}
################################################################################
sub calcule_niveaux_inferieurs{
	
	my $ref_dalles_a_calc = $_[0];
	my $interpol = $_[1];
	my $ref_niveau_bas = $_[2];
	my $type_dalle = $_[3];
	my $indice_niveau_min_calcul = $_[4];
	my $indice_niveau_max_calcul = $_[5];
	
	my %hash_niveau_liste = %{$ref_dalles_a_calc};
	
	my $nb_calc = 0;
	my $string_script2 = "";
	
	for (my $i = $indice_niveau_min_calcul ; $i < $indice_niveau_max_calcul + 1 ; $i++){
		my $niveau_inf = $niveaux_ranges[$i];
		if(defined $hash_niveau_liste{"$niveau_inf"}){
			&ecrit_log("Calcul niveau $niveau_inf.");
			my @dalles_a_calc = @{$hash_niveau_liste{"$niveau_inf"}};
			foreach my $dal(@dalles_a_calc){
				if (defined $dalle_cache_dessous{$dal}){
					&ecrit_log("Calcul de $dal.");
					
					# si la dalle existe en lien symbolique, on doit casser le lien pour ne pas deteriorer
					if (exists $dalles_liens{$dal}){
						$string_script2 .= "if [ -L \"$dal\" ] ; then rm -f $dal ; fi\n";
					}
					
					# creation le cas echeant des repertoires parents
					my $rep_parent_dalle_niveau_inf = dirname($dal);
					&ecrit_log("Creation des eventuels repertoires manquants de $rep_parent_dalle_niveau_inf.");
					&cree_repertoires_recursifs($rep_parent_dalle_niveau_inf);
					
					if($bool_perte == 0 && $bool_reprojection == 0){
						#calcul
						
						my @list_cache_dessous = @{$dalle_cache_dessous{$dal}};
						
						my $string_dessous = "";
						foreach my $dalle_dessous(@list_cache_dessous){
							my $fichier_pointe;
							# si la dalle n'existe pas on met la dalle no_data
							#if(!(-e $dalle_dessous)){
							# avec le script il faut faire autrement
							if(!(exists $dalles_calculees{$dalle_dessous} || exists $dalles_liens{$dalle_dessous})){
								if($type_dalle eq "image"){
									$fichier_pointe = $dalle_no_data;
								}elsif($type_dalle eq "mtd"){
									$fichier_pointe = $dalle_no_data_mtd;
								}else{
									print "[CALCULE_PYRAMIDE] Probleme de programmation : type $type_dalle incorrect.\n";
									exit;
								}
								
							}else{
# 								# lecture du lien de la dalle cache pour test 
# 								$fichier_pointe = readlink("$dalle_dessous");
# 								if ( ! defined $fichier_pointe){
# 									$fichier_pointe = "$dalle_dessous";
#								}
								
								my $nom_dalle_cache = basename($dalle_dessous);
								
								# mise en format travail de la dalle dalle_cache
								# desctruction de la dalle_cache temporaire si elle existe
								$string_script2 .= "if [ -r \"$rep_temp/$nom_dalle_cache\" ] ; then rm -f $rep_temp/$nom_dalle_cache ; fi\n";
								$string_script2 .= "$programme_copie_image -s -r $taille_dalle_pix $dalle_dessous $rep_temp/$nom_dalle_cache\n";
								$fichier_pointe = "$rep_temp/$nom_dalle_cache";
							}
							$string_dessous .= " $fichier_pointe";
						}
						# TODO ajouter l'interpolation dans la ligne de commande -i $interpol!!
						$string_script2 .= "$programme_ss_ech $string_dessous $dal\n";
						
					}else{
						# on recupere a partir d'un cache existant une dalle en format de travail						
						$string_script2 .= "wget --no-proxy -O $dal \"http://".$localisation_serveur_rok4."?LAYERS=".$nom_layer_pour_requetes_wms."&SERVICE=WMS&VERSION=".$version_wms."&REQUEST=GetMap&FORMAT=image/tiff&CRS=".$systeme_target."&BBOX=".$cache_arbre_x_min{$dal}.",".$cache_arbre_y_min{$dal}.",".$cache_arbre_x_max{$dal}.",".$cache_arbre_y_max{$dal}."&WIDTH=".$taille_image_pix_x."&HEIGHT=".$taille_image_pix_y."\"";
					}
					
					$dalles_calculees{$dal} = "toto";
					$nb_calc += 1;
					 
				}
				
			}
			
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
			$string_script2 .= &passage_pivot($niveau_taille_tuile_x{"$niveau_termine"}, $niveau_taille_tuile_y{"$niveau_termine"}, \@dalles_change_format);

		}
	}
	
	return ($nb_calc, $string_script2);

}
################################################################################

sub arbre2cache{
	
	my @liste_toutes_dalles = @{$_[0]};
	my $x_min_dalle_origine = $_[1];
	my $x_max_dalle_origine = $_[2];
	my $y_min_dalle_origine = $_[3];
	my $y_max_dalle_origine = $_[4];
	
	my $bool_ok = 0;
	
	foreach my $dalle_arbre(@liste_toutes_dalles){
		#transformer en x_y
		my ($x_min_dalle_arbre, $x_max_dalle_arbre, $y_min_dalle_arbre, $y_max_dalle_arbre) = &indice_arbre2xy("$dalle_arbre", $x_min_dalle_origine, $x_max_dalle_origine, $y_min_dalle_origine, $y_max_dalle_origine);
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
		
		# remplissage des infos de la dalle cache
		$cache_arbre_x_min{$new_nom_dalle} = $x_min_dalle_arbre;
		$cache_arbre_x_max{$new_nom_dalle} = $x_max_dalle_arbre;
		$cache_arbre_y_min{$new_nom_dalle} = $y_min_dalle_arbre;
		$cache_arbre_y_max{$new_nom_dalle} = $y_max_dalle_arbre;
		$cache_arbre_res{$new_nom_dalle} = $niveau_res{"$niveau_dalle"};
		
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
	my $id_dalle_haut = $_[3];
	my $x_min_dalle_haut = $_[4];
	my $x_max_dalle_haut = $_[5];
	my $y_min_dalle_haut = $_[6];
	my $y_max_dalle_haut = $_[7];
	my $res_dalle_haut  = $_[8];
	my $indice_niveau_dalle_haut = $_[9];
	my $res_minimum_calcul = $_[10];
	my $res_maximum_calcul = $_[11];
	
	my $nombre_dalles = 0;
	$nombre_dalles += definit_bloc_dalle($id_dalle_haut, $x_min_dalle_haut, $x_max_dalle_haut, $y_min_dalle_haut, $y_max_dalle_haut, $res_dalle_haut, $indice_niveau_dalle_haut, $ref_liste, $type, $pourcentage, $res_minimum_calcul, $res_maximum_calcul);
	
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
	
	my $tms_complet = &extrait_tms_from_pyr($xml_pyramide);
# 	my $nom_tms = $data->{tileMatrixSet};
# 	my $tms_complet = $path_tms."/".$nom_tms.".tms";
	
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
		print "[CALCULE_PYRAMIDE] Pas de formats d'images trouves dans le fichier $xml_pyramide.\n";
		exit;
	}else{
		if($nb_formats_temp > 1){
			&ecrit_log("WARNING Plusieurs formats d'images trouves dans le fichier $xml_pyramide => retenu : $formats_temp[0].");
			print "[CALCULE_PYRAMIDE] WARNING Plusieurs formats d'images trouves dans le fichier $xml_pyramide => retenu : $formats_temp[0].\n";
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
	
	my $string_script3 = "";
	
	foreach my $dal2(@dalles_travail){
		&ecrit_log("Passage en format pivot de $dal2.");
		$string_script3 .= "if [ -r \"$rep_temp/temp.tif\" ] ; then rm -f $rep_temp/temp.tif ; fi\n";
		
		# TODO introduire la couleur dans $programme_format_pivot
		$string_script3 .= "$programme_format_pivot $dal2 -c $compress -t $taille_pix_x_tuile $taille_pix_y_tuile $rep_temp/temp.tif\n";
		$string_script3 .= "rm -f $dal2\n";
		$string_script3 .= "mv $rep_temp/temp.tif $dal2\n";

	}
	
	return $string_script3;
	
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
	# 01 12
	# 23 43
	my ($x0,$y0) = &reproj_point($x_min_poly, $y_max_poly, $srs_ini_poly, $srs_fin_poly);
	my ($x1,$y1) = &reproj_point($x_max_poly, $y_max_poly, $srs_ini_poly, $srs_fin_poly);
	my ($x3,$y3) = &reproj_point($x_max_poly, $y_min_poly, $srs_ini_poly, $srs_fin_poly);
	my ($x2,$y2) = &reproj_point($x_min_poly, $y_min_poly, $srs_ini_poly, $srs_fin_poly);
	
	# on ne teste que les x car reproj est implemente comme ca, si erreur x et y en erreur
	if(!($x0 ne "erreur" && $x1 ne "erreur" && $x2 ne "erreur" && $x3 ne "erreur")){
		&ecrit_log("Erreur a la reprojection de $srs_ini_poly en $srs_fin_poly.");
		exit;
	}
	# teste si les coordonnees ne sont pas hors champ
	if(!($x0 ne "*" && $x1 ne "*" && $x2 ne "*" && $x3 ne "*" && $y0 ne "*" && $y1 ne "*" && $y2 ne "*" && $y3 ne "*" )){
		&ecrit_log("Erreur a la reprojection de $srs_ini_poly en $srs_fin_poly, coordonnees potentiellement hors champ.");
		exit;
	}
	
	# determination de la bbox resultat
	my $x_min_result = 99999999999;
	if($x0 < $x_min_result){
		$x_min_result = $x0;
	}
	if($x1 < $x_min_result){
		$x_min_result = $x1;
	}
	if($x3 < $x_min_result){
		$x_min_result = $x3;
	}
	if($x2 < $x_min_result){
		$x_min_result = $x2;
	}
	my $x_max_result = -99999999999;
	if($x0 > $x_max_result){
		$x_max_result = $x0;
	}
	if($x1 > $x_max_result){
		$x_max_result = $x1;
	}
	if($x3 > $x_max_result){
		$x_max_result = $x3;
	}
	if($x2 > $x_max_result){
		$x_max_result = $x2;
	}
	my $y_min_result = 99999999999;
	if($y0 < $y_min_result){
		$y_min_result = $y0;
	}
	if($y1 < $y_min_result){
		$y_min_result = $y1;
	}
	if($y3 < $y_min_result){
		$y_min_result = $y3;
	}
	if($y2 < $y_min_result){
		$y_min_result = $y2;
	}
	my $y_max_result = -99999999999;
	if($y0 > $y_max_result){
		$y_max_result = $y0;
	}
	if($y1 > $y_max_result){
		$y_max_result = $y1;
	}
	if($y3 > $y_max_result){
		$y_max_result = $y3;
	}
	if($y2 > $y_max_result){
		$y_max_result = $y2;
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
sub dalles_impactees{
	
	my $origine_x_dallage = $_[0];
	my $origine_y_dallage = $_[1];
	my $pas_x_dallage = $_[2];
	my $pas_y_dallage = $_[3];
	my $x_min_dalles_source = $_[4];
	my $x_max_dalles_source = $_[5];
	my $y_min_dalles_source = $_[6];
	my $y_max_dalles_source = $_[7];
	my $bool_nombre = $_[8];
	
	# indice des dalles dans le dallage (commencent a 0)
	# (i,j) <= ($x_min_dalles_source, $y_max_dalles_source)
	# (m,n) <= ($x_max_dalles_source, $y_min_dalles_source)
	my $i = &calcule_indice_dallage($origine_x_dallage, $pas_x_dallage, $x_min_dalles_source);
	my $j = &calcule_indice_dallage($origine_y_dallage, $pas_y_dallage, $y_max_dalles_source);
	my $m = &calcule_indice_dallage($origine_x_dallage, $pas_x_dallage, $x_max_dalles_source);
	my $n = &calcule_indice_dallage($origine_y_dallage, $pas_y_dallage, $y_min_dalles_source);
	
	# si on ne demande que le nombre
	if ($bool_nombre == 1){
		my $nb_impact = ($m - $i + 1) * ($n - $j + 1);
		return $nb_impact;
	}
	
	# calcul du nombre de chiffres de l'indice
	# les nombre a comparer sont la plus petite puissance de 2 > m (resp n)
	my $nombre_compare1 = 0;
	if($m != 0){
		$nombre_compare1 = ceil(log($m)/log(2));
	}
	my $nombre_compare2 = 0;
	if($n != 0){
		$nombre_compare2 = ceil(log($n)/log(2));
	}
	my $w = max($nombre_compare1,$nombre_compare2);
	
	my @liste_dalles_impactees;
	my (%hash_dalles_x_min, %hash_dalles_x_max, %hash_dalles_y_min, %hash_dalles_y_max);
	
	# remplissage des informations
	# x et y sont les numeros d'indice dans le dallage courant
	for (my $x = $i ; $x < $m + 1; $x++){
		for (my $y = $j ; $y < $n + 1 ; $y++){
			# conversion de x et y en binaire B??
			my $indice_x_binaire = sprintf("%b", $x);;
			my $indice_y_binaire = sprintf("%b", $y);;
			# avec la convention
			# 0 1
			# 2 3
			my $indice_dalle_arbre = &formate_zero(($indice_x_binaire + (2 * $indice_y_binaire)),$w);
			push(@liste_dalles_impactees, "$indice_dalle_arbre");
			$hash_dalles_x_min{"$indice_dalle_arbre"} = $origine_x_dallage + ($x * $pas_x_dallage);
			$hash_dalles_x_max{"$indice_dalle_arbre"} = $origine_x_dallage + (($x + 1) * $pas_x_dallage);
			$hash_dalles_y_min{"$indice_dalle_arbre"} = $origine_y_dallage - (($y + 1) * $pas_y_dallage);
			$hash_dalles_y_max{"$indice_dalle_arbre"} = $origine_y_dallage - ( $y * $pas_y_dallage);
		}
	}
	
	return (\@liste_dalles_impactees, \%hash_dalles_x_min, \%hash_dalles_x_max, \%hash_dalles_y_min, \%hash_dalles_y_max);
}
################################################################################
sub formate_zero{
  
	my $variable = $_[0];
	my $nb_chiffres = $_[1];
	
	my $diff = $nb_chiffres - length($variable);
	
	for(my $compteur = 0 ; $compteur < $diff ; $compteur++){
		$variable = "0".$variable;
	}
	
	return $variable;
  
}
################################################################################
sub calcule_indice_dallage{
	
	my $origine_coord = $_[0];
	my $pas = $_[1];
	my $coord = $_[2];
	
	my $indice;
	
	#indice de la dalle en ligne ou en colonne dans le dallage
	# abs(coord - origine) = q * pas + rest on prend la valeur absolue car les y vont en descendant
	my $rest = (abs($coord - $origine_coord)) % $pas;
	
	# si le reste est nul on decale d'un cran a gauche pour etre sur d'avoir tout les recouvrements
	if($rest == 0){
		# indice = abs(q - 1)
		$indice = ( (abs($coord - $origine_coord)) / $pas ) - 1 ;
	}else{
		# indice = q
		$indice = (abs($coord - $origine_coord) - $rest) / $pas;
	}
	
	return $indice;
}
################################################################################
sub infos_niveau_travail_niveau_max{
	
	my $x_min_source = $_[0];
	my $x_max_source = $_[1];
	my $y_min_source = $_[2];
	my $y_max_source = $_[3];
	my $x_min_niveau_maxi = $_[4];
	my $y_max_niveau_maxi = $_[5];
	my $res_maxi = $_[6];
	my $taille_image_x_maxi = $_[7];
	my $taille_image_y_maxi = $_[8];
	my $indice_level_maxi = $_[9];
	my $level_maxi = $_[10];
	
	# ????
	my $systeme_src = $_[11];
	my $systeme_dst = $_[12];
	
	my $res_temp = $res_maxi;
	my $pas_x = $res_temp * $taille_image_x_maxi;
	my $pas_y = $res_temp * $taille_image_y_maxi;
	my $indice_level_temp = $indice_level_maxi;
	my $niveau_temp = $level_maxi;
	
	# infos du niveau maxi
	my ($ref_liste_dalles_niveau_max, $ref_hash_x_min_niveau_max, $ref_hash_x_max_niveau_max, $ref_hash_y_min_niveau_max, $ref_hash_y_max_niveau_max) = &dalles_impactees($x_min_niveau_maxi, $y_max_niveau_maxi, $pas_x, $pas_y, $x_min_source, $x_max_source, $y_min_source, $y_max_source, 0);
	my @liste_dalles_niveau_max = @{$ref_liste_dalles_niveau_max};
	my %dal_niveau_max_x_min = %{$ref_hash_x_min_niveau_max};
	my %dal_niveau_max_x_max = %{$ref_hash_x_max_niveau_max};
	my %dal_niveau_max_y_min = %{$ref_hash_y_min_niveau_max};
	my %dal_niveau_max_y_max = %{$ref_hash_y_max_niveau_max};
	
	my $nombre_dalles_impactees = &dalles_impactees($x_min_niveau_maxi, $y_max_niveau_maxi, $pas_x, $pas_y, $x_min_source, $x_max_source, $y_min_source, $y_max_source, 1);

	while($nombre_dalles_impactees < $nombre_jobs){
	
		$indice_level_temp -= 1;
		$niveau_temp = $niveaux_ranges[$indice_level_temp];
		$res_temp /= 2;
		$pas_x /= 2;
		$pas_y /= 2;
		$nombre_dalles_impactees = &dalles_impactees($x_min_niveau_maxi, $y_max_niveau_maxi, $pas_x, $pas_y, $x_min_source, $x_max_source, $y_min_source, $y_max_source, 1);
	}
	# infos du niveau d'elagage
	my ($ref_liste_dalles_impactees, $ref_hash_x_min_travail, $ref_hash_x_max_travail, $ref_hash_y_min_travail, $ref_hash_y_max_travail) = &dalles_impactees($x_min_niveau_maxi, $y_max_niveau_maxi, $pas_x, $pas_y, $x_min_source, $x_max_source, $y_min_source, $y_max_source, 0);
	my @liste_dalles_niveau_travail = @{$ref_liste_dalles_impactees};
	my %dal_travail_x_min = %{$ref_hash_x_min_travail};
	my %dal_travail_x_max = %{$ref_hash_x_max_travail};
	my %dal_travail_y_min = %{$ref_hash_y_min_travail};
	my %dal_travail_y_max = %{$ref_hash_y_max_travail};
	
	return (\@liste_dalles_niveau_travail, \%dal_travail_x_min, \%dal_travail_x_max, \%dal_travail_y_min, \%dal_travail_y_max, \@liste_dalles_niveau_max, \%dal_niveau_max_x_min, \%dal_niveau_max_x_max, \%dal_niveau_max_y_min, \%dal_niveau_max_y_max, $res_temp, $indice_level_temp, $niveau_temp);
	
}
################################################################################
sub liste_liens{
	my @liste_rep = @{$_[0]};
	
	my %hash_liens;
	
	foreach my $rep(@liste_rep){
		opendir REP, $rep or die "[CALCULE_PYRAMIDE] Impossible d'ouvrir le repertoire $rep.";
		my @fichiers = readdir REP;
		closedir REP;
		foreach my $fichier(@fichiers){
			next if ($fichier =~ /^\.\.?$/);
			if(-l "$rep/$fichier"){
				$hash_liens{"$rep/$fichier"} = "toto";
			}elsif(-d "$rep/$fichier"){
				my @tab_temp = ("$rep/$fichier");
				my %hash_temp = %{&liste_liens(\@tab_temp)};
				# ajout du hash au hash principal
				@hash_liens{keys %hash_temp} = values %hash_temp;
			}
		}
	}
	
	return \%hash_liens;
}
################################################################################
sub cree_string_temps_torque{

	my $nombre_cliches = $_[0];
	
	# on considere 1dalle par minute	
	
	my $nb_heures = floor($nombre_cliches / 60);
	my $minutes = $nombre_cliches - (60 * $nb_heures);
	$nb_heures = &formate_zero($nb_heures, 2);
	$minutes = &formate_zero($minutes, 2);
	my $temps_torque = "#PBS -l cput=".$nb_heures.":".$minutes.":00";
	
	return $temps_torque;
}
