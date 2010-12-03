#!/usr/bin/perl -w 

use strict;
use Getopt::Std;
use Cwd 'abs_path';
use File::Basename;
use POSIX qw(ceil floor);
use cache(
# 	'%produit_format_param',
	'$type_mtd_pyr_param',
	'$format_mtd_pyr_param',
	'$profondeur_pyr_param',
	'$nom_fichier_dalle_source_param',
	'$nom_rep_images_param',
	'$nom_rep_mtd_param',
	'$nom_fichier_mtd_source_param',
	'%produit_nb_canaux_param',
	'%produit_tms_param',
	'$xsd_pyramide_param',
	'lecture_tile_matrix_set',
	'$rep_logs_param',
	'%format_format_pyr_param',
	'%produit_nomenclature_param',
);
# CONSTANTES
$| = 1;
our($opt_p, $opt_i, $opt_r, $opt_c, $opt_s, $opt_t, $opt_n, $opt_d, $opt_m, $opt_x, $opt_f, $opt_a, $opt_y, $opt_w, $opt_h);

# my %produit_format = %produit_format_param;
my $nom_fichier_dalle_source = $nom_fichier_dalle_source_param;
my $nom_fichier_mtd_source = $nom_fichier_mtd_source_param;
my $nom_rep_images = $nom_rep_images_param;
my $nom_rep_mtd = $nom_rep_mtd_param;

my $type_mtd_pyr = $type_mtd_pyr_param;
my $format_mtd_pyr = $format_mtd_pyr_param;
my $profondeur_pyr = $profondeur_pyr_param;
my %produit_nb_canaux = %produit_nb_canaux_param;
my %produit_tms = %produit_tms_param;
my $xsd_pyramide = $xsd_pyramide_param;
my $rep_log = $rep_logs_param;
my %format_format_pyr = %format_format_pyr_param;
my %produit_nomenclature = %produit_nomenclature_param;
################################################################################

# verification de l'existence des fichiers annexes
if (!(-e $xsd_pyramide && -f $xsd_pyramide) ){
	print "[PREPARE_PYRAMIDE] Le fichier $xsd_pyramide est introuvable.\n";
	exit;
}

##### MAIN
my $time = time();
my $log = $rep_log."/log_prepare_pyramide_$time.log";

open LOG, ">>$log" or die "[PREPARE_PYRAMIDE] Impossible de creer le fichier $log.";
&ecrit_log("commande : @ARGV");

getopts("p:i:r:c:s:t:n:d:m:x:fa:y:w:h:");

my $bool_getopt_ok = 1;
if ( ! defined ($opt_p and $opt_i and $opt_r and $opt_c and $opt_s and $opt_t and $opt_n and $opt_x) ){
	$bool_getopt_ok = 0;
	print "[PREPARE_PYRAMIDE] Nombre d'arguments incorrect.\n\n";
	&ecrit_log("ERREUR Nombre d'arguments incorrect.");
	if(! defined $opt_p){
		print "[PREPARE_PYRAMIDE] Veuillez specifier un parametre -p.\n";
	}
	if(! defined $opt_i){
		print "[PREPARE_PYRAMIDE] Veuillez specifier un parametre -i.\n";
	}
	if(! defined $opt_r){
		print "[PREPARE_PYRAMIDE] Veuillez specifier un parametre -r.\n";
	}
	if(! defined $opt_s){
		print "[PREPARE_PYRAMIDE] Veuillez specifier un parametre -s.\n";
	}
	if(! defined $opt_c){
		print "[PREPARE_PYRAMIDE] Veuillez specifier un parametre -c.\n";
	}
	if(! defined $opt_t){
		print "[PREPARE_PYRAMIDE] Veuillez specifier un parametre -t.\n";
	}
	if(! defined $opt_n){
		print "[PREPARE_PYRAMIDE] Veuillez specifier un parametre -n.\n";
	}
	if(! defined $opt_x){
		print "[PREPARE_PYRAMIDE] Veuillez specifier un parametre -x.\n";
	}
}
if(defined $opt_f){
	if(! defined $opt_a){
		$bool_getopt_ok = 0;
		print "[PREPARE_PYRAMIDE] -f est present : veuillez specifier un parametre -a.\n";
	}
	if(! defined $opt_y){
		$bool_getopt_ok = 0;
		print "[PREPARE_PYRAMIDE] -f est present : veuillez specifier un parametre -y.\n";
	}
	if(! defined $opt_w){
		$bool_getopt_ok = 0;
		print "[PREPARE_PYRAMIDE] -f est present : veuillez specifier un parametre -w.\n";
	}
	if(! defined $opt_h){
		$bool_getopt_ok = 0;
		print "[PREPARE_PYRAMIDE] -f est present : veuillez specifier un parametre -h.\n";
	}
}

