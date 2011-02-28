#!/usr/bin/perl -w

use strict;
use Term::ANSIColor;
use XML::Simple;

my $fichier_parametres = $ARGV[0];

# programme de preparation des pyramides
my $programme_prepare_pyramide = "./prepare_pyramide.pl";
# programme d'initialisation des pyramides
my $programme_initialise_pyramide = "./initialise_pyramide.pl";
# programme de calcul des scripts
my $programme_calcule_pyramide = "./calcule_pyramide.pl";
# programme de mise a jour de la configuration du serveur
my $programme_maj_conf_serveur = "./maj_conf_serveur.pl";
# programme de mise en lecture seule de la pyramide produite
my $programme_pyramide_lecture_seule = "./pyramide_lecture_seule.pl";
# programme de retour en arriere a une etape donnee
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
# nom du produit ex : scan25
my $ss_produit;
# repertoire d'images source ou fichier de dallage issu d'une preparation de pyramide precedente
my $images_source;
# idem pour les mtd
my $masques_metadonnees;
# repertoire de creation de la pyramide
my $repertoire_pyramide;
# type de compression des images de la pyramide
my $compression_images_pyramide;
# srs de la pyramide
my $systeme_coordonnees_pyramide;
# repertoire pour les fichiers temporaires du traitement
my $repertoire_fichiers_temp;
# annee des donnees de la pyramide
my $annee;
# departement des donnees de la pyramide (optionnel)
my $departement;
# nom du layer de la pyramide
my $fichier_layer;
# srs des donnees source
my $systeme_coordonnees_source;
# pourcentage de dilataion a appliquer aux dalles cache pour tester l'intersection avec les dalles source
my $pcent_dilatation_dalles_base;
# pourcentage de dilatation a appliquer aux dalles source reprojetees pour tester l'intersection avec les dalles cache
my $pcent_dilatation_reproj;
# nom des scripts creees par calcule_pyramide : ex : /home/charlotte/test_pyramide/script_pyramide_test
# (ce nom sera suffixe avec un identifiant de job)
my $prefixe_nom_script;
# taille des dalles du cache en pixels
my $taille_dalles_pixels;
# nombre minimum de sous-scripts a creer pour paralleliser les calculs
my $nombre_batch_min;
# chemin de localisation vers le serveur ROK4 (poour creation des requetes WMS) : ex : machine.ign.fr/rok4/bin/rok4
my $localisation_rok4;
# nom du layer a requetter pour le calcul de pyramides avec pertes / reprojection
my $layer_requetes;

# booleen permettant l'utilisation des la nomenclature standard IGN des dalles raster
my $bool_nomenclature_IGN = 0;
# resolution X des dalles source (uniquement pour la nomenclature IGN)
my $resolution_x_source;
# resolution Y des dalles source (uniquement pour la nomenclature IGN)
my $resolution_y_source;
# taille des dalles source en pixels selon l'axe X (uniquement pour la nomenclature IGN)
my $taille_pix_x_source;
# taille des dalles source en pixels selon l'axe Y (uniquement pour la nomenclature IGN)
my $taille_pix_y_source;

# lecture du fichier de parametrage pour initialiser les variables
&initialise_parametres($fichier_parametres);

# resultat du prepare_pyramide.pl
# nom du fichier .pyr
my $fichier_pyramide;
# nom du fichier de dallage des images source
my $fichier_dalles_source;
# nom du fichier de dallage des mtd source (le cas echeant)
my $fichier_mtd_source;

print "[MAJ_CACHE] Execution de $programme_prepare_pyramide ...\n";
# string ajoutee a la ligne de commande en cas de traitment des masques de mtd
my $rep_mtd = "";
if (defined $masques_metadonnees){
	$rep_mtd = "-m $masques_metadonnees";
}
# string ajoutee a la ligne de commande en cas de calcul de pyramide par departement
my $dep = "";
if (defined $departement){
	$dep = "-d $departement";
}
# string ajoutee a la ligne de commande en cas d'utilisation de la nomenclature standard IGN pour les raster
my $param_IGN = "";
if($bool_nomenclature_IGN == 1){
	$param_IGN = "-f -a $resolution_x_source -y $resolution_y_source -w $taille_pix_x_source -h $taille_pix_y_source"
}
# creation de la ligne de commande de preparation de la pyramide
my $commande_prepare = "$programme_prepare_pyramide -p $ss_produit $param_IGN -i $images_source $rep_mtd -r $repertoire_pyramide -c $compression_images_pyramide -s $systeme_coordonnees_pyramide -t $repertoire_fichiers_temp -n $annee $dep -x $taille_dalles_pixels -l $fichier_layer";
# execution de la commande et recuperation des sorties dans un tableau
my @result_prepare = `$commande_prepare`; 
#etude des resultats
# booleen vrai en cas d'erreur du programme prepare_pyramide.pl
my $bool_erreur_prepare = 0;
foreach my $ligne_prepare(@result_prepare){
	# si on a eu des erreurs : le programme precede ses sorties de son nom entre crochets et/ou indique l'usage
	if($ligne_prepare =~ /\[prepare_pyramide\]|usage/i){
		$bool_erreur_prepare = 1;
		last;
	}
}
if($bool_erreur_prepare == 0){
	# recuperation des fichiers produits pour les passer en parametre au programme suivant
	# la premiere ligne contient le nom du fichier de pyramide .pyr
	chomp($fichier_pyramide = $result_prepare[0]);
	# la deuxieme ligne contient le nom du fichier de dallage des images source
	chomp($fichier_dalles_source = $result_prepare[1]);
	# la troisieme ligne contient le nom du fichier de dallage des masques de mtd source (s'il existe)
	if (defined $result_prepare[2] && $result_prepare[2] ne ""){
		chomp($fichier_mtd_source = $result_prepare[2]);
	}
}else{
	print colored ("[MAJ_CACHE] Des erreurs se sont produites a l'execution de la commande\n$commande_prepare", 'white on_red');
	print "\n\n";
	# eciture de la sortie standard du programme prepare_pyramide.pl
	print "@result_prepare\n";
	# appel au programme de ROLLBACK
	system("$programme_rollback $fichier_parametres");
	exit;
}

