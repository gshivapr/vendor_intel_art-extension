
/**
*
* Simplest case when optimization should be applied
*  1 sinking expected
*
**/

public class Main {                                                                                                                                                   
    final int iterations = 1100;

    final int testLoopDivInt() {
        int testVar = 5000;

        int i = 0;
        while (i < iterations) {
            testVar /= 5;
            i++;
        }
        return testVar;
    }


    final int $noinline$testTryCatch() {
        int res = 0;
        res += testLoopDivInt();
        try {
            if (testLoopDivInt() > 0) {
                throw new Exception("My exception"); 
            }
        } catch (Exception e) {
            res ^= testLoopDivInt();
        } finally {
            res -= testLoopDivInt();
        }
        res *= testLoopDivInt();
        return res;
    }

    public static void main(String[] args) {
         System.out.println(new Main().$noinline$testTryCatch());
    }
}  

