========================
Machine IR Profile (MIP)
========================

.. contents::
   :local:

Introduction
============
MIP is an instrumentation framework for collecting runtime statistics and using
the profiles for analysis or as input to the compiler for optimization. The
metrics we collect include function call count, function call order, and a
sample of the dynamic call graph. There is also a lightweight mode where
function coverage and basic block coverage are collected with minimal impact
on the performance and size of the instrumented binaries. MIP is currently
implemented for both ELF and MachO with the following architectures:

* x86_64
* aarch64
* armv7 (thumb1 and thumb2)

MIP uses three file types (``.mip``, ``.mipmap``, ``.mipraw``) which are
described in :doc:`MIPBinaryFormat <MIPBinaryFormat>`.

Motivation
----------
Existing instrumentation frameworks such as :doc:`XRay <XRay>` and
`Clang Coverage <https://clang.llvm.org/docs/SourceBasedCodeCoverage.html>`
produce instrumented binaries that are too large or too slow to be used in
many scenarios without sacrificing runtime behavior. Our goal for MIP is to
instrument binaries with minimal size overhead.

Usage
=====
Building
--------
To build an instrumented binary, use the ``-fmachine-profile-generate`` clang
flag.

.. code-block:: console

  $ clang -fmachine-profile-generate main.cpp


Running
-------
Run the instrumented binary like normal to cover as many use cases as
possible. When the program exits the default behavior is to write a raw
profile to ``default.mipraw``. This path can be overwritten with the
``LLVM_MIP_PROFILE_FILENAME`` environment variable.

.. code-block:: console

  $ LLVM_MIP_PROFILE_FILENAME=/path/to/profile.mipraw ./a.out


Post Processing
---------------
After collecting one or more raw profiles (``.mipraw`` files) we can merge
them into one profile (``.mip`` file) for analysis or optimization. First, we
need to extract and create an empty profile from our instrumented binary.

.. code-block:: console

  $ llvm-objcopy --dump-section=__llvm_mipmap=default.mipmap <binary> /dev/null
  $ llvm-mipdata create -p default.mip default.mipmap

Then we can use the :doc:`llvm-mipdata <CommandGuide/llvm-mipdata>` tool to
merge our raw profiles.

.. code-block:: console

  $ llvm-mipdata merge -p default.mip default.mipraw [default2.mipraw ...]

The resulting ``default.mip`` file can be used as input to the
``-fmachine-profile-use=default.mip`` frontent flag, or by the
:doc:`llvm-mipdata <CommandGuide/llvm-mipdata>` tool for analysis.

.. code-block:: console

  $ llvm-mipdata show -p default.mip
