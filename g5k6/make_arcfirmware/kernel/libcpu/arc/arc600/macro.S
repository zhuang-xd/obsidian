/*
 * File      : macro.S
 *
 * Macro definition
 *
 *  _push_task_regs
 *  _pop_task_regs
 *  _push_int_regs
 *  _pop_int_regs
 *  _pop_isr_regs 1/2
 *  _push_isr_regs 1/2
 *
 *
 *  _push_register
 */

    .file "macro.S"

.set TASK_TYPE_0, 0x4b534154
.set TASK_TYPE_1, 0x314b4e4c
.set TASK_TYPE_2, 0x324b4e4c

.macro _push_task_regs
    st.aw r31, [sp, -4]
    st.aw r27, [sp, -4]
    st.aw r25, [sp, -4]
    st.aw r24, [sp, -4]
    st.aw r23, [sp, -4]
    st.aw r22, [sp, -4]
    st.aw r21, [sp, -4]
    st.aw r20, [sp, -4]
    st.aw r19, [sp, -4]
    st.aw r18, [sp, -4]
    st.aw r17, [sp, -4]
    st.aw r16, [sp, -4]
    st.aw r15, [sp, -4]
    st.aw r14, [sp, -4]
    st.aw r13, [sp, -4]
.endm

.macro _pop_task_regs
    ld.ab r13, [sp, +4]
    ld.ab r14, [sp, +4]
    ld.ab r15, [sp, +4]
    ld.ab r16, [sp, +4]
    ld.ab r17, [sp, +4]
    ld.ab r18, [sp, +4]
    ld.ab r19, [sp, +4]
    ld.ab r20, [sp, +4]
    ld.ab r21, [sp, +4]
    ld.ab r22, [sp, +4]
    ld.ab r23, [sp, +4]
    ld.ab r24, [sp, +4]
    ld.ab r25, [sp, +4]
    ld.ab r27, [sp, +4]
    ld.ab r31, [sp, +4]
.endm

.macro _push_int_regs
    st.aw r31, [sp, -4]
    st.aw r27, [sp, -4]
    ;;st.aw r26, [sp, -4]
    st.aw r25, [sp, -4]
    st.aw r24, [sp, -4]
    st.aw r23, [sp, -4]
    st.aw r22, [sp, -4]
    st.aw r21, [sp, -4]
    st.aw r20, [sp, -4]
    st.aw r19, [sp, -4]
    st.aw r18, [sp, -4]
    st.aw r17, [sp, -4]
    st.aw r16, [sp, -4]
    st.aw r15, [sp, -4]
    st.aw r14, [sp, -4]
    st.aw r13, [sp, -4]
    st.aw r12, [sp, -4]
    st.aw r11, [sp, -4]
    st.aw r10, [sp, -4]
    st.aw r9, [sp, -4]
    st.aw r8, [sp, -4]
    st.aw r7, [sp, -4]
    st.aw r6, [sp, -4]
    st.aw r5, [sp, -4]
    st.aw r4, [sp, -4]
    st.aw r3, [sp, -4]
    st.aw r2, [sp, -4]
    st.aw r1, [sp, -4]
    st.aw r0, [sp, -4]
 	st.aw r57, [sp, -4]
 	st.aw r56, [sp, -4]
 	lr    r0,  [0x9f]
 	st.aw r0,  [sp, -4]
 	lr    r0,  [0xa0]
 	st.aw r0,  [sp, -4]
 	lr    r0,  [0x41]
 	st.aw r0,  [sp, -4]
.endm

