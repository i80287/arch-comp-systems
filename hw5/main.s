.include "console_tools.i"
# array_tools.i depends on console_tools.i and stack_tools.i (Rars doesn't have #ifdef)
# and therefore must be included later
.include "array_tools.i"

.data
	array_begin: .space 40
	array_end:
.text
main:
	read_uint_a0_less_eq_imm(10)
	input_array(array_begin, a0)
	push(s1)
	array_sum(array_begin, a0, s1, t1)
	beqz t1 print_sum
	puts("Overflow occured. Last sum that was calculated before:\n")
print_sum:
	puts("Sum of the array: ")
	print_int(s1)
	pop(s1)
	exit_imm(0)
