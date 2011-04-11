#ifndef _ACCUMULATOR_
#define _ACCUMULATOR_

//#include <ostream>
#include <iostream>
#include <fstream>
#include <ctime>
//#include <cstdlib>
#include <vector>

/**
 * Collecte les messages de logs de plusieurs threads et les écrit dans un flux de sortie.
 * 
 * Chaque thread peut ajouter des messages à logger avec addMessage. 
 * Un unique thread indépendant encapsulé dans la classe écrit les messages sur un flux de sortie.
 * Une telle architecture permet aux thread apellant de ne pas être bloqués par des latences dues aux I/O.
 * Les accumulateurs seront eux même encapsulés dans des Loggers, plusieurs loggers peuvent utiliser un même accumulateur.
 */
class Accumulator {
  private:

  /**
   * Etat de la classe utilisé pour la destruction du thread encapsulé.
   * status  > 0 : Etat normale
   * status <= 0 : En cours de destruction, le thread d'écriture écrit les messages du buffer avant destruction effective, aucun nouveau message n'est accepté.
   */
  int status;

  /** Id du thread d'écriture spécifique */
  pthread_t threadId;

  /** mutex pour synchroniser les ajouts et retraits de message du buffer */
  pthread_mutex_t mutex; 

  /** Condition d'attente du thread d'écriture  */
  pthread_cond_t  cond_get;

  /** Condition d'attente des thread d'ajout de message */
  pthread_cond_t  cond_add;

  /** Position du plus ancien message présent dans le buffer */
  int front;

  /** Nombre de messages actuellement bufferisés */
  int size;

  /** Buffer de string contenant les messages */
	std::vector<std::string> buffer;

  /**
   * Boucle principale d'écriture exécutée par un thread spcifique encapsulé dans la classe.
   * Cette boucle se charge de récuperrer des messages dans le buffer de message et de 
   * les écrire dans le flux de sortie. Ainsi les éventuelles latences d'écriture de fichier 
   * sont supportées par ce thread et non par les thread qui initient les écritures de log.
   *
   * @arg arg un pointeur vers l'objet Accumulator concerné
   */
  static void* loop(void* arg);


  /**
   * Attend (et bloque) l'arrivée d'un nouveau message dans le buffer.
   * Cette fonction est exclusivement utilisée par le thread encapsulé.
   * Cette fonction retourne dès qu'un message est dispoible dans buffer ou lorsque l'objet rentre en phase de destruction
   */
  bool waitMessage();

  /**
   * Retire le premier message du buffer et l'envoie dans flux de sortie.
   * Cette fonction est exclusivement utilisée par le thread encapsulé.
   * Cette fonction décrémente size et ne doit pas être apellée avec un buffer vide.
   */
  void flushFrontMessage();

  
  /** Constructeur de copie privé pour éviter toute copie de l'objet */
  Accumulator(Accumulator&) {}

  protected:

  /**  
   * Renvoie le flux de sortie pour écrire des lignes de log.
   * Cette fonction sera apellée par le thread encapsulé et implémenté par différentes classe filles
   */
  virtual std::ostream& getStream() = 0;


  /**
   * Rentre dans l'état "en cours de destruction" et attend que le thread encapsulé s'arrêter proprement.
   * Cette fonction doit être apellée par le destructeur de la classe fille.
   * Question : pourquoi ne pas le faire dans le destructeur de Accumulator ?
   * Réponse : parceque lorsqu'on rentre dans le destructeur de Accumulator, la classe fille a déjà été détruite
   *           et l'on a plus accès à la fonction virtuelle getStream() pour écrire les messages en cours avant destruction.
   */
  void stop();

  public:

  /**
   * Ajoute un message dans la file d'attente des messages à écrire sur le flux de sortie.
   * Cette fonction peut bloquer lorsque le buffer est plein. Dans ce cas le thread apellant attend que le thread encapsulé libère de la place dans le buffer.
   * L'ordre d'enregistrement des messages concurents dépend de la politique de gestion des mutex (ça doit être du FIFO).
   * 
   * @param message Le message à écrire, ne pas oublier de rajouter des retours à la ligne si l'on veut écrire des lignes
   */
  bool addMessage(std::string message);

  /** Constructeur permettant de définir la capacité du buffer de messages. */
  Accumulator(int capacity);
  
  /** Destructeur virtual car nous avons un classe abstraite */
  virtual ~Accumulator();

};


/**
 * Implémentation basique d'un accumulateur basé sur un flux de sortie
 */
class StreamAccumulator : public Accumulator {
  private:
  /** Le flux de sortie */
  std::ostream& out;

  protected:
  /** Implémentation de la fonction virtuelle de la classe mère */
  virtual std::ostream& getStream() {return out;}

  public:

  /** Constructeur */
  StreamAccumulator(std::ostream &out = std::cerr, int capacity = 1024) : Accumulator(capacity), out(out) {}

  /** 
   * Destructeur apellant la fonction stop.
   * Le flux out n'est pas fermé, ceci est laissé du programmeur si nécessaire.
   * Raison : Il n'est pas toujours souhaitable de fermer le flux (par exemple std::cerr).
   */
  virtual ~StreamAccumulator() {stop();}
};


/**
 * Implémenation basique d'un accumulateur basé sur un flux de sortie
 */
class RollingFileAccumulator : public Accumulator {
  private:

  /** Le flux courrant de sortie */
  std::ofstream out;

  /** Date jusqu'à laquelle le flux courrant est valide*/ 
  time_t validity;

  /** Intervale de temps entre deux chagement de fichiers (en secondes) */ 
  time_t period;

  /** 
   * Préfixe des fichiers de log
   *
   * Les fichiers auront la forme prefixe-YYYY-MM-DD-HH.log
   *
   */
  std::string filePrefix;

  protected:
  /** Implémentation de la fonction virtuelle de la classe mère */
  virtual std::ostream& getStream();

  public:

  /** Constructeur */
  RollingFileAccumulator(std::string filePrefix, int period, int capacity = 1024) : Accumulator(capacity), filePrefix(filePrefix), validity(0), period(period) {}

  /** 
   * Destructeur apellant la fonction stop.
   * Le flux ou n'est pas fermé, ceci est laissé du programmeur si nécessaire.
   * Raison : Il n'est pas toujours souhaitable de fermer le flux (par exemple std::cerr).
   */
  virtual ~RollingFileAccumulator() {
    stop();
    out.close();
  }
};





#endif

