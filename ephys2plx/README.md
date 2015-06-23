ephys2plx
=========

Here are some Matlab .m files and a Matlab mex function (c language) used to convert Open Ephys data files to the Plexon .plx file, which can be opened by Plexon Offline-Sorter.

#Compile mex file
The two mex files ("ephys2plx\_mex.mexmaci64" and "ephys2plx\_mex.mexw64") are compiled with Matlab 2013a, if your Matlab version is different than **2013a**, you may need to compile **ephys2plx_mex.c**

To compile the ephys2plx_mex.c file:

1. start Matlab
2. if you haven't selected a C compiler, you could execute **mex -setup** in matlab Command Window to select one.
(if you haven't installed a C compiler on your computer, probably Windows system, you could find a supported and compatible compilers for the Matlab version you are using from [MathWorks](http://www.mathworks.com). Here is a list of compatible compilers for Matlab 2014a [Supported and Compatible Compilers – Release 2014a](http://www.mathworks.com/support/compilers/R2014a/))
3. execute **mex ephys2plx_mex.c** in matlab Command Window

#Choose continuous data files to be converted

Line 1 in the **ephys2plx.m** file sets continuous data from which processor will be converted into .plx file.
For example, the raw data from FPGA will be 

ConProcessor = 'Sources/Rhythm FPGA'
