;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
 ;;;;;;;;;     file :boot for init arc

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

  .file "basecode.s"
  .include "macro_mcu_pd.S"

  .section .isr_vector,"ax",@progbits
  .global _reset

  .type _reset, @function
  .extern _arc_handler, _arm_handler, _isr_handler, _firq_handler
  .extern _bss_start, _bss_end, main
  .extern mem_err_handler, ins_err_handler

; .equ sys_log_base, 0xa7f80000
; .equ sys_log_size, 0x00080000

.align 8
_reset:     b _dummy_start
            nop

.align 8
mem_err: 	JAL mem_err_handler   	;vector 1

.align 8
ins_err: 	JAL  ins_err_handler   	;vector 2

.align 8
i_rsvd3:  	
    mov %ilink2, 3
    b vector_irq_3

.align 8
i_rsvd4:  	
    mov %ilink2, 4
    b vector_irq_3

.align 8
i_rsvd5:
    mov %ilink2, 5
  	b vector_irq_3

.align 8
i_rsvd6:  	
    mov %ilink2, 6
    b vector_irq_3

.align 8
i_rsvd7: 
    mov %ilink2, 7
 	b vector_irq_3

.align 8
i_rsvd8:  
    mov %ilink2, 8
	b vector_irq_3

.align 8
i_rsvd9:
    mov %ilink2, 9
  	b vector_irq_3

.align 8
i_rsvd10:
    mov %ilink2, 10
  	b vector_irq_3

.align 8
i_rsvd11:  
    mov %ilink2, 11
	b vector_irq_3

.align 8
i_rsvd12:  	
    mov %ilink2, 12
    b vector_irq_3

.align 8
i_rsvd13:  	
    mov %ilink2, 13
    b vector_irq_3

.align 8
i_rsvd14:
    mov %ilink2, 14
  	b vector_irq_3

.align 8
i_rsvd15:
    mov %ilink2, 15
  	b vector_irq_3

.align 8
i_rsvd16:
    mov %ilink2, 16
  	b vector_irq_3

.align 8
i_rsvd17: 
    mov %ilink2, 17
 	b vector_irq_3

.align 8
i_rsvd18:  	
    mov %ilink2, 18
    b vector_irq_3

.align 8
i_rsvd19:  	
    mov %ilink2, 19
    b vector_irq_3

.align 8
i_rsvd20:  
    mov %ilink2, 20
	b vector_irq_3

.align 8
i_rsvd21:
    mov %ilink2, 21
  	b vector_irq_3

.align 8
i_rsvd22: 
    mov %ilink2, 22
 	b vector_irq_3

.align 8
i_rsvd23:  
    mov %ilink2, 23
	b vector_irq_3

.align 8
i_rsvd24:  	
    mov %ilink2, 24
    b vector_irq_3

.align 8
i_rsvd25:  	
    mov %ilink2, 25
    b vector_irq_3

.align 8
i_rsvd26:  	
    mov %ilink2, 26
    b vector_irq_3

.align 8
i_rsvd27:  
    mov %ilink2, 27
	b vector_irq_3

.align 8
i_gpio28:  
    mov %ilink2, 28
	b vector_irq_3

.align 8
i_gpio29:  
    mov %ilink2, 29
	b vector_irq_3

.align 8
i_tbd30: 
    mov %ilink2, 30
 	b vector_irq_3

.align 8
i_tbd31:  	
    mov %ilink2, 31
    b vector_irq_3

; don't modify the following code...
.global _arc_fw_info
.global _arc_fw_magic
.global _arc_fw_sdk_git_version
.global _arc_fw_compile_entry
.global _arc_fw_load_addr
.global _arc_fw_boot_args
.global _arc_fw_bootargs_size
.global _arc_fw_xbus_sysinfo
.global _arc_fw_ready_flag
_arc_fw_info:
_arc_fw_magic:
    .word 0x5A5A5A5A      ; Magic Number
_arc_fw_sdk_git_version:
    .word _sdk_git_version; ARC firmware sdk git version
