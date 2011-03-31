package cache;
use strict;
use Cwd 'abs_path';
use Term::ANSIColor;
use XML::Simple;
use XML::LibXML;
use Exporter;
our @ISA=('Exporter');
our @EXPORT=(
	'$type_mtd_pyr_param',
	'$format_mtd_pyr_param',
	'$profondeur_pyr_param',
	'$nom_fichier_dalle_source_param',
	'$nom_rep_images_param',
	'$nom_rep_mtd_param',
	'$base_param',
	'%base10_base_param',
	'$nom_fichier_mtd_source_param',
	'$color_no_data_param',
	'$dalle_no_data_rgb_param',
	'$dalle_no_data_gray_param',
	'$dalle_no_data_rgealti_param',
	'$programme_ss_ech_param',
	'cree_repertoires_recursifs',
	'$programme_format_pivot_param',
	'%produit_nb_canaux_param',
	'$xsd_pyramide_param',
	'lecture_tile_matrix_set',
	'$dalle_no_data_mtd_param',
	'$programme_dalles_base_param',
	'$programme_copie_image_param',
	'$rep_logs_param',
	'lecture_repertoires_pyramide',
	'%format_format_pyr_param',
	'$dilatation_reproj_param',
	'$programme_reproj_param',
	'reproj_point',
	'%produit_nomenclature_param',
	'$nom_fichier_first_jobs_param',
	'$nom_fichier_last_jobs_param',
	'cree_nom_pyramide',
	'cherche_pyramide_recente_lay',
	'extrait_tms_from_pyr',
	'$version_wms_param',
	'%produit_nb_bits_param',
	'%produit_couleur_param',
	'$string_erreur_batch_param',
	'$srs_wgs84g_param',
	'valide_xml',
	'$xsd_parametres_cache_param',
	'%produit_sample_format_param',
);
################################################################################

######### CONSTANTES

# type des masques de mtd
our $type_mtd_pyr_param = "INT32_DB_LZW";
# format des masques de mtd
our $format_mtd_pyr_param = "TIFF_LZW_INT8";

# profondeur du chemin au sens des specs du cache WMS du GPP3
our $profondeur_pyr_param = 2;

# nom du fichier de dallage (description des caracteristiques) des images source
our $nom_fichier_dalle_source_param = "dalles_source_image";
# nom du fichier de dallage des masques de mtd
our $nom_fichier_mtd_source_param = "dalles_source_metadata";

# nom du repertoire des images des pyramides
our $nom_rep_images_param = "IMAGE";
# nom du repertoire des masques de mtd des pyramides
our $nom_rep_mtd_param = "METADATA";

# base dans laquelle sont exprmiees les noms de dalles
our $base_param = 36;
# association entre denomination en base 10 et en base 36
our %base10_base_param = (
	0 => 0, 1 => 1, 2 => 2, 3 => 3, 4 => 4, 5 => 5,
	6 => 6, 7 => 7, 8 => 8, 9 => 9,	10 => "A",
	11 => "B", 12 => "C", 13 => "D", 14 => "E", 15 => "F",
	16 => "G", 17 => "H", 18 => "I", 19 => "J",	20 => "K",
	21 => "L", 22 => "M", 23 => "N", 24 => "O", 25 => "P",
	26 => "Q", 27 => "R", 28 => "S", 29 => "T", 30 => "U",
	31 => "V", 32 => "W", 33 => "X", 34 => "Y", 35 => "Z", 
);

# couleur de no_data pour les images en hexadecimal
our $color_no_data_param = "FFFFFF";

# chemin vers l'image no_data en RGB
our $dalle_no_data_rgb_param = "../share/pyramide/4096_4096_FFFFFF_rgb.tif";
# chemin vers l'image no_data en niveaux de gris
our $dalle_no_data_gray_param = "../share/pyramide/4096_4096_FFFFFF_gray.tif";
# chemin vers l'image no_data pour les masques de mtd
our $dalle_no_data_mtd_param = "../share/pyramide/mtd_4096_4096_black_32b.tif";
# chemin vers l'image no_data pour le rgealti
our $dalle_no_data_rgealti_param = "../share/pyramide/4096_4096_-99999_gray.tif";

# apres deploiement : le ./ est pour etre sur qu'on utilise les programmes compiles en local
# nom du programme permettant d'aggreger 4 images en divisant la reoslution par 2 (niveaux inferieurs de la pyramide)
our $programme_ss_ech_param = "merge4tiff";
# nom du programme permettant de convertir des images en format cache
our $programme_format_pivot_param = "tiff2tile";
# nom du programme permettant de creer les images du niveau le plus bas de la pyramide
our $programme_dalles_base_param = "dalles_base";
# nom du programme permettant de copier (et/ou transformer en termes de format, compression) des images
our $programme_copie_image_param = "tiffcp";

