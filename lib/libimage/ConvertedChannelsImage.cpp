#include "ConvertedChannelsImage.h"

ConvertedChannelsImage::ConvertedChannelsImage(int channels, Image *sourceImage):Image(sourceImage->getWidth(),
                                                                                       sourceImage->getHeight(),
                                                                                       channels,
                                                                                       sourceImage->getResX(),
                                                                                       sourceImage->getResY(),
                                                                                       sourceImage->getBbox()),
    sourceImage (sourceImage), channelsOut (channels), channelsIn (sourceImage->getChannels()), alphaOut (255)
{

}

ConvertedChannelsImage::ConvertedChannelsImage(int channels, Image *sourceImage, int alpha):Image(sourceImage->getWidth(),
                                                                                       sourceImage->getHeight(),
                                                                                       channels,
                                                                                       sourceImage->getResX(),
                                                                                       sourceImage->getResY(),
                                                                                       sourceImage->getBbox()),
    sourceImage (sourceImage), channelsOut (channels), channelsIn (sourceImage->getChannels()), alphaOut (alpha)
{

}

ConvertedChannelsImage::~ConvertedChannelsImage()
{
    delete sourceImage;
}

int ConvertedChannelsImage::getline ( uint8_t *buffer, int line ) {

    //on considère que channelsIn et channelsOut sont différents
    //Sinon cette classe n'a aucun intérêt
    uint8_t *bufferIn = new uint8_t[channelsIn*width];

    //on récupère les données sources
    int sizeIn = sourceImage->getline(bufferIn,line);
    if (sizeIn <= 0) {
        return -1;
    }

    if (channelsIn == 1) {
        // 1 canal en entrée
        if (channelsOut == 2) {
            // 2 canaux en sortie
            //on rajoute simplement un canal alpha

            int k = 0;

            for (int i = 0; i < sizeIn; i++) {
                buffer[k] = bufferIn[i];
                buffer[k+1] = alphaOut;
                k = k+2;
            }

            //on nettoie
            delete [] bufferIn;
            return channelsOut*width;

        }

        if (channelsOut == 3) {
            // 3 canaux en sortie
            // on copie le premier dans les deux autres

            int k = 0;

            for (int i = 0; i < sizeIn; i++) {
                buffer[k] = bufferIn[i];
                buffer[k+1] = bufferIn[i];
                buffer[k+2] = bufferIn[i];
                k = k+3;
            }

            //on nettoie
            delete [] bufferIn;
            return channelsOut*width;

        }

        if (channelsOut == 4) {
            // 4 canaux en sortie
            // on copie le premier dans les deux autres et on ajoute un canal alpha

            int k = 0;

            for (int i = 0; i < sizeIn; i++) {
                buffer[k] = bufferIn[i];
                buffer[k+1] = bufferIn[i];
                buffer[k+2] = bufferIn[i];
                buffer[k+3] = alphaOut;
                k = k+4;
            }

            //on nettoie
            delete [] bufferIn;
            return channelsOut*width;

        }

    }

    if (channelsIn == 2) {
        // 2 canaux en entrée
        if (channelsOut == 1) {
            // 1 canal en sortie
            // on conserve uniquement le premier canal
            // en considérant que le deuxieme est de l'alpha

            int k = 0;

            for (int i = 0; i < sizeIn; i=i+2) {
                buffer[k] = bufferIn[i];
                k++;
            }

            //on nettoie
            delete [] bufferIn;
            return channelsOut*width;

        }

        if (channelsOut == 3) {
            // 3 canaux en sortie
            //on copie le premier dans les deux autres

            int k = 0;

            for (int i = 0; i < sizeIn; i=i+2) {
                buffer[k] = bufferIn[i];
                buffer[k+1] = bufferIn[i];
                buffer[k+2] = bufferIn[i];
                k=k+3;
            }

            //on nettoie
            delete [] bufferIn;
            return channelsOut*width;

        }

        if (channelsOut == 4) {
            // 4 canaux en sortie
            // on copie le premier dans les deux autres et on ajoute le canal alpha (2 vers 4)

            int k = 0;

            for (int i = 0; i < sizeIn; i=i+2) {
                buffer[k] = bufferIn[i];
                buffer[k+1] = bufferIn[i];
                buffer[k+2] = bufferIn[i];
                buffer[k+3] = bufferIn[i+1];
                k=k+4;
            }

            //on nettoie
            delete [] bufferIn;
            return channelsOut*width;

        }

    }

    if (channelsIn == 3) {
        // 3 canaux en entrée
        if (channelsOut == 1) {
            // 1 canal en sortie
            // on transforme les couleurs en niveaux de gris

            int k = 0;

            for (int i = 0; i < sizeIn; i = i+3) {
                buffer[k] = greyscale(bufferIn[i],bufferIn[i+1],bufferIn[i+2]);
                k++;
            }

            //on nettoie
            delete [] bufferIn;
            return channelsOut*width;

        }

        if (channelsOut == 2) {
            // 2 canaux en sortie
            // on transforme les couleurs en niveaux de gris et on ajoute un canal alpha

            int k = 0;

            for (int i = 0; i < sizeIn; i = i+3) {
                buffer[k] = greyscale(bufferIn[i],bufferIn[i+1],bufferIn[i+2]);
                buffer[k+1] = alphaOut;
                k = k+2;
            }

            //on nettoie
            delete [] bufferIn;
            return channelsOut*width;

        }

        if (channelsOut == 4) {
            // 4 canaux en sortie
            // on ajoute un canal alpha

            int k = 0;

            for (int i = 0; i < sizeIn; i = i+3) {
                buffer[k] = bufferIn[i];
                buffer[k+1] = bufferIn[i+1];
                buffer[k+2] = bufferIn[i+2];
                buffer[k+3] = alphaOut;
                k = k+4;
            }

            //on nettoie
            delete [] bufferIn;
            return channelsOut*width;

        }


    }

    if (channelsIn == 4) {
        // 4 canaux en entrée
        if (channelsOut == 1) {
            // 1 canal en sortie
            // on transforme les couleurs en niveaux de gris et on ne considère pas le canal alpha

            int k = 0;

            for (int i = 0; i < sizeIn; i = i+4) {
                buffer[k] = greyscale(bufferIn[i],bufferIn[i+1],bufferIn[i+3]);
                k++;
            }

            //on nettoie
            delete [] bufferIn;
            return channelsOut*width;

        }

        if (channelsOut == 2) {
            // 2 canaux en sortie
            // on transforme les couleurs en niveaux de gris et on conserve le canal alpha dans le deuxieme canal

            int k = 0;

            for (int i = 0; i < sizeIn; i = i+4) {
                buffer[k] = greyscale(bufferIn[i],bufferIn[i+1],bufferIn[i+3]);
                buffer[k+1] = alphaOut;
                k = k+2;
            }

            //on nettoie
            delete [] bufferIn;
            return channelsOut*width;


        }

        if (channelsOut == 3) {
            // 3 canaux en sortie
            // on va simplement ne pas considérer le 4eme canal
            // qui est en théorie le canal alpha

            //on créé le nouveau buffer
            int k = 0;

            for(int i = 0; i < sizeIn; i++) {
                if (i%4 == 3) continue;
                buffer[k] = bufferIn[i];
                k++;
            }

            //on nettoie
            delete [] bufferIn;
            return channelsOut*width;
        }
    }


    return 0;

}

int ConvertedChannelsImage::getline ( uint16_t *buffer, int line ) {

    return 0;
}

int ConvertedChannelsImage::getline ( float *buffer, int line ) {

    return 0;
}

int ConvertedChannelsImage::greyscale(int r, int g, int b) {

    return lround(0.2126 * r + 0.7152 * g + 0.0722 * b);

}

float ConvertedChannelsImage::greyscaleFloat(int r, int g, int b) {

    return 0.2126 * ((double)r / 255.0) + 0.7152 * ((double)g / 255.0) + 0.0722 * ((double)b / 255.0);

}

