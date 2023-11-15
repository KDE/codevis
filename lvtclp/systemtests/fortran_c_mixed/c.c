extern void cal1_(double *a, double *b, double *c);

int cal_c()
{
    double a = 1.0;
    double b = 1.0;
    double c = 1.0;
    cal1_(&a, &b, &c);
    return c;
}

// A function that has '_' suffix, but it is implemented in C (not Fortran binding)
void c_func_(); // Declaration (up to now, don't know if there'll be a definition in C code)
void c_func_()
{
} // Definition. Assume there won't be a Fortran version.
