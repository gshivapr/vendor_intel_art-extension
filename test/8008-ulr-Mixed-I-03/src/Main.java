
public class Main {

    // Test a loop with expected inlining and invariant hoisting
    // Several loops in a row, all should be removed


    public static int getValue() {
        return 5;
    }


    public static int foo(int x) {
        return x;
    }

    public static int loop() {
        int res = 0;
        int invar1 = 555;
        for (int i = 0; i < 10000; i++) {
            res += getValue() * 2 + invar1;
        }
        res += 5;
        for (int i = 0; i < 10000; i++) {
            res = getValue() * 5 - invar1;
        }
        
        int j = -1;
        do  {
            res = getValue() * 5 - foo(6)*invar1;
            j++;
        } while (j < 10000);
        return res;
    }

    public static void main(String[] args) {
        int res = new Main().loop();
        System.out.println(res);
    }
}
