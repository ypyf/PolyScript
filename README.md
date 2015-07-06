XVM
====

A game script virtual machine.

##Examples

###Example 1

    global s

	proc Main
 
		local i
	
		mov i, 0
		mov s, "Hello XVM!"
	L0:
		jg i, 10, L1
		print s
		setchar s, i, "."
		inc i
		jmp L0
	L1:
		mov _RetVal, 0
		ret

	endp

Program output:

	Hello XVM!
	.ello XVM!
	..llo XVM!
	...lo XVM!
	....o XVM!
	..... XVM!
	......XVM!
	.......VM!
	........M!
	.........!
	..........
