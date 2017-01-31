OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)

SECTIONS
{
	/*
	 * Locate the .start section at the origin. The boot ROM will be told to
	 * jump to address zero, and that's where we want to put the first
	 * instruction we execute.
	 */
	. = 0x40000000;
	.start : { KEEP(*(.start)) }
	
	.init_modules_section :
	{
		PROVIDE( __init_modules_start = . );
		KEEP(*(SORT(.init_modules_section.*)))
		KEEP(*(.init_modules_section))
		PROVIDE( __init_modules_end = . );
	}

	.process_section :
	{
		PROVIDE( __process_start = . );
		KEEP(*(SORT(.process_section.*)))
		KEEP(*(.process_section))
		PROVIDE( __process_end = . );
	}
	
	.power_down_section :
	{
		PROVIDE( __pdown_start = . );
		KEEP(*(SORT(.power_down_section.*)))
		KEEP(*(.power_down_section))
		PROVIDE( __pdown_end = . );
	}
	
	.device_section :
	{
		PROVIDE( __device_start = . );
		KEEP(*(SORT(.device_section.*)))
		KEEP(*(.device_section))
		PROVIDE( __device_end = . );
	}
		
	.disc_section :
	{
		PROVIDE( __disc_start = . );
		KEEP(*(SORT(.disc_section.*)))
		KEEP(*(.disc_section))
		PROVIDE( __disc_end = . );
	}
	
	.app_section :
	{
		PROVIDE( __app_start = . );
		KEEP(*(SORT(.app_section.*)))
		KEEP(*(.app_section))
		PROVIDE( __app_end = . );
		PROVIDE( __app_entry = . - 4 );
	}
	
	.cmd_section :
	{
		PROVIDE( __cmd_start = . );
		KEEP(*(SORT(.cmd_section.*)))
		KEEP(*(.cmd_section))
		PROVIDE( __cmd_end = . );
	}
	
	.list_section :
	{
		PROVIDE( __list_start = . );
		KEEP(*(SORT(.list_section.*)))
		KEEP(*(.list_section))
		PROVIDE( __list_end = . );
	}
		

    .text :
    {
	    . = ALIGN(4);
	    
        *(.text)                   /* remaining code */
        *(.text.*)                   /* remaining code */
        *(.rodata)                 /* read-only data (constants) */
        *(.rodata*)
        *(.glue_7)
        *(.glue_7t)

	    . = ALIGN(4);
		_etext = .;
		
		/* This is used by the startup in order to initialize the .data secion */
		_sidata = _etext;
    }
    
    .ARM.exidx :
    {
    	__exidx_start = . ;
    	*(.ARM.exidx* .gnu.linkonce.armexidx.*)
    	__exidx_end = . ;
    }

    .data  :
    {
		. = ALIGN(4);
		/* This is used by the startup in order to initialize the .data secion */
		_sdata = . ;
		_data = . ;
		
		*(.RAMtext)
		*(.data)
		*(.data.*)

		
		. = ALIGN(4);
		/* This is used by the startup in order to initialize the .data secion */
		_edata = . ;
    }
    
    .got_section :
    {
	    . = ALIGN(4);
		_sgot = . ;

		*(.got)

		_egot = . ;
		
		*(.got.plt)
		
	    . = ALIGN(4);
    }   
    
    .bss :
    {
		. = ALIGN(4);
		/* This is used by the startup in order to initialize the .bss secion */
		_sbss = .;
		_bss_start = .;
		_bss = .;
		
		*(.bss)
		*(COMMON)
		
		. = ALIGN(4);
		/* This is used by the startup in order to initialize the .bss secion */
		_ebss = . ;
		_bss_end = .;
    }
    
    _end = .;
	
    PROVIDE ( end = _ebss );
    PROVIDE ( _end = _ebss );
    PROVIDE(__HEAP_START = _end );
    
    
}
