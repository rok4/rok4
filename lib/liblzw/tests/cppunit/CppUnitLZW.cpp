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

#include <cppunit/extensions/HelperMacros.h>

#include <string.h>
#include "lzwDecoder.h"
#include "lzwEncoder.h"
#include <stdlib.h>
#include <time.h>
#include <algorithm>


class CppUnitLZW : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE ( CppUnitLZW );

    CPPUNIT_TEST ( smallString );

    CPPUNIT_TEST ( largeString );

    CPPUNIT_TEST ( veryLargeString );

    CPPUNIT_TEST ( randomData );

    CPPUNIT_TEST ( whiteData );

    /* CPPUNIT_TEST ( smallStringStream );

     CPPUNIT_TEST ( largeStringStream );

     CPPUNIT_TEST ( veryLargeStringStream );

     CPPUNIT_TEST ( randomDataStream );

     CPPUNIT_TEST ( whiteDataStream );*/

    CPPUNIT_TEST_SUITE_END();

protected:
    void compressCppUncompress ( std::string message );
    void compressCppUncompress ( uint8_t* rawBuffer, size_t rawBufferSize );

    /*  void compressStreamUncompress ( std::string message );
        void compressStreamUncompress ( uint8_t* rawBuffer, size_t rawBufferSize );*/

public:


    void smallString();
    void largeString();
    void veryLargeString();
    void randomData();
    void whiteData();

    /*    void smallStringStream();
        void largeStringStream();
        void veryLargeStringStream();
        void randomDataStream();
        void whiteDataStream();*/

};

CPPUNIT_TEST_SUITE_REGISTRATION ( CppUnitLZW );
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION ( CppUnitLZW, "CppUnitLZW" );

void CppUnitLZW::compressCppUncompress ( uint8_t* rawBuffer, size_t rawBufferSize ) {
    std::cout << "RAW Buffer Size = " << rawBufferSize << std::endl;
    //std::cout << "RAW Buffer = " << std::endl << rawBuffer << std::endl << "End Of Buffer" << std::endl ;

    size_t lzwCppBufferSize = 0;
    uint8_t* lzwCppBuffer;

    size_t decodedBufferSize = 0;
    uint8_t* decodedBuffer;

    size_t streamDecodedBufferSize = 0;
    uint8_t* streamDecodedBuffer;

    lzwEncoder encoder;

    lzwCppBuffer = encoder.encode ( rawBuffer,rawBufferSize,lzwCppBufferSize );
    std::cout << "LZW Cpp Buffer Size = " << lzwCppBufferSize << std::endl;
    //std::cout << "LZW Cpp Buffer = " << std::endl << lzwCppBuffer << std::endl << "End Of Buffer" << std::endl ;

    CPPUNIT_ASSERT_MESSAGE ( "Cpp Compression failed", lzwCppBufferSize != 0 );

    lzwDecoder decoder ( 12 );

    decodedBuffer = decoder.decode ( lzwCppBuffer, lzwCppBufferSize, decodedBufferSize );
    std::cout << "Decompression termine" << std::endl;

    std::cout << "Decoded Buffer Size = " << decodedBufferSize << std::endl;
    //std::cout << "Decoded Buffer = " << std::endl << decodedBuffer << std::endl << "End Of Buffer" << std::endl ;
    CPPUNIT_ASSERT_MESSAGE ( "Decompression failed", decodedBufferSize == rawBufferSize );
//
    for ( size_t pos = rawBufferSize -1; pos; --pos ) {
        CPPUNIT_ASSERT_MESSAGE ( "Information altered",  * ( rawBuffer+pos ) == * ( decodedBuffer+pos ) );
    }

    delete[] lzwCppBuffer;
    lzwCppBuffer=0;
    delete[] ( decodedBuffer );
    decodedBuffer=0;

}
/*
void CppUnitLZW::compressStreamUncompress ( uint8_t* rawBuffer, size_t rawBufferSize ) {
    std::cout << "RAW Buffer Size = " << rawBufferSize << std::endl;
    //std::cout << "RAW Buffer = " << std::endl << rawBuffer << std::endl << "End Of Buffer" << std::endl ;

    size_t lzwCppBufferSize = 0;
    uint8_t* lzwCppBuffer;

    size_t lzwStreamBufferSize = 0;
    uint8_t* lzwStreamBuffer;

    size_t decodedBufferSize = 0;
    uint8_t* decodedBuffer;

    size_t streamDecodedBufferSize = 0;
    uint8_t* streamDecodedBuffer;

    lzwEncoder encoder;

    //lzwCppBuffer = encoder.encode(rawBuffer,rawBufferSize,lzwCppBufferSize);
    //std::cout << "LZW Cpp Buffer Size = " << lzwCppBufferSize << std::endl;
    //std::cout << "LZW Cpp Buffer = " << std::endl << lzwCppBuffer << std::endl << "End Of Buffer" << std::endl ;

    //CPPUNIT_ASSERT_MESSAGE ( "Cpp Compression failed", lzwCppBufferSize != 0 );

    lzwEncoder streamEncoder;

    lzwStreamBuffer = encoder.encodeAlt(rawBuffer,rawBufferSize,lzwStreamBufferSize);

    CPPUNIT_ASSERT_MESSAGE ( "Stream Compression failed", lzwStreamBufferSize != lzwCppBufferSize );

    lzwDecoder decoder(12);

    decodedBuffer = decoder.decode ( lzwStreamBuffer, lzwStreamBufferSize, decodedBufferSize );
    std::cout << "Decompression termine" << std::endl;

    std::cout << "Decoded Buffer Size = " << decodedBufferSize << std::endl;
    //std::cout << "Decoded Buffer = " << std::endl << decodedBuffer << std::endl << "End Of Buffer" << std::endl ;
    CPPUNIT_ASSERT_MESSAGE ( "Decompression failed", decodedBufferSize == rawBufferSize );
//
    for ( size_t pos = rawBufferSize -1; pos; --pos ) {
        CPPUNIT_ASSERT_MESSAGE ( "Information altered",  * ( rawBuffer+pos ) == * ( decodedBuffer+pos ) );
    }

    delete[] lzwCppBuffer;
    lzwCppBuffer=0;
    delete[] lzwStreamBuffer;
    lzwStreamBuffer=0;

    delete[] (decodedBuffer);
    decodedBuffer=0;

}
*/

