.include "stack_tools.i"

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
	read_int_hint_string: .asciz %hint_string
.text
	li a7 4
	la a0 read_int_hint_string
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
