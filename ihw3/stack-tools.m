.macro push(%reg)
	addi sp sp -4
	sw %reg (sp)
.end_macro

.macro pop(%reg)
	lw %reg (sp)
	addi sp sp 4
.end_macro

.eqv STACK_CANARY_VALUE 2039

.macro push_canary(%reg)
	li %reg STACK_CANARY_VALUE
	push(%reg)
.end_macro

.macro pop_canary(%reg)
	pop(%reg)
	addi %reg %reg -STACK_CANARY_VALUE
.end_macro
