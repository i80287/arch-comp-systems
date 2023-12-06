# Module with tests for the "integrate" function

.include "console_tools.i"

.data
	a_1: .double 1
	b_1: .double 4
	a_2: .double -5.5
	b_2: .double -7
	a_3: .double 16.9
	b_3: .double 1.2
	a_4: .double 0.1
	b_4: .double 0.2
	a_5: .double 0.02
	b_5: .double -0.3
	abs_epsilon: .double 0.0001

# This macro call "integrate" function and compares function result with actual value of the integral
#   %x_0_imm - immediate value of the left boundary x_0
#   %x_1_imm - immediate value of the right boundary x_1
#   %a_flt_label - label of the double precision float value representing 'a' parameter
#   %b_flt_label - label of the double precision float value representing 'b' parameter
#	 %test_number_reg - register with counter of the tests (will be increased by 1 after the end of this macro)
.macro run_test(%x_0_imm, %x_1_imm, %a_flt_label, %b_flt_label, %test_number_reg)
	li a0 %x_0_imm
	li a1 %x_1_imm
	fld fa0 %a_flt_label t0
	fld fa1 %b_flt_label t0
	jal integrate

	# Calculate (a * x_1 + b * (x_1)^{4} / 4) - (a * x_0 + b * (x_0)^{4} / 4) =
	# = a * (x_1 - x_0) + b ((x_1)^{4} - (x_0)^{4}) / 4
	li t0 %x_0_imm
	fcvt.d.w ft0 t0			  # ft0: x_0
	li t0 %x_1_imm
	fcvt.d.w ft1 t0           # ft1: x_1
	fld ft2, %a_flt_label, t0 # ft2: a
	fld ft3, %b_flt_label, t0 # ft3: b
	fsub.d ft4 ft1 ft0 		  # ft4: x_1 - x_0
	fmul.d ft4 ft2 ft4		  # ft4: a * (x_1 - x_0)
	fmul.d ft5 ft0 ft0		  # ft5: x_0 * x_0
	fmul.d ft5 ft5 ft5		  # ft5: x_0 * x_0 * x_0 * x_0
	fmul.d ft6 ft1 ft1		  # ft6: x_1 * x_1
	fmsub.d ft5 ft6 ft6 ft5   # ft5: x_1 * x_1 * x_1 * x_1 - x_0 * x_0 * x_0 * x_0
	fmul.d ft5 ft3 ft5        # ft5: b * (x_1^{4} - x_0^{4})
	li t0 4
	fcvt.d.wu ft6 t0          # ft6: 4.0f
	fdiv.d ft5 ft5 ft6        # ft5: b * (x_1^{4} - x_0^{4}) / 4
	fadd.d ft4 ft4 ft5        # ft4: a * (x_1 - x_0) + b * (x_1^{4} - x_0^{4}) / 4
	fsub.d ft4 fa0 ft4        # ft4: difference of the result of integrate function and actual answer
	fabs.d ft4 ft4            # | ft4 |
	fld ft5 abs_epsilon t0
	fle.d t0 ft4 ft5          # t0: | ft4 | <= 0.0001

	puts("Test ")
	print_int(%test_number_reg)
	bnez t0 test_passed
	puts(" not")
test_passed:
	puts(" passed.\n")
	addi %test_number_reg %test_number_reg 1
.end_macro

.global run_tests
.text
run_tests:
	push(ra) # save ra to the stack
	push(s3) # save s3 to the stack

	li s3 1 # counter of the running tests
	run_test(1, 2, a_1, b_1, s3)
	run_test(2, 1, a_1, b_1, s3)
	run_test(-2, 5, a_2, b_2, s3)
	run_test(5, -2, a_2, b_2, s3)
	run_test(1, 1, a_3, b_3, s3)
	run_test(0, 3, a_3, b_3, s3)
	run_test(3, 0, a_3, b_3, s3)
	run_test(-1, 2, a_4, b_4, s3)
	run_test(-4, 23, a_5, b_5, s3)

	pop(s3) # restore s3 from the stack
	pop(ra) # restore ra from the stack
	ret
