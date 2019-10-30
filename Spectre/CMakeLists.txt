add_llvm_library( LLVMSpectre MODULE
	# Spectre.cpp
	4elisa.cpp
	frame.cpp
	frame2.cpp

	PLUGIN_TOOL
	opt
	)