void CppUnitLZW::compressCppUncompress ( std::string message ) {
    size_t rawBufferSize = message.length() +1;
    uint8_t* rawBuffer = new uint8_t[rawBufferSize];
    memcpy ( rawBuffer,message.c_str(),rawBufferSize );

    compressCppUncompress ( rawBuffer, rawBufferSize );
    delete[] rawBuffer;
}

/*
void CppUnitLZW::compressStreamUncompress(std::string message)
{
    size_t rawBufferSize = message.length() +1;
    uint8_t* rawBuffer = new uint8_t[rawBufferSize];
    memcpy ( rawBuffer,message.c_str(),rawBufferSize );

    compressCppUncompress ( rawBuffer, rawBufferSize );
    delete[] rawBuffer;
}*/


void CppUnitLZW::smallString() {
    compressCppUncompress ( "LLLZW compression Algorithm implementation test" );
}

void CppUnitLZW::largeString() {
    compressCppUncompress ( "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nunc pretium, lacus bibendum pellentesque fringilla, sapien urna sollicitudin nulla, vel aliquet risus nulla ut eros. Duis at magna urna, nec tincidunt eros. Maecenas eu risus velit. Duis id orci metus, in aliquam erat. Pellentesque id nibh non diam rhoncus mollis. Mauris elit ipsum, egestas quis tempus vitae, lobortis ac risus. Duis faucibus laoreet felis nec tincidunt. Nunc faucibus scelerisque nunc ac lacinia. Sed eu lectus nunc, et ultricies mi. Fusce fermentum vulputate est, non luctus ligula elementum vitae. Sed bibendum, libero eget eleifend hendrerit, mi magna pellentesque leo, a gravida nulla velit quis dui. Suspendisse rhoncus consequat lectus at tristique. Aenean ultrices posuere ipsum eu pretium. Mauris sed arcu elementum risus lacinia luctus quis tempus velit. Nullam massa turpis, eleifend ut sagittis eu, tempor non sem. Quisque sit amet justo sit amet nisi sollicitudin accumsan ac at turpis. Donec vel nisi non magna eleifend sagittis nec interdum urna. Morbi ac ipsum mauris, id adipiscing diam. Maecenas ultricies vulputate vulputate. Aliquam erat volutpat. Curabitur sit amet dui nec purus fringilla cursus sed ac turpis. Vestibulum risus ipsum, sodales id placerat nec, adipiscing vitae velit. Nunc nec leo nec enim sagittis tempus. Vivamus tincidunt, ante eget ornare consectetur, velit diam convallis quam, at egestas erat nunc non velit. In vitae justo eget leo ullamcorper lacinia. Vivamus rhoncus lobortis libero, ut pellentesque erat rhoncus non. Nam eros dolor, euismod sed eleifend sed, molestie vel erat. Aliquam odio leo, porta a convallis at, pellentesque at mi. Nullam blandit interdum ultrices. Phasellus cursus magna quis massa vestibulum vel convallis erat tempus. Nulla sodales consectetur metus, at tempus velit porttitor nec. Donec eros arcu, pharetra sed suscipit in, rutrum non lorem.Vivamus lacinia rutrum nisi a ultricies. Sed lacinia bibendum elementum. Nam elementum risus eu tortor laoreet lobortis. Pellentesque sem tortor, pharetra nec placerat ac, fermentum eu nunc. Pellentesque habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas. Ut ac imperdiet augue. Nam id placerat nulla. Maecenas tristique lobortis leo, vel vehicula quam feugiat ut. Duis dapibus placerat eros, elementum sodales metus venenatis a. Mauris in porta orci. Vivamus vulputate consequat purus, eget commodo felis porttitor et. Vivamus sit amet magna nec neque tempor viverra. Nulla facilisi. Nam semper ligula ac nulla placerat fringilla. Pellentesque non leo turpis, id fringilla lacus.Aenean libero nulla, semper ut molestie vitae, pharetra eget dolor. Mauris cursus tellus consequat mauris molestie quis suscipit eros vestibulum. Duis varius, quam at vehicula vehicula, justo sapien posuere leo, nec interdum mauris justo nec mi. Morbi viverra lobortis dolor et vestibulum. Morbi erat sapien, imperdiet vitae lobortis eget, sodales et enim. Curabitur tempus imperdiet tortor, eu eleifend mauris elementum eget. Curabitur sapien elit, scelerisque vitae ornare eget, tincidunt sed nisl. Pellentesque habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas. Proin ligula nisl, volutpat vel ultrices pellentesque, gravida ac nunc.Proin tempor pulvinar pulvinar. Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia Curae; Pellentesque ac molestie ante. Aliquam euismod, lectus id condimentum sollicitudin, leo magna convallis magna, eget feugiat nulla dolor nec massa. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed ullamcorper massa ut urna semper hendrerit. Nullam leo diam, porttitor in euismod non, scelerisque eu leo. Sed risus arcu, vestibulum eu faucibus nec, pellentesque et leo. Maecenas quis velit elit. Nulla ipsum nisl, tristique et vehicula sed, mattis at nibh. Etiam viverra lobortis nulla, condimentum tristique nisi viverra quis. Nulla in mi sed odio convallis suscipit quis quis libero. Quisque orci orci, ornare id sagittis eget, semper sit amet magna. Quisque rutrum fringilla dui, vel vestibulum nisl dapibus nec. Donec eget mattis elit. Nulla facilisi.Cras tortor nibh, interdum non porta in, iaculis eget lectus. Cum sociis natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. Nam quam quam, vulputate ut placerat at, interdum condimentum ipsum. Nulla vitae posuere risus. Donec vitae velit ac sapien vestibulum ultricies. Morbi nulla ligula, euismod ut consectetur vel, eleifend in metus. In condimentum scelerisque adipiscing. Donec ipsum est, viverra id dapibus vel, dignissim eget massa. Fusce sed mollis erat. Duis in ligula purus, sed dignissim nisl. Cras congue augue eu lacus tristique pharetra vitae in ligula. Proin auctor aliquet neque, et rhoncus turpis pulvinar eget.Vivamus id nisl nec lacus tincidunt interdum lacinia ac diam. Fusce sapien diam, lacinia eget feugiat et, luctus vel sem. Donec consectetur sollicitudin est, sit amet pretium massa tempus sit amet. Suspendisse nec eros libero. Mauris imperdiet pretium pretium. Duis ut neque non mauris placerat consequat at rhoncus augue. Vivamus condimentum euismod metus mattis aliquam.Mauris in lectus orci." );
}