.macro _pop_int_regs
    ld.ab r0, [sp, +4]
    sr    r0, [0x41]
    ld.ab r0, [sp, +4]
    sr    r0, [0xa0]
    ld.ab r0, [sp, +4]
    sr    r0, [0x9f]
 	ld.ab r0, [sp, +4]
 	mov r56, r0
 	ld.ab r0, [sp, +4]
 	mov r57, r0
    ld.ab r0, [sp, +4]
    ld.ab r1, [sp, +4]
    ld.ab r2, [sp, +4]
    ld.ab r3, [sp, +4]
    ld.ab r4, [sp, +4]
    ld.ab r5, [sp, +4]
    ld.ab r6, [sp, +4]
    ld.ab r7, [sp, +4]
    ld.ab r8, [sp, +4]
    ld.ab r9, [sp, +4]
    ld.ab r10, [sp, +4]
    ld.ab r11, [sp, +4]
    ld.ab r12, [sp, +4]
    ld.ab r13, [sp, +4]
    ld.ab r14, [sp, +4]
    ld.ab r15, [sp, +4]
    ld.ab r16, [sp, +4]
    ld.ab r17, [sp, +4]
    ld.ab r18, [sp, +4]
    ld.ab r19, [sp, +4]
    ld.ab r20, [sp, +4]
    ld.ab r21, [sp, +4]
    ld.ab r22, [sp, +4]
    ld.ab r23, [sp, +4]
    ld.ab r24, [sp, +4]
    ld.ab r25, [sp, +4]
    ;;ld.ab r26, [sp, +4]
    ld.ab r27, [sp, +4]
    ld.ab r31, [sp, +4]
.endm


.macro _push_isr_regs, level
    st.aw ilink\level, [sp, -4]				;; push ilink1
    lr    ilink\level, [status32_l\level]
    st.aw ilink\level, [sp, -4]				;; push status_l1

    _push_int_regs

    lr    r0, [lp_start]					;; push lp_start
    st.aw r0, [sp, -4]
    lr    r0, [lp_end]
    st.aw r0, [sp, -4]						;; push lp_end
    st.aw lp_count, [sp, -4]				;; push lp_count
.endm


.macro _pop_isr_regs, level
    ld.ab r0, [sp, +4]						;; pop lp_count
    mov   lp_count, r0
    ld.ab r0, [sp, +4]
    sr    r0, [lp_end]							;; pop lp_end
    ld.ab r0, [sp, +4]
    sr    r0, [lp_start]							;; pop lp_start

    _pop_int_regs

    ld.ab ilink\level, [sp, +4]				;; pop status_l1
    sr    ilink\level, [status32_l\level]
    ld.ab ilink\level, [sp, +4]				;; pop ilink1
.endm

.macro _push_task_type, id
    mov     r3, TASK_TYPE_\id
    st.aw   r3, [sp, -4]        /* push task type */
.endm

.macro _pop_task_type
    ld.ab r3, [sp, +4]          /* pop task type to r3 */
.endm