# association entre grande famille de produit et nombre de canaux des images
our %produit_nb_canaux_param = (
    # RGB
	"ortho" => 3,
	# niveaux de gris 
	"parcellaire" => 1,
	# RGB
	"franceraster" => 3,
	# RGB
	"scan" => 3,
	# alti
	"rgealti" => 1,
);

# nombre de bits par canal des images
our %produit_nb_bits_param = (
	"ortho" => 8,
	"parcellaire" => 8,
	"franceraster" => 8,
	"scan" => 8,
	"rgealti" => 32,
);

# association entre grande famille de produit et denomination de l'espace de couleur
our %produit_couleur_param = (
	"ortho" => "rgb",
	"parcellaire" => "gray",
	"franceraster" => "rgb",
	"scan" => "rgb",
	"rgealti" => "gray",
);

# chemin vers le schema contraignant les fichiers XML de pyramide
our $xsd_pyramide_param = "../config/pyramids/pyramid.xsd";

# chemin vers le schema contraignant les fichiers XML de parametrage de cache en entre de maj_cache.pl
our $xsd_parametres_cache_param = "../share/pyramide/parametres_cache.xsd";

# chemin vers le repertoire contenant les TMS
my $path_tms_param = "../config/tileMatrixSet";

# chemin vers le repertoire ou mettre les logs
our $rep_logs_param = "../log";

# association entre compression des images et denomination du type dans le .pyr
our %format_format_pyr_param = (
	"raw" => "TIFF_INT8",
	"jpeg" => "TIFF_JPG_INT8",
	"png" => "TIFF_PNG_INT8",
	"floatraw" => "TIFF_FLOAT32",
);

# nom du programme permettant de calculer des coordonnees reprojetees
our $programme_reproj_param = "cs2cs";
# nom de la variable d'environnement associee au programme precednt
my $env_reproj_param = "PROJ_LIB";
# chemin vers le repertoire necessaire a l'execution du programme precedent (valeur de la variable d'environnement)
my $repertoire_reproj_param = "../config/proj";

# association entre grande famille de produit et nomenclature standard des dalles a l'IGN sous forme d'experessiosn reguliere pour Perl
our %produit_nomenclature_param = (
	# 2 a 3 chiffres ou 2A ou 2B (departement) ou des lettres (bdortho agglo) suivi de - suivi de 4 chiffres (annee)
	# suivi de - suivi de 4 ou 5 chiffres (coordonnee X) suivi de - suivi de 4 ou 5 chiffres (coordonnee Y)
	"ortho" => "(?:\\d{2,3}|2[AB]|\\w+)-\\d{4}-(\\d{4,5})-(\\d{4,5})",
 	# BDP_ suivi de 2 chiffres (annee) suivi de _ suivi de 4 chiffres (coordonnee X) suivi de _ suivi de 4 chiffres (coordonnee Y)
	"parcellaire" => "BDP_\\d{2}_(\\d{4})_(\\d{4})",
 	# fr suivi de 2 lettres (om pour l'ombrage, st pour standard, pm pour premium) suivi de _ suivi de 4 chiffres (echelle)
 	# suivi de k_ suivi de _ suivi de 4 chiffres (coordonnee X) suivi de _ suivi de 4 chiffres (coordonnee Y)
	"franceraster" => "fr\\w{2}_\\d{4}k_(\\d{4})_(\\d{4})",
 	# SC suivi de 2 a 4 lettres (25, 50, 100, 1000, dep, reg) suivi d'eventuellement ( _ suivi de 3 a 4 lettres (pour le scan25 TOUR TOPO EDR) )
 	# suivi de _ suivi de 4 chiffres (coordonnee X) suivi de _ suivi de 4 chiffres (coordonnee Y)
	"scan" => "SC\\w{2,4}(?:_\\w{3,4})?_(\\d{4})_(\\d{4})",
);

# nom du fichier contenant la liste des scripts a executer en priorite
our $nom_fichier_first_jobs_param = "first_jobs.txt";
# nom du fichier contenant la liste des scripts a executer apres les premiers scripts
our $nom_fichier_last_jobs_param = "last_jobs.txt";

# version du WMS (a inserer dans les requetes WMS)
our $version_wms_param = "1.3.0";

