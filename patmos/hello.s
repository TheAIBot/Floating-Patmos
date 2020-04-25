	.file	"/tmp/hello-14e019.bc"
	.text
	.globl	main
	.align	16
	.type	main,@function
	.fstart	main, .Ltmp0-main, 16
main:                                        # @main
.LBB0_0:                                     # %entry
	         sub	$r31 = $r31, 12      # encoding: [0x00,0x7f,0xf0,0x0c]
	         li	$r1 = 5              # encoding: [0x00,0x02,0x00,0x05]
	         swc	[$r31] = $r1         # encoding: [0x02,0xc5,0xf0,0x80]
	         li	$r1 = 3              # encoding: [0x00,0x02,0x00,0x03]
	         swc	[$r31 + 1] = $r1     # encoding: [0x02,0xc5,0xf0,0x81]
	         lwc	$r1 = [$r31]         # encoding: [0x02,0x83,0xf1,0x00]
	         nop	                     # encoding: [0x00,0x40,0x00,0x00]
	         shadd 	$r1 = $r1, $r1       # encoding: [0x02,0x02,0x10,0x8c]
	         swc	[$r31 + 2] = $r1     # encoding: [0x02,0xc5,0xf0,0x82]
	         mov	$r1 = $r0            # encoding: [0x00,0x02,0x00,0x00]
	         add	$r31 = $r31, 12      # encoding: [0x00,0x3f,0xf0,0x0c]
	         retnd	                     # encoding: [0x06,0x00,0x00,0x00]
.Ltmp0:
.Ltmp1:
	.size	main, .Ltmp1-main


