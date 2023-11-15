extern void cal1_(double *a, double *b, double *c);

int cal_c()
{
    double a = 1.0;
    double b = 1.0;
    double c = 1.0;
    cal1_(&a, &b, &c);
    return c;
}
