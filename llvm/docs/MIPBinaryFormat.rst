======================================
Machine IR Profile (MIP) Binary Format
======================================

.. contents::
  :local:

Introduction
============
MIP, which is described in :doc:`MachineProfile <MachineProfile>`, uses three file types to hold
profile data: ``.mip``, ``.mipmap``, ``.mipraw``. Each format has a common
header and the data is layed out in little endian format.

Header Format
=============

.. code-block:: c

  Header {
    i8[4] : Magic = {0xFB, 'M', 'I', 'P'}  // 0xFB 0x4D 0x49 0x50
    i16 : Version = 0x8
    i16 : FileType
    i32 : ProfileType
    i32 : ModuleHash                       // MD5Hash(LinkUnitName)
    i64 : RawSectionOffset                 // Relative to the map section
    i32 : Reserved
    i32 : OffsetToData = 0x20              // 32 bytes
  }

``FileType``
------------
=========== ======
Type        Value
----------- ------
Raw         0x0001
Ret         0x0002
Map         0x0004
Profile     0x0008
64 Bit      0x0010
32 Bit      0x0020
=========== ======

``ProfileType``
---------------
============================== ==========
Type                           Value
------------------------------ ----------
Function Coverage              0x00000001
Block Coverage                 0x00000002
Function Timestamp             0x00000004
Function Call Count            0x00000008
Return Address Instrumentation 0x00000010
============================== ==========

``.mipraw``
===========
If block coverage is not enabled then ``BlockCount`` is zero for all profiles
and padding is not necessary.

Function Coverage
-----------------
.. code-block:: c

  Header
  RawProfile[] {
    i8 : IsCovered
    i8[BlockCount] : IsCovered
    // Pad to 8 bits
  }

Function Timestamp + Function Call Count
----------------------------------------
.. code-block:: c

  Header
  RawProfile[] {
    i32 : CallCount
    i32 : Timestamp
    i8[BlockCount] : IsCovered
    // Pad to 32 bits
  }

``.mipmap``
===========
.. code-block:: c

  Header
  FunctionProfile[] {
    i64 : RawProfilePCOffset
    i64 : FunctionPCOffset
    i32 : FunctionSize
    i32 : ControlFlowGraphSignature
    i32 : NonEntryBlockCount
    BlockProfile[NonEntryBlockCount] {
      i32 : Offset
    }
    i32 : FunctionNameLength
    char[FunctionNameLength] : FunctionName
    // Pad to 64 bits
  }

``.mip``
========

.. code-block:: c

  Header
  i64 : ProfileCount
  FunctionProfile[ProfileCount] {
    i64 : FunctionSignature  // MD5Hash(FunctionName)
    i32 : RawProfileDataOffset
    i32 : FunctionStartOffset
    i32 : FunctionSize
    i32 : ControlFlowGraphSignature
    i32 : NonEntryBlockCount
    i32 : MergeCount
    i64 : CallCount
    i64 : TimestampSum
    BlockProfile[NonEntryBlockCount] {
      i32 : Offset
      i8  : IsCovered
    }
    i32 : CallEdgeCount
    CallEdges[CallEdgeCount] {
      // Reserved
    }
  }
  i64 : FunctionNamesLength
  char[FunctionNamesLength] : FunctionNames // nullbyte separated
