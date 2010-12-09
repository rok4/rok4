#ifndef DATA_SOURCE_H
#define DATA_SOURCE_H

#include <stdint.h>// pour uint8_t
#include <cstddef> // pour size_t
#include <string>  // pour std::string

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
		 * @return size Taille des données en octets (0 en cas d'échec)
		 * @return Pointeur vers les données qui ne doit pas être utilisé après destrucion ou libération des données (0 en cas d'échec)
		 *
		 */
		virtual const uint8_t* getData(size_t &size) = 0;

		/**
		 * Libère les données mémoire allouées.
		 *
		 * Le pointeur obtenu par getData() ne doit plus être utilisé après un appel à releaseData().
		 * Le choix de libérer effectivement les données est laissé à l'implémentation, un nouvel appel
		 * à getData() doit pouvoir être possible après libération même si ce n'est pas la logique voulue.
		 * Dans ce cas, la classe doit recharger en mémoire les données libérées.
		 *
		 * @return true en cas de succès.
		 */
		virtual bool releaseData() = 0;

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



class DataSourceProxy : public DataSource {
	private:
		// Status pour déterminer quelle source de données il faut utiliser.
		// UNKNOWN (valeur initiale) la validité de dataSource n'est pas encore connue. 
		// DATA    dataSource est valide   => l'utiliser
		// NODATA  dataSource est invalide => utiliser noDataSource
		enum {UNKNOWN, DATA, NODATA} status;

		DataSource* dataSource;
		DataSource& noDataSource;

		inline DataSource& getDataSource() {
			switch(status) {
				case UNKNOWN:
					size_t size;
					if(dataSource && dataSource->getData(size)) {status = DATA; return *dataSource;}
					else {status = NODATA; return noDataSource;}
				case DATA:
					return *dataSource;
				case NODATA: 
					return noDataSource;
			}
		}

	public:


		DataSourceProxy	(DataSource* dataSource, DataSource& noDataSource) :
			status(UNKNOWN), dataSource(dataSource), noDataSource(noDataSource) {}

		~DataSourceProxy() {delete dataSource;}

		inline const uint8_t* getData(size_t &size) {return getDataSource().getData(size);}
		inline bool releaseData()                   {return getDataSource().releaseData();}
		inline std::string gettype()                {return getDataSource().gettype();}
		inline int getHttpStatus()                  {return getDataSource().getHttpStatus();}
};





/**
 * Classe transformant un DataStream en DataSource.
 */
class BufferedDataSource : public DataSource {
	private:
		std::string type;
		int httpStatus;
		size_t dataSize;
		uint8_t* data;

	public:
		/** 
		 * Constructeur. 
		 * Le paramètre dataStream est complètement lu. Il est donc inutilisable par la suite.
		 */
		BufferedDataSource(DataStream& dataStream);

		/** Destructeur **/
		~BufferedDataSource() {delete[] data;}

		/** Implémentation de l'interface DataSource **/
		const uint8_t* getData(size_t &size) {
			size = dataSize;
			return data;
		}

		/**
		 * Le buffer ne peut pas être libéré car on n'a pas de moyen de le reremplir pour un éventuel futur getData 
		 * @return false
		 */
		bool releaseData() {return false;}

		/** @return le type du dataStream */
		std::string gettype() {return type;}

		/** @return le status du dataStream */
		int getHttpStatus() {return httpStatus;}
};



/**
 * Classe Transformant un DataSource en DataStream
 */
// TODO class StreamedDataSource : public DataStream {};



#endif
