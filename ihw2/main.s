.include "console_tools.i"

.data
.global main
.text
main:
	puts("Starting internal tests...\n")
	# Run tests that will test integrate function
	# See "tests.s" for more details
	jal run_tests

	puts("Internal tests ended.\nNow please input two integeres as interval boundaries and then two rational numbers as coefficients a and b\n")
	read_int_a0_with_hint("Input left boundary\n> ")
	push(a0)   # save 'x_0' to the stack
	read_int_a0_with_hint("Input right boundary\n> ")
	push(a0)   # save 'x_1' to the stack
	read_double_fa0_with_hint("Input a parameter\n> ")
	pushd(fa0) # save 'a' to the stack
	read_double_fa0_with_hint("Input b parameter\n> ")
	fmv.d fa1 fa0 # move 'b' to fa1
	popd(fa0)  # move 'a' to fa0
	pop(a1)    # move 'x_1' to a1
	pop(a0)    # move 'x_0' to a0
	jal integrate
	pushd(fa0)  # save integral value to the stack
	puts("Approximate value of the integral is:\n")
	popd(fa0)
	print_double_fa0()
	exit()  # Exit with default code 0
