#!/usr/bin/perl -w 

use strict;
use Getopt::Std;
use Term::ANSIColor;
use Cwd 'abs_path';
use File::Basename;
use POSIX qw(ceil);
use cache(
# 	'%produit_format_param',
	'$taille_dalle_pix_param',
	'$type_mtd_pyr_param',
	'$format_mtd_pyr_param',
	'$profondeur_pyr_param',
	'$nom_fichier_dalle_source_param',
	'$nom_rep_images_param',
	'$nom_rep_mtd_param',
	'$nom_fichier_mtd_source_param',
	'$min_tile_param',
	'$max_tile_param',
	'%produit_nb_canaux_param',
	'%produit_tms_param',
	'$xsd_pyramide_param',
	'lecture_tile_matrix_set',
	'$rep_logs_param',
	'%format_format_pyr_param',
);
# CONSTANTES
$| = 1;
our($opt_p, $opt_i, $opt_r, $opt_c, $opt_s, $opt_t, $opt_n, $opt_d, $opt_m);

# my %produit_format = %produit_format_param;
my $nom_fichier_dalle_source = $nom_fichier_dalle_source_param;
my $nom_fichier_mtd_source = $nom_fichier_mtd_source_param;
my $nom_rep_images = $nom_rep_images_param;
my $nom_rep_mtd = $nom_rep_mtd_param;
my $taille_dalle_pix = $taille_dalle_pix_param;
my $type_mtd_pyr = $type_mtd_pyr_param;
my $format_mtd_pyr = $format_mtd_pyr_param;
my $profondeur_pyr = $profondeur_pyr_param;
my $min_tile = $min_tile_param;
my $max_tile = $max_tile_param;
my %produit_nb_canaux = %produit_nb_canaux_param;
my %produit_tms = %produit_tms_param;
my $xsd_pyramide = $xsd_pyramide_param;
my $rep_log = $rep_logs_param;
my %format_format_pyr = %format_format_pyr_param;
################################################################################

# verification de l'existence des fichiers annexes
if (!(-e $xsd_pyramide && -f $xsd_pyramide) ){
	print colored ("[PREPARE_PYRAMIDE] Le fichier $xsd_pyramide est introuvable.", 'white on_red');
	print "\n";
	exit;
}

##### MAIN
my $time = time();
my $log = $rep_log."/log_prepare_pyramide_$time.log";

open LOG, ">>$log" or die colored ("[PREPARE_PYRAMIDE] Impossible de creer le fichier $log.", 'white on_red');
&ecrit_log("commande : @ARGV");

getopts("p:i:r:c:s:t:n:d:m:");

if ( ! defined ($opt_p and $opt_i and $opt_r and $opt_c and $opt_s and $opt_t and $opt_n ) ){
	print colored ("[PREPARE_PYRAMIDE] Nombre d'arguments incorrect.", 'white on_red');
	print "\n\n";
	&ecrit_log("ERREUR Nombre d'arguments incorrect.");
	if(! defined $opt_p){
		print colored ("[PREPARE_PYRAMIDE] Veuillez specifier un parametre -p.", 'white on_red');
		print "\n";
	}
	if(! defined $opt_i){
		print colored ("[PREPARE_PYRAMIDE] Veuillez specifier un parametre -i.", 'white on_red');
		print "\n";
	}
	if(! defined $opt_r){
		print colored ("[PREPARE_PYRAMIDE] Veuillez specifier un parametre -r.", 'white on_red');
		print "\n";
	}
	if(! defined $opt_s){
		print colored ("[PREPARE_PYRAMIDE] Veuillez specifier un parametre -s.", 'white on_red');
		print "\n";
	}
	if(! defined $opt_c){
		print colored ("[PREPARE_PYRAMIDE] Veuillez specifier un parametre -c.", 'white on_red');
		print "\n";
	}
	if(! defined $opt_t){
		print colored ("[PREPARE_PYRAMIDE] Veuillez specifier un parametre -t.", 'white on_red');
		print "\n";
	}
	if(! defined $opt_n){
		print colored ("[PREPARE_PYRAMIDE] Veuillez specifier un parametre -n.", 'white on_red');
		print "\n";
	}
	&usage();
	exit;
}

print "[PREPARE_PYRAMIDE] Preparation au calcul de pyramide de cache Geoportail.\n";

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

# verifier les parametres
if ($produit !~ /^ortho_raw|ortho_jpeg|parcellaire|scan|franceraster$/i){
	print colored ("[PREPARE_PYRAMIDE] Produit mal specifie.", 'white on_red');
	print "\n";
	&ecrit_log("ERREUR Produit mal specifie.");
	exit;
}else{
	$produit = lc($produit);
	
}