void CppUnitLZW::veryLargeString() {
    compressCppUncompress ( "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nunc pretium, lacus bibendum pellentesque fringilla, sapien urna sollicitudin nulla, vel aliquet risus nulla ut eros. Duis at magna urna, nec tincidunt eros. Maecenas eu risus velit. Duis id orci metus, in aliquam erat. Pellentesque id nibh non diam rhoncus mollis. Mauris elit ipsum, egestas quis tempus vitae, lobortis ac risus. Duis faucibus laoreet felis nec tincidunt. Nunc faucibus scelerisque nunc ac lacinia. Sed eu lectus nunc, et ultricies mi. Fusce fermentum vulputate est, non luctus ligula elementum vitae. Sed bibendum, libero eget eleifend hendrerit, mi magna pellentesque leo, a gravida nulla velit quis dui. Suspendisse rhoncus consequat lectus at tristique. Aenean ultrices posuere ipsum eu pretium. Mauris sed arcu elementum risus lacinia luctus quis tempus velit. Nullam massa turpis, eleifend ut sagittis eu, tempor non sem. Quisque sit amet justo sit amet nisi sollicitudin accumsan ac at turpis. Donec vel nisi non magna eleifend sagittis nec interdum urna. Morbi ac ipsum mauris, id adipiscing diam. Maecenas ultricies vulputate vulputate. Aliquam erat volutpat. Curabitur sit amet dui nec purus fringilla cursus sed ac turpis. Vestibulum risus ipsum, sodales id placerat nec, adipiscing vitae velit. Nunc nec leo nec enim sagittis tempus. Vivamus tincidunt, ante eget ornare consectetur, velit diam convallis quam, at egestas erat nunc non velit. In vitae justo eget leo ullamcorper lacinia. Vivamus rhoncus lobortis libero, ut pellentesque erat rhoncus non. Nam eros dolor, euismod sed eleifend sed, molestie vel erat. Aliquam odio leo, porta a convallis at, pellentesque at mi. Nullam blandit interdum ultrices. Phasellus cursus magna quis massa vestibulum vel convallis erat tempus. Nulla sodales consectetur metus, at tempus velit porttitor nec. Donec eros arcu, pharetra sed suscipit in, rutrum non lorem.Vivamus lacinia rutrum nisi a ultricies. Sed lacinia bibendum elementum. Nam elementum risus eu tortor laoreet lobortis. Pellentesque sem tortor, pharetra nec placerat ac, fermentum eu nunc. Pellentesque habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas. Ut ac imperdiet augue. Nam id placerat nulla. Maecenas tristique lobortis leo, vel vehicula quam feugiat ut. Duis dapibus placerat eros, elementum sodales metus venenatis a. Mauris in porta orci. Vivamus vulputate consequat purus, eget commodo felis porttitor et. Vivamus sit amet magna nec neque tempor viverra. Nulla facilisi. Nam semper ligula ac nulla placerat fringilla. Pellentesque non leo turpis, id fringilla lacus.Aenean libero nulla, semper ut molestie vitae, pharetra eget dolor. Mauris cursus tellus consequat mauris molestie quis suscipit eros vestibulum. Duis varius, quam at vehicula vehicula, justo sapien posuere leo, nec interdum mauris justo nec mi. Morbi viverra lobortis dolor et vestibulum. Morbi erat sapien, imperdiet vitae lobortis eget, sodales et enim. Curabitur tempus imperdiet tortor, eu eleifend mauris elementum eget. Curabitur sapien elit, scelerisque vitae ornare eget, tincidunt sed nisl. Pellentesque habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas. Proin ligula nisl, volutpat vel ultrices pellentesque, gravida ac nunc.Proin tempor pulvinar pulvinar. Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia Curae; Pellentesque ac molestie ante. Aliquam euismod, lectus id condimentum sollicitudin, leo magna convallis magna, eget feugiat nulla dolor nec massa. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed ullamcorper massa ut urna semper hendrerit. Nullam leo diam, porttitor in euismod non, scelerisque eu leo. Sed risus arcu, vestibulum eu faucibus nec, pellentesque et leo. Maecenas quis velit elit. Nulla ipsum nisl, tristique et vehicula sed, mattis at nibh. Etiam viverra lobortis nulla, condimentum tristique nisi viverra quis. Nulla in mi sed odio convallis suscipit quis quis libero. Quisque orci orci, ornare id sagittis eget, semper sit amet magna. Quisque rutrum fringilla dui, vel vestibulum nisl dapibus nec. Donec eget mattis elit. Nulla facilisi.Cras tortor nibh, interdum non porta in, iaculis eget lectus. Cum sociis natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. Nam quam quam, vulputate ut placerat at, interdum condimentum ipsum. Nulla vitae posuere risus. Donec vitae velit ac sapien vestibulum ultricies. Morbi nulla ligula, euismod ut consectetur vel, eleifend in metus. In condimentum scelerisque adipiscing. Donec ipsum est, viverra id dapibus vel, dignissim eget massa. Fusce sed mollis erat. Duis in ligula purus, sed dignissim nisl. Cras congue augue eu lacus tristique pharetra vitae in ligula. Proin auctor aliquet neque, et rhoncus turpis pulvinar eget.Vivamus id nisl nec lacus tincidunt interdum lacinia ac diam. Fusce sapien diam, lacinia eget feugiat et, luctus vel sem. Donec consectetur sollicitudin est, sit amet pretium massa tempus sit amet. Suspendisse nec eros libero. Mauris imperdiet pretium pretium. Duis ut neque non mauris placerat consequat at rhoncus augue. Vivamus condimentum euismod metus mattis aliquam.Mauris in lectus orci. Cras at augue malesuada nibh tristique semper sed quis nunc. In hac habitasse platea dictumst. Nullam aliquam augue et felis commodo eleifend. Duis eget magna nibh, ac lacinia nisl. Suspendisse blandit velit eget nisl imperdiet et vestibulum dui rutrum. Fusce faucibus ultrices semper. Mauris eu neque ligula, eu accumsan purus. Phasellus iaculis, enim sit amet bibendum rhoncus, velit dolor accumsan quam, et eleifend risus enim sed enim. Proin auctor eleifend turpis commodo ultricies. Integer dapibus dolor urna, et accumsan erat. Suspendisse id erat id elit elementum pharetra. In hac habitasse platea dictumst. Donec metus justo, suscipit nec ultricies sit amet, mattis sit amet justo. Praesent eu tortor ac libero faucibus eleifend sit amet at mi. Nulla nec posuere orci.Suspendisse potenti. Duis turpis nisl, porttitor vitae mattis mattis, porta et quam. Ut convallis enim vel nisl lobortis non cursus velit adipiscing. Sed aliquam luctus arcu et accumsan. Quisque facilisis, odio commodo condimentum rutrum, ante arcu aliquam metus, a tristique justo mi varius sem. Morbi nec elit sed eros rhoncus tincidunt quis porta magna. Ut nec cursus leo.Etiam ultrices leo sed odio mattis pulvinar eget at lacus. Aenean non sapien et nisi convallis dapibus. Cras quis ante ante. Maecenas ac lacus eget ante vestibulum dapibus. Donec vitae est a nisi hendrerit varius quis sed nibh. Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia Curae; In pretium pharetra diam quis blandit. Nunc orci dolor, accumsan vulputate vulputate vel, pharetra quis dui. Suspendisse in neque sapien, et mollis leo. Maecenas lectus nulla, bibendum a tincidunt at, consequat quis lorem. Suspendisse ut egestas ipsum.Nullam ut sapien felis. Curabitur hendrerit congue orci at aliquet. Maecenas a lacus non quam auctor dictum ut a turpis. Morbi justo purus, suscipit ut faucibus nec, tempor vel risus. Phasellus sodales viverra turpis, vel ornare risus facilisis ut. Praesent eu odio tellus, a luctus arcu. Ut magna mi, porta in blandit hendrerit, laoreet ut tellus. Sed porttitor aliquet faucibus. Nulla neque libero, consectetur eget sagittis et, venenatis at turpis.Vivamus nec eros eros, sit amet suscipit lectus. Nam commodo bibendum facilisis. Sed tellus tortor, fringilla ut viverra et, posuere non nibh. Etiam porttitor, nisi eget imperdiet mattis, metus nisl aliquet tortor, at venenatis arcu est eu erat. Nulla feugiat odio tortor. Phasellus consequat felis vel erat tempus et dignissim metus imperdiet. Fusce vel magna nec arcu luctus ullamcorper. Mauris vitae tellus non nibh accumsan fringilla. Vestibulum semper aliquam rutrum. Nunc dapibus mattis dolor, eu congue nisi cursus eu. Donec volutpat ante eget diam vulputate et viverra lorem fermentum. Suspendisse eu mauris urna. Vestibulum non molestie lectus. Phasellus ac ante tellus, ut varius nibh. Proin est sapien, egestas eu lobortis sit amet, interdum in dolor. Praesent sit amet lectus mauris, a dapibus orci.Fusce tristique nisl id velit dictum ultrices. Duis non elit in massa porta interdum. Fusce vel nibh a magna dignissim convallis ut sed arcu. Sed ac magna sapien. Maecenas elit nunc, malesuada sed convallis in, rutrum sed neque. Nunc at mauris faucibus enim dictum fermentum eget id sapien. Etiam adipiscing pretium lectus non commodo. Donec posuere, eros ut laoreet tempor, odio mi euismod massa, id feugiat nunc leo fringilla mauris. Quisque justo lacus, adipiscing ac pharetra id, mattis eleifend tellus. Vivamus vitae nulla libero. Mauris varius nunc vel nisi tincidunt ultrices. Praesent non neque in arcu lobortis pretium quis ut neque. Proin vel erat sed erat malesuada iaculis vel sit amet ligula. Cum sociis natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. Pellentesque fringilla bibendum ornare. Nulla et urna vitae metus lacinia porta non ac arcu.Etiam adipiscing luctus pulvinar. Pellentesque vitae lorem ligula, sed convallis ligula. Ut lobortis congue condimentum. Sed vel dolor et quam interdum consectetur id ac erat. Vestibulum porta aliquet nunc vel vestibulum. Donec sed justo ut mauris bibendum aliquam. Pellentesque habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas. Nullam ultrices semper placerat.Etiam vehicula elit vulputate neque interdum molestie. Vivamus varius, augue eget semper interdum, magna mauris euismod enim, at interdum leo turpis at lectus. Aliquam suscipit lectus vitae libero commodo nec ornare justo ultrices. Praesent pulvinar vehicula lacus consectetur volutpat. Suspendisse potenti. Vivamus tellus magna, mollis vel tincidunt volutpat, tristique non arcu. Praesent molestie tortor vitae nibh tempor cursus. Donec vel est suscipit lorem posuere sagittis. Aenean tincidunt ipsum at est fringilla tristique. Sed turpis duis." );
}

