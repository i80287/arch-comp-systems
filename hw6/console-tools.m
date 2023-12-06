.include "stack-tools.m"

.macro read_uint_a0_less_eq_imm(%imm_number_border)
	push(t0)
__loop_with_border_check:
	puts("Enter non-negative number <= ")
	print_int_imm(%imm_number_border)
	puts(":\n> ")
	li a7 5
	ecall
	li t0 %imm_number_border
	bleu a0 t0 __loop_with_border_check_end
	puts("Sorry, try again\n")
	j __loop_with_border_check
__loop_with_border_check_end:
	pop(t0)
.end_macro

.macro read_int_a0_with_hint(%hint_string)
.data
	__hint_string: .asciz %hint_string
.text
	li a7 4
	la a0 __hint_string
	ecall
	li a7 5
	ecall
.end_macro

.macro read_double_fa0_with_hint(%hint_string)
.data
	read_double_hint_string: .asciz %hint_string
.text
	li a7 4
	la a0 read_double_hint_string
	ecall
	li a7 7
	ecall
.end_macro

#########################################################
# Macro to read string from the rars dialog window      #
#                                                       #
# %prompt_string  - string litral that will be shown to #
#                   the user                            #
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
__read_loop:
	la a0 __prompt_string
	mv a1 %buffer_address
	mv a2 %buffer_len_reg
	li a7 54
	ecall
	li t0 -4
	bne a1 t0 __no_error # if a1 == -4, then input string was longer then %buffer_len_reg - 2
	write_literal_error_rwnd("Input was too long")
	b __read_loop
__no_error:
	bnez a1 __not_empty_input
	mv t0 %buffer_address
	sb a1 (t0)          # if a1 == 0, then input is empty, store '\0' in the buffer
__not_empty_input:
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

###################################################
# Macro to write chars from the label to the rars #
#  dialog info window                             #
#                                                 #
# %str_label - label of the string that will be   #
#              shown to the user                  #
#                                                 #
###################################################
.macro write_label_rwnd(%str_label)
.text
	la a0 %str_label
	li a1 1
	li a7 55
	ecall
.end_macro

#######################################################
# Macro to write chars in the buffer to the rars      #
#  dialog info window                                 #
#                                                     #
# %buffer_address - address of the buffer whose chars #
#                   will be shown to the user         #
#                                                     #
#######################################################
.macro write_buffer_rwnd(%buffer_address)
.text
	mv a0 buffer_address
	li a1 1
	li a7 55
	ecall
.end_macro

.macro print_int(%reg_int)
	li a7 1
	mv a0 %reg_int
	ecall
.end_macro

.macro print_int_imm(%imm_int)
	li a7 1
	li a0 %imm_int
	ecall
.end_macro

.macro print_int_a0()
	li a7 1
	ecall
.end_macro

.macro print_double(%reg_double)
	li a7 3
	mv fa0 %reg_double
	ecall
.end_macro

.macro print_double_imm(%imm_double)
.data
	print_double_imm_label: .double %imm_double
.text
	fld fa0 print_double_imm_label t0 # t0 can be lost in the system call anyway
	li a7 3
	ecall
.end_macro

.macro print_double_fa0()
	li a7 3
	ecall
.end_macro

.macro puts(%out_string)
.data
	__output_string: .asciz %out_string
.text
	li a7 4
	la a0 __output_string
	ecall
.end_macro

.macro put_char_imm(%imm_char)
	li a7 11
	li a0 %imm_char
	ecall
.end_macro

.macro put_newline()
	put_char_imm('\n')
.end_macro

.macro exit_imm(%imm_exit_code)
	li a7 93
	li a0 %imm_exit_code
	ecall
.end_macro

.macro exit()
	exit_imm(0)
.end_macro