my $fichier_tms = $produit_tms{$produit};
if (! (-e $fichier_tms && -f $fichier_tms) ){
	print colored ("[PREPARE_PYRAMIDE] Le fichier $fichier_tms est introuvable.", 'white on_red');
	print "\n";
	exit;
}

if (!(-e $rep_images_source && -d $rep_images_source)){
	print colored ("[PREPARE_PYRAMIDE] Le repertoire $rep_images_source n'existe pas.", 'white on_red');
	print "\n";
	&ecrit_log("ERREUR Le repertoire $rep_images_source n'existe pas.");
	exit;
}
if (defined $rep_masque_mtd && (!(-e $rep_masque_mtd && -d $rep_masque_mtd))){
	print colored ("[PREPARE_PYRAMIDE] Le repertoire $rep_masque_mtd n'existe pas.", 'white on_red');
	print "\n";
	&ecrit_log("ERREUR Le repertoire $rep_masque_mtd n'existe pas.");
	exit;
}
if (!(-e $rep_fichiers_dallage && -d $rep_fichiers_dallage)){
	print colored ("[PREPARE_PYRAMIDE] Le repertoire $rep_fichiers_dallage n'existe pas.", 'white on_red');
	print "\n";
	&ecrit_log("ERREUR Le repertoire $rep_fichiers_dallage n'existe pas.");
	exit;
}
if($compression_pyramide !~ /^raw|jpeg|png$/i){
	print colored ("[PREPARE_PYRAMIDE] Le parametre de compression $compression_pyramide est incorrect.", 'white on_red');
	print "\n";
	&ecrit_log("ERREUR Le parametre de compression $compression_pyramide est incorrect..");
	exit;
}
if(defined $departement && $departement !~ /^\d{2,3}|2[AB]$/i){
	print colored ("[PREPARE_PYRAMIDE] Departement mal specifie.", 'white on_red');
	print "\n";
	&ecrit_log("ERREUR Departement mal specifie : $departement.");
	exit;
}
if($annee !~ /^\d{4}(?:\-\d{2})?$/i){
	print colored ("[PREPARE_PYRAMIDE] Annee mal specifiee.", 'white on_red');
	print "\n";
	&ecrit_log("ERREUR Annee mal specifiee : $annee.");
	exit;
}

# action 1 : creer dalles_source_image et dalles_source_metadata
&ecrit_log("Recensement des images dans $rep_images_source.");
print "[PREPARE_PYRAMIDE] Recensement des images dans $rep_images_source.\n";
my ($ref_images_source, $nb_images) = &cherche_images($rep_images_source);
&ecrit_log("$nb_images images dans $rep_images_source.");
print "[PREPARE_PYRAMIDE] $nb_images images dans $rep_images_source.\n";
&ecrit_log("Recensement des infos des images de $rep_images_source.");
print "[PREPARE_PYRAMIDE] Recensement des infos des images de $rep_images_source.\n";
my ($reference_hash_x_min, $reference_hash_x_max, $reference_hash_y_min, $reference_hash_y_max, $reference_hash_res_x, $reference_hash_res_y, $x_min_bbox, $y_max_bbox) = &cherche_infos_dalle($ref_images_source);
&ecrit_log("Ecriture du fichier des dalles source.");
print "[PREPARE_PYRAMIDE] Ecriture du fichier des dalles source.\n";
&ecrit_dallage_source($ref_images_source, $reference_hash_x_min, $reference_hash_x_max, $reference_hash_y_min, $reference_hash_y_max, $reference_hash_res_x, $reference_hash_res_y, $rep_fichiers_dallage, "image");
# seulement si les mtd sont specifiees
if(defined $rep_masque_mtd){
	&ecrit_log("Recensement des mtd dans $rep_masque_mtd.");
	print "[PREPARE_PYRAMIDE] Recensement des mtd dans $rep_masque_mtd.\n";
	my ($ref_mtd_source, $nb_mtd) = &cherche_images($rep_masque_mtd);
	&ecrit_log("$nb_mtd mtd dans $rep_masque_mtd.");
	print "[PREPARE_PYRAMIDE] $nb_mtd mtd dans $rep_masque_mtd.\n";
	&ecrit_log("Recensement des infos des mtd de $rep_masque_mtd.");
	print "[PREPARE_PYRAMIDE] Recensement des infos des mtd de $rep_masque_mtd.\n";
	my ($mtd_hash_x_min, $mtd_hash_x_max, $mtd_hash_y_min, $mtd_hash_y_max, $mtd_hash_res_x, $mtd_hash_res_y, $non_utilise1, $non_utilise2) = &cherche_infos_dalle($ref_images_source);
	&ecrit_log("Ecriture du fichier des mtd source.");
	print "[PREPARE_PYRAMIDE] Ecriture du fichier des mtd source.\n";
	&ecrit_dallage_source($ref_mtd_source, $mtd_hash_x_min, $mtd_hash_x_max, $mtd_hash_y_min, $mtd_hash_y_max, $mtd_hash_res_x, $mtd_hash_res_y, $rep_fichiers_dallage, "mtd");
}


