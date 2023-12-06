#############################################################
# This function copies the string pointed to by a1, into a  #
# 	string at the buffer pointed to by a0. The programmer is #
# 	responsible for allocating a destination buffer large    #
#	enough, that is, strlen(a1) + 1                          #
#############################################################
	.text
.globl strcpy
strcpy:
	or t0 a0 a1
	beqz t0 strcpy_ret # if a0 or a1 equal to 0, return a0
	mv t0 a0
loop:
	lbu t1 (a1)  # copy next byte from a1 to a0
	sb t1 (t0)
	addi t0 t0 1
	addi a1 a1 1
	# if t1 != 0, copy next byte.
	# Otherwise, we copied whole string a1 to the a0 and added termination byte to the end.
	bnez t1 loop
strcpy_ret:
	ret
