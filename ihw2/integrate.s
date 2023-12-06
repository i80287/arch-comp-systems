# Functions to work out the integral a + b * x^{3} dx from x0 to x1
#
# Function takes start x_0 in a0, end x_1 in a1, a in fa0, b in fa1
# and calculates integral using trapezoid method, i.e.
# 
# 1. Divide segment [x_0; x_1] into uniform grid with a spatial step h
#
# 2. I = \sum{ x in grid from x_0 to x_1 - h } (f(x) + f(x + h)) * h / 2
#
# In this function we will apply small optimization:
#
# Step 1: I := \sum{ x in grid from x_0 to x_1 - h } (f(x) + f(x + h))
# Step 2: I := I * h / 2
# 
# Returns: integral of a + b * x^{3} dx from x_0 to x_1 in the register fa0
#
.global integrate
.data
	max_steps: .double 131072
.text
integrate:
	# if x_0 > x_1 -> we should calculate the opposite integral, i.e. -1 * (integral f(x) dx from x_1 to x_0)
	sgt t0 a0 a1 # t0 will be flag to swap x_0 (in a0) and x_1 (in a1) and return the opposite integral in the end
	beqz t0 dont_swap_x_0_and_x_1
	xor a0 a0 a1
	xor a1 a0 a1            # swap a0 and a1
	xor a0 a0 a1
dont_swap_x_0_and_x_1:

	fcvt.d.w ft1 a0         # ft1 will be current iterating x
	fcvt.d.w ft2 a1         # ft2 will be last value of x

	fmul.d ft3 ft1 ft1      # ft3: x0 * x0
	fmul.d ft3 ft3 ft1      # ft3: x0 * x0 * x0
	fmadd.d ft3 fa1 ft3 fa0 # ft3:  b * x0^{3} + a; ft3 contains previous value of f(x)
	fcvt.d.w ft5 zero       # ft5: 0; ft5 will be the needed sum I

	fsub.d ft6 ft2 ft1      # ft6: x_1 - x_0
	fld ft0 max_steps t1    # ft0: max_stets
	fdiv.d ft0 ft6 ft0      # ft0: (x_1 - x_0) / max_steps (ft0 is h i.e. x step)
	feq.d t1 ft0 ft5        # t1: ft0 == 0 (h is zero, i.e x0 == x1, then integral = 0)
	bnez t1 integral_loop_end

integral_loop:
	fadd.d ft1 ft1 ft0      # x += h (x step)
	fgt.d t1 ft1 ft2
	bnez t1 integral_loop_end
	fmul.d ft4 ft1 ft1      # ft4: x * x
	fmul.d ft4 ft4 ft1      # ft4: x * x * x
	fmadd.d ft4 fa1 ft4 fa0 # ft4: b * x^{3} + a; ft4 contains current value of f(x)
	fadd.d ft6 ft3 ft4      # ft6: f(x-h) + f(x)
	fadd.d ft5 ft5 ft6      # I += ft6
	fmv.d ft3 ft4           # set ft3 (previos value) equal to ft4 (current value)
	j integral_loop
integral_loop_end:
	fmul.d ft5 ft5 ft0     # I = I * h
	li t1 2					  # t0: 2
	fcvt.d.wu ft0 t1       # ft0: 2.0f
	fdiv.d ft5 ft5 ft0     # I = I / 2

	beqz t0 not_opposite_integral
	fneg.d ft5 ft5
not_opposite_integral:
	fmv.d fa0 ft5
	ret
