message(STATUS "Detecting your architecture..")
try_run(RUN_OUT COMPILE_OUT ${Rok4_SOURCE_DIR}/cmake/tools/ArchDetect/build ${Rok4_SOURCE_DIR}/cmake/tools/ArchDetect/main.c)

set(BUILD_ARCHITECTURE "Unkown")

if(RUN_OUT EQUAL 1)
	set(BUILD_ARCHITECTURE "i386")
elseif(RUN_OUT EQUAL 2)
	set(BUILD_ARCHITECTURE "amd64")
elseif(RUN_OUT EQUAL 3)
	set(BUILD_ARCHITECTURE "arm")
elseif(RUN_OUT EQUAL 4)
	set(BUILD_ARCHITECTURE "arm64")
endif(RUN_OUT EQUAL 1)

message(STATUS "Your architecture : ${BUILD_ARCHITECTURE}")

