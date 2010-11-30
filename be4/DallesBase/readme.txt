// TODO : faire un schema explicatif

septembre 2010

---------------------------------------------------------------------------------------------
./target/bin/dalles_base -f test/fichiertest.txt -i lanczos -n 990099 -t img -s 3 -b 8 -p rgb
---------------------------------------------------------------------------------------------


Le but de ce programme est de construire l'image constituant le niveau zéro de la pyramide d'images
à partir d'une liste de nouvelles images (vector<Image*> ImageIn).

---

Pour cela, on va trier et regrouper les nouvelles images en paquets d'images (vector< vector<Image*> > TabImageIn ) de même résolution en x et en y.

Pour chaque paquet d'images constitué, on va fusionner les images du paquet en une unique ExtendedCompoundImage.
Puis, chaque ExtendedCompoundImage est resamplée en une ResampleImage (avec comme paramètres les résolutions demandées en sortie).

Enfin, les différentes ResampleImage obtenues sont fusionnées afin d'obtenir l'image finale (ExtendedCompoundImage* pECImage).



