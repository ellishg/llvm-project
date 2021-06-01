llvm-mipdata - MIP tool
=======================

.. program:: llvm-mipdata

SYNOPSIS
--------

:program:`llvm-mipdata` *command* [*args...*]

DESCRIPTION
-----------

The :program:`llvm-mipdata` tool creates, manipulates, and reports
information about :doc:`MIP <../MachineProfile>` profiles (``.mip`` files).

General Options
---------------

.. option:: --help

  Print a summary of command line options.

.. option:: --profile=<profile>, -p <profile>

  Specify the profile to work on.

.. option:: --output=<output>, -o <output>

  Specify the output file name.

.. option:: --version

  Print the current version of this tool.


COMMANDS
--------

* :ref:`create <create>`
* :ref:`merge <merge>`
* :ref:`show <show>`

.. _create:

CREATE
------

SYNOPSIS
^^^^^^^^

:program:`llvm-mipdata create` [*options*] **-p <profile>** <map_file>

DESCRIPTION
^^^^^^^^^^^

:program:`llvm-mipdata create` creates an empty <profile> from a <map_file>
(``.mipmap`` file).

.. _merge:

MERGE
-----

SYNOPSIS
^^^^^^^^

:program:`llvm-mipdata merge` [*options*] **-p <profile>** <raw_files>

DESCRIPTION
^^^^^^^^^^^

:program:`llvm-mipdata merge` merges raw profiles (``.mipraw`` files) into the
profile.

OPTIONS
^^^^^^^

.. option:: --strict

  With strict mode, merging a corrupt raw profile will fail the command.
  Disabled by default.

.. _show:

SHOW
----

SYNOPSIS
^^^^^^^^

:program:`llvm-mipdata show` [*options*] **-p <profile>**

DESCRIPTION
^^^^^^^^^^^

:program:`llvm-mipdata show` prints the data from the given profile.

OPTIONS
^^^^^^^

.. option:: --debug=<debug_info>, -d <debug_info>

  Use <debug_info> to symbolicate addresses in the profile.

.. option:: --regex=<regex>, -r <regex>

  Only process function names that match <regex>.


EXAMPLE
-------

.. code-block:: console

  $ llvm-mipdata create -p default.mip default.mipmap
  $ llvm-mipdata merge -p default.mip default-0000.mipraw default-0001.mipraw default-0002.mipraw
  $ llvm-mipdata show -p default.mip --debug a.out
