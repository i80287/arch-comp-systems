# array_tools.i depends on console_tools.i and stack_tools.i (Rars doesn't have #ifdef)
# and therefore they must be included before this file.

# Inputs %reg_arr_length integer numbers and writes
# them into the array defined at %arr_label.
# After this macro %reg_arr_length is guaranteed
# to be the same as before this macro.
.macro input_array(%arr_label, %reg_arr_length)
	push(%reg_arr_length)
	push(s1)
	push(s2)
	la s1 %arr_label          # s1 is array begin pointer
	slli s2 %reg_arr_length 2 # s2 = n * sizeof(i32)
	add s2 s1 s2              # arr_end = arr_begin + length. arr_end is array end pointer
__input_array_loop:
	beq s1 s2 __input_array_loop_end
	read_int_a0_with_hint("input integer\n> ")
	sw a0 (s1)
	addi s1 s1 4
	j __input_array_loop
__input_array_loop_end:
	pop(s2)
	pop(s1)
	pop(%reg_arr_length)
.end_macro

# Finds the sum of the array of ints with label %arr_label.
# Length of the array is %reg_arr_length.
# Sum of the elements will be in the %reg_sum.
# %reg_overflow_flag = 1 if overflow occured and 0 otherwise.
.macro array_sum(%arr_label, %reg_arr_length, %reg_sum, %reg_overflow_flag)
__prologue:
	push(%reg_arr_length)
	push(s2)
	push(s3)
	push(s4)

	la s2 %arr_label          # arr_begin
	slli t0 %reg_arr_length 2 # t0 = n * sizeof(i32);
	add s3 s2 t0              # arr_end = arr_begin + n * 4
	mv s4 zero                # sum = 0;
__proc_array_loop:
	lw t0 (s2)
	add t5 s4 t0 # t5 = sum + *s2;
	# bool carry = ((t0 > 0) & (sum > sum + t0)) | ((t0 < 0) & (sum < sum + t0));
	sgtz t1 t0   # t1 = t0 > 0;
	sgt t2 s4 t5 # t2 = sum > sum + t0;
	and t3 t1 t2 # t3 = (t0 > 0) & (sum > sum + t0);
	sltz t1 t0   # t1 = t0 < 0;
	slt t2 s4 t5 # t2 = sum < sum + t0;
	and t4 t1 t2 # t4 = (t0 < 0) & (sum < sum + to);
	or %reg_overflow_flag t3 t4  # bool carry = t3 | t4;
	bnez %reg_overflow_flag __epilogue
	mv s4 t5     # sum = sum + *s2;
	addi s2 s2 4
	bne s2 s3 __proc_array_loop

__epilogue:
	mv %reg_sum s4
	pop(s4)
	pop(s3)
	pop(s2)
	pop(%reg_arr_length)
.end_macro
