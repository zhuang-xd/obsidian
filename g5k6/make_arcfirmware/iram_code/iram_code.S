/* SPDX-License-Identifier: GPL-2.0 */
    .section .iram_data,"ax",@progbits
    .globl  iram_code
iram_code:
    .incbin ARC_BIN_FILE
    .globl  iram_code_end
iram_code_end:
