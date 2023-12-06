.macro push(%reg)
	addi sp sp -4
	sw %reg (sp)
.end_macro

.macro pop(%reg)
	lw %reg (sp)
	addi sp sp 4
.end_macro

.macro pushd(%reg)
	addi sp sp -8
	fsd %reg (sp)
.end_macro

.macro popd(%reg)
	fld %reg (sp)
	addi sp sp 8
.end_macro
