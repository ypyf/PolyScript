XVM
====

A game script virtual machine.

##Examples

###Example 1
    
    /* hello.xss */
    
    func main()
    {
        var i;
        i = 10;
        while (i > 0)
        {
            print "Hello World";
            i -= 1;
        }
    }
