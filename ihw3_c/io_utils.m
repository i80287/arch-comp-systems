.include "stack_utils.m"

#########################################################
# Macro to read string from the rars dialog window      #
#                                                       #
# %prompt_string  - string literal that will be shown   #
#                   to the user                         #
#                                                       #
# %buffer_address - register with address of the buffer #
#						  that will contain input string      #
#                                                       #
# %buffer_len_reg - reigster with max capacity of the   #
#                   buffer                              #
#                                                       #
#########################################################
.macro read_string_rwnd(%prompt_string, %buffer_address, %buffer_len_reg)
.data
	__prompt_string: .asciz %prompt_string
.text
	push(ra)
	push(%buffer_address)
	push(%buffer_len_reg)
__read_loop:
	mv a1 %buffer_address
	mv a2 %buffer_len_reg
	la a0 __prompt_string
	li a7 54
	ecall
	li t0 -4
	bne a1 t0 __no_error # if a1 == -4, then input string was longer then %buffer_len_reg - 2
	write_literal_error_rwnd("Input was too long")
	b __read_loop

__no_error:
	pop(%buffer_len_reg)
	pop(%buffer_address)

	beqz a1 __not_empty_input
	sb a1 (%buffer_address)   # if a1 == 0, then input is empty, store '\0' in the buffer
	b __rs_rwnd_macro_end

__not_empty_input:
	# Strip '\n' from the end
	add a0 %buffer_address %buffer_len_reg
	li a2 '\n'
__rstrip_newline_loop:
	addi a0 a0 -1
	lbu a1 (a0)
	bne a1 a2 __not_newline
	sb zero (a0)
	b __rs_rwnd_macro_end	
__not_newline:
	bgt a0 %buffer_address __rstrip_newline_loop

__rs_rwnd_macro_end:
	pop(ra)
.end_macro

#########################################################
# Macro to write string to the rars dialog error window #
#                                                       #
# %error_string - string that will be shown to the user #
#                                                       #
#########################################################
.macro write_literal_error_rwnd(%error_string)
.data
	__error_string: .asciz %error_string
.text
	la a0	__error_string
	li a1 0
	li a7 55
	ecall
.end_macro

########################################################
# Macro to write string to the rars dialog info window #
#                                                      #
# %info_string - string that will be shown to the user #
#	           	                                        #
########################################################
.macro write_literal_string_rwnd(%info_string)
.data
	__info_string: .asciz %info_string
.text
	la a0	__info_string
	li a1 1
	li a7 55
	ecall
.end_macro

################################################
# Service to display a message to the rars     #
#  dialog info window                          #
#                                              #
# %literal_string - literal string that is the #
#                   message to user            #
#                                              #
# Output: a0 = Yes (0), No (1), or Cancel(2)   #
#                                              #
################################################
.macro confirm_literal_msg_rwnd(%literal_string)
.data
	__confirm_msg: .asciz %literal_string
.text
	la a0 __confirm_msg
	li a7 50
	ecall
.end_macro

.macro print_int(%reg_int)
	li a7 1
	mv a0 %reg_int
	ecall
.end_macro

.macro print_string(%out_string)
.data
	output_string: .asciz %out_string
.text
	li a7 4
	la a0 output_string
	ecall
.end_macro

.macro exit()
	li a7 93
	mv a0 zero
	ecall
.end_macro
