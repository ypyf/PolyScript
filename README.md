PolyScript
====

A game script language.

##Examples

###Example 1
    
    /* hello.poly */
    
    func main()
    {
        var i = 10;

        while (i > 0)
        {
            print "Hello World";
            --i;
        }
    }
    
###Example 2
    
    /* fibonacci.poly */
    
    func fib(n)
    {
        if (n == 1)
            return 0;

        if (n == 2)
            return 1;

        return fib(n-1) + fib(n-2);
    }   
