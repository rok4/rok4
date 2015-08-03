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

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <list>
#include <fcntl.h>
#include <sys/stat.h>
#include <iostream>
#include <fstream>

#include "config.h"
#include "ProcessFactory.h"

ProcessFactory::ProcessFactory(int max, std::string f)
{
    nbMaxPid = max;
    nbCurrentPid = 0;
    file = f;

    if (file != "") {
        //on réucpère les anciens processus
        updatePreviousProcess();

    }



}

ProcessFactory::~ProcessFactory()
{
    this->killAllPid();
}

bool ProcessFactory::createProcess() {

    bool processCreated = false;
    pid_t pid;

    if (nbCurrentPid < nbMaxPid) {

        pid = fork();

        if (pid != -1) {
            //a process has been created
            lastPid = pid;
            nbCurrentPid++;
            listCurrentPid.push_back(pid);
            processCreated = true;

        } else {
            //can't create a process for any reason
        }

    } else {
        //can't create child process because max is already taken
    }

    return processCreated;

}


void ProcessFactory::checkCurrentPid() {
    int status;
    std::list<pid_t> tmpList = listCurrentPid;

    for(std::list<int>::iterator it = tmpList.begin(); it != tmpList.end(); it++) {
        waitpid(*it,&status,WNOHANG);
        if (kill(*it,0) == -1) {
            //process doesn't run any more
            listCurrentPid.remove(*it);
            nbCurrentPid--;
        }
    }


}

void ProcessFactory::checkAndSaveAllPid() {
    int status;
    std::list<pid_t> tmpList = listCurrentPid;

    for(std::list<int>::iterator it = tmpList.begin(); it != tmpList.end(); it++) {
        waitpid(*it,&status,WNOHANG);
        if (kill(*it,0) == -1) {
            //process doesn't run any more
            listCurrentPid.remove(*it);
            nbCurrentPid--;
        }
    }

    if (file != "") {

        if (listPreviousPid.size() != 0) {
            tmpList = listPreviousPid;
            for(std::list<int>::iterator it = tmpList.begin(); it != tmpList.end(); it++) {
                waitpid(*it,&status,WNOHANG);
                if (kill(*it,0) == -1) {
                    //process doesn't run any more
                    listPreviousPid.remove(*it);
                }
            }
        }

        writeFile();
    }

}


void ProcessFactory::killAllPid() {
    int status;
    std::list<pid_t> tmpList = listCurrentPid;
    int rfile;

    for(std::list<int>::iterator it = tmpList.begin(); it != tmpList.end(); it++) {
        kill(*it,SIGKILL);
        waitpid(*it,&status,WNOHANG);
        //process doesn't run any more
        listCurrentPid.remove(*it);
        nbCurrentPid--;

    }

    if (file != "") {

        if (listPreviousPid.size() != 0) {
            tmpList = listPreviousPid;
            for(std::list<int>::iterator it = tmpList.begin(); it != tmpList.end(); it++) {
                waitpid(*it,&status,WNOHANG);
                if (kill(*it,0) == -1) {
                    //process doesn't run any more
                    listPreviousPid.remove(*it);
                }
            }
        }

        //delete file

        if (nbCurrentPid == 0 && listPreviousPid.size() == 0) {
            rfile = remove(file.c_str());
        } else {
            writeFile();
        }
    }

}

void ProcessFactory::updatePreviousProcess() {


    //on va voir si des precedents rok4 ont laissé des
    //processus zombies
    struct stat bufferF;
    int pFile;
    std::string line;
    pid_t pid;
    int status;
    int nbReadLine = 0;

    if (file != "") {
        if (stat (file.c_str(),&bufferF) == 0) {
            //le fichier existe donc on le lit
            std::ifstream ifile (file.c_str());
            if (ifile.is_open()) {
                //on lit
                while ( getline (ifile,line) && nbReadLine < MAX_NB_PROCESS) {

                    if (sscanf ( line.c_str(),"%d",&pid)) {
                        //on lit bien un entier donc un pid
                        waitpid(pid,&status,WNOHANG);
                        if (kill(pid,0) == -1) {
                            //process doesn't run any more
                        } else {
                            //process is still running
                            if (nbCurrentPid < nbMaxPid) {
                                nbCurrentPid++;
                                listCurrentPid.push_back(pid);
                                lastPid = pid;
                            } else {
                                listPreviousPid.push_back(pid);
                            }
                        }
                    }
                    nbReadLine++;
                }
                ifile.close();

                //on réécrit le fichier avec les pid encore actifs
                writeFile();

            } else {
                //impossible de lire le fichier
            }

        } else {
            //le fichier n'existe pas donc on le cree
            pFile = open(file.c_str(),O_EXCL|O_CREAT,S_IWRITE);
            close(pFile);
        }
    }

}


void ProcessFactory::writeFile() {

    if (file != "") {


        std::ofstream ofile;
        ofile.open (file.c_str(), std::ofstream::out | std::ofstream::trunc);
        if (ofile.is_open()) {
            for(std::list<int>::iterator it = listCurrentPid.begin(); it != listCurrentPid.end(); it++) {
                ofile << *it << std::endl;
            }
            for(std::list<int>::iterator it = listPreviousPid.begin(); it != listPreviousPid.end(); it++) {
                ofile << *it << std::endl;
            }
            ofile.close();
        } else {
            //impossible d'écrire dans le fichier
        }

    }
}

void ProcessFactory::initializeLogger(std::string file) {

    Accumulator *oldAcc = 0;
    for ( int i=0; i<=nbLogLevel; i++ ) {
        if (Logger::getAccumulator(( LogLevel ) i)) {
             oldAcc = Logger::getAccumulator(( LogLevel ) i);
             break;
        }
    }
    Accumulator *acc = new StaticFileAccumulator ( file );
    for ( int i=0; i<=nbLogLevel; i++ ) {
        if (Logger::getAccumulator(( LogLevel ) i)) {
             Logger::setCurrentAccumulator(( LogLevel ) i,acc);
        }
    }
    delete oldAcc;


}

void ProcessFactory::destroyLogger() {
    Accumulator *acc = 0;
    for ( int i=0; i<=nbLogLevel; i++ ) {
        if (Logger::getAccumulator(( LogLevel ) i)) {
             acc = Logger::getAccumulator(( LogLevel ) i);
             break;
        }
    }
    delete acc;
}
