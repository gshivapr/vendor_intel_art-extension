

public class Main extends TestUtils {

    public int test1(int x) {
        int res = x;
        for (int i = x; i > 0; i -= 100) {
            res <<= 3;
            res += res;
            res += i;
        }
        try {
            throw new Exception("");
        } catch (Exception e) {
        } finally {
        }
        return res;
    }

    public int test2(int x) {
        int res = x;
        try {
            for (int i = 10; i > 0; i -= 100 * x) {
                res <<= 4;
                res += res;
                res += i;
                if (i == res + x)
                    throw new Exception("");
            }
        } catch (Exception e) {
        } finally {
        }
        return res;
    }

    public int test3(int x) {
        int res = x;
        try {
            throw new Exception("");
        } catch (Exception e) {
            for (int i = 1; i > 0; i -= 100) {
                res <<= 5;
                res += res;
                res += i;
            }
        } finally {
        }
        return res;
    }

    public int test4(int x) {
        int res = x;
        try {
            throw new Exception("");
        } catch (Exception e) {
        } finally {
            for (int i = 100; i > -100; i -= 100) {
                res <<= 3;
                res += res;
                res += i;
            }
        }
        return res;
    }

    public int test5(int x) {
        int res = x;
        try {
            throw new Exception("");
        } catch (Exception e) {
        } finally {
        }
        for (int i = 0; i > -10000; i -= 100) {
            res <<= 4;
            res += res;
            res += i;
        }
        return res;
    }

    public static void main(String[] args) {
        TestUtils.runTests(new Main(), 10);
    }
}
