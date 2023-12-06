.include "stack-tools.m"

.text
.globl main
main:
main_prologue:
	push(s0)           # Save s0
	push(s1)           # Save s1
	push(s2)           # Save s2

	mv s1 zero         # initial digit = 0
	li s2 16           # max digit 'F' + 1
print_loop:
	mv a0 s1
	li a1 0
	jal print_to_dlsim # Print s1 to the first indicator.
	li a0 1
	jal clear_dlsim    # Clear second indicator.
	li a0 1000
	li a7 32
	ecall              # Sleep for 1 second.
	mv a0 s1
	li a1 1
	jal print_to_dlsim # Print s1 to the second indicator.
	li a0 0
	jal clear_dlsim    # Clear first indicator.
	li a0 1000
	li a7 32
	ecall              # Sleep for 1 second.
	addi s1 s1 1       # Increase digit.
	bne s1 s2 print_loop # digit != 16 ?

	# Print dot '.' symbol on the right indicator and clear left indicator.
	jal print_dot_dlsim
	lui s0 0xFFFF0   # Load base MMIO address 0xFFFF0000 to s0.
	la t0 kb_handler # Register keyboard interrupt handler.
	csrw t0 utvec    # Save keyboard_handler address to utvec.
	csrsi ustatus 1  # Enable exceptions.
   csrsi uie 0x100  # Enable handling of this specific interrupt.
	li t0 0x8F
	sb t0 0x12(s0)   # Set 7-th bit of 0xFFFF0012 to 1 to enable interrupts in Digital Lab Sim.

	li s1 0          # Keyboard buttons pressed counter.
	li s2 16         # Max keyboard buttons pressed.	
wait_loop:
	wfi              # Wait for the next keyboard button pressed.
	addi s1 s1 1     # Increase counter after handling interrupt.
	bne s1 s2 wait_loop # Counter != 16 ?

main_epilogue:
	pop(s2)          # Restore s2
	pop(s1)          # Restore s1
	pop(s0)          # Restore s0
	li a7 10
	ecall            # Exit with code 0
.data
kb_handler_reg_space_1: # Space to save register
	.space 4
kb_handler_reg_space_2: # Space to save register
	.space 4
tr_zeros_lookup_table:  # Trailing zeros lookup table
	.byte 0
	.byte 0
	.byte 1
	.byte 0
	.byte 2
	.byte 0
	.byte 1
	.byte 0	
	.byte 3
.text
kb_handler:
kb_handler_prologue:
	csrw a0 uscratch                # Save a0
	sw a1 kb_handler_reg_space_1 a0 # Save a1
	sw ra kb_handler_reg_space_2 a1 # Save ra

	# Scan row 1
	li a0 1
	sb a0 0x12(s0)
	lbu a1 0x14(s0)
	bnez a1 key_detected
	# Scan row 2
	li a0 2
	sb a0 0x12(s0)
	lbu a1 0x14(s0)
	bnez a1 key_detected
	# Scan row 3
	li a0 4
	sb a0 0x12(s0)
	lbu a1 0x14(s0)
	bnez a1 key_detected
	# Scan row 4
	li a0 8
	sb a0 0x12(s0)
	lbu a1 0x14(s0)
	bnez a1 key_detected
	# No key is pressed, print out dot '.' symbol
	jal print_dot_dlsim
	b kb_handler_epilogue

key_detected:
	# Get row and column indexes from a0 and a1
	la ra	tr_zeros_lookup_table
	add a0 ra a0
	lbu a0 (a0)
	srli a1 a1 4
	add a1 ra a1
	lbu a1 (a1)
	# Now a0 is zero based row and a1 is zero base column
	# digit = 4 * row + column = 4 * row | column
	slli a0 a0 2
	or a0 a0 a1
	li a1 0            # First indicator
	jal print_to_dlsim

kb_handler_epilogue:
   li   a0, 0x8F      # Set 7-th bit of 0xFFFF0012 to 1 to enable interrupts in Digital Lab Sim
   sb   a0, 0x12(s0)
	lw ra kb_handler_reg_space_2 # Restore ra
   lw a1 kb_handler_reg_space_1 # Restore a1
   csrr a0 uscratch             # Restore a0
	uret
