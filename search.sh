#!/bin/bash

cd ~/linux-4.15
for d in */; do
	echo $d
	cd $d
	for f in *.bc; do
		echo $f
		# test on this .bc file
		~/llvm-project/build/bin/opt -load ~/llvm-project/build/lib/LLVMSpectre.so -s < $f > /dev/null
	done
	cd ..
done
