#!/usr/bin/perl -w 
use File::Copy;

my $rep = $ARGV[0];

# creation d'un rep temporaire pour les calculs intermediaires
my $rep_temp = "passe_2pivot_".time();
if (! (-e $rep_temp && -d $rep_temp)){
	mkdir "$rep_temp", 0775 or die colored ("[PASSE_2PIVOT] Impossible de creer le repertoire $rep_temp.", 'white on_red');
}
$| = 1;
my $nb = 0;

$nb += &passe_tiff2tile($rep);

print "$nb dalles transformees.\n";

sub passe_tiff2tile{

	my $repertoire = $_[0];
	
	print "[PASSE_2PIVOT] Repertoire $repertoire ...\n";
	
	my $nombre = 0;
	
	opendir REP, $repertoire or die "Impossible d'ouvrir le repertoire $repertoire.";
	my @fichiers = readdir REP;
	closedir REP;
	
	foreach my $fic(@fichiers){
		next if($fic =~ /^\.\.?$/);
		if ($fic =~ /\.tif/i){
			print ".";
			system("/exavol/private/only4diffusio/charlotte/pascal/tiff2tile $repertoire/$fic -c jpeg -t 256 256 $rep_temp/temp.tif");
			my $suppr = unlink("$repertoire/$fic");
			if($suppr != 1){
				print "ERREUR a la destruction de $repertoire/$fic.\n";
			}
			my $bool_sucsess = move("$rep_temp/temp.tif","$repertoire/$fic");
			if($bool_sucsess  == 0){
				print "ERREUR a au renommage $repertoire/$fic.\n";
			}
			$nombre += 1;
		}elsif(-d "$repertoire/$fic"){
			$nombre += &passe_tiff2tile("$repertoire/$fic");
		}
	}
	print "\n";
	return $nombre;
	
}
