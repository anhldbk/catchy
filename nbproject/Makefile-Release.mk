#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-Linux
CND_DLIB_EXT=so
CND_CONF=Release
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/src/hash/hash_ring.o \
	${OBJECTDIR}/src/hash/hash_wrapper.o \
	${OBJECTDIR}/src/hash/md5.o \
	${OBJECTDIR}/src/hash/sha1.o \
	${OBJECTDIR}/src/hash/sort.o \
	${OBJECTDIR}/src/hash/xxhash.o \
	${OBJECTDIR}/src/main.o

# Test Directory
TESTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}/tests

# Test Files
TESTFILES= \
	${TESTDIR}/TestFiles/f3 \
	${TESTDIR}/TestFiles/f2 \
	${TESTDIR}/TestFiles/f4 \
	${TESTDIR}/TestFiles/f1 \
	${TESTDIR}/TestFiles/f5 \
	${TESTDIR}/TestFiles/f7 \
	${TESTDIR}/TestFiles/f6

# Test Object Files
TESTOBJECTFILES= \
	${TESTDIR}/tests/FragmentEntryTest.o \
	${TESTDIR}/tests/FragmentTableTest.o \
	${TESTDIR}/tests/HashRingTest.o \
	${TESTDIR}/tests/JumpTableTest.o \
	${TESTDIR}/tests/RegionTest.o \
	${TESTDIR}/tests/SeqLockTest.o \
	${TESTDIR}/tests/TimeTableTest.o

# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/catchy

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/catchy: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/catchy ${OBJECTFILES} ${LDLIBSOPTIONS}

${OBJECTDIR}/src/hash/hash_ring.o: src/hash/hash_ring.cpp
	${MKDIR} -p ${OBJECTDIR}/src/hash
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -std=c++14 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/hash/hash_ring.o src/hash/hash_ring.cpp

${OBJECTDIR}/src/hash/hash_wrapper.o: src/hash/hash_wrapper.cpp
	${MKDIR} -p ${OBJECTDIR}/src/hash
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -std=c++14 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/hash/hash_wrapper.o src/hash/hash_wrapper.cpp

${OBJECTDIR}/src/hash/md5.o: src/hash/md5.cpp
	${MKDIR} -p ${OBJECTDIR}/src/hash
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -std=c++14 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/hash/md5.o src/hash/md5.cpp

${OBJECTDIR}/src/hash/sha1.o: src/hash/sha1.cpp
	${MKDIR} -p ${OBJECTDIR}/src/hash
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -std=c++14 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/hash/sha1.o src/hash/sha1.cpp

${OBJECTDIR}/src/hash/sort.o: src/hash/sort.cpp
	${MKDIR} -p ${OBJECTDIR}/src/hash
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -std=c++14 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/hash/sort.o src/hash/sort.cpp

${OBJECTDIR}/src/hash/xxhash.o: src/hash/xxhash.cpp
	${MKDIR} -p ${OBJECTDIR}/src/hash
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -std=c++14 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/hash/xxhash.o src/hash/xxhash.cpp

${OBJECTDIR}/src/main.o: src/main.cpp
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -std=c++14 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/main.o src/main.cpp

# Subprojects
.build-subprojects:

# Build Test Targets
.build-tests-conf: .build-tests-subprojects .build-conf ${TESTFILES}
.build-tests-subprojects:

${TESTDIR}/TestFiles/f3: ${TESTDIR}/tests/FragmentEntryTest.o ${OBJECTFILES:%.o=%_nomain.o}
	${MKDIR} -p ${TESTDIR}/TestFiles
	${LINK.cc} -o ${TESTDIR}/TestFiles/f3 $^ ${LDLIBSOPTIONS}   

${TESTDIR}/TestFiles/f2: ${TESTDIR}/tests/FragmentTableTest.o ${OBJECTFILES:%.o=%_nomain.o}
	${MKDIR} -p ${TESTDIR}/TestFiles
	${LINK.cc} -o ${TESTDIR}/TestFiles/f2 $^ ${LDLIBSOPTIONS}   

