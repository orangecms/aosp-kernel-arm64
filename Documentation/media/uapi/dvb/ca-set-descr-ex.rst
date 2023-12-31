.. -*- coding: utf-8; mode: rst -*-

.. _CA_SET_DESCR_EX:

============
CA_SET_DESCR_EX
============

Name
----

CA_SET_DESCR_EX


Synopsis
--------

.. c:function:: int ioctl(fd, CA_SET_DESCR, struct ca_descr_ex *desc)
    :name: CA_SET_DESCR_EX


Arguments
---------

``fd``
  File descriptor returned by a previous call to :c:func:`open() <cec-open>`.

``msg``
  Pointer to struct :c:type:`ca_descr_ex`.


Description
-----------

.. note:: This ioctl is undocumented. Documentation is welcome.


Return Value
------------

On success 0 is returned, on error -1 and the ``errno`` variable is set
appropriately. The generic error codes are described at the
:ref:`Generic Error Codes <gen-errors>` chapter.
