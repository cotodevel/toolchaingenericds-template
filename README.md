This is the Toolchain Generic default template project:

Compile Toolchain: To compile this project you will need to follow the steps at https://bitbucket.org/Coto88/toolchaingenericds : Then simply extract the project somewhere.

Compile this project: Open msys, through msys commands head to the directory your extracted this project. Then write: make clean make

After compiling, run the example in NDS.

Project Specific description: 
The standard TGDS arm7 binary (non TGDS ARM7 custom code) donor, is taken from this project. 
For this to be effective, the ARM9 binary must be at least TGDS compatible. 
(not related to ARM9 NTR map, but rather the init code, otherwise both ARM cores will hang)

Coto
