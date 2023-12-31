.. -*- coding: utf-8; mode: rst -*-

.. _dmx_types:

****************
Demux Data Types
****************

Output for the demux
====================

.. c:type:: dmx_output

.. tabularcolumns:: |p{5.0cm}|p{12.5cm}|

.. flat-table:: enum dmx_output
    :header-rows:  1
    :stub-columns: 0


    -  .. row 1

       -  ID

       -  Description

    -  .. row 2

       -  .. _DMX-OUT-DECODER:

	  DMX_OUT_DECODER

       -  Streaming directly to decoder.

    -  .. row 3

       -  .. _DMX-OUT-TAP:

	  DMX_OUT_TAP

       -  Output going to a memory buffer (to be retrieved via the read
	  command). Delivers the stream output to the demux device on which
	  the ioctl is called.

    -  .. row 4

       -  .. _DMX-OUT-TS-TAP:

	  DMX_OUT_TS_TAP

       -  Output multiplexed into a new TS (to be retrieved by reading from
	  the logical DVR device). Routes output to the logical DVR device
	  ``/dev/dvb?.dvr?``, which delivers a TS multiplexed from
	  all filters for which ``DMX_OUT_TS_TAP`` was specified.

    -  .. row 5

       -  .. _DMX-OUT-TSDEMUX-TAP:

	  DMX_OUT_TSDEMUX_TAP

       -  Like :ref:`DMX_OUT_TS_TAP <DMX-OUT-TS-TAP>` but retrieved
	  from the DMX device.


dmx_input_t
===========

.. c:type:: dmx_input

.. code-block:: c

    typedef enum
    {
	DMX_IN_FRONTEND, /* Input from a front-end device.  */
	DMX_IN_DVR       /* Input from the logical DVR device.  */
    } dmx_input_t;


dmx_pes_type_t
==============

.. c:type:: dmx_pes_type


.. code-block:: c

    typedef enum
    {
	DMX_PES_AUDIO0,
	DMX_PES_VIDEO0,
	DMX_PES_TELETEXT0,
	DMX_PES_SUBTITLE0,
	DMX_PES_PCR0,

	DMX_PES_AUDIO1,
	DMX_PES_VIDEO1,
	DMX_PES_TELETEXT1,
	DMX_PES_SUBTITLE1,
	DMX_PES_PCR1,

	DMX_PES_AUDIO2,
	DMX_PES_VIDEO2,
	DMX_PES_TELETEXT2,
	DMX_PES_SUBTITLE2,
	DMX_PES_PCR2,

	DMX_PES_AUDIO3,
	DMX_PES_VIDEO3,
	DMX_PES_TELETEXT3,
	DMX_PES_SUBTITLE3,
	DMX_PES_PCR3,

	DMX_PES_OTHER
    } dmx_pes_type_t;


struct dmx_filter
=================

.. c:type:: dmx_filter

.. code-block:: c

     typedef struct dmx_filter
    {
	__u8  filter[DMX_FILTER_SIZE];
	__u8  mask[DMX_FILTER_SIZE];
	__u8  mode[DMX_FILTER_SIZE];
    } dmx_filter_t;


.. c:type:: dmx_sct_filter_params

struct dmx_sct_filter_params
============================


.. code-block:: c

    struct dmx_sct_filter_params
    {
	__u16          pid;
	dmx_filter_t   filter;
	__u32          timeout;
	__u32          flags;
    #define DMX_CHECK_CRC       1
    #define DMX_ONESHOT         2
    #define DMX_IMMEDIATE_START 4
    #define DMX_KERNEL_CLIENT   0x8000
    };


struct dmx_pes_filter_params
============================

.. c:type:: dmx_pes_filter_params

.. code-block:: c

    struct dmx_pes_filter_params
    {
	__u16          pid;
	dmx_input_t    input;
	dmx_output_t   output;
	dmx_pes_type_t pes_type;
	__u32          flags;
#ifdef CONFIG_AMLOGIC_DVB_COMPAT
/*bit 8~15 for mem sec_level*/
#define DMX_MEM_SEC_LEVEL1   (1 << 10)
#define DMX_MEM_SEC_LEVEL2   (1 << 11)
#define DMX_MEM_SEC_LEVEL3   (1 << 12)

/*bit 16~23 for output */
#define DMX_ES_OUTPUT        (1 << 16)
/*set raw mode, it will send the struct dmx_sec_es_data, not es data*/
#define DMX_OUTPUT_RAW_MODE	 (1 << 17)

/*24~31 one byte for audio type, dmx_audio_format_t*/
#define DMX_AUDIO_FORMAT_BIT 24
#endif
    };


struct dmx_event
================

.. c:type:: dmx_event

.. code-block:: c

     struct dmx_event
     {
	 dmx_event_t          event;
	 time_t               timeStamp;
	 union
	 {
	     dmx_scrambling_status_t scrambling;
	 } u;
     };


struct dmx_stc
==============

.. c:type:: dmx_stc

.. code-block:: c

    struct dmx_stc {
	unsigned int num;   /* input : which STC? 0..N */
	unsigned int base;  /* output: divisor for stc to get 90 kHz clock */
	__u64 stc;      /* output: stc in 'base'*90 kHz units */
    };


struct dmx_caps
===============

.. c:type:: dmx_caps

.. code-block:: c

     typedef struct dmx_caps {
	__u32 caps;
	int num_decoders;
    } dmx_caps_t;


enum dmx_source
===============

.. c:type:: dmx_source

.. code-block:: c

    typedef enum dmx_source {
	DMX_SOURCE_FRONT0 = 0,
	DMX_SOURCE_FRONT1,
	DMX_SOURCE_FRONT2,
	DMX_SOURCE_FRONT3,
	DMX_SOURCE_DVR0   = 16,
	DMX_SOURCE_DVR1,
	DMX_SOURCE_DVR2,
	DMX_SOURCE_DVR3
    } dmx_source_t;

enum dmx_input_source
=====================
.. c:type:: dmx_input_source

.. code-block:: c

typedef enum dmx_input_source {
       INPUT_DEMOD,
       INPUT_LOCAL,
       INPUT_LOCAL_SEC
} dmx_input_source_t;

struct dmx_non_sec_es_header
============================
.. c:type:: dmx_non_sec_es_header

.. code-block:: c

struct dmx_non_sec_es_header {
       __u8 pts_dts_flag;
       __u64 pts;
       __u64 dts;
       __u32 len;
};

struct dmx_sec_es_data
======================
.. c:type:: dmx_sec_es_data

.. code-block:: c

struct dmx_sec_es_data {
       __u8 pts_dts_flag;
       __u64 pts;
       __u64 dts;
       __u32 buf_start;
       __u32 buf_end;
       __u32 data_start;
       __u32 data_end;
};
