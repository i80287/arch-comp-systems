.include "array_tools.i"

.data
	input_arr_1_begin:
   	.word 1
   	.word 8
   	.word 2
   	.word 7
   	.word 3
   	.word 6
   	.word 4
   	.word 5
	input_arr_1_end:
	correct_arr_1_begin:
   	.word 1
   	.word 2
   	.word 3
   	.word 4
   	.word 5
   	.word 6
   	.word 7
   	.word 8
	correct_arr_1_end:

	input_arr_2_begin:
		.word 15
		.word	11
		.word 7
		.word 3
		.word -1
	input_arr_2_end:
	correct_arr_2_begin:
		.word -1
		.word	3
		.word 7
		.word 11
		.word 15
	correct_arr_2_end:

	input_arr_3_begin:
		.word 783
		.word -695
		.word 568
		.word 721
		.word -227
		.word -894
		.word -869
		.word 359
		.word 137
		.word 795
		.word -904
		.word 37
		.word 159
		.word 251
		.word 197
		.word 599
		.word -665
		.word -943
		.word 534
		.word 346
	input_arr_3_end:
	correct_arr_3_begin:
		.word -943
		.word -904
		.word -894
		.word -869
		.word -695
		.word -665
		.word -227
		.word 37
		.word 137
		.word 159
		.word 197
		.word 251
		.word 346
		.word 359
		.word 534
		.word 568
		.word 599
		.word 721
		.word 783
		.word 795
	correct_arr_3_end:

	input_arr_4_begin:
		.word 14910
		.word 55161
		.word 38632
		.word -3163
		.word 10302
		.word -26546
		.word 32788
		.word -63087
		.word -20989
		.word 31635
		.word 24849
		.word 62360
		.word -53538
		.word -22041
		.word 43390
		.word 19888
		.word -26733
		.word 2567
		.word -58003
		.word 38394
		.word 48696
		.word 37786
		.word -59144
		.word 61385
		.word 59065
		.word 59187
		.word 20251
		.word 38084
		.word 14252
		.word 35212
		.word -37640
		.word -43811
		.word -30864
		.word 45056
		.word 52660
		.word -38576
		.word -36924
		.word -49677
		.word -21781
		.word -14430
	input_arr_4_end:
	correct_arr_4_begin:
		.word -63087
		.word -59144
		.word -58003
		.word -53538
		.word -49677
		.word -43811
		.word -38576
		.word -37640
		.word -36924
		.word -30864
		.word -26733
		.word -26546
		.word -22041
		.word -21781
		.word -20989
		.word -14430
		.word -3163
		.word 2567
		.word 10302
		.word 14252
		.word 14910
		.word 19888
		.word 20251
		.word 24849
		.word 31635
		.word 32788
		.word 35212
		.word 37786
		.word 38084
		.word 38394
		.word 38632
		.word 43390
		.word 45056
		.word 48696
		.word 52660
		.word 55161
		.word 59065
		.word 59187
		.word 61385
		.word 62360
	correct_arr_4_end:

	# Empty array test
	input_arr_5_begin:
	input_arr_5_end:
	correct_arr_5_begin:
	correct_arr_5_end:

# %test_number_reg will be increased by 1 after the end of this macro
.macro run_test(%input_arr_begin_label, %input_arr_end_label, %correct_arr_begin_label, %test_number_reg)
	push(s1)
	push(s2)

	la a0 %input_arr_begin_label
	la a1 %input_arr_end_label
	sub a1 a1 a0
	srli a1 a1 2
	mv s1 a0
	mv s2 a1
	jal heap_sort

	la t0 %correct_arr_begin_label
	array_equals(s1, t0, s2, t1)
	mv s1 t1

	puts("Test ")
	print_int(%test_number_reg)
	bnez s1 test_passed
	puts(" not")
test_passed:
	puts(" passed.\n")
	addi %test_number_reg %test_number_reg 1

	pop(s2)
	pop(s1)
.end_macro

.global run_auto_tests
	.text
run_auto_tests:
	push(ra)
	push(s3)

	li s3 1 # test counter
	run_test(input_arr_1_begin, input_arr_1_end, correct_arr_1_begin, s3)
	run_test(input_arr_2_begin, input_arr_2_end, correct_arr_2_begin, s3)
 	run_test(input_arr_3_begin, input_arr_3_end, correct_arr_3_begin, s3)
	run_test(input_arr_4_begin, input_arr_4_end, correct_arr_4_begin, s3)
 	run_test(input_arr_5_begin, input_arr_5_end, correct_arr_5_begin, s3)

	pop(s3)
	pop(ra)
	ret
