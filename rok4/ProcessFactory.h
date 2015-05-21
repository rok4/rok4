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
#include <vector>

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
     * \~french \brief Liste des pid de processus en cours
     * \~english \brief Pid list en current process
     */
    std::vector<pid_t> listCurrentPid;

public:
    /**
     * \~french
     * \brief Constructeur
     * \param[in] Nombre max de processus que l'on peut créer
     * \~english
     * \brief Constructor
     * \param[in] Max number of process
     */
    ProcessFactory(int max);

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
    void setListCurrentPid(std::vector<pid_t> list) {
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
    std::vector<pid_t> getListCurrentPid() {
        return listCurrentPid;
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
     * \~english
     * \brief Check if all child process are finished and update current list of process
     */
    void checkAllPid();

    /**
     * \~french
     * \brief Tue tout les processus fils en cours
     * \~english
     * \brief Kill all current child process
     */
    void killAllPid();
};

#endif // PROCESSFACTORY_H