_arc_fw_compile_entry:
    .word _reset          ; ARC firmware compile entry
_arc_fw_load_addr:
    .word 0
_arc_fw_boot_args:
    .word 0               ; ARC firmware bootargs memory address
_arc_fw_bootargs_size:
    .word 0               ; ARC firmware bootargs memory size
_arc_fw_xbus_sysinfo:
    .word 0               ; Address for xbus_sys_info...
_arc_fw_ready_flag:
    .word 0               ; is ARC ready?
_arc_fw_run_times:        ;
    .word 0               ; is ARC ready?

_dummy_start:
    mov	  r2, _arc_fw_run_times
    ld.di r0, [r2]
    add   r1, r0, 1
    cmp   r1, 0
    bne   _update_cnt
    nop
    add   r1, r1, 1
_update_cnt:
    st.di r1, [r2]
    cmp r0, 0
    bne _ddr_retention_bootup
    nop

    lr    r0, [pc]
    and   %r0, %r0, 0xfffff000	;4KB aligned, now r0 must be the load address...
    mov	  %r1, _arc_fw_load_addr
    st.di %r0, [%r1]
    JAL _start
    nop
 
 _start:
	mov		%sp, _estack					; initialize stack pointer
	and 	%sp, %sp, 0xfffffffc			; make stack 4-bytes aligned.
	sub		%sp, %sp, 16					; allocate expected call frame
	mov		%gp, __SDATA_BEGIN__			; initialize small-data base register
	mov		%fp, 0							; initialize frame pointer

	;;; set the TBR
	sr     _reset, [0x25]                   ; reset TBR
	sr     0, [0x200]                       ; set all Level 1 isr.

	;;; enable D-cache.
	lr 		%r0, [0x48]
	and 	%r0, %r0, -2 ;;;enable  D-cache
	;;;or 	    %r0, %r0, 1  ;;;disable D-cache ;;;FIXME
	sr 		%r0, [0x48]

	;;; enable I-cache.
	lr 		%r0, [0x11]
	and 	%r0, %r0, -2
	sr		%r0, [0x11]

    ; clear system stack.
    mov     %r0, __sstack
    mov     %r1, _estack
    mov     %r2, 0xdeadbeef
clear_stack:
    st      %r2, [%r0]
    add     %r0, %r0, 4
    brlo    %r0, %r1, clear_stack

	; clear bss section.
	mov 	%r0, __bss_start
	mov 	%r1, __bss_end
	breq	%r0, %r1, 1f
	mov 	%r2, 0
clear_bss:
	st		%r2, [%r0]
	add		%r0, %r0, 4
	brlo	%r0, %r1, clear_bss
1:
    ; do performance test.
	; clear log area
.if CLEAR_SYS_LOG == 1
	mov    %r0, sys_log_base
	add    %r1, %r0, sys_log_size
	mov    %r2, 0
clear_log:
	st     %r2, [%r0]
	add    %r0, %r0, 4
	brlo   %r0, %r1, clear_log
.endif

2:
    j 	   rtthread_startup

    nop
    nop
    nop

_ddr_retention_bootup:
    ;;; set the TBR
    sr     _reset, [0x25]                   ; reset TBR
    sr     0, [0x200]                       ; set all Level 1 isr.

    ;;; enable D-cache.
    lr 		%r0, [0x48]
    and 	%r0, %r0, -2
    sr 		%r0, [0x48]
    ;;; enable I-cache.
    lr 		%r0, [0x11]
    and 	%r0, %r0, -2
    sr		%r0, [0x11]

    mov     r0, _estack
    sub     r0, r0, 4
    ld      sp, [r0]

    _pop_mcu_pd_regs          ;restore stack
    mov     %sp, %ilink2 ;restore SP from ilink2

    mov     r0, 0 ;return 0
    j_s	[blink]
    nop_s
    nop_s
    nop_s

  .global machine_shutdown
  .type machine_shutdown, @function
machine_shutdown:
    flag 1
    nop
    nop
    nop
    b machine_shutdown
    nop
