        .word 36
        nop
        addi r28 = r0, 65
        callnd uart1
        addi r28 = r0, 66
        callnd uart1
        addi r28 = r0, 67
        callnd uart1
        halt
		nop
		nop
		nop

		.word   28
#Send r28 to uart
#Wait for uart to be ready
uart1:	add 	r26 = r0, 0xf0080000
x2:		lwl     r27  = [r26 + 0]
		nop
		btest p1 = r27, r0
	(!p1)	brnd	x2
# Write r28 to uart
		swl	[r26 + 1] = r28
		retnd
		
