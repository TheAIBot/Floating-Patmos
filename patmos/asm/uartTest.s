        .word 36
        nop
        addl r26 = r0, 0x41424344
        callnd uart1
        addi r26 = r0, 66
        callnd uart1
        addi r26 = r0, 67
        callnd uart1
        halt
		nop
		nop
		nop

		.word   28
#Send r26 to uart
#Wait for uart to be ready
uart1:	add 	r28 = r0, 0xf0080000
x2:		lwl     r27  = [r28 + 0]
		nop
		btest p1 = r27, r0
	(!p1)	brnd	x2
# Write r26 to uart
		swl	[r28 + 1] = r26
		sri r26 = r26, 8
		swl	[r28 + 1] = r26
		sri r26 = r26, 8
		swl	[r28 + 1] = r26
		sri r26 = r26, 8
		swl	[r28 + 1] = r26
        retnd
		
