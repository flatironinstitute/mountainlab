MountainLab installation
========================

Supported operating systems
---------------------------

Linux/Ubuntu and Debian are the currently supported development platforms. Other Linux flavors should also work. There are plans to support Mac, but Windows is not currently supported. 

Prerequisites
-------------

If you are on Ubuntu 16.04 or later, you can get away with using package managers to install the prerequisites:

.. code :: bash

	sudo apt-get install software-properties-common
	sudo apt-add-repository ppa:ubuntu-sdk-team/ppa
	sudo apt-get update
	sudo apt-get install qtdeclarative5-dev qt5-default qtbase5-dev qtscript5-dev make g++

	sudo apt-get install libfftw3-dev nodejs npm
	sudo apt-get install python3 python3-pip

	# Note: you may want to use a virtualenv or other system to manage your python packages
	pip3 install numpy scipy matplotlib pybind11 cppimport

Otherwise, if you are on a different operating system, use the following links installing the prequisites:

* :doc:`Qt5 (version 5.5 or later) <qt5_installation>` 
* :doc:`FFTW <fftw_installation>`
* :doc:`NodeJS <nodejs_installation>`
* :doc:`python3 together with some packages <python installation>`
* Optional: matlab or octave

Compilation
-----------

Important: You should be a regular user when you perform this step -- do not use sudo here or your files will be owned by root.

First time:

.. code :: bash

	git clone https://github.com/magland/mountainlab.git
	cd mountainlab
	
	# check out a specific branch by name
	git checkout ms4
	
	./compile_components.sh
	# See also nogui_compile.sh to compile only the non-gui components, for example if you are running processing on a non-gui server.

Subsequent updates:

.. code :: bash

	cd mountainlab
	git pull
	./compile_components.sh


You must add mountainlab/bin to your PATH environment variable. For example append the following to your ~/.bashrc file, and open a new terminal (or, source .bashrc):

.. code :: bash

	export PATH=/path/to/mountainlab/bin:$PATH

Note: depending on how nodejs is named on your system, you may need to do the following (or something like it):

.. code :: bash

	sudo  ln -s /usr/bin/node /usr/bin/nodejs

Testing the installation
------------------------

.. code :: bash

	cd mountainlab/examples/001_kron_mountainsort

Then follow the instructions in the instructions.txt file

If you get stuck
----------------

If necessary, contact Jeremy. I'm happy to help, and we can improve the docs. I'm also happy to invite you to the slack team for troubleshooting, feedback, etc.

