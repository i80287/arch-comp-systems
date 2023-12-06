.include "stack-tools.m"

.data
reversed_replacements_table:
.align 2
	.ascii "I\0\0\0"
	.align 2
	.ascii "II\0\0"
	.align 2
	.ascii "III\0"
	.align 2
	.ascii "VI\0\0"
	.align 2
	.ascii "V\0\0\0"
	.align 2
	.ascii "IV\0\0"
	.align 2 
	.ascii "IIV\0"
	.align 2
	.ascii "IIIV"
	.align 2
	.ascii "XI\0\0"
	.align 2

replacements_len_table:
	.word 1 # "I"
	.word 2 # "II"
	.word 3 # "III"
	.word 2 # "IV"
	.word 1 # "V"
	.word 2 # "VI"
	.word 3 # "VII"
	.word 4 # "VIII"
	.word 2 # "IX"

.macro is_replaceable_digit_t0(%reg)
	addi t0 %reg -49 # t0: %reg - '1'
	sltiu t0 t0 9    # t0 = (unsigned)(%reg - '1') < '9' - '1' + 1
.end_macro

#############################################################
# This function will replace all digits in range ['1'; '9'] #
#  with corresponding roman digits                          #
# It is programmer's responsobility to ensure that input    #
# 	string has enough capacity for the replacement           #
#                                                           #
# Input:                                                    #
#  a0 - address of the null-terminated ascii string         #
#                                                           #
#  a1 - length of the string                                #
#                                                           #
# Return:                                                   #
#  a0 - new length after replacements                       #
#                                                           #
#############################################################
.text
.globl str_replace_digits
str_replace_digits:
str_replace_digits_prologue:
	push(ra)
	push(s1)

	mv t1 a0          # t1: pointer to the start of the input string
	add t2 a0 a1      # t2: pointer to the end of the input string
	mv t3 a1          # t3: new length, initially t1 = a1
	la t4 replacements_len_table

	# Return if length = 0, this case it handled
	#  separately from loop1, so that we can make
	#  post-loop check in loop1 (a bit faster, less branches)
	beqz a1 str_replace_digits_epilogue

loop1:
	lbu t5 (t1)
	is_replaceable_digit_t0(t5)
	beqz t0 not_needed_digit
	addi t5 t5 -49    # digit - '1'
	slli t5 t5 2      # (digit - '1') * sizeof(word)
	add s1 t4 t5      # s1: pointer to replacements_len_table + (digit - '1') * sizeof(word)
	lw s1 (s1)        # s1: replacements_len_table[(digit - '1') * sizeof(word)]
	addi s1 s1 -1     # subtract 1 because this one char will be replaced with n chars, so delta = n - 1
	add t3 t3 s1
not_needed_digit:
	addi t1 t1 1
	bne t1 t2 loop1

	# t1 is pointer to the last char in string
	add t1 a0 a1
	addi t1 t1 -1
	# t2 is pointer to the last char in extended string
	add t2 a0 t3
	addi t2 t2 -1

	la t6 reversed_replacements_table
loop2:
	lbu t5(t1)
	is_replaceable_digit_t0(t5)
	bnez t0 replace_digit
	# Here we just copy t5 to the end
	sb t5 (t2)
	addi t2 t2 -1
loop2_iteration:
	addi t1 t1 -1
	bge t1 a0 loop2

str_replace_digits_epilogue:
	pop(s1)
	pop(ra)
	add a0 a0 t3
	sb zero (a0) # write '\0' to the end of the string
	mv a0 t3
	ret

replace_digit:
	addi t5 t5 -49    # digit - '1'
	slli t5 t5 2      # (digit - '1') * sizeof(word)

	add s1 t4 t5      # s1: pointer to replacements_len_table + (digit - '1') * sizeof(word)
	lw s1 (s1)        # s1: replacements_len_table[(digit - '1') * sizeof(word)]

	add t0 t6 t5      # t0: pointer to reversed_replacements_table + (digit - '1') * sizeof(word)
	lw t0 (t0)        # t0: reversed_replacements_table[(digit - '1') * sizeof(word)]

	sb t0 (t2)        # Write first char
	addi t2 t2 -1
	addi s1 s1 -1
	beqz s1 loop2_iteration
	srli t0 t0 8

	sb t0 (t2)        # Write second char
	addi t2 t2 -1
	addi s1 s1 -1
	beqz s1 loop2_iteration
	srli t0 t0 8

	sb t0 (t2)        # Write third char
	addi t2 t2 -1
	addi s1 s1 -1
	beqz s1 loop2_iteration
	srli t0 t0 8

	sb t0 (t2)        # Write fourth char
	addi t2 t2 -1
	b loop2_iteration
