.macro push(%reg)
	addi sp sp -4
	sw %reg (sp)
.end_macro

.macro pop(%reg)
	lw %reg (sp)
	addi sp sp 4
.end_macro

.macro push_64(%reg)
	addi sp sp -8
	sw %reg (sp)
.end_macro

.macro pop_64(%reg)
	lw %reg (sp)
	addi sp sp 8
.end_macro
