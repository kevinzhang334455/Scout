#! /bin/sh

Usage () {
	printf "\n\nUSAGE:\n"
	printf "configure.sh [-boost_dir PATH] [-llvm_dir PATH]\n"
	printf "             [-llvm_lib_dir PATH]\n"
	printf "default for boost_dir is ../boost_1_41_0\n"
	printf "default for llvm_dir is ../llvm\n"
	printf "default for llvm_lib_dir is llvm_dir/lib/(Debug|Release)\n"
	printf "all PATHs may be absolute or relative\n"
	exit 1
} 


boost_dir=../boost_1_41_0
llvm_dir=../llvm
llvm_lib_dir=

while [ $# -gt 0 ]; do
	if [ $1 = "-boost_dir" ]; then
		boost_dir=$2
	elif [ $1 = "-llvm_dir" ]; then
		llvm_dir=$2
	elif [ $1 = "-llvm_lib_dir" ]; then
		llvm_lib_dir=$2
  else
    Usage
	fi
	shift
	shift
done

if [ -d "$boost_dir" ]; then
	boost_dir=$(cd $boost_dir && pwd)
else
	printf "Boost root directory (boost_dir) not found."
	Usage
fi

if [ -d "$llvm_dir" ]; then
	llvm_dir=$(cd $llvm_dir && pwd)
else
	printf "LLVM root directory (llvm_dir) not found."
	Usage
fi

if [ -z "$llvm_lib_dir" ]; then
  if ! [ -d "$llvm_dir/lib" ]; then
		printf "LLVM lib directory (llvm_lib_dir) not found."
		Usage
	fi
elif [ -d "$llvm_lib_dir" ]; then
	llvm_lib_dir=$(cd $llvm_lib_dir && pwd)
else
	printf "LLVM lib directory (llvm_lib_dir) not found."
	Usage
fi


printf "#Scout configuration for qmake and make\n\n" > Scout.config
printf "BOOST_DIR = $boost_dir\n" >> Scout.config
printf "\nLLVM_DIR = $llvm_dir\n" >> Scout.config
if [ -n "$llvm_lib_dir" ]; then
  printf "LLVM_LIB_DIR = $llvm_lib_dir\n" >> Scout.config
fi
printf "\n#End of configuration\n" >> Scout.config


printf "#Scout configuration for cmake\n\n" > clangAddons/CMakeLists.config
printf "set(BOOST_DIR $boost_dir)\n" >> clangAddons/CMakeLists.config
printf "\nset(LLVM_DIR $llvm_dir)\n" >> clangAddons/CMakeLists.config
if [ -n "$llvm_lib_dir" ]; then
  printf "set(LLVM_LIB_DIR $llvm_lib_dir)\n" >> clangAddons/CMakeLists.config
fi
printf "\n#End of configuration\n" >> clangAddons/CMakeLists.config

exit 0
