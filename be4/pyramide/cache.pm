package cache;
use strict;
use Cwd 'abs_path';
use Term::ANSIColor;
use XML::Simple;
use XML::LibXML;
use Exporter;
our @ISA=('Exporter');
our @EXPORT=(
# 	'%produit_format_param',
#	'$taille_dalle_pix_param',
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
	'$dalle_no_data_param',
	'%produit_res_utiles_param',
	'$programme_ss_ech_param',
	'cree_repertoires_recursifs',
	'$programme_format_pivot_param',
	'%produit_nb_canaux_param',
# 	'%produit_tms_param',
	'$xsd_pyramide_param',
# 	'$path_tms_param',
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
);
################################################################################

######### CONSTANTES

our %produit_format_param = (
	"ortho" => "TIFF_JPG_INT8",
 	"parcellaire" => "TIFF_PNG_INT8",
 	"franceraster" => "TIFF_INT8",
 	"scan" => "TIFF_INT8",
);

# en parametre des scripts
#our $taille_dalle_pix_param = 4096;

our $type_mtd_pyr_param = "INT32_DB_LZW";
our $format_mtd_pyr_param = "TIFF_LZW_INT8";
our $profondeur_pyr_param = 2;

our $nom_fichier_dalle_source_param = "dalles_source_image.txt";
our $nom_fichier_mtd_source_param = "dalles_source_metadata.txt";
our $nom_rep_images_param = "IMAGE";
our $nom_rep_mtd_param = "METADATA";

our $base_param = 36;
our %base10_base_param = (
0 => 0, 1 => 1, 2 => 2, 3 => 3, 4 => 4, 5 => 5,
	6 => 6, 7 => 7, 8 => 8, 9 => 9,	10 => "A",
	11 => "B", 12 => "C", 13 => "D", 14 => "E", 15 => "F",
	16 => "G", 17 => "H", 18 => "I", 19 => "J",	20 => "K",
	21 => "L", 22 => "M", 23 => "N", 24 => "O", 25 => "P",
	26 => "Q", 27 => "R", 28 => "S", 29 => "T", 30 => "U",
	31 => "V", 32 => "W", 33 => "X", 34 => "Y", 35 => "Z",
	      
);

our $color_no_data_param = "FFFFFF";

our $dalle_no_data_param = "../share/pyramide/4096_4096_FFFFFF.tif";
our $dalle_no_data_mtd_param = "../share/pyramide/mtd_4096_4096_black_32b.tif";

my @res_utiles_ortho = (0.25, 8388608);
my @res_utiles_parcel = (0.05, 8388608);
my @res_utiles_franceraster = (0.25, 8388608);
# TODO determiner resolutions utiles scans
my @res_utiles_scan25;
my @res_utiles_scan50;
my @res_utiles_scan100;
my @res_utiles_scandep;
my @res_utiles_scanreg;
my @res_utiles_scan1000;
our %produit_res_utiles_param = (
	"ortho" => \@res_utiles_ortho,
	"parcellaire" => \@res_utiles_parcel,
	"franceraster" => \@res_utiles_franceraster,
	"scan25" => \@res_utiles_scan25,
	"scan50" => \@res_utiles_scan50,
	"scan100" => \@res_utiles_scan100,
	"scandep" => \@res_utiles_scandep,
	"scanreg" => \@res_utiles_scanreg,
	"scan1000" => \@res_utiles_scan1000,
);

# apres deploiement : le ./ est pour etre sur qu'on utilise les programmes compiles en local
our $programme_ss_ech_param = "merge4tiff";
our $programme_format_pivot_param = "tiff2tile";
our $programme_dalles_base_param = "dalles_base";
our $programme_copie_image_param = "tiffcp";

our %produit_nb_canaux_param = (
    "ortho" => 3,
	"parcellaire" => 1,
	"franceraster" => 3,
	"scan" => 3,
);

# apres deploiement
our $xsd_pyramide_param = "../config/pyramids/pyramid.xsd";
my $path_tms_param = "../config/tileMatrixSet";
# my $tms_base = $path_tms_param."/FR_LAMB93.tms";
# 
# 
# our %produit_tms_param = (
# 	"ortho" => $tms_base,
# 	"parcellaire" => $tms_base,
# 	"franceraster" => $tms_base,
# 	"scan" => $tms_base,
# );

# pour deploiement
our $rep_logs_param = "../log";

our %format_format_pyr_param = (
	"raw" => "TIFF_INT8",
	"jpeg" => "TIFF_JPG_INT8",
	"png" => "TIFF_PNG_INT8",
);

our $programme_reproj_param = "cs2cs";

our %produit_nomenclature_param = (
	"ortho" => "(?:\\d{2,3}|2[AB]|\\w+)-\\d{4}-(\\d{4,5})-(\\d{4,5})",
 	"parcellaire" => "BDP_\\d{2}_(\\d{4})_(\\d{4})",
 	"franceraster" => "fr\\w{2}_\\d{4}k_(\\d{4})_(\\d{4})",
 	"scan" => "SC\\w{2,4}(?:_\\w{3,4})?_(\\d{4})_(\\d{4})",
);

our $nom_fichier_first_jobs_param = "../tmp/first_jobs.txt";
our $nom_fichier_last_jobs_param = "../tmp/last_jobs.txt";

################################################################################

########## FONCTIONS