print "[MAJ_CACHE] Execution de $programme_initialise_pyramide ...\n";
# creation de la ligne de commande d'appel a l'initialisation de la pyramide (creation de liens vers la pyramide precedente)
my $commande_initialise = "$programme_initialise_pyramide -l $fichier_layer -p $fichier_pyramide";
# execution de la commande et recuperation des sorties dans un tableau
my @result_intialise = `$commande_initialise`;
#etude des resultats
# booleen vrai si des erreurs se sont produits dans le programme initialise_pyramide.pl
my $bool_erreur_initialise = 0;
foreach my $ligne_init(@result_intialise){
	# si on a eu des erreurs : le programme precede ses sorties de son nom entre crochets et/ou affiche l'usage
	if($ligne_init =~ /\[initialise_pyramide\]|usage/i){
		$bool_erreur_initialise = 1;
		last;
	}
}
if($bool_erreur_initialise == 1){
	print colored ("[MAJ_CACHE] Des erreurs se sont produites a l'execution de la commande\n$commande_initialise", 'white on_red');
	print "\n\n";
	# ecriture des sorties produites par initialise_pyramide.pl (en cas d'erreur)
	print "@result_intialise\n";
	# appel au programme de ROLLBACK
	system("$programme_rollback $fichier_parametres");
	exit;
}

print "[MAJ_CACHE] Execution de $programme_calcule_pyramide ...\n";
# string ajoutee a la ligne de commande an cas de traitement des masques de mtd
my $mtd_source = "";
if (defined $fichier_mtd_source){
	$mtd_source = "-m $fichier_mtd_source";
}
# string ajoutee a la ligne de commande an cas de calcul de pyramide avec perte et/ou reprojection impliquant des requetes WMS
my $param_requetes = "";
if(defined $localisation_rok4){
	$param_requetes .= "-k $localisation_rok4 ";
}
if(defined $layer_requetes){
	$param_requetes .= "-l $layer_requetes";
}
# creation de la ligne de commande de calcul des scripts
my $commande_calcule_batch = "$programme_calcule_pyramide -p $ss_produit -f $fichier_dalles_source $mtd_source -s $systeme_coordonnees_source -x $fichier_pyramide -d $pcent_dilatation_dalles_base -r $pcent_dilatation_reproj -n $prefixe_nom_script -t $taille_dalles_pixels -j $nombre_batch_min $param_requetes -m $repertoire_fichiers_temp";
# execution de la commande et recuperation des sorties dans un tableau
my @result_calcule = `$commande_calcule_batch`;
#etude des resultats
# booleeen vrai en cas d'erreur dans le script calcule_pyramide.pl
my $bool_erreur_calcule = 0;
foreach my $ligne_calc(@result_calcule){
	# si on a eu des erreurs le programme precede ses sorties de son nom entre crochets et/ou affiche l'usage
	if($ligne_calc =~ /\[calcule_pyramide\]|usage/i){
		$bool_erreur_calcule = 1;
		last;
	}
}
if($bool_erreur_calcule == 1){
	print colored ("[MAJ_CACHE] Des erreurs se sont produites a l'execution de la commande\n$commande_calcule_batch", 'white on_red');
	print "\n\n";
	# ecriture des sorties du programme calcule_pyramide.pl (en cas d'erreur)
	print "@result_calcule\n";
	# appel au programme de ROLLBACK
	system("$programme_rollback $fichier_parametres");
	exit;
}

# on a les batchs
# system("$programme_maj_conf_serveur -l $fichier_layer -p $fichier_pyramide");
# system("$programme_pyramide_lecture_seule -p $fichier_pyramide");

