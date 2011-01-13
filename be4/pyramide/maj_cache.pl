#!/usr/bin/perl -w

use strict;
use Term::ANSIColor;
use XML::Simple;

my $fichier_parametres = $ARGV[0];

my $programme_prepare_pyramide = "./prepare_pyramide.pl";
my $programme_initialise_pyramide = "./initialise_pyramide.pl";
my $programme_calcule_pyramide = "./calcule_pyramide.pl";
my $programme_maj_conf_serveur = "./maj_conf_serveur.pl";
my $programme_pyramide_lecture_seule = "./pyramide_lecture_seule.pl";
my $programme_rollback = "./rollback.pl";

# verification de la presence des perl
my $verif_programme_prepare_pyramide = `which $programme_prepare_pyramide`;
if ($verif_programme_prepare_pyramide eq ""){
	print colored ("[MAJ_CACHE] Le programme (preparation de pyramides) $programme_prepare_pyramide est introuvable.", 'white on_red');
	print "\n";
	exit;
}
my $verif_programme_initialise_pyramide = `which $programme_initialise_pyramide`;
if ($verif_programme_initialise_pyramide eq ""){
	print colored ("[MAJ_CACHE] Le programme (initialisation de pyramides) $programme_initialise_pyramide est introuvable.", 'white on_red');
	print "\n";
	exit;
}
my $verif_programme_calcule_pyramide = `which $programme_calcule_pyramide`;
if ($verif_programme_calcule_pyramide eq ""){
	print colored ("[MAJ_CACHE] Le programme (calcul de pyramides) $programme_calcule_pyramide est introuvable.", 'white on_red');
	print "\n";
	exit;
}
my $verif_programme_maj_conf_serveur = `which $programme_maj_conf_serveur`;
if ($verif_programme_maj_conf_serveur eq ""){
	print colored ("[MAJ_CACHE] Le programme (mise a jour de la configuration du serveur) $programme_maj_conf_serveur est introuvable.", 'white on_red');
	print "\n";
	exit;
}
my $verif_programme_pyramide_lecture_seule = `which $programme_pyramide_lecture_seule`;
if ($verif_programme_pyramide_lecture_seule eq ""){
	print colored ("[MAJ_CACHE] Le programme (mise en leture seule de l'arborescence de la pyramide) $programme_pyramide_lecture_seule est introuvable.", 'white on_red');
	print "\n";
	exit;
}
my $verif_programme_rollback = `which $programme_rollback`;
if ($verif_programme_rollback eq ""){
	print colored ("[MAJ_CACHE] Le programme (annulation des actions non validees) $programme_rollback est introuvable.", 'white on_red');
	print "\n";
	exit;
}

# parametres dans le fichier
my $ss_produit;
my $produit;
my $images_source;
my $masques_metadonnees;
my $repertoire_pyramide;
my $compression_images_pyramide;
my $systeme_coordonnees_pyramide;
my $repertoire_fichiers_dallage;
my $annee;
my $departement;
my $fichier_layer;
my $systeme_coordonnees_source;
my $pcent_dilatation_dalles_base;
my $pcent_dilatation_reproj;
my $prefixe_nom_script;
my $taille_dalles_pixels;
my $nombre_batch_min;
my $localisation_rok4;
my $layer_requetes;

my $bool_nomenclature_IGN = 0;
my $resolution_x_source;
my $resolution_y_source;
my $taille_pix_x_source;
my $taille_pix_y_source;

&initialise_parametres($fichier_parametres);

# resultat du prepare 
my $fichier_pyramide;
my $fichier_dalles_source;
my $fichier_mtd_source;

print "[MAJ_CACHE] Execution de $programme_prepare_pyramide ...\n";
my $rep_mtd = "";
if (defined $masques_metadonnees){
	$rep_mtd = "-m $masques_metadonnees";
}
my $dep = "";
if (defined $departement){
	$dep = "-d $departement";
}
my $param_IGN = "";
if($bool_nomenclature_IGN == 1){
	$param_IGN = "-f -a $resolution_x_source -y $resolution_y_source -w $taille_pix_x_source -h $taille_pix_y_source"
}
my $commande_prepare = "$programme_prepare_pyramide -p $produit $param_IGN -i $images_source $rep_mtd -r $repertoire_pyramide -c $compression_images_pyramide -s $systeme_coordonnees_pyramide -t $repertoire_fichiers_dallage -n $annee $dep -x $taille_dalles_pixels -l $fichier_layer";
my @result_prepare = `$commande_prepare`; 
#etude des resultats
my $bool_erreur_prepare = 0;
foreach my $ligne_prepare(@result_prepare){
	# si on a eu des erreurs
	if($ligne_prepare =~ /\[prepare_pyramide\]|usage/i){
		$bool_erreur_prepare = 1;
		last;
	}
}
if($bool_erreur_prepare == 0){
	chomp($fichier_pyramide = $result_prepare[0]);
	chomp($fichier_dalles_source = $result_prepare[1]);
	if (defined $result_prepare[2] && $result_prepare[2] ne ""){
		chomp($fichier_mtd_source = $result_prepare[2]);
	}
}else{
	print colored ("[MAJ_CACHE] Des erreurs se sont produites a l'execution de la commande\n$commande_prepare", 'white on_red');
	print "\n\n";
	print "@result_prepare\n";
	# ROLLBACK
	system("$programme_rollback $fichier_parametres");
	exit;
}

