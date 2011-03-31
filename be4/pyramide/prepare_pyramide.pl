#!/usr/bin/perl -w 

use strict;
use Getopt::Std;
use Cwd 'abs_path';
use File::Basename;
use File::Copy;
use POSIX qw(floor);
use cache(
	'$type_mtd_pyr_param',
	'$format_mtd_pyr_param',
	'$profondeur_pyr_param',
	'$nom_fichier_dalle_source_param',
	'$nom_rep_images_param',
	'$nom_rep_mtd_param',
	'$nom_fichier_mtd_source_param',
	'%produit_nb_canaux_param',
	'$xsd_pyramide_param',
	'lecture_tile_matrix_set',
	'$rep_logs_param',
	'%format_format_pyr_param',
	'%produit_nomenclature_param',
	'cree_nom_pyramide',
	'cherche_pyramide_recente_lay',
	'extrait_tms_from_pyr',
	'valide_xml',
);
#### CONSTANTES

# nom du fichier de dallage des dalles source (description des caracteristiques des images)
my $nom_fichier_dalle_source = $nom_fichier_dalle_source_param;
# nom du fichier de dallages de masques de mts source (description des caracteristiques des masques)
my $nom_fichier_mtd_source = $nom_fichier_mtd_source_param;
# nom du repertoire des images dans la pyramide
my $nom_rep_images = $nom_rep_images_param;
# nom du repertoire des mtd dans la pyramide
my $nom_rep_mtd = $nom_rep_mtd_param;
# type de mtd dans la pyramide
my $type_mtd_pyr = $type_mtd_pyr_param;
# format des mtd dans la pyramide
my $format_mtd_pyr = $format_mtd_pyr_param;
# profondeur de chemin des fichiers dans la pyramide
my $profondeur_pyr = $profondeur_pyr_param;
# association entre grande famille de produit de nombre de canaux des images
my %produit_nb_canaux = %produit_nb_canaux_param;
# chemin vers le schema XML qui contraint les fichiers XML de pyramide
my $xsd_pyramide = $xsd_pyramide_param;
# chemin vers le repertoire ou mettre les logs
my $rep_log = $rep_logs_param;
# association entre type de compression des images et format dans la pyramide
my %format_format_pyr = %format_format_pyr_param;
# association entre grande famille de produit et nomenclature standard des dalles IGN (sous forme d'expression reguliere)
my %produit_nomenclature = %produit_nomenclature_param;
################################################################################

# pas de bufferisation des sorties ecran
$| = 1;

#### VARIABLES GLOBALES

# valeur des parametres de la ligne de commande
our($opt_p, $opt_i, $opt_r, $opt_c, $opt_s, $opt_t, $opt_n, $opt_d, $opt_m, $opt_x, $opt_f, $opt_a, $opt_y, $opt_w, $opt_h, $opt_l);

# nom de la grande famille de produits
my $produit;
# nom du produit
my $ss_produit;
# chemin vers le repertoire des images source ou chemin vers un fichier de dallage issu d'un calcul precedent
my $images_source;
# chemin vers le repertoire des masques de mtd source ou chemin vers un fichier de dallage issu d'un calcul precedent
my $masque_mtd;
# repertoire ou creer la pyramide
my $rep_pyramide;
# type de compression des images de la pyramide
my $compression_pyramide;
# systeme de coordonnees de la pyramide
my $srs_pyramide;
# repertoire des fichiers de dallage a creer
my $rep_fichiers_dallage;
# annee de production (ou trimestre) des dalles source (ex : 2010 ou 2010-01)
my $annee;
# departement des dalles source (optionnel)
my $departement;
# taille des dalles de la pyramide en pixels
my $taille_dalle_pix;
# resolution des images source selon l'axe X (utilise seulement si la nomenclature des dalles IGN est specifiee)
my $resolution_source_x;
# resolution des images source selon l'axe Y (utilise seulement si la nomenclature des dalles IGN est specifiee)
my $resolution_source_y;
# taille des images source en pixels selon l'axe X (utilise seulement si la nomenclature des dalles IGN est specifiee)
my $taille_pix_source_x;
# taille des images source en pixels selon l'axe Y (utilise seulement si la nomenclature des dalles IGN est specifiee)
my $taille_pix_source_y;
# fichier du tms
my $fichier_tms;

#### VARIABLES GLOBALES NON INITIALISEES

# chemin vers le fichier de dallage des images source (a creer)
my $nom_fichier_dallage_image;
# chemin vers le fichier de dallage des mtd source (a creer)
my $nom_fichier_dallage_mtd;
# coordonnees des coins du rectangle englobant les donnees source
my ($x_min_bbox, $x_max_bbox, $y_min_bbox, $y_max_bbox);
# chemin vers le fichier de pyramide .pyr
my $fichier_pyramide_final;

##### MAIN

# verification des parametres et initialisation des variables globales
my $bool_init_ok = &init();
if($bool_init_ok == 0){
	exit;
}

# action 1 : creer dalles_source_image
($nom_fichier_dallage_image, $x_min_bbox, $x_max_bbox, $y_min_bbox, $y_max_bbox) = &cree_fichier_dallage($images_source, $rep_fichiers_dallage , "image", $x_min_bbox, $x_max_bbox, $y_min_bbox, $y_max_bbox);

# action 2 : creer dalles_source_metadata
($nom_fichier_dallage_mtd, $x_min_bbox, $x_max_bbox, $y_min_bbox, $y_max_bbox) = &cree_fichier_dallage($masque_mtd, $rep_fichiers_dallage, "mtd", $x_min_bbox, $x_max_bbox, $y_min_bbox, $y_max_bbox);

# action 3 : creer le pyramide en XML et les repertoires sur le systeme de fichiers
$fichier_pyramide_final = &cree_pyramide($produit, $ss_produit, $compression_pyramide, $srs_pyramide, $annee, $departement, $fichier_tms, $taille_dalle_pix, $type_mtd_pyr, $format_mtd_pyr, $profondeur_pyr, $x_min_bbox, $x_max_bbox, $y_min_bbox, $y_max_bbox, $xsd_pyramide);


