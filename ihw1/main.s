.include "array_tools.i"

.data
	# array A
	a_array_begin: .space 40
	a_array_end:
	# array B
   b_array_begin: .space 40
   b_array_end:
.global main
	.text
main:
	push(s1) # save s1 to tha stack

	puts("Starting internal tests...\n")
	# Run tests that will test heap sort algorithm.
	# See "auto_tests.s" for more details
	jal run_auto_tests
	puts("Internal tests ended.\nNow please input length of the input array.\n")

	# Read uint <= 10 into a0
	read_uint_a0_less_eq_imm(10)
	mv s1 a0
	# Input array that starts at label buffer_arr_begin and has length a0
	input_array(a_array_begin, a0)

	la t0 a_array_begin
	la t1 b_array_begin
	array_copy(t0, t1, s1)
	puts("Copied input array A to the new array B.\n")

	puts("Array B before sorting:\n")
	# Print array B before sorting
	print_array(b_array_begin, s1)
	put_newline()

	mv a1 s1
	la a0 b_array_begin
	# Sort array B
	jal heap_sort

	puts("Array B after sorting:\n")
	# Print array B after sorting
	print_array(b_array_begin, s1)
	put_newline()

	pop(s1) # restore s1 from the stack
	# Exit with default code 0
	exit()