${TESTDIR}/TestFiles/f4: ${TESTDIR}/tests/HashRingTest.o ${OBJECTFILES:%.o=%_nomain.o}
	${MKDIR} -p ${TESTDIR}/TestFiles
	${LINK.cc} -o ${TESTDIR}/TestFiles/f4 $^ ${LDLIBSOPTIONS}   

${TESTDIR}/TestFiles/f1: ${TESTDIR}/tests/JumpTableTest.o ${OBJECTFILES:%.o=%_nomain.o}
	${MKDIR} -p ${TESTDIR}/TestFiles
	${LINK.cc} -o ${TESTDIR}/TestFiles/f1 $^ ${LDLIBSOPTIONS}   

${TESTDIR}/TestFiles/f5: ${TESTDIR}/tests/SeqLockTest.o ${OBJECTFILES:%.o=%_nomain.o}
	${MKDIR} -p ${TESTDIR}/TestFiles
	${LINK.cc} -o ${TESTDIR}/TestFiles/f5 $^ ${LDLIBSOPTIONS}   

${TESTDIR}/TestFiles/f7: ${TESTDIR}/tests/RegionTest.o ${OBJECTFILES:%.o=%_nomain.o}
	${MKDIR} -p ${TESTDIR}/TestFiles
	${LINK.cc} -o ${TESTDIR}/TestFiles/f7 $^ ${LDLIBSOPTIONS}   

${TESTDIR}/TestFiles/f6: ${TESTDIR}/tests/TimeTableTest.o ${OBJECTFILES:%.o=%_nomain.o}
	${MKDIR} -p ${TESTDIR}/TestFiles
	${LINK.cc} -o ${TESTDIR}/TestFiles/f6 $^ ${LDLIBSOPTIONS}   


${TESTDIR}/tests/FragmentEntryTest.o: tests/FragmentEntryTest.cpp 
	${MKDIR} -p ${TESTDIR}/tests
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -I. -std=c++14 -MMD -MP -MF "$@.d" -o ${TESTDIR}/tests/FragmentEntryTest.o tests/FragmentEntryTest.cpp


${TESTDIR}/tests/FragmentTableTest.o: tests/FragmentTableTest.cpp 
	${MKDIR} -p ${TESTDIR}/tests
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -I. -std=c++14 -MMD -MP -MF "$@.d" -o ${TESTDIR}/tests/FragmentTableTest.o tests/FragmentTableTest.cpp


${TESTDIR}/tests/HashRingTest.o: tests/HashRingTest.cpp 
	${MKDIR} -p ${TESTDIR}/tests
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -I. -std=c++14 -MMD -MP -MF "$@.d" -o ${TESTDIR}/tests/HashRingTest.o tests/HashRingTest.cpp


${TESTDIR}/tests/JumpTableTest.o: tests/JumpTableTest.cpp 
	${MKDIR} -p ${TESTDIR}/tests
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -I. -std=c++14 -MMD -MP -MF "$@.d" -o ${TESTDIR}/tests/JumpTableTest.o tests/JumpTableTest.cpp


${TESTDIR}/tests/SeqLockTest.o: tests/SeqLockTest.cpp 
	${MKDIR} -p ${TESTDIR}/tests
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -I. -std=c++14 -MMD -MP -MF "$@.d" -o ${TESTDIR}/tests/SeqLockTest.o tests/SeqLockTest.cpp


${TESTDIR}/tests/RegionTest.o: tests/RegionTest.cpp 
	${MKDIR} -p ${TESTDIR}/tests
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -I. -std=c++14 -MMD -MP -MF "$@.d" -o ${TESTDIR}/tests/RegionTest.o tests/RegionTest.cpp


${TESTDIR}/tests/TimeTableTest.o: tests/TimeTableTest.cpp 
	${MKDIR} -p ${TESTDIR}/tests
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -I. -std=c++14 -MMD -MP -MF "$@.d" -o ${TESTDIR}/tests/TimeTableTest.o tests/TimeTableTest.cpp


