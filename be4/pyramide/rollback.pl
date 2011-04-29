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

# parametre de la ligne de commande : XML de parametrage
my $fichier_parametres = $ARGV[0];

# chemin vers le repertoire de la pyramide 
my $repertoire_pyramide;
# chemin vers le repertoire des fichiers temporaires 
my $repertoire_fichiers_temp;
# chemin vers le fichier layer concerne par la pyramide
my $fichier_layer;
# prefixe des scripts de creation de la pyramide
my $prefixe_nom_script;
# SRS de la pyramide
my $systeme_coordonnees_pyramide;
# nom du produit
my $produit;
# type de compression des images de la pyramide
my $compression_images_pyramide;
# annee des donnees source
my $annee;
# departement des donnees source
my $departement;

### MAIN

# initialisation des variables par parsage du XML de parametrage
&initialise_parametres($fichier_parametres);

print colored ("[ROLLBACK] Annulation...", 'white on_green');
print "\n";

if( defined $repertoire_fichiers_temp && $repertoire_fichiers_temp ne ""){
	# destruction des scripts first et last
	if(defined $nom_fichier_first_jobs_param && $nom_fichier_first_jobs_param ne ""){
		system("rm -f $repertoire_fichiers_temp/$nom_fichier_first_jobs_param");
	}
	if(defined $nom_fichier_last_jobs_param && $nom_fichier_last_jobs_param ne ""){
		system("rm -f $repertoire_fichiers_temp/$nom_fichier_last_jobs_param");
	}
	# destruction des fichiers de dallage
	if(defined $nom_fichier_dalle_source_param && $nom_fichier_dalle_source_param ne ""){
		system("rm -f $repertoire_fichiers_temp/$nom_fichier_dalle_source_param"."*");
	}
	if(defined $repertoire_fichiers_temp && $repertoire_fichiers_temp ne ""){
		system("rm -f $repertoire_fichiers_temp/$nom_fichier_mtd_source_param"."*");
	}
}

if(defined $prefixe_nom_script && $prefixe_nom_script ne ""){
	# destruction des scripts
	system("rm -f $prefixe_nom_script"."*");
}

# remise a l'ancien LAY s'il existe pour faire comme si la pyramide dont il est question n'avait jamais existe
my $old_layer = $fichier_layer.".old";
if(-e $old_layer && -f $old_layer){
	rename($old_layer, $fichier_layer);
}

# destruction du repertoire de la pyramide
my $srs_pyramide = uc($systeme_coordonnees_pyramide);
my $nom_pyramide = &cree_nom_pyramide($produit, $compression_images_pyramide, $srs_pyramide, $annee, $departement);
if(defined $repertoire_pyramide && $repertoire_pyramide ne "" && defined $nom_pyramide && $nom_pyramide ne ""){
	system("rm -rf $repertoire_pyramide/$nom_pyramide");
}


print colored ("OK.", 'white on_green');
print "\n";

# fin MAIN

# focntion : parsage du XMl de paramtrage
sub initialise_parametres{
	
	# parametre : chemin vers le XML de parametrage
	my $xml_parametres = $_[0];
	
	my $bool_ok = 0;
	
	# stockage du contenu du XML dans une variable (utilisation du module XML::Simple)
	my $xml_fictif = new XML::Simple(KeyAttr=>[]);
	# lire le fichier XML
	my $data = $xml_fictif->XMLin("$xml_parametres");
	# contenu de la balise <ss_produit>
	my $ss_produit = $data->{ss_produit};
	# deduction de la grande famille de produit
	if ($ss_produit =~ /scan/){
		$produit = "scan";
	}else{
		$produit = $ss_produit;
	}
	
	# contenu de la balise <repertoire_pyramide>
	$repertoire_pyramide = $data->{repertoire_pyramide};
	# contenu de la balise <repertoire_fichiers_temporaires>
	$repertoire_fichiers_temp = $data->{repertoire_fichiers_temporaires};
	# contenu de la balise <prefixe_nom_script>
	$prefixe_nom_script = $data->{prefixe_nom_script};
	# contenu de la balise <systeme_coordonnees_pyramide>
	$systeme_coordonnees_pyramide = $data->{systeme_coordonnees_pyramide};
	# contenu de la balise <compression_images_pyramide>
	$compression_images_pyramide = $data->{compression_images_pyramide};
	# contenu de la balise <fichier_layer>
	$fichier_layer = $data->{fichier_layer};
	# contenu de la balise <annee>
	$annee = $data->{annee};
	# contenu de la balise <departement> (optionnelle)
	if (defined $data->{departement} ){
		$departement = $data->{departement};
	}
	
	$bool_ok = 1;
	
	return $bool_ok;
}
