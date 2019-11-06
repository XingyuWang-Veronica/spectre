add_llvm_library( LLVMSpectre MODULE
	# Spectre.cpp
	# 4elisa.cpp
	# frame.cpp
	# frame2.cpp
	i-cache.cpp
	# function-pointer.cpp

	PLUGIN_TOOL
	opt
	)
