import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;


public class Main {
    /*Simple test*/
    private int $noinline$testNestedIfElsesD3(int p) {
        int[] a = new int[100];
        a[p] = 12;
        int res = a[p];
        for (int i = 0; i < 1000000; i++) {
            double x = 0;
            if (res * res * res <39534543) {
                if (res * res >35534543) {
                    if (res * 70 < 20) {
                        x += Math.sin(res);
                    }
                    else {
                        x += Math.cos(res);
                    }
                }
                else {
                    if (res * 90 > 40) {
                        x -= Math.sin(res);
                    }
                    else {
                        x -= Math.cos(res);
                    }
                }
            }
            res += i;
        }
        return res;
    }

    public void RunTests()
    {
        System.out.println($noinline$testNestedIfElsesD3(1));
    }

    public static void main(String[] args)
    {
        new Main().RunTests();
    }
}

