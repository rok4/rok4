/*
 * Copyright © (2011-2013) Institut national de l'information
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

/**
 * \file ProcessFactory.cpp
 * \~french
 * \brief Implémentation de la classe ProcessFactory pour créer des processus
 * \~english
 * \brief Implement the ProcessFactory class, to create process
 */

#ifndef PROCESSFACTORY_H
#define PROCESSFACTORY_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <list>
#include <iostream>

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * Un ProcessFactory stocke les informations utiles pour créer un processus fils du processus lancé
 * Il contient des informations tel que le nombre maximum de processus que l'on peut créer et combien sont en cours
 * \brief Gestion du processus fils
 * \~english
 * The ProcessFactory stores information concerning process created by the main process.
 * \brief Manipulating child process
 */



class ProcessFactory
{

private:

    /**
     * \~french \brief PID du dernier process fils créé
     * \~english \brief Pid of the last child process created
     */
    pid_t lastPid;

    /**
     * \~french \brief Nombre max de process fils que l'on peut créer
     * \~english \brief Max number of child process
     */
    int nbMaxPid;

    /**
     * \~french \brief Nombre de process fils en cours d'execution
     * \~english \brief Number of current child process
     */
    int nbCurrentPid;

    /**
     * \~french \brief Temps d'execution laissé au processus
     * \~english \brief Execution time allowed for the process
     */
    int timeBeforeAutoKill;

    /**
     * \~french \brief Liste des pid de processus en cours
     * \~english \brief Pid list of current process
     */
    std::list<pid_t> listCurrentPid;

    /**
     * \~french \brief Fichier contenant la liste des pid de processus en cours
     * utilisé pour la persistance des données si un serveur tombe
     * \~english \brief File containing the pid list of current process
     * used for persistency of data if a server fails
     */
    std::string file;

    /**
     * \~french \brief Liste des pid de processus en cours, lancés par une instance ultérieur de Rok4
     * On les garde en mémoire si leur nombre dépasse nbMaxPid mais reste inférieur à MAX_NB_PROCESS
     * Et on vérifiera régulièrement s'ils sont terminés
     * \~english \brief Pid list of current process, set by a previous instance of Rok4
     * We save them if they are more than nbMaxPid but less than MAX_NB_PROCESS
     * And we will check if they are finished
     */
    std::list<pid_t> listPreviousPid;

public:
    /**
     * \~french
     * \brief Constructeur
     * \param[in] Nombre max de processus que l'on peut créer
     * \param[in] fichier dans lequel on écrire les processus en cours
     * On peut mettre un nom vide pour ne pas créer et maintenir de fichier
     * \~english
     * \brief Constructor
     * \param[in] Max number of process
     * \param[in] file used to write current pid
     * We can use a null name to avoid this file
     */
    ProcessFactory(int max, std::string f, int time=300);

    /**
     * \~french
     * \brief Destructeur
     * \~english
     * \brief Destructor
     */
    ~ProcessFactory();

    /**
     * \~french
     * \brief Modifie la valeur lastPid
     * \param[in] lastPid
     * \~english
     * \brief Set lastPid
     * \param[in] lastPid
     */
    void setLastPid(pid_t pid) {
        lastPid = pid;
    }

    /**
     * \~french
     * \brief Récupère la valeur lastPid
     * \return lastPid
     * \~english
     * \brief Get lastPid
     * \return lastPid
     */
    pid_t getLastPid() {
        return lastPid;
    }

    /**
     * \~french
     * \brief Modifie la valeur nbMaxPid
     * \param[in] nbMaxPid
     * \~english
     * \brief Set nbMaxPid
     * \param[in] nbMaxPid
     */
    void setnbMaxPid(int max) {
        nbMaxPid = max;
    }

    /**
     * \~french
     * \brief Récupère la valeur nbMaxPid
     * \return nbMaxPid
     * \~english
     * \brief Get nbMaxPid
     * \return nbMaxPid
     */
    int getnbMaxPid() {
        return nbMaxPid;
    }

    /**
     * \~french
     * \brief Modifie la valeur nbCurrentPid
     * \param[in] nbCurrentPid
     * \~english
     * \brief Set nbCurrentPid
     * \param[in] nbCurrentPid
     */
    void setnbCurrentPid(int cur) {
        nbCurrentPid = cur;
    }

    /**
     * \~french
     * \brief Récupère la valeur nbCurrentPid
     * \return nbCurrentPid
     * \~english
     * \brief Get nbCurrentPid
     * \return nbCurrentPid
     */
    int getnbCurrentPid() {
        return nbCurrentPid;
    }