${OBJECTDIR}/src/hash/hash_ring_nomain.o: ${OBJECTDIR}/src/hash/hash_ring.o src/hash/hash_ring.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/hash
	@NMOUTPUT=`${NM} ${OBJECTDIR}/src/hash/hash_ring.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -std=c++14 -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/hash/hash_ring_nomain.o src/hash/hash_ring.cpp;\
	else  \
	    ${CP} ${OBJECTDIR}/src/hash/hash_ring.o ${OBJECTDIR}/src/hash/hash_ring_nomain.o;\
	fi

${OBJECTDIR}/src/hash/hash_wrapper_nomain.o: ${OBJECTDIR}/src/hash/hash_wrapper.o src/hash/hash_wrapper.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/hash
	@NMOUTPUT=`${NM} ${OBJECTDIR}/src/hash/hash_wrapper.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -std=c++14 -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/hash/hash_wrapper_nomain.o src/hash/hash_wrapper.cpp;\
	else  \
	    ${CP} ${OBJECTDIR}/src/hash/hash_wrapper.o ${OBJECTDIR}/src/hash/hash_wrapper_nomain.o;\
	fi

${OBJECTDIR}/src/hash/md5_nomain.o: ${OBJECTDIR}/src/hash/md5.o src/hash/md5.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/hash
	@NMOUTPUT=`${NM} ${OBJECTDIR}/src/hash/md5.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -std=c++14 -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/hash/md5_nomain.o src/hash/md5.cpp;\
	else  \
	    ${CP} ${OBJECTDIR}/src/hash/md5.o ${OBJECTDIR}/src/hash/md5_nomain.o;\
	fi

${OBJECTDIR}/src/hash/sha1_nomain.o: ${OBJECTDIR}/src/hash/sha1.o src/hash/sha1.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/hash
	@NMOUTPUT=`${NM} ${OBJECTDIR}/src/hash/sha1.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -std=c++14 -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/hash/sha1_nomain.o src/hash/sha1.cpp;\
	else  \
	    ${CP} ${OBJECTDIR}/src/hash/sha1.o ${OBJECTDIR}/src/hash/sha1_nomain.o;\
	fi

${OBJECTDIR}/src/hash/sort_nomain.o: ${OBJECTDIR}/src/hash/sort.o src/hash/sort.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/hash
	@NMOUTPUT=`${NM} ${OBJECTDIR}/src/hash/sort.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -std=c++14 -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/hash/sort_nomain.o src/hash/sort.cpp;\
	else  \
	    ${CP} ${OBJECTDIR}/src/hash/sort.o ${OBJECTDIR}/src/hash/sort_nomain.o;\
	fi

${OBJECTDIR}/src/hash/xxhash_nomain.o: ${OBJECTDIR}/src/hash/xxhash.o src/hash/xxhash.cpp 
	${MKDIR} -p ${OBJECTDIR}/src/hash
	@NMOUTPUT=`${NM} ${OBJECTDIR}/src/hash/xxhash.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -std=c++14 -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/hash/xxhash_nomain.o src/hash/xxhash.cpp;\
	else  \
	    ${CP} ${OBJECTDIR}/src/hash/xxhash.o ${OBJECTDIR}/src/hash/xxhash_nomain.o;\
	fi

${OBJECTDIR}/src/main_nomain.o: ${OBJECTDIR}/src/main.o src/main.cpp 
	${MKDIR} -p ${OBJECTDIR}/src
	@NMOUTPUT=`${NM} ${OBJECTDIR}/src/main.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -std=c++14 -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/main_nomain.o src/main.cpp;\
	else  \
	    ${CP} ${OBJECTDIR}/src/main.o ${OBJECTDIR}/src/main_nomain.o;\
	fi

# Run Test Targets
.test-conf:
	@if [ "${TEST}" = "" ]; \
	then  \
	    ${TESTDIR}/TestFiles/f3 || true; \
	    ${TESTDIR}/TestFiles/f2 || true; \
	    ${TESTDIR}/TestFiles/f4 || true; \
	    ${TESTDIR}/TestFiles/f1 || true; \
	    ${TESTDIR}/TestFiles/f5 || true; \
	    ${TESTDIR}/TestFiles/f7 || true; \
	    ${TESTDIR}/TestFiles/f6 || true; \
	else  \
	    ./${TEST} || true; \
	fi

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
