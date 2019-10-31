#!/bin/bash

cd ~/linux-4.15
for d in */ ; do
	# echo $d
	cd $d
	for f in *.ll; do
		echo $f
		~/llvm-project/build/bin/llvm-as $f
	done
	cd ..
done

