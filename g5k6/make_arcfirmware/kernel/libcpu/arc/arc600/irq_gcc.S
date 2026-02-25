/*
 * File      : irq_gcc.S
 *
 * irq vector and handler
 *
 */

.file       "irq_gcc.S"
.include    "macro.S"

.global		mem_err_handler
.global		ins_err_handler
.global     vector_irq_3
.global     vector_irq_4
.global     vector_irq_5
.global     vector_irq_6
.global     vector_irq_7
.global     vector_irq_8
.global     vector_irq_9
.global     vector_irq_10
.global     vector_irq_11
.global     vector_irq_12
.global     vector_irq_13
.global     vector_irq_14
.global     vector_irq_15
.global     vector_irq_16
.global     vector_irq_17
.global     vector_irq_18
.global     vector_irq_19
.global     vector_irq_20
.global     vector_irq_21
.global     vector_irq_22
.global     vector_irq_23
.global     vector_irq_24
.global     vector_irq_25
.global     vector_irq_26
.global     vector_irq_27
.global     vector_irq_28
.global     vector_irq_29
.global     vector_irq_30
.global     vector_irq_31

mem_err_handler:
	_push_register
	mov 	%r0,	%sp
	jl		rt_hw_trap_memerr

ins_err_handler:
	_push_register
	mov 	%r0,	%sp
	jl		rt_hw_trap_inserr

vector_irq_3:
	_push_isr_regs 1
	_push_task_type 1
	mov r0, sp
	mov	%sp, _estack ; switch to interrupt stack
	st.aw r0, [sp, -4] ; save sp into interrupt stack
	jl		rt_interrupt_enter
	mov 	%r0,	%ilink2
	;j		vector_irq


/* add more vector_irq here */


/* level 1 interrupt handler */
vector_irq:
	jl		rt_hw_trap_irq
	jl		rt_interrupt_leave

	ld.ab   r0, [sp, +4]
	mov     sp, r0 ; switch to thread_stack
    sr     3, [0x43]
	/* if rt_thread_switch_interrupt_flag == 1, jump to		*/
	/* rt_hw_context_switch_interrupt_do and never return	*/
	ld  	r0, [rt_thread_switch_interrupt_flag]
	brne  	r0, 0, rt_hw_context_switch_interrupt_do

	/* without context switch	*/
	_pop_task_type
	_pop_isr_regs 1
	j.f 	[ilink1]
	nop_s

/* do context swtich */
rt_hw_context_switch_interrupt_do:
	mov_s   r0, 0
	st  	r0, [rt_thread_switch_interrupt_flag]	/* clear flag */

	ld  	r2, [rt_interrupt_from_thread]
	ld		r3, [rt_interrupt_to_thread]

	st      sp, [r2]	/* store sp in preempted (from) tasks TCB */
	ld		sp, [r3]	/* get new (to) task stack pointer */

	 _pop_task_type     /* r3 = task_type */

    mov     r2, TASK_TYPE_0
    cmp     r3, r2
    beq     _to_task_type0

_to_task_type1:
    _pop_isr_regs 1     /* to interrupt task */
    j.f     [ilink1]
    nop_s

_to_task_type0:
    _pop_task_regs      /* to normal task */
    j       [blink]
    nop_s