# pour recuperation par d'autres scripts, on ecrit sur la sortie standard
# 1 nom fichier pyr
print "$fichier_pyramide_final\n";
# 2 nom dallage_image
print "$nom_fichier_dallage_image\n";
# 3 nom_dallage_mtd
if($nom_fichier_dallage_mtd ne ""){
	print "$nom_fichier_dallage_mtd\n";
}

&fin();

################################################################################

###### FONCTIONS

sub usage{
	
	my $bool_ok = 0;

	print "\nUsage : \nprepare_pyramide.pl -p produit [-f -a resolution_x_source -y resolution_y_source -w taille_pix_x_source -h taille_pix_x_source] -i images_source [-m masques_metadonnees] -r path/repertoire_pyramide -c compression_images_pyramide -t path/repertoire_fichiers_dallage -s systeme_coordonnees_pyramide -n annee [-d departement] -x taille_dalles_pixels -l nom_layer\n";
	print "\nproduit :\n";
 	print "\tortho\n\tparcellaire\n\tscan[25|50|100|dep|reg|1000]\n\tfranceraster\n";
 	print "\n-f (optionnel) : pour utiliser la nomenclature standard des produits IGN\n";
 	print "\t-a resolution en X ; -y resolution en Y en UNITES DU SRS des images SOURCE si -f est defini, sinon aucun effet\n";
 	print "\t-w taille pixels en X ; -h taille pixels en Y des images SOURCE si -f est defini, sinon aucun effet\n";
	print "\t-i images_source : un repertoire d'images ou un fichiers de dalles source issu d'un calcul precedent\n";
	print "\t-m masques_metadonnees (optionnel) : un repertoire de mtd ou un fichiers de mtd source issu d'un calcul precedent\n";
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
# examine recursivement un repertoire et en extrait un reference vers un tableau contenant la liste des images
sub cherche_images{
	
	# parametre : chemin vers le repertoire a examiner
	my $repertoire = $_[0];
	
	# liste des images
	my @img_trouvees;
	
	# ouverture du repertoire et stockage de la liste de ses fichiers dans un tableau
	opendir REP, $repertoire or die "[PREPARE_PYRAMIDE] Impossible d'ouvrir le repertoire $repertoire.";
	my @fichiers = readdir REP;
	closedir REP;
	# boucle sur les fichiers
	foreach my $fic(@fichiers){
		# on ecarte . et .. sinon on boucle a l'infini
		next if ($fic =~ /^\.\.?$/);
		# si le fichier a une extension .tif et qu'elle ne contient pas 10m dans son nom (on ecarte les imagettes)
		if ($fic =~ /\.tif$/i && $fic !~ /10m/){
			my $image;
			# si le repertoire etudie est en absolu (comment par un /), on stocke directement le chemin de l'image tel quel
			if(substr("$repertoire", 0, 1) eq "/"){
				$image = "$repertoire/$fic";
			}else{
				# sinon on prend le chemin absolu
				$image = abs_path("$repertoire/$fic");
			}
			push(@img_trouvees, $image);
			next;
		}
		# si le fichier est en realite un repertoire (mais pas un repertoire d'imagettes contenant 10m dans son nom)
		if(-d "$repertoire/$fic" && $fic !~ /10m/){
			# on ajoute les images de ce sous-repertoire a celles du repertoire examine
			my ($ref_images, $nb_temp) = &cherche_images("$repertoire/$fic");
			my @images_supp = @{$ref_images};
			push(@img_trouvees, @images_supp);
			next;
		}
	}
	
	# determination du nombre d'images trouvees
	my $nombre = @img_trouvees;
	
	return (\@img_trouvees, $nombre);

}
################################################################################
# extraction des caracteristiques de chaque image de la liste en parametre : x_min, x_max, y_min, y_max, resolution en x, resolution en y
# et extraction des coordonnees de coins du rectangle englobant ces donnees
sub cherche_infos_dalle{
	
	# parametre : reference vers un tableau contenant une liste de chemins vers des images
	my $ref_images = $_[0];
	# recuperation du parametre precedent ous forme de tableau
	my @imgs = @{$ref_images};
	
	# association entre chemin d'une image et x_min
	my %hash_x_min;
	# association entre chemin d'une image et x_max
	my %hash_x_max;
	# association entre chemin d'une image et y_min
	my %hash_y_min;
	# association entre chemin d'une image et y_max
	my %hash_y_max;
	
	# association entre chemin d'une image et resolution en x
	my %hash_res_x;
	# association entre chemin d'une image et resolution en y
	my %hash_res_y;
	
	# x_min du rectangle englobant les images
	my $x_min_source = 9999999999999;
	# x_max du rectangle englobant les images
	my $x_max_source = 0;
	# y_min du rectangle englobant les images
	my $y_min_source = 9999999999999;
	# y_max du rectangle englobant les images
	my $y_max_source = 0;
	
	# boucle sur les chemin des images en utilisant un indice d'incrementation
	for(my $i = 0; $i < @imgs; $i++){
		# cas ou on utilise la noenclature standard des dalles IGN
		if(defined $opt_f){
			# on ne recupere que le nom de l'image (sans chemin)
			my $nom_image = basename($imgs[$i]);
			# test si le nom de l'image correspond bien a la nomenclature attendue
			if($nom_image =~ /^$produit_nomenclature{$produit}/i){
				# initialisation d'un multiplicateur (utilise pour calculer les coordonnees des coins de la dalle)
				my $multiplicateur = 1000;
				# traditionnellement les coordonnees sont exprimees en km, mais il peut arriver qu'elles soient en hm
				#  si on est en hexadecimal
				if(length($1) == 5 && length($2) == 5){
					# on doit seulement multiplier par 100 les coordonnees pour les obtenir en metres
					$multiplicateur = 100;
				}
				# association entre chemin de l'image et son x_min
				$hash_x_min{$imgs[$i]} = $1 * $multiplicateur;
				# association entre chemin de l'image et son y_max
				$hash_y_max{$imgs[$i]} = $2 * $multiplicateur;
				# association entre chemin de l'image et sa resolution en x
				$hash_res_x{$imgs[$i]} = $resolution_source_x;
				# association entre chemin de l'image et sa resolution en y
				$hash_res_y{$imgs[$i]} = $resolution_source_y;
				# calcul du x_max de la dalle puis association au chemin de l'image
				$hash_x_max{$imgs[$i]} = $hash_x_min{$imgs[$i]} + $resolution_source_x * $taille_pix_source_x;
				# calcul du y_min de la dalle puis association au chemin de l'image
				$hash_y_min{$imgs[$i]} = $hash_y_max{$imgs[$i]} - $resolution_source_y * $taille_pix_source_y;
			}else{
				# on indique que la dalle n'a pas la nomanclature attendue
				print "[PREPARE_PYRAMIDE] ERREUR : Nomenclature de $nom_image incorrecte.\n";
				&ecrit_log("ERREUR Nomenclature de $nom_image incorrecte.");
			}
			
		}else{
			# recuperation x_min x_max y_min y_max res_x res_y par appel au programme gdalinfo
			# stockage du resultat de l'appel au programme dans un tableau
			my @result = `.\/gdalinfo $imgs[$i]`;
			
			# boucle sur les lignes du resultat obtenu par gdalinfo
			foreach my $resultat(@result){
				# cette ligne correspond a l'expression reguliere : on va en deduire le x_min et le y_max
				if($resultat =~ /Upper Left\s*\(\s*(\d+(?:\.\d+)?),\s*(\d+(?:\.\d+)?)\)/i){
					$hash_x_min{$imgs[$i]} = $1;
					$hash_y_max{$imgs[$i]} = $2;
				# cette ligne correspond a l'expression reguliere : on va en deduire le x_max et le y_min
				}elsif($resultat =~ /Lower Right\s*\(\s*(\d+(?:\.\d+)?),\s*(\d+(?:\.\d+)?)\)/i){
					$hash_x_max{$imgs[$i]} = $1;
					$hash_y_min{$imgs[$i]} = $2;
				# cette ligne correspond a l'expression reguliere : on va en deduire la resolution en x et en y
				# la reoslution en y est prise positive par convention alors que gdalinfo nous la donne en negatif
				}elsif($resultat =~ /Pixel Size = \((\d+)\.(\d+), ?\-(\d+)\.(\d+)\)/){
					$hash_res_x{$imgs[$i]} = $1 + ( $2 / (10**length($2)));
					$hash_res_y{$imgs[$i]} = $3 + ( $4 / (10**length($4)));
				}
			}
		}
		
		# actualisation des xmin et ymax du rectangle enblobant les donnees initiales avec la nouvelle dalle etudiee
		if($hash_x_min{$imgs[$i]} < $x_min_source){
			$x_min_source = $hash_x_min{$imgs[$i]};
		}
		if($hash_y_max{$imgs[$i]} > $y_max_source){
			$y_max_source = $hash_y_max{$imgs[$i]}
		}
		# actualisation des xmax et ymin du rectangle enblobant les donnees initiales avec la nouvelle dalle etudiee
		if($hash_x_max{$imgs[$i]} > $x_max_source){
			$x_max_source = $hash_x_max{$imgs[$i]};
		}
		if($hash_y_min{$imgs[$i]} < $y_min_source){
			$y_min_source = $hash_y_min{$imgs[$i]}
		}
		
	}

	my @refs = (\%hash_x_min, \%hash_x_max, \%hash_y_min, \%hash_y_max, \%hash_res_x, \%hash_res_y, $x_min_source, $x_max_source, $y_min_source, $y_max_source);
	
	# on retourne : 
	# reference vers un hash d'association entre chemin de l'image et x_min
	# reference vers un hash d'association entre chemin de l'image et x_max
	# reference vers un hash d'association entre chemin de l'image et y_min
	# reference vers un hash d'association entre chemin de l'image et y_max
	# reference vers un hash d'association entre chemin de l'image et resolution en x
	# reference vers un hash d'association entre chemin de l'image et resolution en y
	# coordonnees des 4 coins de la bbox des dalles initiales
	return @refs;
	
}
################################################################################
# cree le fichier de dallage et donne son nom en retour
sub ecrit_dallage_source{
	
	# parametre : reference vers un tableau contenant la liste des chemins de images
	my $ref_tableau_image = $_[0];
	# recuperation du parametre precednet sous forme de tableau
	my @tableau_images = @{$ref_tableau_image};
	
	# parametre : reference vers un hash associant au chemin d'une image son x_min
	my $ref_hash_x_min = $_[1];
	# parametre : reference vers un hash associant au chemin d'une image son x_max
	my $ref_hash_x_max = $_[2];
	# parametre : reference vers un hash associant au chemin d'une image son y_min
	my $ref_hash_y_min = $_[3];
	# parametre : reference vers un hash associant au chemin d'une image son y_max
	my $ref_hash_y_max = $_[4];
	# recuperation des 4 parametres precedents sous forme de hash
	my %dalle_x_min = %{$ref_hash_x_min};
	my %dalle_x_max = %{$ref_hash_x_max};
	my %dalle_y_min = %{$ref_hash_y_min};
	my %dalle_y_max = %{$ref_hash_y_max};
	
	# parametre : reference vers un hash associant au chemin d'une image sa resolution en x
	my $ref_hash_res_x = $_[5];
	# parametre : reference vers un hash associant au chemin d'une image sa resolution en y
	my $ref_hash_res_y = $_[6];
	# recuperation des 2 parametres precedents sous forme de hash
	my %dalle_res_x = %{$ref_hash_res_x};
	my %dalle_res_y = %{$ref_hash_res_y};
	
	# parametre : chemin vers le repertoire ou creer le fichier de dallage
	my $rep_fichier = $_[7];
	# parametre : type de fichier a creer (image ou mtd)
	my $type = $_[8];
	
	# nom du fichier de dallage
	my $fichier_dallage_source = &cree_nom_fichier_dallage_source($rep_fichier, $type, $compression_pyramide, $annee, $departement);
	
	# creation du fichier
	open DALLAGE, ">$fichier_dallage_source" or die "[PREPARE_PYRAMIDE] Impossible de creer le fichier $fichier_dallage_source.";
	
	# boucle sur les chemins des images du tableau
	foreach my $image(@tableau_images){
		# ecriture des caracteristiques de l'image dans le fichier semon un formalisme etabli : 
		# chemin_image	x_min	y_max	x_max	y_min	resolution_x	resolution_y
		print DALLAGE "$image\t$dalle_x_min{$image}\t$dalle_y_max{$image}\t$dalle_x_max{$image}\t$dalle_y_min{$image}\t$dalle_res_x{$image}\t$dalle_res_y{$image}\n";
	}
	close DALLAGE;
	
	# on retourne le nom du fichier cree
	return $fichier_dallage_source;
}
################################################################################
# creation du fichier XML de pyramide et deduction des repertoires des donnes de la pyramide
sub cree_xml_pyramide{
	
	# parametre : nom du fichier de pyramide a creer
	my $nom_fichier = $_[0];
	# parametre : chemin vers le repertoire de la pyramide
	my $rep_pyr = $_[1];
	# parametre : chemin vers le XML du TMS
	my $tms = $_[2];
	# parametre : taille des dalle sde la pyramide en pixels
	my $taille_dalle = $_[3];
	# parametre : format des images de la pyramide
	my $format_dalle = $_[4];
	# parametre : nombre de canaux des images de la pyramide
	my $nb_canaux = $_[5];
	# parametre : type des mtd de la pyramide
 	my $type_mtd = $_[6];
 	# parametre : format des mtd de la pyramide
 	my $format_mtd = $_[7];
 	# parametre : profondeur de chemin des donnes de la pyramide
	my $profondeur = $_[8];
	# 4 parametre : coordonnees de la BBox des dalles composant la pyramide
	my $x_min_donnees = $_[9];
	my $x_max_donnees = $_[10];
	my $y_min_donnees = $_[11];
	my $y_max_donnees = $_[12];
	
	# tableau contenant la liste des repertoire des donnes de la pyramide (image et mtd compris)
	my @liste_repertoires = ();
	
	# lecture du TMS associe a la pyramide pour en deduire des informations
	my ($ref_id, $ref_id_resolution, $ref_id_taille_pix_tuile_x, $ref_id_taille_pix_tuile_y, $ref_origine_x, $ref_origine_y, $var_inutile1) = &lecture_tile_matrix_set($tms);
	# tableau contenant la liste des identifiant des TM du TMS
	my @id_tms = @{$ref_id};
	# association entre id du niveau et resolution
	my %tms_level_resolution = %{$ref_id_resolution};
	# association entre id du niveau et taille des tuiles en pixels selon l'axe x
	my %tms_level_taille_pix_tuile_x = %{$ref_id_taille_pix_tuile_x};
	# association entre id du niveau et taille des tuiles en pixels selon l'axe y
	my %tms_level_taille_pix_tuile_y = %{$ref_id_taille_pix_tuile_y};
	# association entre id du niveau et origine du TM en x (coin NO)
	my %tms_level_origine_x = %{$ref_origine_x};
	# association entre id du niveau et origine du TM en y (coin NO)
	my %tms_level_origine_y = %{$ref_origine_y};
	
	# nom du TMS (debarrasse de son chemin et de son extension) qui apparait dans le XML de pyramide
	my $nom_tms = substr(basename($tms), 0, length(basename($tms)) - 4);
	
	# chemin vers le repertoire des images de la pyramide
	my $rep_images;
	# chemin vers le repertoire des mtd de la pyramide
	my $rep_mtd;
	# si le chemin vers le repertoire de la pyramide est absolu (commence par un /), on l'utilise directement
	if (substr($rep_pyr, 0, 1) eq "/"){
		$rep_images = $rep_pyr."/".$nom_rep_images;
		$rep_mtd = $rep_pyr."/".$nom_rep_mtd;
	# on doit passer par la fonction qui retourne le chemin absolu
	}else{
		$rep_images = abs_path($rep_pyr."/".$nom_rep_images);
		$rep_mtd = abs_path($rep_pyr."/".$nom_rep_mtd);
	}
	
	# la liste des repertoires de la pyramide commence par les repertoires d'image et de mtd
	push(@liste_repertoires, "$rep_images", "$rep_mtd");
	
	# chemin vers le fichier XML de pyramide
	my $fichier_complet = "$rep_pyr/$nom_fichier";
	
	# creation du fichier XML de pyramide
	open PYRAMIDE, ">$fichier_complet" or die "[PREPARE_PYRAMIDE] Impossible de creer le fichier $fichier_complet.";
	# ecriture dans le fichier
	print PYRAMIDE "<?xml version='1.0' encoding='US-ASCII'?>\n";
	print PYRAMIDE "<Pyramid>\n";
	print PYRAMIDE "\t<tileMatrixSet>$nom_tms</tileMatrixSet>\n";
	# boucle sur les niveaux du TMS
	foreach my $level(@id_tms){
		
		# resolution des donnees du niveau
		my $resolution = $tms_level_resolution{"$level"};
		# emprise en metres des donnees du niveau
		my $taille_image_m = $resolution * $taille_dalle;
		# taille des tuiles en pixels selon l'axe x des donnees du niveau
		my $taille_tuile_x = $tms_level_taille_pix_tuile_x{"$level"};
		# taille des tuiles en pixels selon l'axe y des donnees du niveau
		my $taille_tuile_y = $tms_level_taille_pix_tuile_y{"$level"};
		# origine en x du repere du niveau
		my $origine_x = $tms_level_origine_x{"$level"};
		# origine en y du repere du niveau
		my $origine_y = $tms_level_origine_y{"$level"};
		
		# on regarde si la taille des dalles est bien un multiple de la taille des tuiles
		# division euclidienne 
		my $reste_x = $taille_dalle % $taille_tuile_x;
		my $reste_y = $taille_dalle % $taille_tuile_y;
		
		# si la taille n'est pas un multiple de la taille des tuiles on sort
		if($reste_x != 0 || $reste_y != 0){
			# fermeture du handler du fichier
			close PYRAMIDE;
			# suppression du fichier
			unlink "$fichier_complet";
			# ecriture de l'anomalie dans le log	
			print "[PREPARE_PYRAMIDE] ERREUR : Taille images $taille_dalle non multiple taille tuile du TMS au niveau $level.\n";
			&ecrit_log("ERREUR Taille des images $taille_dalle non multiple taille tuile du TMS au niveau $level.");
			exit;
		}
		
		# nombre de tuiles dans une dalle selon l'axe x
		my $nb_tuile_x = $taille_dalle / $taille_tuile_x;
		# nombre de tuiles dans une dalle selon l'axe y
		my $nb_tuile_y = $taille_dalle / $taille_tuile_y;
		
		# on doit indiquer dans le fichier l'emprise des dalles source (de leur rectangle englobant)
		# en termes d'indice (de numero) de tuile dans le niveau pour chaque coin
		# calcul de cet indice pour les 4 coins
		# indice de la tuile dans le TM du x_min de la BBox des dalles source
		my $min_tuile_x = floor(($x_min_donnees - $origine_x) / ($resolution * $taille_tuile_x));
		# indice de la tuile dans le TM du x_max de la BBox des dalles source
		my $max_tuile_x = floor(($x_max_donnees - $origine_x) / ($resolution * $taille_tuile_x));
		# indice de la tuile dans le TM du y_min de la BBox des dalles source
		my $min_tuile_y = floor(($origine_y - $y_max_donnees) / ($resolution * $taille_tuile_y));
		# indice de la tuile dans le TM du y_max de la BBox des dalles source
		my $max_tuile_y = floor(($origine_y - $y_min_donnees) / ($resolution * $taille_tuile_y));
		
		# ecriture dans le fichier
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
		# les limites sont mises par rapport aux donnees source mises a jour
		# l'info sera corrigee dans l'initialisation de la pyramide (en fonction des donnees deja presentes dans le layer)
		print PYRAMIDE "\t\t<TMSLimits>\n";
		print PYRAMIDE "\t\t\t<minTileRow>$min_tuile_x</minTileRow>\n";
		print PYRAMIDE "\t\t\t<maxTileRow>$max_tuile_x</maxTileRow>\n";
		print PYRAMIDE "\t\t\t<minTileCol>$min_tuile_y</minTileCol>\n";
		print PYRAMIDE "\t\t\t<maxTileCol>$max_tuile_y</maxTileCol>\n";
		print PYRAMIDE "\t\t</TMSLimits>\n";
		print PYRAMIDE "\t</level>\n";
		# le chemin vers le repertoire des images (et des mtd) de ce niveau fait partie de la liste des repertoires
		push(@liste_repertoires, "$rep_images/$taille_image_m", "$rep_mtd/$taille_image_m");
	}
	
	print PYRAMIDE "</Pyramid>\n";
	# fermeture du handler du fichier
	close PYRAMIDE;
	
	# on retourne :
	# reference vers un tableau contenant la liste des chemins absolus vers les tous les repertoires de donnees de la pyramide
	# nom du fichier cree
	return (\@liste_repertoires, $fichier_complet);
}

################################################################################
# ecrit un message dans le fichier de log
sub ecrit_log{
	
	# parametre : chaine de caracteres a inserer dans le log
	my $message = $_[0];
	
	my $bool_ok = 0;
	
	# recuperation du nom de la machine sur Linux
    chomp(my $machine_utilisee = `hostname`);
	
	# pour dater le message
	my $T = localtime();
	# ecriture dans le fichier de log
	printf LOG "$machine_utilisee %s %s\n", $T, $message;
	
	$bool_ok = 1;
	return $bool_ok;
}
################################################################################
# creation du chemin vers un fichier de dallage
sub cree_nom_fichier_dallage_source{
	
	# parametre : chemin vers le repertoire de creation du fichier
	my $rep_creation = $_[0];
	# parametre : type de dallage (image ou mtd)
	my $type_dallage = $_[1];
	# parametre : compression des images de la pyramide
	my $compression = $_[2];
	# parametre : annee de production des donnees source
	my $annee_dallage = $_[3];
	# parametre optionnel : departement des donnees source
	my $dep_dallage = $_[4];
	
	# le chemin vers le fichier commence par le chemin vers le repertoire du fichier
	my $nom_fichier_dallage = $rep_creation."/";
	
	# le prefixe du nom du fichier est different selon qu'il s'agit d'images ou de masques de mtd
	if ($type_dallage eq "image"){
		$nom_fichier_dallage .= $nom_fichier_dalle_source;
	}elsif($type_dallage eq "mtd"){
		$nom_fichier_dallage .= $nom_fichier_mtd_source;
	}else{
		# si on a affaire a un troisieme type, on sort
		print "[PREPARE_PYRAMIDE] Probleme de programmation : type $type_dallage incorrect.\n";
		exit;
	}
	
	# ajout du suffixe en fonction des caraceristiques
	$nom_fichier_dallage .= "_".$compression."_".$annee_dallage;
	if(defined $dep_dallage){
		$nom_fichier_dallage .= "_".$dep_dallage;
	}
	$nom_fichier_dallage .= ".txt";
	
	# on retourene le nom du fichier complet
	return $nom_fichier_dallage;
}
################################################################################
# lit un fichier de dallage pour en extraire la BBox de ses donnees
sub extrait_bbox_dallage{
	
	# parametre : chemin vers le ficheir de dallage a lire
	my $fichier_a_lire = $_[0];
	
	# coordonnees des 4 coins de la bbox du chantier
	my $xmin = 99999999999;
	my $xmax = 0;
	my $ymin = 99999999999;
	my $ymax = 0;
	
	# ouverture du fichier et stockage de son contenu dans un tableau
	open SOURCE, "<$fichier_a_lire" or die "[PREPARE_PYRAMIDE] Impossible d'ouvrir le fichier $fichier_a_lire.";
	my @lignes = <SOURCE>;
	close SOURCE;
	
	# boucle sur les lignes du fichier (entites du tableau)
	foreach my $ligne(@lignes){
		# suppression du saut de ligne final eventuel
		chomp($ligne);
		# decoupage de la ligne selon les tabulations et stackage dans un tableau
		my @infos_dalle = split /\t/, $ligne;
		# s'il n'y a pas au moins 7 valeurs, c'est que le fichier est mal formatte, on ne peut pas extraire les infos 
		if(! (defined $infos_dalle[0] && defined $infos_dalle[1] && defined $infos_dalle[2] && defined $infos_dalle[3] && defined $infos_dalle[4] && defined $infos_dalle[5] && defined $infos_dalle[6]) ){
			&ecrit_log("ERREUR de formatage du fichier $fichier_a_lire");
			print "[PREPARE_PYRAMIDE] Le fichier $fichier_a_lire est mal formatte, on ne peut extraire la BBox des dalles source.\n";
			exit;
		}
		
		# mise a jour de la BBox en fonction de la dalle etudiee
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
	
	# on retourne :
	# les coordonnees des 4 coins de la BBox des donnees du dallage
	return ($xmin, $xmax, $ymin, $ymax);
}
################################################################################
# initialise le traitement : verification des parametres et initialisation des variables globales
sub init{
	
	# le nom du log va comporter une info de date
	my $time = time();
	# nom du fichier de log
	my $log = $rep_log."/log_prepare_pyramide_$time.log";
	
	# creation du fichier de log
	open LOG, ">>$log" or die "[PREPARE_PYRAMIDE] Impossible de creer le fichier $log.";
	&ecrit_log("commande : @ARGV");
	
	# recuperation des parametres de la ligne de commande
	getopts("p:i:r:c:s:t:n:d:m:x:fa:y:w:h:l:");
	
	my $bool_getopt_ok = 1;
	# sortie si tous les parametres obligatoires ne sont pas presents
	if ( ! defined ($opt_p and $opt_i and $opt_r and $opt_c and $opt_s and $opt_t and $opt_n and $opt_x and $opt_l) ){
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
		if(! defined $opt_l){
			print "[PREPARE_PYRAMIDE] Veuillez specifier un parametre -l.\n";
		}
	}
	# si le parametre -f a ete specifie, il faut absolument les parametres a, y, w et h
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
	
	# on sort de la fonction si deja les parametres ne sont pas tous presents
	if ($bool_getopt_ok == 0){
		&usage();
		return $bool_getopt_ok;
	}
	
	# nom de la grande famille de produits
	$produit = $opt_p;
	# chemin vers le repertoire des images source ou chemin vers un fichier de dallage issu d'un calcul precedent
	$images_source = $opt_i;
	# chemin vers le repertoire des masques de mtd source ou chemin vers un fichier de dallage issu d'un calcul precedent
	if (defined $opt_m){
		$masque_mtd = $opt_m;
	}
	# repertoire ou creer la pyramide
	$rep_pyramide = $opt_r;
	# creation du repertoire de la pyramide
	if ( !(-e $rep_pyramide && -d $rep_pyramide) ){
		mkdir "$rep_pyramide", 0775 or die "[PREPARE_PYRAMIDE] Impossible de creer le repertoire $rep_pyramide.";
	}
	# type de compression des images de la pyramide
	$compression_pyramide = $opt_c;
	# systeme de coordonnees de la pyramide
	my $RIG = $opt_s;
	# formattage du SRS en majuscule
	$srs_pyramide = uc($RIG);
	# on remplace les : par des _ car cette string peut etre le nom d'un repertoire 
	$srs_pyramide =~ s/:/_/g;
	# repertoire des fichiers de dallage a creer
	$rep_fichiers_dallage = $opt_t;
	# annee de production (ou trimestre) des dalles source (ex : 2010 ou 2010-01)
	$annee = $opt_n;
	# departement des dalles source (optionnel)
	if (defined $opt_d){
		$departement = uc($opt_d);
	}
	# taille des dalles de la pyramide en pixels
	$taille_dalle_pix = $opt_x;
	# affectation des variables $resolution_source_x $resolution_source_y $taille_pix_source_x $taille_pix_source_y si la nomenclature IGN est utilisee
	if (defined $opt_f){
		$resolution_source_x = $opt_a;
		$resolution_source_y = $opt_y;
		$taille_pix_source_x = $opt_w;
		$taille_pix_source_y = $opt_h;
	}
	# chemin vers le fichier layer concernant la pyramide
	my $fichier_layer = $opt_l;
	
	# verifier les parametres
	# sortie si un des parametres est mal formate ou un des fichiers ou repertoires attendus est absent
	my $bool_param_ok = 1;
	if ($produit !~ /^(?:ortho|parcellaire|scan(?:25|50|100|dep|reg|1000)|franceraster|rgealti)$/i){
		print "[PREPARE_PYRAMIDE] Produit mal specifie.\n";
		&ecrit_log("ERREUR Produit mal specifie.");
		$bool_param_ok = 0;
	}else{
		$ss_produit = lc($produit);
		if($produit =~ /^scan(?:25|50|100|dep|reg|1000)$/i){
			$produit = "scan";
		}else{
			$produit = $ss_produit;
		}
	}
	# verification de la presence du fichier lay
	if (! (-e $fichier_layer && -f $fichier_layer) ){
		print "[PREPARE_PYRAMIDE] Le fichier $fichier_layer est introuvable.\n";
		$bool_param_ok = 0;
	}
	# extraction du precedent .pyr depuis le fichier de layer
	my $fichier_pyr_ancien = &cherche_pyramide_recente_lay($fichier_layer);
	if (! (-e $fichier_pyr_ancien && -f $fichier_pyr_ancien) ){
		print "[PREPARE_PYRAMIDE] Le fichier $fichier_pyr_ancien est introuvable.\n";
		$bool_param_ok = 0;
	}
	# extraction du nom du tms depuis le .pyr
	$fichier_tms = &extrait_tms_from_pyr($fichier_pyr_ancien);
	if (! (-e $fichier_tms && -f $fichier_tms) ){
		print "[PREPARE_PYRAMIDE] Le fichier $fichier_tms est introuvable.\n";
		$bool_param_ok = 0;
	}
	if (!(-e $images_source)){
		print "[PREPARE_PYRAMIDE] Le repertoire ou le fichier $images_source n'existe pas.\n";
		&ecrit_log("ERREUR Le repertoire ou le fichier $images_source n'existe pas.");
		$bool_param_ok = 0;
	}
	if (defined $masque_mtd && (!(-e $masque_mtd))){
		print "[PREPARE_PYRAMIDE] Le repertoire ou le fichier $masque_mtd n'existe pas.\n";
		&ecrit_log("ERREUR Le repertoire ou le fichier $masque_mtd n'existe pas.");
		$bool_param_ok = 0;
	}
	if (!(-e $rep_fichiers_dallage && -d $rep_fichiers_dallage)){
		print "[PREPARE_PYRAMIDE] Le repertoire $rep_fichiers_dallage n'existe pas.\n";
		&ecrit_log("ERREUR Le repertoire $rep_fichiers_dallage n'existe pas.");
		$bool_param_ok = 0;
	}
	if($compression_pyramide !~ /^raw|jpeg|png/floatraw$/i){
		print "[PREPARE_PYRAMIDE] Le parametre de compression $compression_pyramide est incorrect.\n";
		&ecrit_log("ERREUR Le parametre de compression $compression_pyramide est incorrect.");
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
	# verification de l'existence des fichiers annexes
	if (!(-e $xsd_pyramide && -f $xsd_pyramide) ){
		print "[PREPARE_PYRAMIDE] Le fichier $xsd_pyramide est introuvable.\n";
		&ecrit_log("ERREUR Le fichier $xsd_pyramide est introuvable.");
		$bool_param_ok = 0;
	}
	
	# on retourne :
	# un booleen : 1 pour OK , 0 sinon
	return $bool_param_ok;
}
################################################################################
# cree un fichier de dallage source et extrait la BBOX des donnees source
sub cree_fichier_dallage{
	
	# source du dallage : repertoire ou fichier
	my $source = $_[0];
	# repertoire destination du fichier de dallage
	my $repertoire_fichier_dallage = $_[1];
	# type des donnes (image ou mtd)
	my $type_donnees = $_[2];
	# BBOX des donnees avant etudes des sources
	my $x_min_bbox_ini = $_[3];
	my $x_max_bbox_ini = $_[4];
	my $y_min_bbox_ini = $_[5];
	my $y_max_bbox_ini = $_[6];
	
	# chemin vers le fichier de dallage
	my $nom_fichier_dallage = "";
	# BBOX des donnees
	my($x_min_bbox_dallage, $x_max_bbox_dallage, $y_min_bbox_dallage, $y_max_bbox_dallage);
	
	# initialisation de la BBOX
	if(defined $x_min_bbox_ini && defined $x_max_bbox_ini && defined $y_min_bbox_ini && defined $y_max_bbox_ini){
		($x_min_bbox_dallage, $x_max_bbox_dallage, $y_min_bbox_dallage, $y_max_bbox_dallage) = ($x_min_bbox_ini, $x_max_bbox_ini, $y_min_bbox_ini, $y_max_bbox_ini);
	}
	
	if(defined $source){
		# references vers des hash d'association entre chemin d'une source et son x_min, son x_max, son y_min, son y_max, sa resolution en x, sa resolution en y
		my ($reference_hash_x_min, $reference_hash_x_max, $reference_hash_y_min, $reference_hash_y_max, $reference_hash_res_x, $reference_hash_res_y);
		# si le parametre $source est un repertoire, on etudie les images
		if(-d $source){
			&ecrit_log("Recensement $type_donnees dans $source.");
			# constitution d'une reference vers un tableau contenant tous les chemins vers les source
			my ($ref_source, $nb_source) = &cherche_images($source);
			# on donne le nombre d'images source trouvees
			&ecrit_log("$nb_source $type_donnees dans $source.");
			&ecrit_log("Recensement infos $type_donnees de $source.");
			# les variables definies precedemment sont affectees en examinant chaque dalle
			($reference_hash_x_min, $reference_hash_x_max, $reference_hash_y_min, $reference_hash_y_max, $reference_hash_res_x, $reference_hash_res_y, $x_min_bbox_dallage, $x_max_bbox_dallage, $y_min_bbox_dallage, $y_max_bbox_dallage) = &cherche_infos_dalle($ref_source);
			&ecrit_log("Ecriture du fichier des dalles source.");
			# ecriture du fichier de dallage en fonction des caracteristiques des images stockees dans les variables precedentes
			$nom_fichier_dallage = &ecrit_dallage_source($ref_source, $reference_hash_x_min, $reference_hash_x_max, $reference_hash_y_min, $reference_hash_y_max, $reference_hash_res_x, $reference_hash_res_y, $repertoire_fichier_dallage, $type_donnees);
		
		}elsif(-f $source){
			# le parametre $source est un fichier : on utilise l'existant
			&ecrit_log("Utilisation du fichier des dalles source existant.");
			$nom_fichier_dallage = $repertoire_fichier_dallage."/".basename($source);
			# copie de l'ancien fichier de dallage dans l'emplacement du nouveau
			copy($source, $nom_fichier_dallage);
			# extraction du rectangle englobant les donness source du fichier de dallage si besoin
			if( !(defined $x_min_bbox_dallage && defined $x_max_bbox_dallage && defined $y_min_bbox_dallage && defined $y_max_bbox_dallage)){
				($x_min_bbox_dallage, $x_max_bbox_dallage, $y_min_bbox_dallage, $y_max_bbox_dallage) = &extrait_bbox_dallage($nom_fichier_dallage);
			}
			
		}
	}
	
	# on retourne :
	# nom du fichier de dallage cree
	# coins de la BBOX des donnees
	return ($nom_fichier_dallage, $x_min_bbox_dallage, $x_max_bbox_dallage, $y_min_bbox_dallage, $y_max_bbox_dallage);
}
################################################################################
# cree la pyramide (XML et repertoires)
sub cree_pyramide{
	# nom de la grande famille de produits
	my $produit_pyr = $_[0];
	# nom du produit
	my $ss_produit_pyr = $_[1];
	# type de compression des images de la pyramide
	my $compression_pyr = $_[2];
	# systeme de coordonnees de la pyramide
	my $srs_pyr = $_[3];
	# annee de production (ou trimestre) des dalles source (ex : 2010 ou 2010-01)
	my $annee_pyr = $_[4];
	# departement des dalles source
	my $departement_pyr = $_[5];
	# fichier du tms
	my $tms_pyr = $_[6];
	# taille des dalles de la pyramide en pixels
	my $taille_dalle_pix_pyr = $_[7];
	# type de mtd dans la pyramide
	my $type_mtd_pyramide = $_[8];
	# format des mtd dans la pyramide
	my $format_mtd_pyramide = $_[9];
	# profondeur de chemin des fichiers dans la pyramide
	my $profondeur_pyramide = $_[10];
	# coordonnees des coins du rectangle englobant les donnees source
	my $x_min_bbox_pyr = $_[11];
	my $x_max_bbox_pyr = $_[12];
	my $y_min_bbox_pyr = $_[13];
	my $y_max_bbox_pyr = $_[14];
	# chemin vers le schema XML qui contraint les fichiers XML de pyramide
	my $xsd_pyr = $_[15];
	
	# nom de la pyramide (et nom de son repertoire)
	my $nom_pyramide = &cree_nom_pyramide($ss_produit_pyr, $compression_pyr, $srs_pyr, $annee_pyr, $departement_pyr);
	
	# format des images de la pyramide
	my $format_images = $format_format_pyr{lc($compression_pyr)};
	# nombre de canaux des images de la pyramide
	my $nb_channels = $produit_nb_canaux{$produit_pyr};
	
	# creation du sous-repertoire de la pyramide
	if ( !(-e "$rep_pyramide/$nom_pyramide" && -d "$rep_pyramide/$nom_pyramide") ){
		mkdir "$rep_pyramide/$nom_pyramide", 0775 or die "[PREPARE_PYRAMIDE] Impossible de creer le repertoire $rep_pyramide/$nom_pyramide.";
	}
	
	# nom du fichier XML de pyramide
	my $nom_fichier_pyramide = $nom_pyramide.".pyr";
	
	&ecrit_log("Creation de $nom_fichier_pyramide.");
	# creation du fichier XML de pyramide et recuperation de la liste des repertoires de la pyramide sous forme de reference
	my ($ref_repertoires_a_creer, $nom_fichier_pyramide_final) = &cree_xml_pyramide($nom_fichier_pyramide, "$rep_pyramide/$nom_pyramide", $tms_pyr, $taille_dalle_pix_pyr, $format_images, $nb_channels, $type_mtd_pyramide, $format_mtd_pyramide, $profondeur_pyramide, $x_min_bbox_pyr, $x_max_bbox_pyr, $y_min_bbox_pyr, $y_max_bbox_pyr);
	
	# validation du .pyr par le xsd
	&ecrit_log("Validation de $nom_fichier_pyramide_final.");
	my ($valid, $string_temp_log) = &valide_xml($nom_fichier_pyramide_final, $xsd_pyr);
	if(defined $string_temp_log){
		&ecrit_log($string_temp_log);
		if($string_temp_log !~ /erreur/i){
			if ((!defined $valid) || $valid ne ""){
				my $string_valid = "Pas de message sur la validation";
				if(defined $valid){
					$string_valid = $valid;
				}
				# on sort le resultat de la validation
				print "[PREPARE_PYRAMIDE] Le document n'est pas valide!\n";
				print "$string_valid\n";
				&ecrit_log("ERREUR a la validation de $nom_fichier_pyramide_final par $xsd_pyr : $string_valid");
				exit;
			}
		}else{
			exit;
		}
	}
	
	# creer les sous-repertoires utiles
	&ecrit_log("Creation des repertoires des niveaux de la pyramide.");
	my @repertoires = @{$ref_repertoires_a_creer};
	foreach my $rep_a_creer(@repertoires){
		if ( !(-e $rep_a_creer && -d $rep_a_creer) ){
			mkdir $rep_a_creer, 0775 or die "[PREPARE_PYRAMIDE] Impossible de creer le repertoire $rep_a_creer.";
		}
	}
	&ecrit_log("Repertoires de la pyramide crees.");
	
	# on retourne :
	# chemin vers le fichier de pyramide
	return $nom_fichier_pyramide_final;
}
################################################################################
sub fin{
	
	&ecrit_log("Traitement termine.");
	# fermeture du handler du fichier de log
	close LOG;
	# sortie du programme
	exit;
}
