/*
 * linker script for FH86xx with GNU ld
 * bernard.xiong 2009-10-14
 */

INCLUDE fw_git_version.ld

/* Program Entry, set to mark it as "used" and avoid gc */
MEMORY
{
   /* CODE (rwx) : ORIGIN = 0xa7f00000, LENGTH = 1M */ /* 512KB flash */
}
ENTRY(_reset)
_system_stack_size = 0x2000;

SECTIONS
{
    . = TextBase;
    .text :
    {
        . = ALIGN(4);
        _stext = .;
        KEEP(*(.isr_vector))            /* Startup code */
        . = ALIGN(4);
		*(.text)
        *(.text.*)                      /* remaining code */
        *(.rodata)                      /* read-only data (constants) */
        *(.rodata*)
        *(.glue_7)
        *(.glue_7t)
        *(.gnu.linkonce.t*)

        /* section information for finsh shell */
        . = ALIGN(4);
        __fsymtab_start = .;
        KEEP(*(FSymTab))
        __fsymtab_end = .;
        . = ALIGN(4);
        __vsymtab_start = .;
        KEEP(*(VSymTab))
        __vsymtab_end = .;
        . = ALIGN(4);

        /* section information for RPC */
        . = ALIGN(4);
        __xbus_rpc_start = .;
        KEEP(*(RPCSymTab))
        __xbus_rpc_end = .;
        . = ALIGN(4);

        /* section information for initial. */
        . = ALIGN(4);
        __rt_init_start = .;
        KEEP(*(SORT(.rti_fn*)))
        __rt_init_end = .;

        /* section information for modules */
        . = ALIGN(4);
        __rtmsymtab_start = .;
        KEEP(*(RTMSymTab))
        __rtmsymtab_end = .;

        . = ALIGN(4);
        _etext = .;
    }

    .sdata :
    {
		__SDATA_BEGIN__ = .;
    	*(.sdata .sdata.*)
        *(.sdata*)
    }

    .data : /*sunh AT (_sidata) */
    {
        . = ALIGN(4);
        /* This is used by the startup in order to initialize the .data secion */
        _sdata = . ;

        *(.data)
        *(.data.*)
        *(.gnu.linkonce.d*)

        . = ALIGN(4);
        /* This is used by the startup in order to initialize the .data secion */
        _edata = . ;
    }
    /*} >DATA*/


    .sensor :
    {
        . = ALIGN(4);
        _sensor_drv_start = .;
        KEEP(*.a:(.sensor_drv));
        _sensor_drv_end   = .;
    }

    __bss_start = .;

    .sbss :
    {
    	*(.sbss *.sbss.*)
        *(.sbss*)
        /* . = ALIGN(128);       */
        *(_uncached)
    }

    .bss :
    {
        /* . = ALIGN(128);      */
        /* This is used by the startup in order to initialize the .bss secion */
        _sbss = .;

        *(.bss)
        *(.bss.*)
        *(COMMON)

        . = ALIGN(4);
        /* This is used by the startup in order to initialize the .bss secion */
        _ebss = . ;

        *(.bss.init)

        . = ALIGN(4);
    }
    /* __bss_end must be aligned to 4 bytes. */
    __bss_end = .;

    .stack :
    {
        . = ALIGN(4);
        __sstack = .;
        . = . + _system_stack_size;
        . = ALIGN(4);
        _estack = .;
    }

    /* __bss_start must be aligned to 4 bytes. */
    .init_datum :
	{
	    *(.runlog_data)
        *(.timer_init_data)
		*(.vbus_share_data)
	}

    _end = .;
    __sys_heap_start = .;
    /* Stabs debugging sections.  */
    .stab          0 : { *(.stab) }
    .stabstr       0 : { *(.stabstr) }
    .stab.excl     0 : { *(.stab.excl) }
    .stab.exclstr  0 : { *(.stab.exclstr) }
    .stab.index    0 : { *(.stab.index) }
    .stab.indexstr 0 : { *(.stab.indexstr) }
    .comment       0 : { *(.comment) }
    /* DWARF debug sections.
     * Symbols in the DWARF debugging sections are relative to the beginning
     * of the section so we begin them at 0.  */
    /* DWARF 1 */
    .debug          0 : { *(.debug) }
    .line           0 : { *(.line) }
    /* GNU DWARF 1 extensions */
    .debug_srcinfo  0 : { *(.debug_srcinfo) }
    .debug_sfnames  0 : { *(.debug_sfnames) }
    /* DWARF 1.1 and DWARF 2 */
    .debug_aranges  0 : { *(.debug_aranges) }
    .debug_pubnames 0 : { *(.debug_pubnames) }
    /* DWARF 2 */
    .debug_info     0 : { *(.debug_info .gnu.linkonce.wi.*) }
    .debug_abbrev   0 : { *(.debug_abbrev) }
    .debug_line     0 : { *(.debug_line) }
    .debug_frame    0 : { *(.debug_frame) }
    .debug_str      0 : { *(.debug_str) }
    .debug_loc      0 : { *(.debug_loc) }
    .debug_macinfo  0 : { *(.debug_macinfo) }
    /* SGI/MIPS DWARF 2 extensions */
    .debug_weaknames 0 : { *(.debug_weaknames) }
    .debug_funcnames 0 : { *(.debug_funcnames) }
    .debug_typenames 0 : { *(.debug_typenames) }
    .debug_varnames  0 : { *(.debug_varnames) }
}