    /**
     * \~french
     * \brief Modifie la liste listCurrentPid
     * \param[in] listCurrentPid
     * \~english
     * \brief Set listCurrentPid
     * \param[in] listCurrentPid
     */
    void setListCurrentPid(std::list<pid_t> list) {
        listCurrentPid = list;
    }

    /**
     * \~french
     * \brief Récupère la liste listCurrentPid
     * \return listCurrentPid
     * \~english
     * \brief Get listCurrentPid
     * \return listCurrentPid
     */
    std::list<pid_t> getListCurrentPid() {
        return listCurrentPid;
    }

    /**
     * \~french
     * \brief Modifie la valeur file
     * \param[in] file
     * \~english
     * \brief Set file
     * \param[in] file
     */
    void setFile(std::string f) {
        file = f;
    }

    /**
     * \~french
     * \brief Récupère la valeur file
     * \return file
     * \~english
     * \brief Get file
     * \return file
     */
    std::string getFile() {
        return file;
    }

    /**
     * \~french
     * \brief Modifie la valeur timeBeforeAutoKill
     * \param[in] timeBeforeAutoKill
     * \~english
     * \brief Set timeBeforeAutoKill
     * \param[in] timeBeforeAutoKill
     */
    void setTimeBeforeAutoKill(int f) {
        timeBeforeAutoKill = f;
    }

    /**
     * \~french
     * \brief Récupère la valeur timeBeforeAutoKill
     * \return timeBeforeAutoKill
     * \~english
     * \brief Get timeBeforeAutoKill
     * \return timeBeforeAutoKill
     */
    int getTimeBeforeAutoKill() {
        return timeBeforeAutoKill;
    }

    /**
     * \~french
     * \brief Modifie la liste listPreviousPid
     * \param[in] listPreviousPid
     * \~english
     * \brief Set listPreviousPid
     * \param[in] listPreviousPid
     */
    void setListPreviousPid(std::list<pid_t> list) {
        listPreviousPid = list;
    }

    /**
     * \~french
     * \brief Récupère la liste listPreviousPid
     * \return listPreviousPid
     * \~english
     * \brief Get listPreviousPid
     * \return listPreviousPid
     */
    std::list<pid_t> getListPreviousPid() {
        return listPreviousPid;
    }

    /**
     * \~french
     * \brief Crée un processus fils
     * \return true si ok et false sinon
     * \~english
     * \brief Create a child process
     * \return true if ok, false in the other case
     */
    bool createProcess();

    /**
     * \~french
     * \brief Vérifie si des processus fils sont terminé et met à jour la liste des processus courants
     * Il va également vérifier les processus issus de l'instance précédente et écrire
     * l'ensemble des processus encore courant dans un fichier de sauvegarde
     * \~english
     * \brief Check if all child process are finished and update current list of process
     * Check pid of previous instance et write all running process in a file
     */
    void checkAndSaveAllPid();

    /**
     * \~french
     * \brief Vérifie si des processus fils sont terminé et met à jour la liste des processus courants
     * \~english
     * \brief Check if all child process are finished and update current list of process
     */
    void checkCurrentPid();

    /**
     * \~french
     * \brief Vérifie si des processus fils ont été laissé par une ancienne instance
     *  si oui, s'ils sont terminés ou pas, on met à jour la liste des processus courants
     * \~english
     * \brief Check if all child process have been forgotten by previous instance of rok4
     * if yes, if there are finished or not, we update current list of process
     */
    void updatePreviousProcess();

    /**
     * \~french
     * \brief Ecris un fichier contenant les pid des processus courant et courants d'une
     * ancienne instance
     * \~english
     * \brief Write a file with the current pid and current previous pid
     */
    void writeFile();

    /**
     * \~french
     * \brief Initialise un logger pour ce processus
     * En effet, il est impossible de logger dans le logger du processus parent
     * Pour le moment, on ne log que en static file
     * \param[in] fichier utilisé pour logger
     * \~english
     * \brief Initialize a logger for this processus
     * It is impossible to log in the parent process
     * Log for a static file only
     * \param[in] file used to log
     */
    void initializeLogger(std::string file);

    /**
     * \~french
     * \brief Détruit le logger initialisé
     * \~english
     * \brief Destroy initialized logger
     */
    void destroyLogger();

    /**
     * \~french
     * \brief Tue tout les processus fils en cours
     * \~english
     * \brief Kill all current child process
     */
    void killAllPid();

    /**
     * \~french
     * \brief Fais dormir le processus qui l'appel un temps aléatoire inférieur à la seconde
     * \~english
     * \brief Sleepa processus less than one second
     */
    void randomSleep();
};

#endif // PROCESSFACTORY_H