if ($bool_getopt_ok == 0){
	&usage();
	exit;
}

my $produit = $opt_p;
my $rep_images_source = $opt_i;
my $rep_masque_mtd;
if (defined $opt_m){
	$rep_masque_mtd = $opt_m;
}
my $rep_pyramide = $opt_r;
my $compression_pyramide = $opt_c;
my $RIG = $opt_s;
my $rep_fichiers_dallage = $opt_t;
my $annee = $opt_n;
my $departement;
if (defined $opt_d){
	$departement = uc($opt_d);
}
my $taille_dalle_pix = $opt_x;
my $resolution_source_x;
my $resolution_source_y;
my $taille_pix_source_x;
my $taille_pix_source_y;
if (defined $opt_f){
	$resolution_source_x = $opt_a;
	$resolution_source_y = $opt_y;
	$taille_pix_source_x = $opt_w;
	$taille_pix_source_y = $opt_h;
}
# verifier les parametres
my $bool_param_ok = 1;
if ($produit !~ /^ortho|parcellaire|scan|franceraster$/i){
	print "[PREPARE_PYRAMIDE] Produit mal specifie.\n";
	&ecrit_log("ERREUR Produit mal specifie.");
	$bool_param_ok = 0;
}else{
	$produit = lc($produit);
	
}

my $fichier_tms = $produit_tms{$produit};
if (! (-e $fichier_tms && -f $fichier_tms) ){
	print "[PREPARE_PYRAMIDE] Le fichier $fichier_tms est introuvable.\n";
	$bool_param_ok = 0;
}

if (!(-e $rep_images_source && -d $rep_images_source)){
	print "[PREPARE_PYRAMIDE] Le repertoire $rep_images_source n'existe pas.\n";
	&ecrit_log("ERREUR Le repertoire $rep_images_source n'existe pas.");
	$bool_param_ok = 0;
}
if (defined $rep_masque_mtd && (!(-e $rep_masque_mtd && -d $rep_masque_mtd))){
	print "[PREPARE_PYRAMIDE] Le repertoire $rep_masque_mtd n'existe pas.\n";
	&ecrit_log("ERREUR Le repertoire $rep_masque_mtd n'existe pas.");
	$bool_param_ok = 0;
}
if (!(-e $rep_fichiers_dallage && -d $rep_fichiers_dallage)){
	print "[PREPARE_PYRAMIDE] Le repertoire $rep_fichiers_dallage n'existe pas.\n";
	&ecrit_log("ERREUR Le repertoire $rep_fichiers_dallage n'existe pas.");
	$bool_param_ok = 0;
}
if($compression_pyramide !~ /^raw|jpeg|png$/i){
	print "[PREPARE_PYRAMIDE] Le parametre de compression $compression_pyramide est incorrect.\n";
	&ecrit_log("ERREUR Le parametre de compression $compression_pyramide est incorrect.");
	$bool_param_ok = 0;
}
if(defined $departement && $departement !~ /^\d{2,3}|2[AB]$/i){
	print "[PREPARE_PYRAMIDE] Departement mal specifie.\n";
	&ecrit_log("ERREUR Departement mal specifie : $departement.");
	$bool_param_ok = 0;
}
if($annee !~ /^\d{4}(?:\-\d{2})?$/i){
	print "[PREPARE_PYRAMIDE] Annee mal specifiee.\n";
	&ecrit_log("ERREUR Annee mal specifiee : $annee.");
	$bool_param_ok = 0;
}
if($taille_dalle_pix !~ /^\d+$/i){
	print "[PREPARE_PYRAMIDE] Taille des dalles en pixels mal specifiee.\n";
	&ecrit_log("ERREUR Taille des dalles en pixels mal specifiee : $taille_dalle_pix.");
	$bool_param_ok = 0;
}
if(defined $opt_f){
	if($resolution_source_x !~ /^\d+(?:\.\d+)?$/){
		print "[PREPARE_PYRAMIDE] Resolution X des dalles source mal specifiee.\n";
		&ecrit_log("ERREUR Resolution X des dalles source mal specifiee : $resolution_source_x.");
		$bool_param_ok = 0;
	}
	if($resolution_source_y !~ /^\d+(?:\.\d+)?$/){
		print "[PREPARE_PYRAMIDE] Resolution Y des dalles source mal specifiee.\n";
		&ecrit_log("ERREUR Resolution Y des dalles source mal specifiee : $resolution_source_y.");
		$bool_param_ok = 0;
	}
	if($taille_pix_source_x !~ /^\d+$/){
		print "[PREPARE_PYRAMIDE] Taille pixel X des dalles source mal specifiee.\n";
		&ecrit_log("ERREUR Taille pixel X des dalles source mal specifiee : $taille_pix_source_x.");
		$bool_param_ok = 0;
	}
	if($taille_pix_source_y !~ /^\d+$/){
		print "[PREPARE_PYRAMIDE] Taille pixel Y des dalles source mal specifiee.\n";
		&ecrit_log("ERREUR Taille pixel Y des dalles source mal specifiee : $taille_pix_source_y.");
		$bool_param_ok = 0;
	}
}
if ($bool_param_ok == 0){
	exit;
}