# action 2 : creer pyramid.pyr en XML
my $srs_pyramide = "IGNF_".uc($RIG);
my $nom_pyramide = uc($produit)."_".uc($srs_pyramide)."_".$annee;
if(defined $departement){
	$nom_pyramide .= "_".$departement;
}

my $format_images = $format_format_pyr{lc($compression_pyramide)};
my $nb_channels = $produit_nb_canaux{$produit};

# creation du repertoire de la pyramide
if ( !(-e $rep_pyramide && -d $rep_pyramide) ){
	mkdir "$rep_pyramide", 0775 or die colored ("[PREPARE_PYRAMIDE] Impossible de creer le repertoire $rep_pyramide.", 'white on red');
}
# creation du sous-repertoire de la pyramide
if ( !(-e "$rep_pyramide/$nom_pyramide" && -d "$rep_pyramide/$nom_pyramide") ){
	mkdir "$rep_pyramide/$nom_pyramide", 0775 or die colored ("[PREPARE_PYRAMIDE] Impossible de creer le repertoire $rep_pyramide/$nom_pyramide.", 'white on red');
}

my $nom_fichier_pyramide = $nom_pyramide.".pyr";
&ecrit_log("Creation de $nom_fichier_pyramide.");
print "[PREPARE_PYRAMIDE] Creation de $nom_fichier_pyramide.\n";
my ($ref_repertoires, $nom_fichier_final) = &cree_xml_pyramide($nom_fichier_pyramide, "$rep_pyramide/$nom_pyramide", $fichier_tms, $taille_dalle_pix, $format_images, $nb_channels, $type_mtd_pyr, $format_mtd_pyr, $profondeur_pyr);

# validation du .pyr par le xsd
&ecrit_log("Validation de $nom_fichier_pyramide.");
my $valid = &valide_xml($nom_fichier_final, $xsd_pyramide);
if ($valid ne ""){
	print colored ("[PREPARE_PYRAMIDE] Le document n'est pas valide!", 'white on_red');
	print "\n";
	&ecrit_log("ERREUR a la validation de $nom_fichier_final par $xsd_pyramide : $valid");
}

# action 3 : creer les sous-repertoires utiles
&ecrit_log("Creation des repertoires des niveaux de la pyramide.");
print "[PREPARE_PYRAMIDE] Creation des repertoires des niveaux de la pyramide.\n";
my @repertoires = @{$ref_repertoires};
foreach my $rep_a_creer(@repertoires){
	if ( !(-e $rep_a_creer && -d $rep_a_creer) ){
		mkdir $rep_a_creer, 0775 or die colored ("[PREPARE_PYRAMIDE] Impossible de creer le repertoire $rep_a_creer.", 'white on red');
	}
}
print colored ("[PREPARE_PYRAMIDE] Repertoires de la pyramide crees.\n", 'green');
&ecrit_log("Traitement termine.");
close LOG;
################################################################################

###### FONCTIONS

