##===- projects/sample/lib/sample/Makefile -----------------*- Makefile -*-===##

#
# Indicate where we are relative to the top of the source tree.
#
LEVEL=../..

#
# Give the name of a library.  This will build a dynamic version.
#
TOOLNAME=scout_cli

LINK_COMPONENTS := x86 asmparser bitreader bitwriter codegen \
                   instrumentation ipo linker option selectiondag 

ProjLibsOptions := -lscoutClangInterface -lscoutClangVectorizing -lscoutClangASTProcessing \
           -lclangFrontend -lclangDriver \
           -lclangSerialization -lclangCodeGen -lclangParse -lclangSema \
           -lclangStaticAnalyzerFrontend -lclangStaticAnalyzerCheckers \
           -lclangStaticAnalyzerCore \
           -lclangAnalysis -lclangARCMigrate \
           -lclangRewriteFrontend -lclangRewriteCore \
           -lclangEdit -lclangAST -lclangLex -lclangBasic
 


#
# Include Makefile.common so we know what to do.
#
include $(LEVEL)/Makefile.common

