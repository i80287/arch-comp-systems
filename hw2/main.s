.data
    first_input_hint:  .asciz "Input first integer\n> "
    second_input_hint: .asciz "Input second integer\n> "
    quotient_report:   .asciz "quotient = a / b = "
    remainder_report:  .asciz "remainder = a % b = "
    error_report:      .asciz "Zero division error\n"

.text
main:
    la a0 first_input_hint
    li a7 4 # ~ fputs(a0, stdout)
    ecall
    li a7 5 # ~ scanf("%d", &a0);
    ecall
    
    addi sp, sp, -8 # move stack pointer
    sw s1, 4(sp)    # push s1 to the stack before use it
    mv s1 a0        # store 'a' value to the s1

    la a0 second_input_hint
    li a7 4 # ~ fputs(a0, stdout);
    ecall
    li a7 5
    ecall
    mv a1 a0 # move 'b' value to the a1
    mv a0 s1 # move 'a' value to the a0
    
    lw s1, 4(sp)    # restore s1 value from the stack
    addi sp, sp, 8  # restore stack pointer

    addi sp, sp, -8 # move stack pointer
    sw ra 4(sp)     # push ra to the stack before function call

    jal div_func

    lw ra, 4(sp)    # pop ra from the stack
    addi sp, sp, 8  # restore stack pointer
    
    # jump if (no_errors == false)
    beqz a0 error_occured
    
    # no errors branch
    addi sp, sp, -16 # move stack pointer
    sw s1 12(sp)     # save s1 to the stack
    sw s2 8(sp)      # save s2 to the stack
    sw ra 4(sp)      # save ra to the stack
    
    mv s1, a1 # save q to s1
    mv s2, a2 # save r to s2
    
    la a0 quotient_report
    li a7 4 # ~ fputs(a0, stdout);
    ecall

    mv a0 s1 # a0 = q
    li a7 1  # ~ printf("%d", a0);
    ecall

    li a0 '\n'
    li a7 11 # ~ fputc(a0, stdout);
    ecall
    
    la a0 remainder_report
    li a7 4 # ~ fputs(a0, stdout);
    ecall

    mv a0 s2 # a0 = r
    li a7 1  # ~ printf("%d", a0);
    ecall

    li a0 '\n'
    li a7 11 # ~ fputc(a0, stdout);
    ecall
    
    lw ra 4(sp)      # restore return address
    lw s2 8(sp)      # restore s2
    lw s1 12(sp)     # restore s1
    addi sp, sp, 16  # restore stack pointer

    mv a0, zero # load exit code 0
    j main_end

    # error occured branch
    error_occured:
    la a0 error_report
    li a7 4 # fputs(a0, stdout);
    ecall

    li a0 1 # load exit code 1

    main_end:
    li a7 93 # system call exit2. Exits the program with a code a0
    ecall

.text
div_func: # a / b: (a -> a0, b -> a1) |-> (no_errors -> a0, q -> a1, r -> a2) where a = b * q + r
    mv t0 a0    # t0 = a
    mv t1 a1    # t1 = b
    mv a0, zero # bool no_errors = false;
    mv a1, zero # i32 q = 0;
    mv a2, zero # i32 r = 0;

    beqz t1 div_func_end # if (b == 0) { return; }

    li a0, 1 # no_errors = true;

    # t2 = bool q_sign_carry = !SameSign(a, b);
    # t3 = bool r_sign_carry = a < 0;

    # SameSign(i32 a, i32 b) return (a >= 0) ^ (b < 0);
    # !((a >= 0) ^ (b < 0)) == (a < 0) ^ (b < 0);
    sltz t3, t0    # bool r_sign_carry = t2 = (a < 0);
    sltz t4, t1    # t4 = b < 0
    xor t2, t3, t4 # q_sign_qarry = t2 = (a < 0) ^ (b < 0)
    
    # a = abs(a)
    # if (a < 0) { a = -a; }
    # branchless implementation:
    # t5 = t0 >> 31; // with sign extend
    # t0 = (t0 ^ t5) - t5;
    srai t5, t0, 31 # t5 = t0 >> 31; // (a_sign_bit == 1) ? 1111...1111 : 0;
    xor  t0, t0, t5 # t0 = t0 ^ t5;
    sub t0, t0, t5 # t0 = t0 - t5;
    
    # same branchless implementation for the b = abs(b)
    # t5 = t1 >> 31; // with sign extend
    # t1 = (t1 ^ t5) - t5;
    srai t5, t1, 31 # t5 = t1 >> 31; // (b_sign_bit == 1) ? 1111...1111 : 0;
    xor t1, t1, t5 # t1 = t1 ^ t5;
    sub t1, t1, t5 # t1 = t1 - t5;
    
    # while (a >= b) { a -= b; r++; }
    while_loop_label:
    bltu t0, t1, while_loop_end # while (a >= b)
    sub t0, t0, t1 # a -= b;
    addi a1, a1, 1 # q++;
    j while_loop_label
    while_loop_end:
    
    mv a2, t0 # r = a;

    # if (q_sign_carry) { q = - q; }
    # branchless implementation:
    # t2 = (i32)((u32)q_sign_carry << 31) >> 31; // e.g. t2 = q_sign_carry ? 1111...1111 : 0;
    # q = (q ^ t2) - t2;
    slli t2, t2, 31 # t2 = t2 << 31;
    srai t2, t2, 31 # t2 = q_sign_carry ? 1111...1111 : 0;
    xor a1, a1, t2  # q = q ^ t2;
    sub a1, a1, t2  # q = q - t2;

    # if (r_sign_carry) { r = -r; }
    # branchless implementation:
    # t3 = (i32)((u32)r_sign_carry << 31) >> 31; // e.g. t3 = r_sign_carry ? 1111...1111 : 0;
    # r = (r ^ t3) - t3;
    slli t3, t3, 31 # t3 = t3 << 31;
    srai t3, t3, 31 # t3 = r_sign_carry ? 1111...1111 : 0;
    xor a2, a2, t3  # r = r ^ t3;
    sub a2, a2, t3  # r = r - t3;

    div_func_end:
    ret
