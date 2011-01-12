#!/usr/bin/perl -w
use strict;
use Term::ANSIColor;
use cache(
	'$nom_fichier_first_jobs_param',
	'$nom_fichier_last_jobs_param',
	'$nom_fichier_dalle_source_param',
	'$nom_fichier_mtd_source_param',
	'cree_nom_pyramide',
);

my $fichier_parametres = $ARGV[0];

my $repertoire_pyramide;
my $repertoire_fichiers_dallage;
my $fichier_layer;
my $prefixe_nom_script;
my $systeme_coordonnees_pyramide;
my $produit;
my $compression_images_pyramide;
my $annee;
my $departement;

### MAIN

&initialise_parametres($fichier_parametres);

print colored ("[ROLLBACK] Annulation...", 'white on_green');
print "\n";

# destruction des scripts first et last 
system("rm -f $nom_fichier_first_jobs_param");
system("rm -f $nom_fichier_last_jobs_param");

# destruction des scripts
system("rm -f $prefixe_nom_script"."*");

# destruction des fichiers de dallage
system("rm -f $repertoire_fichiers_dallage/$nom_fichier_dalle_source_param"."*");
system("rm -f $repertoire_fichiers_dallage/$nom_fichier_mtd_source_param"."*");

# remise a l'ancien LAY s'il existe
my $old_layer = $fichier_layer.".old";
if(-e $old_layer && -f $old_layer){
	rename($old_layer, $fichier_layer);
}

# destruction du rep de la pyramide
my $srs_pyramide = "IGNF_".uc($systeme_coordonnees_pyramide);
my $nom_pyramide = &cree_nom_pyramide($produit, $compression_images_pyramide, $srs_pyramide, $annee, $departement);
system("rm -rf $repertoire_pyramide/$nom_pyramide");

print colored ("OK.", 'white on_green');
print "\n";

# fin MAIN


sub initialise_parametres{
	my $xml_parametres = $_[0];
	
	my $bool_ok = 0;
	
	my $xml_fictif = new XML::Simple(KeyAttr=>[]);
	# lire le fichier XML
	my $data = $xml_fictif->XMLin("$xml_parametres");
	my $ss_produit = $data->{ss_produit};
	if ($ss_produit =~ /scan/){
		$produit = "scan";
	}else{
		$produit = $ss_produit;
	}

	$repertoire_pyramide = $data->{repertoire_pyramide};
	$repertoire_fichiers_dallage = $data->{repertoire_fichiers_dallage};
	$prefixe_nom_script = $data->{prefixe_nom_script};
	$systeme_coordonnees_pyramide = $data->{systeme_coordonnees_pyramide};
	$compression_images_pyramide = $data->{compression_images_pyramide};
	$fichier_layer = $data->{fichier_layer};
	$annee = $data->{annee};
	if (defined $data->{departement} ){
		$departement = $data->{departement};
	}
	
	$bool_ok = 1;
	
	return $bool_ok;
}