# action 1 : creer dalles_source_image et dalles_source_metadata
&ecrit_log("Recensement des images dans $rep_images_source.");
my ($ref_images_source, $nb_images) = &cherche_images($rep_images_source);
&ecrit_log("$nb_images images dans $rep_images_source.");
&ecrit_log("Recensement des infos des images de $rep_images_source.");
my ($reference_hash_x_min, $reference_hash_x_max, $reference_hash_y_min, $reference_hash_y_max, $reference_hash_res_x, $reference_hash_res_y, $x_min_bbox, $x_max_bbox, $y_min_bbox, $y_max_bbox) = &cherche_infos_dalle($ref_images_source);
&ecrit_log("Ecriture du fichier des dalles source.");
my $nom_fichier_dallage_image = &ecrit_dallage_source($ref_images_source, $reference_hash_x_min, $reference_hash_x_max, $reference_hash_y_min, $reference_hash_y_max, $reference_hash_res_x, $reference_hash_res_y, $rep_fichiers_dallage, "image");
# seulement si les mtd sont specifiees
my $nom_fichier_dallage_mtd = "";
if(defined $rep_masque_mtd){
	&ecrit_log("Recensement des mtd dans $rep_masque_mtd.");
	my ($ref_mtd_source, $nb_mtd) = &cherche_images($rep_masque_mtd);
	&ecrit_log("$nb_mtd mtd dans $rep_masque_mtd.");
	&ecrit_log("Recensement des infos des mtd de $rep_masque_mtd.");
	my ($mtd_hash_x_min, $mtd_hash_x_max, $mtd_hash_y_min, $mtd_hash_y_max, $mtd_hash_res_x, $mtd_hash_res_y, $non_utilise1, $non_utilise2) = &cherche_infos_dalle($ref_mtd_source);
	&ecrit_log("Ecriture du fichier des mtd source.");
	$nom_fichier_dallage_mtd = &ecrit_dallage_source($ref_mtd_source, $mtd_hash_x_min, $mtd_hash_x_max, $mtd_hash_y_min, $mtd_hash_y_max, $mtd_hash_res_x, $mtd_hash_res_y, $rep_fichiers_dallage, "mtd");
}


# action 2 : creer pyramid.pyr en XML
my $srs_pyramide = "IGNF_".uc($RIG);
my $nom_pyramide = uc($produit)."_".uc($compression_pyramide)."_".uc($srs_pyramide)."_".$annee;
if(defined $departement){
	$nom_pyramide .= "_".$departement;
}

my $format_images = $format_format_pyr{lc($compression_pyramide)};
my $nb_channels = $produit_nb_canaux{$produit};

# creation du repertoire de la pyramide
if ( !(-e $rep_pyramide && -d $rep_pyramide) ){
	mkdir "$rep_pyramide", 0775 or die "[PREPARE_PYRAMIDE] Impossible de creer le repertoire $rep_pyramide.";
}
# creation du sous-repertoire de la pyramide
if ( !(-e "$rep_pyramide/$nom_pyramide" && -d "$rep_pyramide/$nom_pyramide") ){
	mkdir "$rep_pyramide/$nom_pyramide", 0775 or die "[PREPARE_PYRAMIDE] Impossible de creer le repertoire $rep_pyramide/$nom_pyramide.";
}

