// The test checks that stack after NullPointerException occurs is correct despite inlining
class Main {
    final static int iterations = 10;
    static Foo[] thingiesArray = new Foo[iterations];

    public static Foo getThingies(Foo[] arr, int i) {
        return arr[i];
    }

    public static void setThingies(Foo[] arr, Foo newThingy, int i) {
        arr[i] = newThingy;
    }
    
    public static void main(String[] args) {
        Foo nextThingy = new Foo(-1);

        for(int i = 0; i < iterations; i++) {
            thingiesArray[i] = new Foo(i);
        }

        try {
            throw new Exception("");
        } catch (Exception e) {
        } finally {
        }

        for(int i = 0; i < iterations; i++) {
            nextThingy = getThingies(thingiesArray, i);
            if (i == iterations - 1)
                thingiesArray = null;
            setThingies(thingiesArray, nextThingy, i);
        }
    }
}
