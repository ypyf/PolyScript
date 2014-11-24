XVM
====

A game script virtual machine.

##Examples

###Example 1

	proc Main
	{
		local s
		local i	

		mov i, 1
		mov s, "Hello world!"
	L0:
		jg i, 10, L1
		print s
		setchar s, i, "."
		inc i
		jmp L0
	L1:
		mov _RetVal, 0
		ret
	}

Program output:
	
	Hello world!
	H.llo world!
	H..lo world!
	H...o world!
	H.... world!
	H.....world!
	H......orld!
	H.......rld!
	H........ld!
	H.........d!
