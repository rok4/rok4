#ifndef DATA_SOURCE_H
#define DATA_SOURCE_H

#include <stdint.h>// pour uint8_t
#include <string>  // pour std::string
#include <cstddef> // pour size_t

/**
 * Interface abstraite permetant d'encapsuler une source de données.
 * La gestion mémoire des données est à la charge des classes d'implémentation.
 */
class DataSource {
public:

	/** Destructeur virtuel */
	virtual ~DataSource() {}

	/**
	 * Donne un accès direct mémoire en lecture aux données. Les données pointées sont en lecture seule.
	 *
	 * @return size Taille des données en octets.
	 * @return Pointeur vers les données qui ne doit pas être utilisé après destrucion ou libération des données.
	 */
	virtual const uint8_t* get_data(size_t &size) = 0;

	/**
	 * Libère les données mémoire allouées.
	 *
	 * Le pointeur obtenu par get_data() ne doit plus être utilisé après un appel à release_data().
	 * Le choix de libérer effectivement les données est laissé à l'implémentation, un nouvel appel
	 * à get_data() doit pouvoir être possible après libération même si ce n'est pas la logique voulue.
	 * Dans ce cas, la classe doit recharger en mémoire les données libérées.
	 *
	 * @return true en cas de succès.
	 */
	virtual bool release_data() = 0;

	/**
	 * Indique le type MIME associé à la donnée source.
	 */
	virtual std::string gettype() = 0;

	/**
         * Indique le statut Http associé à la donnée source.
         */
        virtual int getHttpStatus() = 0;
};



/**
 * Interface abstraite permetant d'encapsuler un flux de données.
 */
class DataStream {
public:

	/** Destructeur virtuel */
	virtual ~DataStream() {}

	/**
	 * Lit les prochaines données du flux. Tout octet ne peut être lu qu'une seule fois.
	 *
	 * Copie au plus size octets de données non lues dans buffer. La valeur de retour indique le nombre
	 * d'octets effectivement lus.
	 *
	 * Une valeur de retour 0 n'indique pas forcément la fin du flux, en effet il peut ne pas y avoir
	 * assez de place dans buffer pour écrire les données. Ce genre de limitation est spécifique à chaque
	 * classe filles qui peut pour des commodités d'implémentation ne pas vouloir tronquer certains blocs
	 * de données.
	 *
	 * @param buffer Pointeur cible.
	 * @param size Espace disponible dans buffer en octets.
	 * @return Nombre d'octets effectivement récupérés.
	 */
	virtual size_t read(uint8_t *buffer, size_t size) = 0;

	/**
	 * Indique la fin du flux. read() renverra systématiquement 0 lorsque la fin du flux est atteinte.
	 *
	 * @return true s'il n'y a plus de données à lire.
	 */
	virtual bool eof() = 0;

	/**
	 * Indique le type MIME associé au flux.
	 */
	virtual std::string gettype() = 0;

	/**
         * Indique le statut Http associé au flux.
         */
        virtual int getHttpStatus() = 0;
};



#endif
