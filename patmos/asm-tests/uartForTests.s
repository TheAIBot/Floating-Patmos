        nop
        .word 50
        br start
		nop
		nop
		.word   28
#Send r26 to uart
#Wait for uart to be ready
uart1:	add 	r28 = r0, 0xf0080000
		add     r25 = r0, 4
t2:		lwl     r27  = [r28 + 0]
		nop
		btest p1 = r27, r0
	(!p1)	br	t2
		nop
		nop
# Write r26 to uart
		swl	[r28 + 1] = r26
		sri r26 = r26, 8
		sub r25 = r25, 1
		cmpineq p1 = r25, 0
	(p1)	br	t2
		nop
		nop
        retnd
start:  nop
