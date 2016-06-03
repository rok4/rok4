/*
 * Copyright © (2011) Institut national de l'information
 *                    géographique et forestière
 *
 * Géoportail SAV <geop_services@geoportail.fr>
 *
 * This software is a computer program whose purpose is to publish geographic
 * data using OGC WMS and WMTS protocol.
 *
 * This software is governed by the CeCILL-C license under French law and
 * abiding by the rules of distribution of free software.  You can  use,
 * modify and/ or redistribute the software under the terms of the CeCILL-C
 * license as circulated by CEA, CNRS and INRIA at the following URL
 * "http://www.cecill.info".
 *
 * As a counterpart to the access to the source code and  rights to copy,
 * modify and redistribute granted by the license, users are provided only
 * with a limited warranty  and the software's author,  the holder of the
 * economic rights,  and the successive licensors  have only  limited
 * liability.
 *
 * In this respect, the user's attention is drawn to the risks associated
 * with loading,  using,  modifying and/or developing or reproducing the
 * software by the user in light of its specific status of free software,
 * that may mean  that it is complicated to manipulate,  and  that  also
 * therefore means  that it is reserved for developers  and  experienced
 * professionals having in-depth computer knowledge. Users are therefore
 * encouraged to load and test the software's suitability as regards their
 * requirements in conditions enabling the security of their systems and/or
 * data to be ensured and,  more generally, to use and operate it in the
 * same conditions as regards security.
 *
 * The fact that you are presently reading this means that you have had
 *
 * knowledge of the CeCILL-C license and that you accept its terms.
 */

#include "Accumulator.h"
#include <time.h>
#include <errno.h>
#include "sys/time.h"


/**
 * Boucle principale d'écriture exécutée par un thread spcifique encapsulé dans la classe.
 * Cette boucle se charge de récuperrer des messages dans le buffer de message et de
 * les écrire dans le flux de sortie. Ainsi les éventuelles latences d'écriture de fichier
 * sont supportées par ce thread et non par les thread qui initient les écritures de log.
 *
 * @arg arg un pointeur vers l'objet Accumulator concerné
 */
void* Accumulator::loop ( void* arg ) {
    Accumulator* A = ( Accumulator* ) arg;

    while ( A->waitMessage() )
        A->flushFrontMessage();

    A->getStream().flush();
}

/**
 * Attend (et bloque) l'arrivée d'un nouveau message dans le buffer.
 * Cette fonction est exclusivement utilisée par le thread encapsulé.
 * Cette fonction retourne dès qu'un message est dispoible dans buffer ou lorsque l'objet rentre en phase de destruction
 */
bool Accumulator::waitMessage() {
    // On prépare une timeout à +1s
    timeval tv;
    timespec tsp;
    gettimeofday ( &tv, NULL );
    tsp.tv_sec  = tv.tv_sec + 1;
    tsp.tv_nsec = tv.tv_usec * 1000;

    // On attend avec un timeout de 1 seconde
    pthread_mutex_lock ( &mutex );
    if ( size == 0 && status > 0 && pthread_cond_timedwait ( &cond_get, &mutex, &tsp ) == ETIMEDOUT ) {
        pthread_mutex_unlock ( &mutex );
        getStream().flush(); // Flush hors mutex (car peut prendre du temps)
        pthread_mutex_lock ( &mutex );
    }

    // maintenant on attend qu'il y ait des messages sans timemout
    while ( size == 0 && status > 0 ) pthread_cond_wait ( &cond_get, &mutex );
    pthread_mutex_unlock ( &mutex );

    // Seul le thread interne peut décrémenter size, donc la contidtion size > 0 ne peut changer après la libération du mutex.
    return ( size > 0 );
}

/**
 * Retire le premier message du buffer et l'envoie dans flux de sortie.
 * Cette fonction est exclusivement utilisée par le thread encapsulé.
 * Cette fonction décrémente size et ne doit pas être apellée avec un buffer vide.
 */
void Accumulator::flushFrontMessage() {
    getStream() << buffer[front];
    //TODO: gérer les cas d'erreur d'écriture du flux.

    getStream().flush();

    // On retire le message du buffer en incrémentant front.
    pthread_mutex_lock ( &mutex );
    front = ( front+1 ) % buffer.size();
    size--;
    pthread_cond_signal ( &cond_add );
    pthread_mutex_unlock ( &mutex );
}


/**
 * Rentre dans l'état en cours de destruction et attend que le thread encapsulé s'arrêter proprement.
 * Cette fonction doit être apellée par le destructeur de la classe fille.
 * Question : pourquoi ne pas le faire dans le destructeur de Accumulator ?
 * Réponse : parceque lorsqu'on rentre dans le destructeur de Accumulator, la classe fille a déjà été détruite
 *           et l'on a plus accès à la fonction virtuelle getStream() écrire les messages en cours avant destruction.
   */