sub cree_repertoires_recursifs{
	
	my $nom_rep = $_[0];
	
	my $bool_ok = 0;
	
	my @split_rep = split /\//, $nom_rep;
	
	# le premier est vide car on part de la racine
	shift @split_rep;
	
	my $rep_test = "";
	foreach my $rep_parent(@split_rep){
		$rep_test .= "/".$rep_parent;
		if( !(-e "$rep_test" && -d "$rep_test") ){
			mkdir "$rep_test", 0775 or die colored ("[CACHE] Impossible de creer le repertoire $rep_test.", 'white on_red');
		}
	}
	
	$bool_ok = 1;
	return $bool_ok;

}
################################################################################
sub lecture_tile_matrix_set{
	
	my $xml_tms = $_[0];
	
	my (@id, %id_resolution, %id_taille_pix_tuile_x, %id_taille_pix_tuile_y, %id_origine_x, %id_origine_y);
	
	my @refs_infos_levels;
	
	my $xml_fictif = new XML::Simple(KeyAttr=>[]);

	# lire le fichier XML
	my $data = $xml_fictif->XMLin("$xml_tms");
	
	my $systeme_reference = $data->{crs};
	
	foreach my $tileMatrix (@{$data->{tileMatrix}}){
		my $id = $tileMatrix->{id};
		push(@id, "$id");
		$id_resolution{"$id"} = $tileMatrix->{resolution};
		$id_origine_x{"$id"} = $tileMatrix->{topLeftCornerX};
		$id_origine_y{"$id"} = $tileMatrix->{topLeftCornerY};
		$id_taille_pix_tuile_x{"$id"} = $tileMatrix->{tileWidth};
		$id_taille_pix_tuile_y{"$id"} = $tileMatrix->{tileHeight};
	}
	
	push(@refs_infos_levels, \@id, \%id_resolution, \%id_taille_pix_tuile_x, \%id_taille_pix_tuile_y, \%id_origine_x, \%id_origine_y, $systeme_reference);
	
	return @refs_infos_levels;
	
}
################################################################################
sub lecture_repertoires_pyramide{
	
	my $xml_pyramide = $_[0];
	
	my (%id_rep_images, %id_rep_mtd);
	
	my @refs_rep_levels;
	
	my $xml_fictif = new XML::Simple(KeyAttr=>[]);

	# lire le fichier XML
	my $data = $xml_fictif->XMLin("$xml_pyramide");
	
	foreach my $level (@{$data->{level}}){
		my $id = $level->{tileMatrix};
		# oblige car abs_path ne marche pas toujours
		my $rep1 = $level->{baseDir};
		if (substr($rep1, 0, 1) eq "/" ){
			$id_rep_images{"$id"} = $rep1;
		}else{
			$id_rep_images{"$id"} = abs_path($rep1);
		}
		my $metadata = $level->{metadata};
 		if (defined $metadata){
			my $rep2 = $metadata->{baseDir};
			if (substr($rep2, 0, 1) eq "/" ){
				$id_rep_mtd{"$id"} = $rep2;
			}else{
				$id_rep_mtd{"$id"} = abs_path($rep2);
			}
 		}
	}
	
	push(@refs_rep_levels, \%id_rep_images, \%id_rep_mtd);
	
	return @refs_rep_levels;
	
}
################################################################################
sub reproj_point{

	my $x_point = $_[0];
	my $y_point = $_[1];
	my $srs_ini = $_[2];
	my $srs_fin = $_[3];
	
	my $x_reproj;
	my $y_reproj;
	
	my $result = `echo $x_point $y_point | $programme_reproj_param -f %.8f +init=$srs_ini +to +init=$srs_fin`;
	my @split2 = split /\s/, $result;
	if(defined $split2[0] && defined $split2[1] && $split2[0] =~ /^\-?\d+(?:\.\d*)?$/ && $split2[1] =~ /^\-?\d+(?:\.\d*)?$/){
		$x_reproj = $split2[0];
		$y_reproj = $split2[1];
	}else{
		# gere par les autres programmes
		return ("erreur", "erreur");
	}
	
	return ($x_reproj, $y_reproj);
}
################################################################################
sub cree_nom_pyramide{
	
	my $produit = $_[0];
	my $compression_pyramide = $_[1];
	my $srs_pyramide = $_[2];
	my $annee = $_[3];
	my $departement = $_[4];
	
	my $nom_pyramide = uc($produit)."_".uc($compression_pyramide)."_".uc($srs_pyramide)."_".$annee;
	if(defined $departement){
		$nom_pyramide .= "_".$departement;
	}
	
	return $nom_pyramide;
}
################################################################################
sub cherche_pyramide_recente_lay{
	
	my $xml_lay = $_[0];
	my $path_fichier_pyramide_recente = "";
	
	my $parser_lay = XML::LibXML->new();
	my $doc_lay = $parser_lay->parse_file("$xml_lay");
	
	my $nom_fichier_pyr = $doc_lay->find("//pyramidList/pyramid")->get_node(0)->textContent;
	$path_fichier_pyramide_recente = abs_path($nom_fichier_pyr);
	
	return $path_fichier_pyramide_recente;
}
################################################################################
sub extrait_tms_from_pyr{
	
	my $fichier_pyr = $_[0];
	
	if(!(-e $path_tms_param && -d $path_tms_param)){
		print colored ("[CACHE] Le repertoire $path_tms_param est introuvable.", 'white on_red');
		print "\n";
		exit;
	}
	
	my $parser_pyr = XML::LibXML->new();
	my $doc_pyr = $parser_pyr->parse_file("$fichier_pyr");
	
	my $nom_tms = $doc_pyr->find("//tileMatrixSet")->get_node(0)->textContent;
	#ajout du chemin
	my $tms = $path_tms_param."/".$nom_tms.".tms";
	
	return $tms;
}
1;
