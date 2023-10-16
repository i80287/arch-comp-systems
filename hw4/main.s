.macro fputs(%input_str)
.data
__inp_str__: .asciz %input_str
.text
	li a7 4
	la a0 __inp_str__
	ecall
.end_macro

.macro fput_newline()
.text
	li a7 11
	li a0 '\n'
	ecall
.end_macro

.macro fprint_uint(%uint_reg)
.text
	li a7 1
	mv a0 %uint_reg
	ecall
.end_macro

.text
	fputs("max n using function with loop\n> ")
	jal max_n_fact_non_rec
	fprint_uint(a0)
	fput_newline()
	fputs("max n using function with recursion\n> ")
	jal max_n_fact_rec
	fprint_uint(a0)
	li a7 10
	ecall

max_n_fact_non_rec:
# max_n_fact_non_rec() -> a0
# returns max n such that n! can be calculated for u32 n without overflow
	li t0 1                 # u32 fact = 1;
	addi t1 t0 1            # u32 mult = 2;
fact_loop_start:
	mul t0 t0 t1         	# fact = fact * mult;
	addi t1 t1 1    			# mult++;
	mulhu t2 t0 t1      	 	# u32 carry = (u32)(((u64)fact * (u64)mult) >> 32);
	beqz t2 fact_loop_start # if (carry == 0) { goto fact_loop_start; } // 32-bit unsigned overflow flag
fact_loop_end:
	addi a0 t1 -1           # return mult - 1;
	ret
factorial_non_rec_end:

max_n_fact_rec:
# max_n_fact_non_rec() -> a0
# returns max n such that n! can be calculated for u32 n without overflow
	li a0 1
	li a1 1
	addi sp sp -4
	sw ra (sp)
	jal max_n_fact_rec_impl # recursive implementation of this function
	lw ra (sp)
	addi sp sp 4
	ret

max_n_fact_rec_impl:
# a0 is current factorial, a1 is current multiplier
	mulhu t0 a0 a1 	  # u32 carry = (u32)(((u64)a0 * (u64)a1) >> 32);
	beqz t0 no_overflow # if (carry != 0) { return a1 - 1; }
	addi a0 a1 -1
	ret
no_overflow:           # return max_n_fact_rec_impl(a0 * a1, a1 + 1);
	mul a0 a0 a1
	addi sp sp -4
	sw ra (sp)
	addi a1 a1 1
   jal max_n_fact_rec_impl
	lw ra(sp)
	addi sp sp 4
	ret

