/*
 * Copyright (c) 2026 luke8086
 * Distributed under the terms of GPL-2 License.
 */

/*
 * src/nf_fcopy.c - binary concatenate files into one
 */

 #include <stdio.h>
 #include <stdlib.h>

 #define BUF_SIZE 4096

 static int
 copy_file(FILE *out, const char *name)
 {
     FILE *in;
     unsigned char buf[BUF_SIZE];
     size_t n;

     in = fopen(name, "rb");

     if (!in) {
         fprintf(stderr, "cannot open '%s'\n", name);
         return -1;
     }

     while ((n = fread(buf, 1, sizeof buf, in)) > 0) {
         if (fwrite(buf, 1, n, out) != n) {
             fprintf(stderr, "write error\n");
             fclose(in);
             return -1;
         }
     }

     if (ferror(in)) {
         fprintf(stderr, "read error on '%s'\n", name);
         fclose(in);
         return -1;
     }

     fclose(in);

     return 0;
 }

 int
 main(int argc, char **argv)
 {
     FILE *out;
     int i;

     if (argc < 3) {
         fprintf(stderr, "Usage: nf_fcopy file1 file2 ... outputfile\n");
         return 1;
     }

     out = fopen(argv[argc - 1], "wb");

     if (!out) {
         fprintf(stderr, "cannot create '%s'\n", argv[argc - 1]);
         return 1;
     }

     for (i = 1; i < argc - 1; i++) {
         if (copy_file(out, argv[i]) != 0) {
             fclose(out);
             return 1;
         }
     }

     if (fclose(out) != 0) {
         fprintf(stderr, "write error\n");
         return 1;
     }

     return 0;
 }
