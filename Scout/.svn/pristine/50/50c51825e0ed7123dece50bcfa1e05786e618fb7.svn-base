#! /bin/bash


function problem {
	printf "\n################################################################################\n"
	printf "################################################################################\n"
	printf "Something went wrong! Please check errors above!\n"
	printf "To force a clean rebuild delete ../llvm amd ../build .\n"
	printf "################################################################################\n"
	printf "################################################################################\n"

	exit 1
}

function help {
	echo "usage:"
	echo "install.sh [-h|--help] [-gui|--no-gui] [-no-gui|--no-gui] [-only-gui|--only-gui]"
	echo "  -h"
	echo "  --help"
	echo "      Print this help"
	echo "  -gui"
	echo "  --gui"
	echo "      Build GUI also"
	echo "  -no-gui"
	echo "  --no-gui"
	echo "      Do not build GUI (default)"
	echo "  -only-gui"
	echo "  --only-gui"
	echo "      Build the GUI, nothing else"

	exit
}

#Set default values
is_new_build=false
build_gui=false
build=true


# Process command line arguments
for arg in "$@"
do
	case "$arg" in
	-gui | --gui)
		build_gui=true
	;;
	-no-gui | --no-gui)
	build_gui=false
	;;
	-only-gui | --only-gui)
		build=false
		build_gui=true
	;;
	-h | --help)
	help
	;;
	*)
		echo "Error: not known argument: $arg"
		exit
	;;
	esac
done


if $build
then
	boost_dir=$(find -L ../ -maxdepth 1 -mindepth 1 -name "boost_1_[4|5|6|7|8|9]*" -type d -print | sort -fnr | head -n1)

	if [ -z "$boost_dir" ]; then
		printf "\nA boost version (1.4x to 1.9x) is expected in the parent directory\n"
		printf "but not found. Please check INSTALL.TXT for further information\n"
		exit 1
	fi

	answer=

	if [ ! -d "../llvm" ] || [ ! -d "../build" ]
	then
		is_new_build=true
		printf "\nThis script will checkout, install and build\n"
		printf "llvm and clang in the parent directory.\n"
		printf "This will consume a lot of space.\n"
		printf "Do you want to continue? (y or n)\n"
		read answer

		if [ $answer = "y" ]; then
			printf "Installation started, please be patient.\n"
		else
			printf "Quick install aborted. Please check INSTALL.TXT for further information.\n"
			exit 1
		fi
	fi

	if [ ! -d "../llvm" ]
	then
		echo ""
		echo "################################################################################"
		echo "###                              Checkout LLVM                               ###"
		echo "################################################################################"
		echo ""
		cd ..
		svn co http://llvm.org/svn/llvm-project/llvm/tags/RELEASE_34/final llvm || problem
		cd llvm/tools || problem
		echo ""
		echo "################################################################################"
		echo "###                              Checkout CLANG                              ###"
		echo "################################################################################"
		echo ""
		svn co http://llvm.org/svn/llvm-project/cfe/tags/RELEASE_34/final clang || problem
		echo ""
		echo "################################################################################"
		echo "###                               Patch CLANG                                ###"
		echo "################################################################################"
		echo ""
		cd clang || problem
		patch -p0 < ../../../Scout/clangAddons/patch/Scout.patch || problem
	else
		echo ""
		echo "################################################################################"
		echo "###                               Update LLVM                                ###"
		echo "################################################################################"
		echo ""
		cd ../llvm  || problem
		svn up || problem
		echo ""
		echo "################################################################################"
		echo "###                               Update CLANG                               ###"
		echo "################################################################################"
		echo ""
		cd tools/clang  || problem
		svn revert . -R  || problem
		svn up || problem
		echo ""
		echo "################################################################################"
		echo "###                               Patch CLANG                                ###"
		echo "################################################################################"
		echo ""
		patch -p0 < ../../../Scout/clangAddons/patch/Scout.patch || problem
	fi
	echo ""
	echo "################################################################################"
	echo "###                               Build LLVM                                 ###"
	echo "################################################################################"
	echo ""
	cd ../../..  || problem
        if [ ! -d "./build" ]
	then
		mkdir build || problem
		cd llvm  || problem
		./configure || problem
		make REQUIRES_RTTI=1 || problem
	else
		cd llvm || problem
		make REQUIRES_RTTI=1 || problem
	fi
	cd ../Scout || problem

	echo "$is_new_build"

	if $is_new_build
	then
		echo ""
		echo "################################################################################"
		echo "###                               Build Scout                                ###"
		echo "################################################################################"
		echo ""
		./configure.sh -boost_dir $boost_dir -llvm_dir ../llvm -llvm_lib_dir ../llvm || problem
		cd clangAddons || problem
		make || problem
		cd .. || problem
	else
		echo ""
		echo "################################################################################"
		echo "###                              Rebuild Scout                               ###"
		echo "################################################################################"
		echo ""
		cd clangAddons || problem
		make || problem
		cd .. || problem
	fi
fi

if $build_gui
then
	echo "################################################################################"
	echo "###                                Build GUI                                 ###"
	echo "################################################################################"
	qmake Scout.pro || problem
	make debug || problem
fi

echo ""
echo "################################################################################"
echo "###                                  Done                                    ###"
echo "################################################################################"
echo ""

exit 0