; .macro save_int_regs
;  	st.aw ilink1, [sp, -4]				;; push ilink1
;  	lr    ilink1, [status32_l1]
;  	st.aw ilink1, [sp, -4]				;; push status_l1
; 
;  	st.aw r31, [sp, -4]			;;blink
;  	st.aw r27, [sp, -4]			;;FP
;  	st.aw r26, [sp, -4]
;  	st.aw r25, [sp, -4]
;  	st.aw r24, [sp, -4]
;  	st.aw r23, [sp, -4]
;  	st.aw r22, [sp, -4]
;  	st.aw r21, [sp, -4]
;  	st.aw r20, [sp, -4]
;  	st.aw r19, [sp, -4]
;  	st.aw r18, [sp, -4]
;  	st.aw r17, [sp, -4]
;  	st.aw r16, [sp, -4]
;  	st.aw r15, [sp, -4]
;  	st.aw r14, [sp, -4]
;  	st.aw r13, [sp, -4]
;  	st.aw r12, [sp, -4]
;  	st.aw r11, [sp, -4]
;  	st.aw r10, [sp, -4]
;  	st.aw r9, [sp, -4]
;  	st.aw r8, [sp, -4]
;  	st.aw r7, [sp, -4]
;  	st.aw r6, [sp, -4]
;  	st.aw r5, [sp, -4]
;  	st.aw r4, [sp, -4]
;  	st.aw r3, [sp, -4]
;  	st.aw r2, [sp, -4]
;  	st.aw r1, [sp, -4]
;  	st.aw r0, [sp, -4]
; 
;  	lr    r0, [lp_start]					;; push lp_start
;  	st.aw r0, [sp, -4]
;  	lr    r0, [lp_end]
;     st.aw r0, [sp, -4]						;; push lp_end
;     st.aw lp_count, [sp, -4]				;; push lp_count
; .endm
; 
; .macro restore_int_regs
;  	ld.ab r0, [sp, +4]						;; pop lp_count
;  	mov   lp_count, r0
;  	ld.ab r0, [sp, +4]
;  	sr    r0, [lp_end]							;; pop lp_end
;  	ld.ab r0, [sp, +4]
;  	sr    r0, [lp_start]							;; pop lp_start
; 
;  	ld.ab r0, [sp, +4]
;  	ld.ab r1, [sp, +4]
;  	ld.ab r2, [sp, +4]
;  	ld.ab r3, [sp, +4]
;  	ld.ab r4, [sp, +4]
;  	ld.ab r5, [sp, +4]
;  	ld.ab r6, [sp, +4]
;  	ld.ab r7, [sp, +4]
;  	ld.ab r8, [sp, +4]
;  	ld.ab r9, [sp, +4]
;  	ld.ab r10, [sp, +4]
;  	ld.ab r11, [sp, +4]
;  	ld.ab r12, [sp, +4]
;  	ld.ab r13, [sp, +4]
;  	ld.ab r14, [sp, +4]
;  	ld.ab r15, [sp, +4]
;  	ld.ab r16, [sp, +4]
;  	ld.ab r17, [sp, +4]
;  	ld.ab r18, [sp, +4]
;  	ld.ab r19, [sp, +4]
;  	ld.ab r20, [sp, +4]
;  	ld.ab r21, [sp, +4]
;  	ld.ab r22, [sp, +4]
;  	ld.ab r23, [sp, +4]
;  	ld.ab r24, [sp, +4]
;  	ld.ab r25, [sp, +4]
;  	ld.ab r26, [sp, +4]
;  	ld.ab r27, [sp, +4]
;  	ld.ab r31, [sp, +4]
; 
;  	ld.ab ilink1, [sp, +4]				;; pop status_l1
;  	sr    ilink1, [status32_l1]
;  	ld.ab ilink1, [sp, +4]				;; pop ilink1
; .endm
; 
.macro _push_register
    st.aw r31, [sp, -4]

    lr    r31, [status32_l2]
    st.aw r31, [sp, -4]				;; push status_l2
    lr    r31, [status32_l1]
    st.aw r31, [sp, -4]				;; push status_l1

    st.aw r30, [sp, -4]
    st.aw r29, [sp, -4]
    ;;st.aw r28, [sp, -4]           ;;don't push sp itself
    st.aw r27, [sp, -4]
    st.aw r26, [sp, -4]
    st.aw r25, [sp, -4]
    st.aw r24, [sp, -4]
    st.aw r23, [sp, -4]
    st.aw r22, [sp, -4]
    st.aw r21, [sp, -4]
    st.aw r20, [sp, -4]
    st.aw r19, [sp, -4]
    st.aw r18, [sp, -4]
    st.aw r17, [sp, -4]
    st.aw r16, [sp, -4]
    st.aw r15, [sp, -4]
    st.aw r14, [sp, -4]
    st.aw r13, [sp, -4]
    st.aw r12, [sp, -4]
    st.aw r11, [sp, -4]
    st.aw r10, [sp, -4]
    st.aw r9, [sp, -4]
    st.aw r8, [sp, -4]
    st.aw r7, [sp, -4]
    st.aw r6, [sp, -4]
    st.aw r5, [sp, -4]
    st.aw r4, [sp, -4]
    st.aw r3, [sp, -4]
    st.aw r2, [sp, -4]
    st.aw r1, [sp, -4]
    st.aw r0, [sp, -4]
.endm
