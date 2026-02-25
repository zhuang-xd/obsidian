.macro _push_mcu_pd_regs
    st.aw r31, [sp, -4] ;blink
    ;r30 is ilink2
    ;r29 is ilink1
    ;r28 is SP
    st.aw r30, [sp, -4] ;ilink2
    st.aw r27, [sp, -4] ;FP
    st.aw r26, [sp, -4] ;GP
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
    st.aw r9,  [sp, -4]
    st.aw r8,  [sp, -4]
    st.aw r7,  [sp, -4]
    st.aw r6,  [sp, -4]
    st.aw r5,  [sp, -4]
    st.aw r4,  [sp, -4]
    st.aw r3,  [sp, -4]
    st.aw r2,  [sp, -4]
    st.aw r1,  [sp, -4]
    st.aw r0,  [sp, -4]
    st.aw r57, [sp, -4]
    st.aw r56, [sp, -4]
    lr    r0,  [0x9f]
    st.aw r0,  [sp, -4]
    lr    r0,  [0xa0]
    st.aw r0,  [sp, -4]
    lr    r0,  [0x41]
    st.aw r0,  [sp, -4]
	
    lr    r0, [lp_start]                    ;; push lp_start
    st.aw r0, [sp, -4]
    lr    r0, [lp_end]
    st.aw r0, [sp, -4]                      ;; push lp_end
    st.aw lp_count, [sp, -4]                ;; push lp_count

    lr    r0, [0x22]   ;;timer0, CONTROL0
    st.aw r0, [sp, -4]
    lr    r0, [0x23]   ;;timer0, LIMIT0
    st.aw r0, [sp, -4]
    mov   r0, 0        ;;timer0, COUNT0,0x21
    st.aw r0, [sp, -4]
.endm

.macro _pop_mcu_pd_regs
    ld.ab r0, [sp, +4]
    sr    r0, [0x21]  ;;timer0, COUNT0,0x21
    ld.ab r0, [sp, +4]
    sr    r0, [0x23]  ;;timer0, LIMIT0
    ld.ab r0, [sp, +4]
    sr    r0, [0x22]  ;;timer0, CONTROL0

    ld.ab r0, [sp, +4]                      ;; pop lp_count
    mov   lp_count, r0
    ld.ab r0, [sp, +4]
    sr    r0, [lp_end]                          ;; pop lp_end
    ld.ab r0, [sp, +4]
    sr    r0, [lp_start]                            ;; pop lp_start
	
    ld.ab r1, [sp, +4]
    sr    r1, [0x41]
    ld.ab r1, [sp, +4]
    sr    r1, [0xa0]
    ld.ab r1, [sp, +4]
    sr    r1, [0x9f]
 	ld.ab r56, [sp, +4]
 	ld.ab r57, [sp, +4]
 	ld.ab r0, [sp, +4]
    ;;ld.ab r1, [sp, +4]
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
    ld.ab r26, [sp, +4]
    ld.ab r27, [sp, +4]
    ld.ab r30, [sp, +4]
    ld.ab r31, [sp, +4]
.endm

