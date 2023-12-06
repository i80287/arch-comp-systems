.include "console_tools.i"

# Inputs %reg_arr_length integer numbers and writes
# them into the array defined at %arr_label.
# After this macro %reg_arr_length is guaranteed
# to be the same as before this macro.
# %reg_arr_length must not be s10 or s11
.macro input_array(%arr_label, %reg_arr_length)
	push(ra)
	push(%reg_arr_length)
	push(s10)
	push(s11)
	la s10 %arr_label          # s10: array_begin pointer
	slli s11 %reg_arr_length 2 # s11: %reg_arr_length * sizeof(int)
	add s11 s10 s11            # s11: array_end = arr_begin + %reg_arr_length * sizeof(int)
input_array_loop:
	beq s10 s11 input_array_loop_end
	read_int_a0_with_hint("input integer\n> ")
	sw a0 (s10)
	addi s10 s10 4
	j input_array_loop
input_array_loop_end:
	pop(s11)
	pop(s10)
	pop(%reg_arr_length)
	pop(ra)
.end_macro

# %reg_arr_length must not be s10 or s11
.macro print_array(%arr_label, %reg_arr_length)
	push(ra)
	push(%reg_arr_length)
	push(s10)
	push(s11)
	la s10 %arr_label          # s10: array_begin pointer
	slli s11 %reg_arr_length 2 # s11: %reg_arr_length * sizeof(int)
	add s11 s10 s11            # s11: array_end = arr_begin + %reg_arr_length * sizeof(int)
print_array_loop:
	beq s10 s11 print_array_loop_end
	lw a0 (s10)
	print_int_a0()
	put_char_imm(' ')
	addi s10 s10 4
	j print_array_loop
print_array_loop_end:
	pop(s11)
	pop(s10)
	pop(%reg_arr_length)
	pop(ra)
.end_macro

# Macro for ans := a[i], where
#	a is %pointer_reg
#  i is %index_reg
#  ans is %ans_reg, that is 32-bit integer
.macro read_at(%pointer_reg, %index_reg, %ans_reg)
	slli %ans_reg %index_reg 2   		  # %ans_reg = %index_reg * sizeof(int)
	add %ans_reg %pointer_reg %ans_reg # %ans_reg = %pointer_reg + %index_reg * sizeof(int)
	lw %ans_reg (%ans_reg)             # %ans_reg = *(%pointer_reg + %index_reg * sizeof(int))
.end_macro

# Macro for a[i] := value
#	a is %pointer_reg
#  i is %index_reg
#  value is %value_reg, that is 32-bit integer
# 
# ra must not be an argument of this macro
.macro write_at(%pointer_reg, %index_reg, %value_reg)
	push(ra)
	slli ra %index_reg 2   # ra = %index_reg * sizeof(int)
	add ra %pointer_reg ra # ra = %pointer_reg + %index_reg * sizeof(int)
	sw %value_reg (ra)     # *(%pointer_reg + %index_reg * sizeof(int)) = %value_reg
	pop(ra)
.end_macro

# Copies %result_reg 32-bit integer numbers from %src_array_pointer_reg to %dst_array_pointer_reg
.macro array_copy(%src_array_pointer_reg, %dst_array_pointer_reg, %length_reg)
	push(s8)
	push(s9)
	push(s10)
	push(s11)
	# If any of input registers is s9, s10 or s11, this will save us from error:
	push(%length_reg)
	push(%src_array_pointer_reg)
	push(%dst_array_pointer_reg)

	mv s9 %length_reg
	mv s10 %src_array_pointer_reg
	mv s11 %dst_array_pointer_reg
copy_while_cycle:
	beqz s9 copy_while_cycle_end
	lw s8 (s10)
	sw s8 (s11)
	addi s10 s10 4
	addi s11 s11 4
	addi s9 s9 -1
	j copy_while_cycle
copy_while_cycle_end:

	pop(%dst_array_pointer_reg)
	pop(%src_array_pointer_reg)
	pop(%length_reg)
	pop(s11)
	pop(s10)
	pop(s9)
	pop(s8)
.end_macro

# Compares two input arrays.
# %result_reg == 1 <=> array 1 is equal to array 2
# %result_reg == 0 <=> array 1 isn't equal to array 2
.macro array_equals(%first_array_pointer_reg, %second_array_pointer_reg, %length_reg, %result_reg)
	push(s8)
	push(s9)
	push(s10)
	push(s11)
	# If any of input registers is s9, s10 or s11, this will save us from error:
	push(%length_reg)
	push(%first_array_pointer_reg)
	push(%second_array_pointer_reg)

	li	%result_reg 1
	mv s9 %length_reg
	mv s10 %first_array_pointer_reg
	mv s11 %second_array_pointer_reg
array_equals_while_cycle:
	beqz s9 array_equals_while_cycle_end
	lw s8 (s10)
	lw %result_reg (s11)
	bne s8 %result_reg not_equals
	addi s10 s10 4
	addi s11 s11 4
	addi s9 s9 -1
	j array_equals_while_cycle
not_equals:
	mv %result_reg zero
array_equals_while_cycle_end:

	pop(%second_array_pointer_reg)
	pop(%first_array_pointer_reg)
	pop(%length_reg)
	pop(s11)
	pop(s10)
	pop(s9)
	pop(s8)
.end_macro
