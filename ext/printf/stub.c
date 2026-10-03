// putchar_ stub for vendored eyalroz/printf.
//
// printf.c references putchar_ from its printf_()/vprintf_() paths, and
// linkers pull the whole printf.obj once any snprintf_ symbol is used, so
// the reference must resolve even though yapb never calls printf_() itself.
// Kept as a separate tu (not inside printf.c) to keep the vendored file
// verbatim and upstream-updatable.

void putchar_ (char character) {
   (void) character; // unreachable in practice, output goes nowhere
}