print "[MAJ_CACHE] Execution de $programme_initialise_pyramide ...\n";
my $commande_initialise = "$programme_initialise_pyramide -l $fichier_layer -p $fichier_pyramide";
my @result_intialise = `$commande_initialise`;
#etude des resultats
my $bool_erreur_initialise = 0;
foreach my $ligne_init(@result_intialise){
	# si on a eu des erreurs
	if($ligne_init =~ /\[initialise_pyramide\]|usage/i){
		$bool_erreur_initialise = 1;
		last;
	}
}
if($bool_erreur_initialise == 1){
	print colored ("[MAJ_CACHE] Des erreurs se sont produites a l'execution de la commande\n$commande_initialise", 'white on_red');
	print "\n\n";
	print "@result_intialise\n";
	# ROLLBACK
	system("$programme_rollback $fichier_parametres");
	exit;
}

print "[MAJ_CACHE] Execution de $programme_calcule_pyramide ...\n";
my $mtd_source = "";
if (defined $fichier_mtd_source){
	$mtd_source = "-m $fichier_mtd_source";
}
my $param_requetes = "";
if(defined $localisation_rok4){
	$param_requetes .= "-k $localisation_rok4 ";
}
if(defined $layer_requetes){
	$param_requetes .= "-l $layer_requetes";
}
my $commande_calcule_batch = "$programme_calcule_pyramide -p $ss_produit -f $fichier_dalles_source $mtd_source -s $systeme_coordonnees_source -x $fichier_pyramide -d $pcent_dilatation_dalles_base -r $pcent_dilatation_reproj -n $prefixe_nom_script -t $taille_dalles_pixels -j $nombre_batch_min $param_requetes";
my @result_calcule = `$commande_calcule_batch`;
#etude des resultats
my $bool_erreur_calcule = 0;
foreach my $ligne_calc(@result_calcule){
	# si on a eu des erreurs
	if($ligne_calc =~ /\[calcule_pyramide\]|usage/i){
		$bool_erreur_calcule = 1;
		last;
	}
}
if($bool_erreur_calcule == 1){
	print colored ("[MAJ_CACHE] Des erreurs se sont produites a l'execution de la commande\n$commande_calcule_batch", 'white on_red');
	print "\n\n";
	print "@result_calcule\n";
	# ROLLBACK
	system("$programme_rollback $fichier_parametres");
	exit;
}

# on a les batchs
# system("$programme_maj_conf_serveur -l $fichier_layer -p $fichier_pyramide");
# system("$programme_pyramide_lecture_seule -p $fichier_pyramide");

sub initialise_parametres{
	my $xml_parametres = $_[0];
	
	my $bool_ok = 0;
	
	my $xml_fictif = new XML::Simple(KeyAttr=>[]);
	# lire le fichier XML
	my $data = $xml_fictif->XMLin("$xml_parametres");
	
	$ss_produit = $data->{ss_produit};
	if ($ss_produit =~ /scan/){
		$produit = "scan";
	}else{
		$produit = $ss_produit;
	}
	
	$images_source = $data->{images_source};
	if (defined $data->{masques_metadonnees}){
		$masques_metadonnees = $data->{masques_metadonnees};
	}
	$repertoire_pyramide = $data->{repertoire_pyramide};
	$compression_images_pyramide = $data->{compression_images_pyramide};
	$systeme_coordonnees_pyramide = $data->{systeme_coordonnees_pyramide};
	$repertoire_fichiers_dallage = $data->{repertoire_fichiers_dallage};
	$annee = $data->{annee};
	if (defined $data->{departement} ){
		$departement = $data->{departement};
	}
	# on regarde si on va utiliser la nomenclature IGN = OUI ou NON dans le XML
	my $nomenclature_IGN = $data->{nomenclature_IGN};
	if($nomenclature_IGN !~ /^oui|non$/i){
		print colored ("[MAJ_CACHE] Balise nomenclature_IGN non conforme dans $xml_parametres.", 'white on_red');
		exit;
	}else{
		if(lc($nomenclature_IGN) eq "oui"){
			# il faut que les 4 infos ci-dessous soient renseignees pour etre utilisees
			if (defined $data->{resolution_x_source} && defined $data->{resolution_y_source} && defined $data->{taille_pixels_x_source} && defined $data->{taille_pixels_y_source}){
				$bool_nomenclature_IGN = 1;
				$resolution_x_source = $data->{resolution_x_source};
				$resolution_y_source = $data->{resolution_y_source};
				$taille_pix_x_source = $data->{taille_pixels_x_source};
				$taille_pix_y_source = $data->{taille_pixels_y_source};
			}
		}
	}
	
	$fichier_layer = $data->{fichier_layer};
	$systeme_coordonnees_source = $data->{systeme_coordonnees_source};
	$pcent_dilatation_dalles_base = $data->{pcent_dilatation_dalles_base};
	$pcent_dilatation_reproj = $data->{pcent_dilatation_reproj};
	$prefixe_nom_script = $data->{prefixe_nom_script};
	$taille_dalles_pixels = $data->{taille_dalles_pixels};
	$nombre_batch_min = $data->{nombre_batch_min};
	if(defined $data->{localisation_rok4}){
		$localisation_rok4 = $data->{localisation_rok4};
	}
	if(defined $data->{layer_wms}){
		$layer_requetes = $data->{layer_wms};
	}
	
	
	
	$bool_ok = 1;
	
	return $bool_ok;
}
