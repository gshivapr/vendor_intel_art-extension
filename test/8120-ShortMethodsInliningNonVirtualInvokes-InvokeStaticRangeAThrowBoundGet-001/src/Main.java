// The test checks that stack after ArrayIndexOutOffBouns Exception occurs is correct despite inlining
class Main {
    final static int iterations = 10;
    static int[] thingiesArray = new int[iterations];

    public static int getThingies(int[] arr, int i, char c1, char c2, char c3, char c4, char c5, char c6) {
        return arr[i];
    }

    public static void setThingies(int[] arr, int newThingy, int i, char c1, char c2, char c3, char c4, char c5, char c6) {
        arr[i] = newThingy;
    }

    public static void main(String[] args) {
        int nextThingy = -10;
        int sumArrElements = 0;
        for(int i = 0; i < iterations; i++) {
            thingiesArray[i] = i;
            sumArrElements = sumArrElements + thingiesArray[i];
        }

        for(int i = 0; i < iterations + 1; i++) {
            nextThingy = getThingies(thingiesArray, i, 'a', 'b', 'c', 'd', 'e', 'f') - i*1;
            setThingies(thingiesArray, nextThingy, i, 'a', 'b', 'c', 'd', 'e', 'f');
        }

    }
}