void CppUnitLZW::randomData() {
    size_t rawBufferSize = 256*256*3;
    uint8_t* rawBuffer = new uint8_t[rawBufferSize];
    srand ( time ( NULL ) );
    for ( size_t pos = rawBufferSize; pos; --pos ) {
        rawBuffer[pos] = 256 * ( rand() / ( RAND_MAX +1.0 ) );

    }

    compressCppUncompress ( rawBuffer,rawBufferSize );
    delete[] rawBuffer;
}

void CppUnitLZW::whiteData() {
    size_t rawBufferSize = 256*256*3;
    uint8_t* rawBuffer = new uint8_t[rawBufferSize];
    for ( size_t pos = rawBufferSize; pos; --pos ) {
        rawBuffer[pos] = 255;
    }

    compressCppUncompress ( rawBuffer,rawBufferSize );
    delete[] rawBuffer;
}

/*
void CppUnitLZW::smallStringStream() {
    compressStreamUncompress ( "LLLZW compression Algorithm implementation test" );
}

void CppUnitLZW::largeStringStream() {
    compressStreamUncompress ( "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nunc pretium, lacus bibendum pellentesque fringilla, sapien urna sollicitudin nulla, vel aliquet risus nulla ut eros. Duis at magna urna, nec tincidunt eros. Maecenas eu risus velit. Duis id orci metus, in aliquam erat. Pellentesque id nibh non diam rhoncus mollis. Mauris elit ipsum, egestas quis tempus vitae, lobortis ac risus. Duis faucibus laoreet felis nec tincidunt. Nunc faucibus scelerisque nunc ac lacinia. Sed eu lectus nunc, et ultricies mi. Fusce fermentum vulputate est, non luctus ligula elementum vitae. Sed bibendum, libero eget eleifend hendrerit, mi magna pellentesque leo, a gravida nulla velit quis dui. Suspendisse rhoncus consequat lectus at tristique. Aenean ultrices posuere ipsum eu pretium. Mauris sed arcu elementum risus lacinia luctus quis tempus velit. Nullam massa turpis, eleifend ut sagittis eu, tempor non sem. Quisque sit amet justo sit amet nisi sollicitudin accumsan ac at turpis. Donec vel nisi non magna eleifend sagittis nec interdum urna. Morbi ac ipsum mauris, id adipiscing diam. Maecenas ultricies vulputate vulputate. Aliquam erat volutpat. Curabitur sit amet dui nec purus fringilla cursus sed ac turpis. Vestibulum risus ipsum, sodales id placerat nec, adipiscing vitae velit. Nunc nec leo nec enim sagittis tempus. Vivamus tincidunt, ante eget ornare consectetur, velit diam convallis quam, at egestas erat nunc non velit. In vitae justo eget leo ullamcorper lacinia. Vivamus rhoncus lobortis libero, ut pellentesque erat rhoncus non. Nam eros dolor, euismod sed eleifend sed, molestie vel erat. Aliquam odio leo, porta a convallis at, pellentesque at mi. Nullam blandit interdum ultrices. Phasellus cursus magna quis massa vestibulum vel convallis erat tempus. Nulla sodales consectetur metus, at tempus velit porttitor nec. Donec eros arcu, pharetra sed suscipit in, rutrum non lorem.Vivamus lacinia rutrum nisi a ultricies. Sed lacinia bibendum elementum. Nam elementum risus eu tortor laoreet lobortis. Pellentesque sem tortor, pharetra nec placerat ac, fermentum eu nunc. Pellentesque habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas. Ut ac imperdiet augue. Nam id placerat nulla. Maecenas tristique lobortis leo, vel vehicula quam feugiat ut. Duis dapibus placerat eros, elementum sodales metus venenatis a. Mauris in porta orci. Vivamus vulputate consequat purus, eget commodo felis porttitor et. Vivamus sit amet magna nec neque tempor viverra. Nulla facilisi. Nam semper ligula ac nulla placerat fringilla. Pellentesque non leo turpis, id fringilla lacus.Aenean libero nulla, semper ut molestie vitae, pharetra eget dolor. Mauris cursus tellus consequat mauris molestie quis suscipit eros vestibulum. Duis varius, quam at vehicula vehicula, justo sapien posuere leo, nec interdum mauris justo nec mi. Morbi viverra lobortis dolor et vestibulum. Morbi erat sapien, imperdiet vitae lobortis eget, sodales et enim. Curabitur tempus imperdiet tortor, eu eleifend mauris elementum eget. Curabitur sapien elit, scelerisque vitae ornare eget, tincidunt sed nisl. Pellentesque habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas. Proin ligula nisl, volutpat vel ultrices pellentesque, gravida ac nunc.Proin tempor pulvinar pulvinar. Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia Curae; Pellentesque ac molestie ante. Aliquam euismod, lectus id condimentum sollicitudin, leo magna convallis magna, eget feugiat nulla dolor nec massa. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed ullamcorper massa ut urna semper hendrerit. Nullam leo diam, porttitor in euismod non, scelerisque eu leo. Sed risus arcu, vestibulum eu faucibus nec, pellentesque et leo. Maecenas quis velit elit. Nulla ipsum nisl, tristique et vehicula sed, mattis at nibh. Etiam viverra lobortis nulla, condimentum tristique nisi viverra quis. Nulla in mi sed odio convallis suscipit quis quis libero. Quisque orci orci, ornare id sagittis eget, semper sit amet magna. Quisque rutrum fringilla dui, vel vestibulum nisl dapibus nec. Donec eget mattis elit. Nulla facilisi.Cras tortor nibh, interdum non porta in, iaculis eget lectus. Cum sociis natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. Nam quam quam, vulputate ut placerat at, interdum condimentum ipsum. Nulla vitae posuere risus. Donec vitae velit ac sapien vestibulum ultricies. Morbi nulla ligula, euismod ut consectetur vel, eleifend in metus. In condimentum scelerisque adipiscing. Donec ipsum est, viverra id dapibus vel, dignissim eget massa. Fusce sed mollis erat. Duis in ligula purus, sed dignissim nisl. Cras congue augue eu lacus tristique pharetra vitae in ligula. Proin auctor aliquet neque, et rhoncus turpis pulvinar eget.Vivamus id nisl nec lacus tincidunt interdum lacinia ac diam. Fusce sapien diam, lacinia eget feugiat et, luctus vel sem. Donec consectetur sollicitudin est, sit amet pretium massa tempus sit amet. Suspendisse nec eros libero. Mauris imperdiet pretium pretium. Duis ut neque non mauris placerat consequat at rhoncus augue. Vivamus condimentum euismod metus mattis aliquam.Mauris in lectus orci." );
}

void CppUnitLZW::veryLargeStringStream() {
    compressStreamUncompress ( "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nunc pretium, lacus bibendum pellentesque fringilla, sapien urna sollicitudin nulla, vel aliquet risus nulla ut eros. Duis at magna urna, nec tincidunt eros. Maecenas eu risus velit. Duis id orci metus, in aliquam erat. Pellentesque id nibh non diam rhoncus mollis. Mauris elit ipsum, egestas quis tempus vitae, lobortis ac risus. Duis faucibus laoreet felis nec tincidunt. Nunc faucibus scelerisque nunc ac lacinia. Sed eu lectus nunc, et ultricies mi. Fusce fermentum vulputate est, non luctus ligula elementum vitae. Sed bibendum, libero eget eleifend hendrerit, mi magna pellentesque leo, a gravida nulla velit quis dui. Suspendisse rhoncus consequat lectus at tristique. Aenean ultrices posuere ipsum eu pretium. Mauris sed arcu elementum risus lacinia luctus quis tempus velit. Nullam massa turpis, eleifend ut sagittis eu, tempor non sem. Quisque sit amet justo sit amet nisi sollicitudin accumsan ac at turpis. Donec vel nisi non magna eleifend sagittis nec interdum urna. Morbi ac ipsum mauris, id adipiscing diam. Maecenas ultricies vulputate vulputate. Aliquam erat volutpat. Curabitur sit amet dui nec purus fringilla cursus sed ac turpis. Vestibulum risus ipsum, sodales id placerat nec, adipiscing vitae velit. Nunc nec leo nec enim sagittis tempus. Vivamus tincidunt, ante eget ornare consectetur, velit diam convallis quam, at egestas erat nunc non velit. In vitae justo eget leo ullamcorper lacinia. Vivamus rhoncus lobortis libero, ut pellentesque erat rhoncus non. Nam eros dolor, euismod sed eleifend sed, molestie vel erat. Aliquam odio leo, porta a convallis at, pellentesque at mi. Nullam blandit interdum ultrices. Phasellus cursus magna quis massa vestibulum vel convallis erat tempus. Nulla sodales consectetur metus, at tempus velit porttitor nec. Donec eros arcu, pharetra sed suscipit in, rutrum non lorem.Vivamus lacinia rutrum nisi a ultricies. Sed lacinia bibendum elementum. Nam elementum risus eu tortor laoreet lobortis. Pellentesque sem tortor, pharetra nec placerat ac, fermentum eu nunc. Pellentesque habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas. Ut ac imperdiet augue. Nam id placerat nulla. Maecenas tristique lobortis leo, vel vehicula quam feugiat ut. Duis dapibus placerat eros, elementum sodales metus venenatis a. Mauris in porta orci. Vivamus vulputate consequat purus, eget commodo felis porttitor et. Vivamus sit amet magna nec neque tempor viverra. Nulla facilisi. Nam semper ligula ac nulla placerat fringilla. Pellentesque non leo turpis, id fringilla lacus.Aenean libero nulla, semper ut molestie vitae, pharetra eget dolor. Mauris cursus tellus consequat mauris molestie quis suscipit eros vestibulum. Duis varius, quam at vehicula vehicula, justo sapien posuere leo, nec interdum mauris justo nec mi. Morbi viverra lobortis dolor et vestibulum. Morbi erat sapien, imperdiet vitae lobortis eget, sodales et enim. Curabitur tempus imperdiet tortor, eu eleifend mauris elementum eget. Curabitur sapien elit, scelerisque vitae ornare eget, tincidunt sed nisl. Pellentesque habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas. Proin ligula nisl, volutpat vel ultrices pellentesque, gravida ac nunc.Proin tempor pulvinar pulvinar. Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia Curae; Pellentesque ac molestie ante. Aliquam euismod, lectus id condimentum sollicitudin, leo magna convallis magna, eget feugiat nulla dolor nec massa. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed ullamcorper massa ut urna semper hendrerit. Nullam leo diam, porttitor in euismod non, scelerisque eu leo. Sed risus arcu, vestibulum eu faucibus nec, pellentesque et leo. Maecenas quis velit elit. Nulla ipsum nisl, tristique et vehicula sed, mattis at nibh. Etiam viverra lobortis nulla, condimentum tristique nisi viverra quis. Nulla in mi sed odio convallis suscipit quis quis libero. Quisque orci orci, ornare id sagittis eget, semper sit amet magna. Quisque rutrum fringilla dui, vel vestibulum nisl dapibus nec. Donec eget mattis elit. Nulla facilisi.Cras tortor nibh, interdum non porta in, iaculis eget lectus. Cum sociis natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. Nam quam quam, vulputate ut placerat at, interdum condimentum ipsum. Nulla vitae posuere risus. Donec vitae velit ac sapien vestibulum ultricies. Morbi nulla ligula, euismod ut consectetur vel, eleifend in metus. In condimentum scelerisque adipiscing. Donec ipsum est, viverra id dapibus vel, dignissim eget massa. Fusce sed mollis erat. Duis in ligula purus, sed dignissim nisl. Cras congue augue eu lacus tristique pharetra vitae in ligula. Proin auctor aliquet neque, et rhoncus turpis pulvinar eget.Vivamus id nisl nec lacus tincidunt interdum lacinia ac diam. Fusce sapien diam, lacinia eget feugiat et, luctus vel sem. Donec consectetur sollicitudin est, sit amet pretium massa tempus sit amet. Suspendisse nec eros libero. Mauris imperdiet pretium pretium. Duis ut neque non mauris placerat consequat at rhoncus augue. Vivamus condimentum euismod metus mattis aliquam.Mauris in lectus orci. Cras at augue malesuada nibh tristique semper sed quis nunc. In hac habitasse platea dictumst. Nullam aliquam augue et felis commodo eleifend. Duis eget magna nibh, ac lacinia nisl. Suspendisse blandit velit eget nisl imperdiet et vestibulum dui rutrum. Fusce faucibus ultrices semper. Mauris eu neque ligula, eu accumsan purus. Phasellus iaculis, enim sit amet bibendum rhoncus, velit dolor accumsan quam, et eleifend risus enim sed enim. Proin auctor eleifend turpis commodo ultricies. Integer dapibus dolor urna, et accumsan erat. Suspendisse id erat id elit elementum pharetra. In hac habitasse platea dictumst. Donec metus justo, suscipit nec ultricies sit amet, mattis sit amet justo. Praesent eu tortor ac libero faucibus eleifend sit amet at mi. Nulla nec posuere orci.Suspendisse potenti. Duis turpis nisl, porttitor vitae mattis mattis, porta et quam. Ut convallis enim vel nisl lobortis non cursus velit adipiscing. Sed aliquam luctus arcu et accumsan. Quisque facilisis, odio commodo condimentum rutrum, ante arcu aliquam metus, a tristique justo mi varius sem. Morbi nec elit sed eros rhoncus tincidunt quis porta magna. Ut nec cursus leo.Etiam ultrices leo sed odio mattis pulvinar eget at lacus. Aenean non sapien et nisi convallis dapibus. Cras quis ante ante. Maecenas ac lacus eget ante vestibulum dapibus. Donec vitae est a nisi hendrerit varius quis sed nibh. Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia Curae; In pretium pharetra diam quis blandit. Nunc orci dolor, accumsan vulputate vulputate vel, pharetra quis dui. Suspendisse in neque sapien, et mollis leo. Maecenas lectus nulla, bibendum a tincidunt at, consequat quis lorem. Suspendisse ut egestas ipsum.Nullam ut sapien felis. Curabitur hendrerit congue orci at aliquet. Maecenas a lacus non quam auctor dictum ut a turpis. Morbi justo purus, suscipit ut faucibus nec, tempor vel risus. Phasellus sodales viverra turpis, vel ornare risus facilisis ut. Praesent eu odio tellus, a luctus arcu. Ut magna mi, porta in blandit hendrerit, laoreet ut tellus. Sed porttitor aliquet faucibus. Nulla neque libero, consectetur eget sagittis et, venenatis at turpis.Vivamus nec eros eros, sit amet suscipit lectus. Nam commodo bibendum facilisis. Sed tellus tortor, fringilla ut viverra et, posuere non nibh. Etiam porttitor, nisi eget imperdiet mattis, metus nisl aliquet tortor, at venenatis arcu est eu erat. Nulla feugiat odio tortor. Phasellus consequat felis vel erat tempus et dignissim metus imperdiet. Fusce vel magna nec arcu luctus ullamcorper. Mauris vitae tellus non nibh accumsan fringilla. Vestibulum semper aliquam rutrum. Nunc dapibus mattis dolor, eu congue nisi cursus eu. Donec volutpat ante eget diam vulputate et viverra lorem fermentum. Suspendisse eu mauris urna. Vestibulum non molestie lectus. Phasellus ac ante tellus, ut varius nibh. Proin est sapien, egestas eu lobortis sit amet, interdum in dolor. Praesent sit amet lectus mauris, a dapibus orci.Fusce tristique nisl id velit dictum ultrices. Duis non elit in massa porta interdum. Fusce vel nibh a magna dignissim convallis ut sed arcu. Sed ac magna sapien. Maecenas elit nunc, malesuada sed convallis in, rutrum sed neque. Nunc at mauris faucibus enim dictum fermentum eget id sapien. Etiam adipiscing pretium lectus non commodo. Donec posuere, eros ut laoreet tempor, odio mi euismod massa, id feugiat nunc leo fringilla mauris. Quisque justo lacus, adipiscing ac pharetra id, mattis eleifend tellus. Vivamus vitae nulla libero. Mauris varius nunc vel nisi tincidunt ultrices. Praesent non neque in arcu lobortis pretium quis ut neque. Proin vel erat sed erat malesuada iaculis vel sit amet ligula. Cum sociis natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. Pellentesque fringilla bibendum ornare. Nulla et urna vitae metus lacinia porta non ac arcu.Etiam adipiscing luctus pulvinar. Pellentesque vitae lorem ligula, sed convallis ligula. Ut lobortis congue condimentum. Sed vel dolor et quam interdum consectetur id ac erat. Vestibulum porta aliquet nunc vel vestibulum. Donec sed justo ut mauris bibendum aliquam. Pellentesque habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas. Nullam ultrices semper placerat.Etiam vehicula elit vulputate neque interdum molestie. Vivamus varius, augue eget semper interdum, magna mauris euismod enim, at interdum leo turpis at lectus. Aliquam suscipit lectus vitae libero commodo nec ornare justo ultrices. Praesent pulvinar vehicula lacus consectetur volutpat. Suspendisse potenti. Vivamus tellus magna, mollis vel tincidunt volutpat, tristique non arcu. Praesent molestie tortor vitae nibh tempor cursus. Donec vel est suscipit lorem posuere sagittis. Aenean tincidunt ipsum at est fringilla tristique. Sed turpis duis." );
}

void CppUnitLZW::randomDataStream()
{
    size_t rawBufferSize = 256*256*3;
    uint8_t* rawBuffer = new uint8_t[rawBufferSize];
    srand(time(NULL));
    for (size_t pos = rawBufferSize -1; pos; --pos){
        rawBuffer[pos] = 256 * (rand() / (RAND_MAX +1.0));

    }

    compressStreamUncompress(rawBuffer,rawBufferSize);
    delete[] rawBuffer;
}

void CppUnitLZW::whiteDataStream()
{
    size_t rawBufferSize = 256*256*3;
    uint8_t* rawBuffer = new uint8_t[rawBufferSize];
    for (size_t pos = rawBufferSize; pos; --pos){
        rawBuffer[pos] = 255;
    }

    compressStreamUncompress(rawBuffer,rawBufferSize);
    delete[] rawBuffer;
}
*/