sub usage{
	
	my $bool_ok = 0;
	
	print colored ("\nUsage : \nprepare_pyramide.pl -p produit -i path/repertoire_images_source [-m path/repertoire_masques_metadonnees] -r path/repertoire_pyramide -c compression_images_pyramide -t path/repertoire_fichiers_dallage -s systeme_coordonnees_pyramide -n annee [-d departement]\n",'black on_white');
	print "\nproduit :\n";
 	print "\tortho_raw\northo_jpeg\n\tparcellaire\n\tscan\n\tfranceraster\n";
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
	
	
	opendir REP, $repertoire or die colored ("[PREPARE_PYRAMIDE] Impossible d'ouvrir le repertoire $repertoire.", 'white on_red');
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
	my $y_max_source = 0;
	
	my $bool_ok = 0;
	
	my $nb_dalles = @imgs;
	my $pourcent = 0;
	print "0";
	
	for(my $i = 0; $i < @imgs; $i++){
#	foreach my $image(@imgs){
		my $nombre_compare = ceil ($nb_dalles * ( ($pourcent + 10) / 100 ) );
		# toutes les 10 images 
		if( $i % 10  == 0){
			if ($i > $nombre_compare){
				$pourcent += 10;
				print "$pourcent";
			}else{
				print ".";
			}
		}
		
		
		# recuperation x_min x_max y_min y_max
		my @result = `.\/gdalinfo $imgs[$i]`;
		
		foreach my $resultat(@result){
			if($resultat =~ /Upper Left\s*\(\s*(\d+(?:\.\d+)?),\s*(\d+(?:\.\d+)?)\)/i){
				$hash_x_min{$imgs[$i]} = $1;
				$hash_y_max{$imgs[$i]} = $2;
				# actualisation des xmin et ymax du chantier
				if($hash_x_min{$imgs[$i]} < $x_min_source){
					$x_min_source = $hash_x_min{$imgs[$i]};
				}
				if($hash_y_max{$imgs[$i]} > $y_max_source){
					$y_max_source = $hash_y_max{$imgs[$i]}
				}
			}elsif($resultat =~ /Lower Right\s*\(\s*(\d+(?:\.\d+)?),\s*(\d+(?:\.\d+)?)\)/i){
				$hash_x_max{$imgs[$i]} = $1;
				$hash_y_min{$imgs[$i]} = $2;
			}elsif($resultat =~ /Pixel Size = \((\d+)\.(\d+), ?\-(\d+)\.(\d+)\)/){
				$hash_res_x{$imgs[$i]} = $1 + ( $2 / (10**length($2)));
				$hash_res_y{$imgs[$i]} = $3 + ( $4 / (10**length($4)));
			}
		}
		
	}
	print "100\n";
	my @refs = (\%hash_x_min, \%hash_x_max, \%hash_y_min, \%hash_y_max, \%hash_res_x, \%hash_res_y, $x_min_source, $y_max_source);
	
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
		print colored ("[PREPARE_PYRAMIDE] Probleme de programmation : type $type incorrect.", 'white on_red');
		print "\n";
		exit;
	}
	
	
	my $bool_ok = 0;
	
	open DALLAGE, ">$fichier_dallage_source" or die colored ("[PREPARE_PYRAMIDE] Impossible de creer le fichier $fichier_dallage_source.\n", 'white on_red');
	
	foreach my $image(@tableau_images){
		print DALLAGE "$image\t$dalle_x_min{$image}\t$dalle_y_max{$image}\t$dalle_x_max{$image}\t$dalle_y_min{$image}\t$dalle_res_x{$image}\t$dalle_res_y{$image}\n";
	}
	close DALLAGE;
	
	print colored ("[PREPARE_PYRAMIDE] Fichier $fichier_dallage_source ecrit.\n", 'green');
	
	$bool_ok = 1;
	
	return $bool_ok;
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
	
	my @liste_repertoires = ();
	
	my ($ref_id, $ref_id_resolution, $ref_id_taille_pix_tuile_x, $ref_id_taille_pix_tuile_y, $ref_inutile1, $ref_inutile2, $var_inutile1) = &lecture_tile_matrix_set($tms);
	my @id_tms = @{$ref_id};
	my %tms_level_resolution = %{$ref_id_resolution};
	my %tms_level_taille_pix_tuile_x = %{$ref_id_taille_pix_tuile_x};
	my %tms_level_taille_pix_tuile_y = %{$ref_id_taille_pix_tuile_y};
	
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
	
	open PYRAMIDE, ">$fichier_complet" or die colored ("[PREPARE_PYRAMIDE] Impossible de creer le fichier $fichier_complet.", 'white on_red');
	print PYRAMIDE "<?xml version='1.0' encoding='US-ASCII'?>\n";
	print PYRAMIDE "<Pyramid>\n";
	print PYRAMIDE "\t<tileMatrixSet>$nom_tms</tileMatrixSet>\n";
	foreach my $level(@id_tms){
		
		my $taille_image_m = $tms_level_resolution{"$level"} * $taille_dalle;
		my $nb_tuile_x = $taille_dalle / $tms_level_taille_pix_tuile_x{"$level"};
		my $nb_tuile_y = $taille_dalle / $tms_level_taille_pix_tuile_y{"$level"};
		
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
		print PYRAMIDE "\t\t<TMSLimits>\n";
		print PYRAMIDE "\t\t\t<minTileRow>$min_tile</minTileRow>\n";
		print PYRAMIDE "\t\t\t<maxTileRow>$max_tile</maxTileRow>\n";
		print PYRAMIDE "\t\t\t<minTileCol>$min_tile</minTileCol>\n";
		print PYRAMIDE "\t\t\t<maxTileCol>$max_tile</maxTileCol>\n";
		print PYRAMIDE "\t\t</TMSLimits>\n";
		print PYRAMIDE "\t</level>\n";
		push(@liste_repertoires, "$rep_images/$taille_image_m", "$rep_mtd/$taille_image_m");
	}
	
	print PYRAMIDE "</Pyramid>\n";
	close PYRAMIDE;
	
	print colored ("[PREPARE_PYRAMIDE] Fichier $fichier_complet ecrit.\n", 'green');
	
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