# chaine de caracteres a inserer dans les scripts : indique si une erreur s'est produite (code de retour d'un programme != 0) et a quelle ligne
our $string_erreur_batch_param = "if [ \$? != 0 ] ; then echo \$0 : Erreur a la ligne \$(( \$LINENO - 1)) >&2 ; fi\n";

# systeme de coordonnees pour exprimer l'emprise des layers
# TODO eventuellement changer la def en fonction des specs
our $srs_wgs84g_param = "IGNF:WGS84G";

# association entre grande famille de produit et sample format : type entier ou flottant
our %produit_sample_format_param = (
	"ortho" => "uint",
	"parcellaire" => "uint",
	"franceraster" => "uint",
	"scan" => "uint",
	"rgealti" => "float",
);
################################################################################

########## FONCTIONS
# permet d'assigner la variable d'environnement du programme de reprojection si celle ci n'existe pas ou si elle est incorrecte
sub proj4_env_variable{
	# test si la variable d'environnement existe est est egale a ce qu'on attend
	if(!(exists $ENV{'$env_reproj_param'} && $ENV{'$env_reproj_param'} eq $repertoire_reproj_param)){
		$ENV{'$env_reproj_param'} = $repertoire_reproj_param;
	}
	
}
################################################################################
# permet de creer des repertoires parents en fonction d'un nom de sous-repertoire 
sub cree_repertoires_recursifs{
	
	my $nom_rep = $_[0];
	
	my $bool_ok = 0;
	
	# on coupe le repertoire selon les / (pour recuperer chaque nom de repertoire) et on stocke dans un tableau
	my @split_rep = split /\//, $nom_rep;
	
	# le premier est vide car on part de la racine
	# suppression du premier element
	shift @split_rep;
	
	my $rep_test = "";
	# boucle sur les repertoires parents
	foreach my $rep_parent(@split_rep){
		$rep_test .= "/".$rep_parent;
		# creation du repertoire s'il n'existe pas
		if( !(-e "$rep_test" && -d "$rep_test") ){
			mkdir "$rep_test", 0775 or die colored ("[CACHE] Impossible de creer le repertoire $rep_test.", 'white on_red');
		}
	}
	
	$bool_ok = 1;
	return $bool_ok;

}
################################################################################
# parse un fichier TMS et en ressort des informations
sub lecture_tile_matrix_set{
	
	# parametre : chemin vers le XML de TMS
	my $xml_tms = $_[0];
	
	# liste des identifiant des TM
	my @id;
	# association entre identifiant du TM et resolution
	my %id_resolution;
	# association entre identifiant du TM et taille des tuiles en pixels selon l'axe X
	my %id_taille_pix_tuile_x;
	# association entre identifiant du TM et taille des tuiles en pixels selon l'axe Y
	my %id_taille_pix_tuile_y;
	# association entre identifiant du TM et coordonnee X de l'origine du TM
	my %id_origine_x;
	# association entre identifiant du TM et coordonnee Y de l'origine du TM
	my %id_origine_y;
	
	my @refs_infos_levels;
	
	# stockage du XMl de TMS dans une variable : utilisation du module XML::Simple
	my $xml_fictif = new XML::Simple(KeyAttr=>[]);

	# lire le fichier XML
	my $data = $xml_fictif->XMLin("$xml_tms");
	
	# contenu de la balise <crs>
	my $systeme_reference = $data->{crs};
	
	# boucle sur le contenu des balises <tileMatrix>
	foreach my $tileMatrix (@{$data->{tileMatrix}}){
		# contenu de la balise <id>
		my $id = $tileMatrix->{id};
		push(@id, "$id");
		# contenu de la balise <resolution>
		$id_resolution{"$id"} = $tileMatrix->{resolution};
		# contenu de la balise <topLeftCornerX>
		$id_origine_x{"$id"} = $tileMatrix->{topLeftCornerX};
		# contenu de la balise <topLeftCornerY>
		$id_origine_y{"$id"} = $tileMatrix->{topLeftCornerY};
		# contenu de la balise <tileWidth>
		$id_taille_pix_tuile_x{"$id"} = $tileMatrix->{tileWidth};
		# contenu de la balise <tileHeight>
		$id_taille_pix_tuile_y{"$id"} = $tileMatrix->{tileHeight};
	}
	
	push(@refs_infos_levels, \@id, \%id_resolution, \%id_taille_pix_tuile_x, \%id_taille_pix_tuile_y, \%id_origine_x, \%id_origine_y, $systeme_reference);
	
	# on retourne :
	# reference vers le liste des id
	# reference vers le hash d'association id => resolution
	# reference vers le hash d'association id => taille des tuiles en pixels selon X
	# reference vers le hash d'association id => taille des tuiles en pixels selon Y
	# reference vers le hash d'association id => coordonnee X de l'origine du TM
	# reference vers le hash d'association id => coordonnee Y de l'origine du TM
	return @refs_infos_levels;
	
}
################################################################################
# parsage d'un fichier XML de pyramide pou en ressortir les repertoires de donnees
sub lecture_repertoires_pyramide{
	
	# parametre : chemin vers le XML de pyramide
	my $xml_pyramide = $_[0];
	
	# association entre id du TM et repertoire des images
	my %id_rep_images;
	# association entre id du TM et repertoire des masques de mtd
	my %id_rep_mtd;
	
	my @refs_rep_levels;
	
	# stockage du contenu du XML dans une variable (utilisation du module XML::Simple)
	my $xml_fictif = new XML::Simple(KeyAttr=>[]);

	# lire le fichier XML
	my $data = $xml_fictif->XMLin("$xml_pyramide");
	
	# boucle sur le contenu des balises <level>
	foreach my $level (@{$data->{level}}){
		# contenu de la balise <tileMatrix>
		my $id = $level->{tileMatrix};
		# contenu de la balise <baseDir>
		my $rep1 = $level->{baseDir};
		# on teste s'il s'agit deja d'un repertoire absolu (commencant par /)
		# oblige car abs_path ne marche pas toujours
		if (substr($rep1, 0, 1) eq "/" ){
			$id_rep_images{"$id"} = $rep1;
		}else{
			$id_rep_images{"$id"} = abs_path($rep1);
		}
		# contenu de la balise <metadata> (optionnel)
		my $metadata = $level->{metadata};
 		if (defined $metadata){
			# on teste s'il s'agit deja d'un repertoire absolu (commencant par /)
			# oblige car abs_path ne marche pas toujours
			my $rep2 = $metadata->{baseDir};
			if (substr($rep2, 0, 1) eq "/" ){
				$id_rep_mtd{"$id"} = $rep2;
			}else{
				$id_rep_mtd{"$id"} = abs_path($rep2);
			}
 		}
	}
	
	push(@refs_rep_levels, \%id_rep_images, \%id_rep_mtd);
	
	# on retourne :
	# reference vers le hash d'association id => repertoire images
	# reference vers le hash d'association id => repertoire masques mtd
	return @refs_rep_levels;
	
}
################################################################################
# calcule les coordonnees reperojetees d'un point
sub reproj_point{
	
	# parametre : coordonnee X
	my $x_point = $_[0];
	# parametre : coordonnee Y
	my $y_point = $_[1];
	# parametre : systeme de coordonnees initial
	my $srs_ini = $_[2];
	# parametre : systeme de coordonnees final
	my $srs_fin = $_[3];
	
	my $x_reproj;
	my $y_reproj;
	
	# pour etre sur que proj utilise le bon parametrage
	&proj4_env_variable;
	
	# lancement du programme de reprojection et stockage du resultat dans une variable
	my $result = `echo $x_point $y_point | $programme_reproj_param -f %.8f +init=$srs_ini +to +init=$srs_fin`;
	# on separe le resultat selon n'importe quel espace
	my @split2 = split /\s/, $result;
	# on regarde qu'on a bien deux coordonnees et qu'elles sont bien formees : un - optionnel suivi de plusieurs chiffres
	# (suivi eventuellement d'un . suivi de plusieurs chiffres)
	if(defined $split2[0] && defined $split2[1] && $split2[0] =~ /^\-?\d+(?:\.\d*)?$/ && $split2[1] =~ /^\-?\d+(?:\.\d*)?$/){
		$x_reproj = $split2[0];
		$y_reproj = $split2[1];
	}else{
		# gere par les autres programmes
		return ("erreur", "erreur");
	}
	
	# on retourne :
	# coordonnees X du point reprojete
	# coordonnees Y du point reprojete
	return ($x_reproj, $y_reproj);
}
################################################################################
# creation d'un nom de XML de pyramide
sub cree_nom_pyramide{
	
	# parametre : nom du produit
	my $produit = $_[0];
	# parametre : type de compression
	my $compression_pyramide = $_[1];
	# parametre : systeme de coordonnees
	my $srs_pyramide = $_[2];
	# parametre : annee
	my $annee = $_[3];
	# parametre : departement (optionnel)
	my $departement = $_[4];
	
	my $nom_pyramide = uc($produit)."_".uc($compression_pyramide)."_".uc($srs_pyramide)."_".$annee;
	if(defined $departement){
		$nom_pyramide .= "_".$departement;
	}
	
	# un nom formate pour le XML de pyramide
	return $nom_pyramide;
}
################################################################################
# parsage d'un XML de layer pour en resosortir le chemin vers le pyramide la plus recente
sub cherche_pyramide_recente_lay{
	
	# parametre : chemin vers le XML de layer
	my $xml_lay = $_[0];
	my $path_fichier_pyramide_recente = "";
	
	# stockage du contenu du XML dans une variable (utilisation du module XML::LibXML)
	my $parser_lay = XML::LibXML->new();
	my $doc_lay = $parser_lay->parse_file("$xml_lay");
	
	# contenu de la premiere balise <pyramidList><pyramid>
	my $nom_fichier_pyr = $doc_lay->find("//pyramidList/pyramid")->get_node(0)->textContent;
	# on prend le chemin absolu au cas ou
	$path_fichier_pyramide_recente = abs_path($nom_fichier_pyr);
	
	return $path_fichier_pyramide_recente;
}
################################################################################
# parsage d'un XML de pyramide pour en extraire le TMS
sub extrait_tms_from_pyr{
	
	# parametre : chemin vers le XML de pyramide
	my $fichier_pyr = $_[0];
	
	# on teste l'existence du repertoire ou sont censes se trouver tous les TMS
	if(!(-e $path_tms_param && -d $path_tms_param)){
		print colored ("[CACHE] Le repertoire $path_tms_param est introuvable.", 'white on_red');
		print "\n";
		exit;
	}
	
	# stockage du contenu du .pyr dans une variable (utilisation du module XML::LibXML)
	my $parser_pyr = XML::LibXML->new();
	my $doc_pyr = $parser_pyr->parse_file("$fichier_pyr");
	
	# contenu de la balise <tileMatrixSet>
	my $nom_tms = $doc_pyr->find("//tileMatrixSet")->get_node(0)->textContent;
	# ajout du chemin
	my $tms = $path_tms_param."/".$nom_tms.".tms";
	
	return $tms;
}
################################################################################
# validation d'un fichier XML par un schema
sub valide_xml{
	
	# parametre : chemin vers le fichier XML a valider
	my $document = $_[0];
	# parametre : chemin vers le fichier de schema XML validant
	my $schema = $_[1];
	
	# resultat de la validation
	my $reponse = '';
	
	# chaine de caractere a inscrire dans un log
	my $string_log = "";
	
	# booleen indiquant que les variables d'environnement existent
	my $bool_env_ok = 1;
	
	# on regarde si la variable d'environnement proxy_Host existe, sinon on ne peut pas valider => on sort
	my $proxy_Host = $ENV{'proxy_Host'};
	if (!defined $proxy_Host){
		$string_log .= "ERREUR : Variable d'environnement proxy_Host non trouvee.\n";
		$bool_env_ok = 0;
	}
	# on regarde si la variable d'environnement proxy_Port existe, sinon on ne peut pas valider => on sort
	my $proxy_Port = $ENV{'proxy_Port'};
	if (!defined $proxy_Port){
		$string_log .= "ERREUR : Variable d'environnement proxy_Port non trouvee.\n";
		$bool_env_ok = 0;
	}
	# on regarde si la variable d'environnement xerces_home existe, sinon on ne peut pas valider => on sort
	my $xerces_home = $ENV{'xerces_home'};
	if (!defined $xerces_home){
		$string_log .= "ERREUR : Variable d'environnement xerces_home non trouvee.\n";
		$bool_env_ok = 0;
	}
	
	# si les variables d'environnement sont OK
	if($bool_env_ok == 1){
		# creation de la ligne de commande qui valide un XML selon un schema
		my $commande_valide = "java -Dhttp.proxyHost=".$proxy_Host." -Dhttp.proxyPort=".$proxy_Port." -classpath ".$xerces_home."/xercesSamples.jar:".$xerces_home."/xml-apis.jar:".$xerces_home."/xercesImpl.jar:".$xerces_home."/resolver.jar:/usr/local/j2sdk/lib/tools.jar xni.XMLGrammarBuilder -F -a ".$schema." -i ".$document." 2>&1";
		$string_log .= "Validation du XML de pyramide : $commande_valide";
		
		# execution de la commande et recuperation du resultat dans une variable
		$reponse = `$commande_valide`;
	}else{
		print "[CACHE] $string_log\n";
	}
		
	# on retourne le resultat de la validation
	return ($reponse, $string_log);
	
}
1;
