# digits_lookup_table:
#  0 -> digit 0 for the display
#  1 -> digit 1 for the display
#   ...
#  15 -> digit F for the display
.data
digits_lookup_table:
	.byte 0x3F # '0' digit
	.byte 0x06 # '1' digit
	.byte 0x5B # '2' digit
	.byte 0x4F # '3' digit
	.byte 0x66 # '4' digit
	.byte 0x6D # '5' digit
	.byte 0x7D # '6' digit
	.byte 0x07 # '7' digit
	.byte 0x7F # '8' digit
	.byte 0x6F # '9' digit
	.byte 0x77 # 'A' digit
	.byte 0x7C # 'B' digit
	.byte 0x39 # 'C' digit
	.byte 0x5E # 'D' digit
	.byte 0x79 # 'E' digit
	.byte 0x71 # 'F' digit

##########################################################
# Function to display digit to the RARS' Digital Lab Sim #
#                                                        #
# Digit is reprsented as 4 lowest bits in the a0         #
#  If a0 >= 16, '.' symbol is display with digit         #
#                                                        #
# Indicator index is passed in the a1                    #
#  If a1 mod 2 = 0, right indicator is used              #
#  If a1 mod 2 = 1, left indicator is used               #
#                                                        #
##########################################################
.text
.globl print_to_dlsim
print_to_dlsim:
	srli t1 a0 4      # t1 = a0 >> 4
	snez t1 t1        # t1 = a0 >= 16. That is if t1 = 1 then '.' symbol will be displayed
	slli t1 t1 7      # t1 = (a0 >= 16) << 7, now t1 is mask of the '.' symbol
	andi a0 a0 0xF    # a0 = a0 % 16
	la t0 digits_lookup_table
	add t0 t0 a0      # Now t0 has address of the digit in table
	lbu a0 (t0)       # Now a0 has representation of digit in a0
	or a0 t1 a0       # Add 7-th bit for the '.' symbol if a0 was >= 16
	
	lui t0 0xFFFF0    # MMIO address base: 0xFFFF0000
	andi a1 a1 1      # a1 % 2 - indicator index
	add a1 t0 a1      # a1 = 0xFFFF0000 + a1 % 2
	sb a0 0x10(a1)    # Write a0 to the address 0xFFFF0010 + a1 % 2
	ret

###################################################
# Function clears right indicator if a0 mod 2 = 0 #  
#  and left indicator if a0 mod 2 = 1             #
###################################################
.globl clear_dlsim
clear_dlsim:
	andi a0 a0 1
	lui t0 0xFFFF0    # MMIO address base: 0xFFFF0000
	add t0 t0 a0
	sb zero 0x10(t0)  # Clear indicator at 0xFFFF0010 + a0 % 2
	ret

###############################################
# Function prints dot '.' symbol on the right #
#  indicator and clears left indicator        #
###############################################
.globl print_dot_dlsim
print_dot_dlsim:
	lui t0 0xFFFF0
	sb zero 0x11(t0)  # Clear first indicator
	li t1 0x80
	sb t1 0x10(t0)    # Draw dot on second indicator
	ret