my $nom_fichier_pyramide = $nom_pyramide.".pyr";
&ecrit_log("Creation de $nom_fichier_pyramide.");
my ($ref_repertoires, $nom_fichier_final) = &cree_xml_pyramide($nom_fichier_pyramide, "$rep_pyramide/$nom_pyramide", $fichier_tms, $taille_dalle_pix, $format_images, $nb_channels, $type_mtd_pyr, $format_mtd_pyr, $profondeur_pyr, $x_min_bbox, $x_max_bbox, $y_min_bbox, $y_max_bbox);

# TODO validation du .pyr par le xsd
# &ecrit_log("Validation de $nom_fichier_pyramide.");
# my $valid = &valide_xml($nom_fichier_final, $xsd_pyramide);
# if ($valid ne ""){
# 	print "[PREPARE_PYRAMIDE] Le document n'est pas valide!\n";
# 	&ecrit_log("ERREUR a la validation de $nom_fichier_final par $xsd_pyramide : $valid");
# }

# action 3 : creer les sous-repertoires utiles
&ecrit_log("Creation des repertoires des niveaux de la pyramide.");
my @repertoires = @{$ref_repertoires};
foreach my $rep_a_creer(@repertoires){
	if ( !(-e $rep_a_creer && -d $rep_a_creer) ){
		mkdir $rep_a_creer, 0775 or die "[PREPARE_PYRAMIDE] Impossible de creer le repertoire $rep_a_creer.";
	}
}
&ecrit_log("Repertoires de la pyramide crees.");

# pour recuperation par d'autres scripts
# 1 nom fichier pyr
print "$nom_fichier_final\n";
# 2 nom dallage_image
print "$nom_fichier_dallage_image\n";
# 3 nom_dallage_mtd
if($nom_fichier_dallage_mtd ne ""){
	print "$nom_fichier_dallage_mtd\n";
}
&ecrit_log("Traitement termine.");
close LOG;
################################################################################

###### FONCTIONS