sub initialise_parametres{
	# parametre de la fonction : fichier XML en parametre du programme
	my $xml_parametres = $_[0];
	
	my $bool_ok = 0;
	
	# stockage du XML de parametrage dans une variable (utilisation du module XML::Simple)
	my $xml_fictif = new XML::Simple(KeyAttr=>[]);
	# lire le fichier XML
	my $data = $xml_fictif->XMLin("$xml_parametres");
	
	# contenu de la balise <ss_produit>
	$ss_produit = $data->{ss_produit};
	
	# contenu de la balise <images_source>
	$images_source = $data->{images_source};
	# la balise <masques_metadonnees> est optionnelle (car le traitement des mtd est optionnel)
	if (defined $data->{masques_metadonnees}){
		# contenu de la balise <masques_metadonnees>
		$masques_metadonnees = $data->{masques_metadonnees};
	}
	# contenu de la balise <repertoire_pyramide>
	$repertoire_pyramide = $data->{repertoire_pyramide};
	# contenu de la balise <compression_images_pyramide>
	$compression_images_pyramide = $data->{compression_images_pyramide};
	# contenu de la balise <systeme_coordonnees_pyramide>
	$systeme_coordonnees_pyramide = $data->{systeme_coordonnees_pyramide};
	# contenu de la balise <repertoire_fichiers_temporaires>
	$repertoire_fichiers_temp = $data->{repertoire_fichiers_temporaires};
	# contenu de la balise <annee>
	$annee = $data->{annee};
	# la balise <departement> est optionnelle (car les pyramides ne sont pas necessairement calculees par departement)
	if (defined $data->{departement} ){
		# contenu de la balise <departement>
		$departement = $data->{departement};
	}
	# on regarde si on va utiliser la nomenclature IGN = OUI ou NON dans le XML
	# contenu de la balise <nomenclature_IGN>
	my $nomenclature_IGN = $data->{nomenclature_IGN};
	# on attend uniquement un contenu du type "oui" ou "non"
	if($nomenclature_IGN !~ /^oui|non$/i){
		print colored ("[MAJ_CACHE] Balise nomenclature_IGN non conforme dans $xml_parametres.", 'white on_red');
		print "\n";
		exit;
	}else{
		if(lc($nomenclature_IGN) eq "oui"){
			# il faut que les 4 infos ci-dessous soient renseignees pour etre utilisees
			if (defined $data->{resolution_x_source} && defined $data->{resolution_y_source} && defined $data->{taille_pixels_x_source} && defined $data->{taille_pixels_y_source}){
				$bool_nomenclature_IGN = 1;
				# contenu de la balise <resolution_x_source>
				$resolution_x_source = $data->{resolution_x_source};
				# contenu de la balise <resolution_y_source>
				$resolution_y_source = $data->{resolution_y_source};
				# contenu de la balise <taille_pixels_x_source>
				$taille_pix_x_source = $data->{taille_pixels_x_source};
				# contenu de la balise <taille_pixels_y_source>
				$taille_pix_y_source = $data->{taille_pixels_y_source};
			}else{
				# sinon la preparation de la pyramide va echouer
				print colored ("[MAJ_CACHE] Les balises resolution_x_source, resolution_y_source, taille_pixels_x_source, taille_pixels_y_source doivent etre renseignees pour utiliser la nomenclature IGN.", 'white on_red');
				print "\n";
				exit;
			}
		}
	}
	
	# contenu de la balise <fichier_layer>
	$fichier_layer = $data->{fichier_layer};
	# contenu de la balise <systeme_coordonnees_source>
	$systeme_coordonnees_source = $data->{systeme_coordonnees_source};
	# contenu de la balise <pcent_dilatation_dalles_base>
	$pcent_dilatation_dalles_base = $data->{pcent_dilatation_dalles_base};
	# contenu de la balise <pcent_dilatation_reproj>
	$pcent_dilatation_reproj = $data->{pcent_dilatation_reproj};
	# contenu de la balise <prefixe_nom_script>
	$prefixe_nom_script = $data->{prefixe_nom_script};
	# contenu de la balise <taille_dalles_pixels>
	$taille_dalles_pixels = $data->{taille_dalles_pixels};
	# contenu de la balise <nombre_batch_min>
	$nombre_batch_min = $data->{nombre_batch_min};
	# la balise <localisation_rok4> est optionnelle (car les pyramides ne sont pas necessairement calculees a partir de pyramides existantes)
	if(defined $data->{localisation_rok4}){
		# contenu de la balise <localisation_rok4>
		$localisation_rok4 = $data->{localisation_rok4};
	}
	# la balise <layer_wms> est optionnelle (car les pyramides ne sont pas necessairement calculees a partir de pyramides existantes)
	if(defined $data->{layer_wms}){
		# contenu de la balise <layer_wms>
		$layer_requetes = $data->{layer_wms};
	}
	
	
	
	$bool_ok = 1;
	
	return $bool_ok;
}
