
class Main {

    public static int runTest() {
                CondVirtBase test = new CondVirtExt();
		int result = 0;
       
        for (int i = 0; i < 100*1000; i++) {
            result += test.getThingies();
        }
		return result;
    }

    public static void main(String[] args) {
        int result = runTest();
        System.out.println("Result " + result);
    }

}