sub usage{
	
	my $bool_ok = 0;
	# TODO ajouter resx resy
	print "\nUsage : \nprepare_pyramide.pl -p produit [-f -a resolution_x_source -y resolution_y_source -w taille_pix_x_source -h taille_pix_x_source] -i path/repertoire_images_source [-m path/repertoire_masques_metadonnees] -r path/repertoire_pyramide -c compression_images_pyramide -t path/repertoire_fichiers_dallage -s systeme_coordonnees_pyramide -n annee [-d departement] -x taille_dalles_pixels\n";
	print "\nproduit :\n";
 	print "\tortho\n\tparcellaire\n\tscan\n\tfranceraster\n";
 	print "\n-f (optionnel) : pour utiliser la nomenclature standard des produits IGN\n";
 	print "\t-a resolution en X ; -y resolution en Y en METRES des images SOURCE si -f est defini, sinon aucun effet\n";
 	print "\t-w taille pixels en X ; -h taille pixels en Y des images SOURCE si -f est defini, sinon aucun effet\n";
	print "\ncompression images pyramide :\n";
	print "\traw\n\tjpeg\n\tpng\n";
	print "\nsysteme_coordonnees :\n";
	print "\tcode RIG de la pyramide a creer : LAMB93 LAMBE WGS84 ...\n";
	print "\nannee :\n";
	print "\tscan25 et scan50 :\n";
	print "\t\t2009-04\n";
	print "\tautres :\n";
	print "\t\t2009\n\n";
	
	$bool_ok = 1;
	return $bool_ok;
}
################################################################################
sub cherche_images{
	
	my $repertoire = $_[0];
	
	my @img_trouvees;
	
	
	opendir REP, $repertoire or die "[PREPARE_PYRAMIDE] Impossible d'ouvrir le repertoire $repertoire.";
	my @fichiers = readdir REP;
	closedir REP;
	foreach my $fic(@fichiers){
		next if ($fic =~ /^\.\.?$/);
		if ($fic =~ /\.tif$/i){
			my $image;
			if(substr("$repertoire/$fic", 0, 1) eq "/"){
				$image = "$repertoire/$fic";
			}else{
				$image = abs_path("$repertoire/$fic");
			}
			push(@img_trouvees, $image);
			next;
		}
		if(-d "$repertoire/$fic"){
			my ($ref_images, $nb_temp) = &cherche_images("$repertoire/$fic");
			my @images_supp = @{$ref_images};
			push(@img_trouvees, @images_supp);
			next;
		}
	}
	
	my $nombre = @img_trouvees;
	
	return (\@img_trouvees, $nombre);

}
################################################################################
sub cherche_infos_dalle{
	
	my $ref_images = $_[0];
	my @imgs = @{$ref_images};
		
	my %hash_x_min;
	my %hash_x_max;
	my %hash_y_min;
	my %hash_y_max;
	
	my %hash_res_x;
	my %hash_res_y;
	
	my $x_min_source = 9999999999999;
	my $x_max_source = 0;
	my $y_min_source = 9999999999999;
	my $y_max_source = 0;
	
	my $bool_ok = 0;
	
	my $nb_dalles = @imgs;
	my $pourcent = 0;
	
	for(my $i = 0; $i < @imgs; $i++){
#	foreach my $image(@imgs){
		
		if(defined $opt_f){
			my $nom_image = basename($imgs[$i]);
			if($nom_image =~ /^$produit_nomenclature{$produit}/i){
				$hash_x_min{$imgs[$i]} = $1;
				$hash_y_max{$imgs[$i]} = $2;
				$hash_res_x{$imgs[$i]} = $resolution_source_x;
				$hash_res_y{$imgs[$i]} = $resolution_source_y;
				$hash_x_max{$imgs[$i]} = $hash_x_min{$imgs[$i]} + $resolution_source_x * $taille_pix_source_x;
				$hash_y_min{$imgs[$i]} = $hash_y_max{$imgs[$i]} - $resolution_source_y * $taille_pix_source_y;
			}else{
				print "[PREPARE_PYRAMIDE] ERREUR : Nomenclature de $nom_image incorrecte.\n";
				&ecrit_log("ERREUR Nomenclature de $nom_image incorrecte.");
			}
			
		}else{
			# recuperation x_min x_max y_min y_max res_x res_y
			my @result = `.\/gdalinfo $imgs[$i]`;
			
			foreach my $resultat(@result){
				if($resultat =~ /Upper Left\s*\(\s*(\d+(?:\.\d+)?),\s*(\d+(?:\.\d+)?)\)/i){
					$hash_x_min{$imgs[$i]} = $1;
					$hash_y_max{$imgs[$i]} = $2;
					
				}elsif($resultat =~ /Lower Right\s*\(\s*(\d+(?:\.\d+)?),\s*(\d+(?:\.\d+)?)\)/i){
					$hash_x_max{$imgs[$i]} = $1;
					$hash_y_min{$imgs[$i]} = $2;
					
				}elsif($resultat =~ /Pixel Size = \((\d+)\.(\d+), ?\-(\d+)\.(\d+)\)/){
					$hash_res_x{$imgs[$i]} = $1 + ( $2 / (10**length($2)));
					$hash_res_y{$imgs[$i]} = $3 + ( $4 / (10**length($4)));
				}
			}
		}
		
		# actualisation des xmin et ymax du chantier
		if($hash_x_min{$imgs[$i]} < $x_min_source){
			$x_min_source = $hash_x_min{$imgs[$i]};
		}
		if($hash_y_max{$imgs[$i]} > $y_max_source){
			$y_max_source = $hash_y_max{$imgs[$i]}
		}
		# actualisation des xmax et ymin du chantier
		if($hash_x_max{$imgs[$i]} > $x_max_source){
			$x_max_source = $hash_x_max{$imgs[$i]};
		}
		if($hash_y_min{$imgs[$i]} < $y_min_source){
			$y_min_source = $hash_y_min{$imgs[$i]}
		}
		
	}

	my @refs = (\%hash_x_min, \%hash_x_max, \%hash_y_min, \%hash_y_max, \%hash_res_x, \%hash_res_y, $x_min_source, $x_max_source, $y_min_source, $y_max_source);
	
	return @refs;
	
}
################################################################################
sub ecrit_dallage_source{
	
	my $ref_tableau_image = $_[0];
	
	my @tableau_images = @{$ref_tableau_image};
	
	my $ref_hash_x_min = $_[1];
	my $ref_hash_x_max = $_[2];
	my $ref_hash_y_min = $_[3];
	my $ref_hash_y_max = $_[4];
	
	my %dalle_x_min = %{$ref_hash_x_min};
	my %dalle_x_max = %{$ref_hash_x_max};
	my %dalle_y_min = %{$ref_hash_y_min};
	my %dalle_y_max = %{$ref_hash_y_max};
	
	my $ref_hash_res_x = $_[5];
	my $ref_hash_res_y = $_[6];
	
	my %dalle_res_x = %{$ref_hash_res_x};
	my %dalle_res_y = %{$ref_hash_res_y};
	
	my $rep_fichier = $_[7];
	my $type = $_[8];
	
	my $fichier_dallage_source;
	if ($type eq "image"){
		$fichier_dallage_source = $rep_fichier."/".$nom_fichier_dalle_source;
	}elsif($type eq "mtd"){
		$fichier_dallage_source = $rep_fichier."/".$nom_fichier_mtd_source;
	}else{
		print "[PREPARE_PYRAMIDE] Probleme de programmation : type $type incorrect.\n";
		exit;
	}
	
	open DALLAGE, ">$fichier_dallage_source" or die "[PREPARE_PYRAMIDE] Impossible de creer le fichier $fichier_dallage_source.";
	
	foreach my $image(@tableau_images){
		print DALLAGE "$image\t$dalle_x_min{$image}\t$dalle_y_max{$image}\t$dalle_x_max{$image}\t$dalle_y_min{$image}\t$dalle_res_x{$image}\t$dalle_res_y{$image}\n";
	}
	close DALLAGE;
		
	return $fichier_dallage_source;
}
################################################################################
sub cree_xml_pyramide{
	
	my $nom_fichier = $_[0];
	my $rep_pyr = $_[1];
	my $tms = $_[2];
	my $taille_dalle = $_[3];
	my $format_dalle = $_[4];
	my $nb_canaux = $_[5];
 	my $type_mtd = $_[6];
 	my $format_mtd = $_[7];
	my $profondeur = $_[8];
	my $x_min_donnees = $_[9];
	my $x_max_donnees = $_[10];
	my $y_min_donnees = $_[11];
	my $y_max_donnees = $_[12];
	
	my @liste_repertoires = ();
	
	my ($ref_id, $ref_id_resolution, $ref_id_taille_pix_tuile_x, $ref_id_taille_pix_tuile_y, $ref_origine_x, $ref_origine_y, $var_inutile1) = &lecture_tile_matrix_set($tms);
	my @id_tms = @{$ref_id};
	my %tms_level_resolution = %{$ref_id_resolution};
	my %tms_level_taille_pix_tuile_x = %{$ref_id_taille_pix_tuile_x};
	my %tms_level_taille_pix_tuile_y = %{$ref_id_taille_pix_tuile_y};
	my %tms_level_origine_x = %{$ref_origine_x};
	my %tms_level_origine_y = %{$ref_origine_y};
	
	my $nom_tms = substr(basename($tms), 0, length(basename($tms)) - 4);
	
	my $rep_images;
	my $rep_mtd;
	if (substr($rep_pyr, 0, 1) eq "/"){
		$rep_images = $rep_pyr."/".$nom_rep_images;
		$rep_mtd = $rep_pyr."/".$nom_rep_mtd;
	}else{
		$rep_images = abs_path($rep_pyr."/".$nom_rep_images);
		$rep_mtd = abs_path($rep_pyr."/".$nom_rep_mtd);
	}
	
	push(@liste_repertoires, "$rep_images", "$rep_mtd");
	
	my $fichier_complet = "$rep_pyr/$nom_fichier";
	
	open PYRAMIDE, ">$fichier_complet" or die "[PREPARE_PYRAMIDE] Impossible de creer le fichier $fichier_complet.";
	print PYRAMIDE "<?xml version='1.0' encoding='US-ASCII'?>\n";
	print PYRAMIDE "<Pyramid>\n";
	print PYRAMIDE "\t<tileMatrixSet>$nom_tms</tileMatrixSet>\n";
	foreach my $level(@id_tms){
		
		my $resolution = $tms_level_resolution{"$level"};
		my $taille_image_m = $resolution * $taille_dalle;
		my $taille_tuile_x = $tms_level_taille_pix_tuile_x{"$level"};
		my $taille_tuile_y = $tms_level_taille_pix_tuile_y{"$level"};
		my $origine_x = $tms_level_origine_x{"$level"};
		my $origine_y = $tms_level_origine_y{"$level"};
		
		my $reste_x = $taille_dalle % $taille_tuile_x;
		my $reste_y = $taille_dalle % $taille_tuile_y;
		
		# si la taille n'est pas un multiple de la taille des tuiles on sort
		if($reste_x != 0 || $reste_y != 0){
			close PYRAMIDE;
			unlink "$fichier_complet";
			print "[PREPARE_PYRAMIDE] ERREUR : Taille images $taille_dalle non multiple taille tuile du TMS au niveau $level.\n";
			&ecrit_log("ERREUR Taille des images $taille_dalle non multiple taille tuile du TMS au niveau $level.");
			exit;
		}
		
		my $nb_tuile_x = $taille_dalle / $taille_tuile_x;
		my $nb_tuile_y = $taille_dalle / $taille_tuile_y;
		
		# bbox des sources : indice de la tuile dans le dallage du niveau du TMS
		my $min_tuile_x = floor(($x_min_donnees - $origine_x) / ($resolution * $taille_tuile_x));
		my $max_tuile_x = floor(($x_max_donnees - $origine_x) / ($resolution * $taille_tuile_x));
		my $min_tuile_y = floor(($origine_y - $y_max_donnees) / ($resolution * $taille_tuile_y));
		my $max_tuile_y = floor(($origine_y - $y_min_donnees) / ($resolution * $taille_tuile_y));
		
		print PYRAMIDE "\t<level>\n";
		print PYRAMIDE "\t\t<tileMatrix>$level</tileMatrix>\n";
		print PYRAMIDE "\t\t<baseDir>$rep_images/$taille_image_m</baseDir>\n";
		print PYRAMIDE "\t\t<format>$format_dalle</format>\n";
		print PYRAMIDE "\t\t<metadata type='$type_mtd'>\n";
		print PYRAMIDE "\t\t\t<baseDir>$rep_mtd/$taille_image_m</baseDir>\n";
		print PYRAMIDE "\t\t\t<format>$format_mtd</format>\n";
		print PYRAMIDE "\t\t</metadata>\n";
		print PYRAMIDE "\t\t<channels>$nb_canaux</channels>\n";
		print PYRAMIDE "\t\t<tilesPerWidth>$nb_tuile_x</tilesPerWidth>\n";
		print PYRAMIDE "\t\t<tilesPerHeight>$nb_tuile_y</tilesPerHeight>\n";
		print PYRAMIDE "\t\t<pathDepth>$profondeur</pathDepth>\n";
		# les limites sont mises par rapport aux donnees mises a jour
		# l'info sera corrigee dans l'initialisation de la pyramide
		print PYRAMIDE "\t\t<TMSLimits>\n";
		print PYRAMIDE "\t\t\t<minTileRow>$min_tuile_x</minTileRow>\n";
		print PYRAMIDE "\t\t\t<maxTileRow>$max_tuile_x</maxTileRow>\n";
		print PYRAMIDE "\t\t\t<minTileCol>$min_tuile_y</minTileCol>\n";
		print PYRAMIDE "\t\t\t<maxTileCol>$max_tuile_y</maxTileCol>\n";
		print PYRAMIDE "\t\t</TMSLimits>\n";
		print PYRAMIDE "\t</level>\n";
		push(@liste_repertoires, "$rep_images/$taille_image_m", "$rep_mtd/$taille_image_m");
	}
	
	print PYRAMIDE "</Pyramid>\n";
	close PYRAMIDE;
		
	return (\@liste_repertoires, $fichier_complet);
}

################################################################################
sub calcule_valeur_ronde{

	my $nombre = $_[0];
	my $pas = $_[1];
	
	# calcule l'entier multiple de $pas le plus proche et inferieur a $nombre
	
	my $reste = $nombre % $pas;
	my $new_nombre = $nombre - $reste;
	
	return $new_nombre;

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
sub valide_xml{

	my $schema = $_[0];
	my $document = $_[1];
	
	my $reponse = '';
	
	$reponse = `/usr/local/j2sdk/bin/java -classpath /usr/local/xerces/xercesSamples.jar:/usr/local/xerces/xml-apis.jar:/usr/local/xerces/xercesImpl.jar:/usr/local/xerces/resolver.jar:/usr/local/j2sdk/lib/tools.jar xni.XMLGrammarBuilder -f -a $schema -i $document 2>&1`;
	
	return $reponse;
	
}
################################################################################
