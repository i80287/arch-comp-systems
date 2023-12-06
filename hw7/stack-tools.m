.macro push(%reg)
	addi sp sp -4
	sw %reg (sp)
.end_macro

.macro pop(%reg)
	lw %reg (sp)
	addi sp sp 4
.end_macro
