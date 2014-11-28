XVM
====

A game script virtual machine.

##Examples

###Example 1

	proc Main
	{
		local s
		local i
	
		mov i, 0
		mov s, "Hello world!"
	L0:
		jg i, 12, L1
		print s
		setchar s, i, "."
		inc i
		pause 50
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
