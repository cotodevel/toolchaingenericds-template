This is the Toolchain Generic default template project:

Compile Toolchain: To compile this project you will need to follow the steps at https://bitbucket.org/Coto88/toolchaingenericds : Then simply extract the project somewhere.

Compile this project: Open msys, through msys commands head to the directory your extracted this project. Then write: make clean make

After compiling, run the example in NDS.

Project Specific description: 
ToolchainGenericDS-template adds basic TGDS functionality to a NintendoDS / Flashcart. You could use it to test your dldi driver/flashcart.
What it does: 
Y: Reads a file named filelist.txt
X: Builds a list of the current root sd card files/directories into a file named filelist.txt
L: Dump the dldi driver from the flashcart if you ran ToolchainGenericDS-template.nds from a flashcart. 
   This driver can be re-used by NDS loaders to increase support of NDS DLDI enabled homebrew for your flashcart if the native flashcart loader fails to load some NDS homebrew.
Start: Basic file browser. Supports compressing(slow)/decompressing LZSS files. (NDS Bios compatible compression)

/release folder has the latest binary precompiled for your convenience.

Many thanks to CUE for the LZSS compressor code. (And many thanks to Nintendo for the LZDD decompressor code because it is used from the NDS Bios)

Coto
