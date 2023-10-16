.data
arr_begin:		.space 40 # 10 * sizeof(i32)
arr_end:
read_n_msg:		.asciz "Enter size of the array\n> "
read_n_error:	.asciz "n must be an integer between 1 and 10 including"
arr_elem_msg:	.asciz "Enter integer number\n> "
sum_msg:		 	.asciz "The sum of the elements: "
even_cnt_msg:	.asciz "Count of the even numbers: "
odd_cnt_msg:	.asciz "Count of the odd numbers: "
error_msg1:		.asciz "An overflow error occured on element "
error_msg2: 	.asciz "Data about "
error_msg3:		.asciz " numbers before the error:"
.align 2

.text
read_n:
	li a7 4 # print_string
	la a0 read_n_msg
	ecall
	li a7 5 # read_int
	ecall
	# a0 < 1 || a0 > 10 <=> !(a0 >= 1 && a0 <= 10) <=> !((u32)(a0 - 1) <= 10 - 1)
	addi t0, a0, -1
	li t1, 9
	bleu t0, t1, read_n_end
	li a7 4 # print_string
	la a0 read_n_error
	ecall
	li a7 11 # print_char
	li a0 '\n'
	ecall
	j read_n
read_n_end:

	addi sp sp -28 # move stack pointer
	sw s1 4(sp)    # push s1 to the stack before using
	sw s2 8(sp) 	# push s2 to the stack before using
	sw s3 12(sp) 	# push s3 to the stack before using
	sw s4 16(sp)   # push s4 to the stack before using
	sw s5 20(sp) 	# push s5 to the stack before using
	sw s6 24(sp) 	# push s6 to the stack before using
	mv s1 a0		   # u32 s1 = n;

read_array:
	la s2 arr_begin # s2 = arr_begin;
	slli t0, s1, 2  # t0 = s1 * sizeof(i32);
	add s3, s2, t0  # s3 = s2 + t0;
read_array_loop:
	li a7 4 # print_string
	la a0 arr_elem_msg
	ecall
	li a7 5 # read_int
	ecall
	sw a0 (s2)
	addi s2 s2 4 # s2 += sizeof(i32);
	bltu s2 s3 read_array_loop
read_array_end:

proc_array:
	slli t0 s1 2 # t0 = n * sizeof(i32);
	sub s2 s3 t0 # i32* arr_begin = arr_end - n;
	mv s4 zero  # i32 sum = 0;
	mv s5 zero  # u32 even_cnt = 0;
	mv s6 zero  # u32 odd_cnt = 0;
proc_array_loop:
	lw t0 (s2)
	add t5 s4 t0 # t5 = sum + *s2;

	# bool carry = ((t0 > 0) & (sum > sum + t0)) | ((t0 < 0) & (sum < sum + t0));
	sgtz t1 t0   # t1 = t0 > 0;
	sgt t2 s4 t5 # t2 = sum > sum + t0;
	and t3 t1 t2 # t3 = (t0 > 0) & (sum > sum + t0);
	sltz t1 t0   # t1 = t0 < 0;
	slt t2 s4 t5 # t2 = sum < sum + t0;
	and t4 t1 t2 # t4 = (t0 < 0) & (sum < sum + to);
	or t3 t3 t4  # bool carry = t3 = t3 | t4;
	bnez t3 overflow_error

	andi t1 t0 1
	seqz t1 t1   # t1 = (t0 & 1) == 0;
	add s5 s5 t1 # even_cnt += (t0 & 1) == 0;
	xori t1 t1 1 # t1 ^= true; // <=> t1 = (t0 & 1) != 0;
	add s6 s6 t1 # odd_cnt += (t0 & 1) != 0;
	mv s4 t5 # sum = sum + *st;

	addi s2 s2 4
	bltu s2 s3 proc_array_loop
proc_array_end:

print_data:
	li a7 4 # print_string
	la a0 sum_msg
	ecall
	li a7 1 # print_int
	mv a0 s4
	ecall
	li a7 11 # print_char
	li a0 '\n'
	ecall
	li a7 4 # print_string
	la a0 even_cnt_msg
	ecall
	li a7 1 # print_int
	mv a0 s5
	ecall
	li a7 11 # print_char
	li a0 '\n'
	ecall
	li a7 4 # print_string
	la a0 odd_cnt_msg
	ecall
	li a7 1 # print_int
	mv a0 s6
	ecall
	li a7 11 # print_char
	li a0 '\n'
	ecall
print_data_end:

	lw s6 24(sp) # pop s6 from the stack
	lw s5 20(sp) # pop s5 from the stack
	lw s4 16(sp) # pop s4 from the stack
	lw s3 12(sp) # pop s3 from the stack
	lw s2 8(sp)  # pop s2 from the stack
	lw s1 4(sp)  # pop s1 from the stack
	addi sp sp 28

	li a7 10
	ecall

# cold code
overflow_error:
	li a7 4 # print_string
	la a0	error_msg1
	ecall
	li a7 1 # print_int
	mv a0 t0
	ecall
	li a7 11 # print_char
	li a0 '\n'
	ecall
	li a7 4 # print_string
	la a0 error_msg2
	ecall
	li a7 1 # print_int
	la a0 arr_begin # t1 = arr_begin
	sub a0 s2 a0 # ptrdiff_t elements_count = (s2 - arr_begin) / sizeof(i32);
	srli a0 a0 2
	ecall
	li a7 4 # print_string
	la a0 error_msg3
	ecall
	li a7 11 # print_char
	li a0 '\n'
	ecall
	j print_data
