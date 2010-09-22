package cache;
use strict;
use Cwd 'abs_path';
use Term::ANSIColor;
use XML::Simple;
use Exporter;
our @ISA=('Exporter');
our @EXPORT=(
	'%produit_format_param',
	'$taille_dalle_pix_param',
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
	'$min_tile_param',
	'$max_tile_param',
	'%produit_nb_canaux_param',
	'%produit_tms_param',
	'$xsd_pyramide_param',
	'$path_tms_param',
	'lecture_tile_matrix_set',
	'$dalle_no_data_mtd_param',
	'$programme_dalles_base_param',
	'$programme_copie_image_param',
);
################################################################################

######### CONSTANTES

our %produit_format_param = (
	"ortho" => "TIFF_JPG_INT8",
	"parcellaire" => "TIFF_PNG_INT8",
	"franceraster" => "TIFF_INT8",
	"scan" => "TIFF_INT8",
);

our $taille_dalle_pix_param = 4096;
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
our $programme_ss_ech_param = "./merge4tiff";
our $programme_format_pivot_param = "./tiff2tile";
our $programme_dalles_base_param = "./dalles_base";
our $programme_copie_image_param = "./tiffcp";
#our $programme_ss_ech_param = "/exavol/private/only4diffusio/charlotte/pascal/merge4tiff";
#our $programme_format_pivot_param = "/exavol/private/only4diffusio/charlotte/pascal/tiff2tile";

# pour les .pyr
our $min_tile_param = 1;
our $max_tile_param = 1000000;

our %produit_nb_canaux_param = (
	"ortho" => 3,
	"parcellaire" => 1,
	"franceraster" => 3,
	"scan" => 3,
);

# apres deploiement
our $xsd_pyramide_param = "../config/pyramids/pyramid.xsd";
our $path_tms_param = "../config/tileMatrixSet";
my $tms_base = $path_tms_param."/FR_LAMB93_test.tms";


our %produit_tms_param = (
	"ortho" => $tms_base,
	"parcellaire" => $tms_base,
	"franceraster" => $tms_base,
	"scan" => $tms_base,
);



#our $xsd_pyramide_param = "/exavol/private/only4diffusio/charlotte/bin_cha/GPP3/pyramid.xsd";
#our $path_tms_param = "/exavol/private/only4diffusio/charlotte/bin_cha/GPP3";

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
			mkdir "$rep_test", 0775 or die colored ("[INITIALISE_PYRAMIDE] Impossible de creer le repertoire $rep_test.", 'white on_red');
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
	
	foreach my $tileMatrix (@{$data->{tileMatrix}}){
		my $id = $tileMatrix->{id};
		push(@id, "$id");
		$id_resolution{"$id"} = $tileMatrix->{resolution};
		$id_origine_x{"$id"} = $tileMatrix->{topLeftCornerX};
		$id_origine_y{"$id"} = $tileMatrix->{topLeftCornerY};
		$id_taille_pix_tuile_x{"$id"} = $tileMatrix->{tileWidth};
		$id_taille_pix_tuile_y{"$id"} = $tileMatrix->{tileHeight};
	}
	
	push(@refs_infos_levels, \@id, \%id_resolution, \%id_taille_pix_tuile_x, \%id_taille_pix_tuile_y, \%id_origine_x, \%id_origine_y);
	
	return @refs_infos_levels;
	
}
################################################################################
1;
