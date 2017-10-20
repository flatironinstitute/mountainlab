MountainLab Documentation
=========================

MountainLab is data processing, sharing and visualization software for scientists. It is built around MountainSort, a spike sorting algorithm, but is designed to more generally applicable.

MountainLab is built in layers in order to maintain flexibility and simplicity. The bottom layer allows users to run individual processing commands from a Linux terminal. The top layers allow cloud-based data processing and sharing of analysis pipelines and results through web browser interfaces.

Installation and Getting started
--------------------------------

:doc:`installation/mountainlab_installation`

Software components
-------------------

The software comprises the following components:

* :doc:`processing_system` (mproc)

  - Manages libraries of registered processors
  - Custom processors can be written in any lanuage (C++, python, matlab/octave, etc.)

..

* :doc:`prv_system` (prv)

  - Large data files are represented using tiny .prv text files that serve as universal handles
  - Separation of huge data files from analysis workspace
  - Facilitates sharing of processing results

..

* :doc:`mda_file_format` (mda)

  - Simple binary file format for n-d arrays

..

* :doc:`processing_pipelines` (mlp)

  - Create and execute processing pipelines

..

* MountainView

  - Desktop visualization of processing results
  - (Currently specific to spike sorting)

..

* Web framework
 
  - Create, exececute and share analysis pipelines via web browser (mlpipeline)
  - Register custom processing servers (cordion, larinet, kulele)

..

Miscellaneous topics
--------------------

.. toctree::
   :maxdepth: 2

  misc/preparing_raw_data
  misc/waveform_drift
  misc/cordion_plans

  


.. Indices and tables
.. ==================

.. * :ref:`genindex`
.. * :ref:`modindex`
.. * :ref:`search`