void Accumulator::stop() {
    pthread_mutex_lock ( &mutex );
    // On indique que l'objet est encours de destruction.
    status = 0;
    // On réveille le thread interne si celui-ci était en train de dormir
    pthread_cond_signal ( &cond_get );
    // Attendre la fin du thread interne
    pthread_mutex_unlock ( &mutex );
    pthread_join ( threadId, NULL );
}

/**
 * Ajoute un message dans la file d'attente des messages à écrire sur le flux de sortie.
 * Cette fonction peut bloquer lorsque le buffer est plein. Dans ce cas le thread apellant attend que le thread encapsulé libère de la place dans le buffer.
 * L'ordre d'enregistrement des messages concurents dépend de la politique de gestion des mutex (ça doit être du FIFO).
 *
 * @param message Le message à écrire, ne pas oublier de rajouter des retours à la ligne si l'on veut écrire des lignes
 * @return true si le message a bien été pris en compte false sinon.
 */
bool Accumulator::addMessage ( std::string message ) {
    // Ne pas accepter de nouveau message en cours de destruction.
    if ( status <= 0 ) return false;


    pthread_mutex_lock ( &mutex );
    // On attend qu'il y ait de la place dans le buffer
    while ( size == buffer.size() ) pthread_cond_wait ( &cond_add, &mutex );

    buffer[ ( front + size ) %buffer.size()] = message;
    size++;
    pthread_cond_signal ( &cond_get );
    pthread_mutex_unlock ( &mutex );
    return true;
}

/** Constructeur permettant de définir la capacité du buffer de messages. */
Accumulator::Accumulator ( int capacity ) : status ( 1 ), buffer ( capacity ), front ( 0 ), size ( 0 ) {
    pthread_mutex_init ( &mutex, 0 );
    pthread_cond_init ( &cond_get, 0 );
    pthread_cond_init ( &cond_add, 0 );

    // On crée et lance le thread interne
    pthread_create ( &threadId, NULL, Accumulator::loop, ( void* ) this );
}

/** Destructeur virtual car nous avons un classe abstraite */
Accumulator::~Accumulator() {

}

/** Destructeur de certains objets de la classe */
void Accumulator::destroy() {
    // Note : Le thread interne doit être arrêté par le destructeur de la classe fille en utilisant stop().
    pthread_cond_destroy ( &cond_get );
    pthread_cond_destroy ( &cond_add );
    pthread_mutex_destroy ( &mutex );
}

/** Implémentation de la fonction virtuelle de la classe mère */
void RollingFileAccumulator::close() {
    if ( out.is_open() ) out.close();
}

/** Implémentation de la fonction virtuelle de la classe mère */
std::ostream& RollingFileAccumulator::getStream() {

    time_t t = time ( 0 );

    if ( t >= validity ) { // On a dépassé la date de validité du fichier de sortie, il faut le changer.
        // On fabrique le nom du fichier de log de la forme prefixe-YYYY-MM-DD-HH.log
        time_t from = period* ( t/period );
        tm* lt = localtime ( &from );
        char fileName[filePrefix.length() + 19];
        sprintf ( fileName, "%s-%4d-%02d-%02d-%02d.log", filePrefix.c_str(), lt->tm_year+1900, lt->tm_mon+1, lt->tm_mday, lt->tm_hour );

        // On ferme le fichier précédant
        if ( out.is_open() ) out.close();

        // On ouvre le nouveau fichier
        out.open ( fileName, std::ios::app );

        // Et on alerte en cas d'erreur  : on ne peut pas utiliser Logger car c'est justement lui qui est en train de planter.
        if ( out.fail() ) std::cerr << "Impossible d'ouvrir le fichier de log: " << fileName << std::endl;
        else validity = from + period;
    }
    return out;
}

/** Implémentation de la fonction virtuelle de la classe mère */
void StaticFileAccumulator::close() {
    if ( out.is_open() ) out.close();
}

/** Implémentation de la fonction virtuelle de la classe mère */
std::ostream& StaticFileAccumulator::getStream() {

     if (! out.is_open() ) {
        // On ouvre le nouveau fichier
        char fileName[file.length() + 1];
        sprintf ( fileName, "%s", file.c_str());
        out.open ( fileName, std::ios::app );
        
        // Et on alerte en cas d'erreur  : on ne peut pas utiliser Logger car c'est justement lui qui est en train de planter.
        if ( out.fail() ) {
            std::cerr << "Impossible d'ouvrir le fichier de log: " << fileName << std::endl;
        }
    }
    return out;
}


