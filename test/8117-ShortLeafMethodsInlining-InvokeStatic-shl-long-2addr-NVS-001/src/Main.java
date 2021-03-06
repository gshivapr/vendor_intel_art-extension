class Main {
    final static int iterations = 10;

    public static long simple_method(long jj, long kk) {
        jj = jj << kk;
        jj = jj << kk;
        return jj;
    }

    public static void main(String[] args) {
        long workJ = 0xFFEFAF23;
        long workK = 0xF8E9BD5A;

        System.out.println("Initial workJ value is " + workJ);

        for(long i = 0; i < iterations; i++) {
            workJ = simple_method(workJ, workK) + i;
        }

        System.out.println("Final workJ value is " + workJ);
    }
}
