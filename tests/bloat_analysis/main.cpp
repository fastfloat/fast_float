

double get1(const char *input);
double get2(const char *input);
double get3(const char *input);
double get4(const char *input);
double get5(const char *input);
double get6(const char *input);
double get7(const char *input);
double get8(const char *input);
double get9(const char *input);
double get10(const char *input);

int main(int arg, char **argv) {
  double x = get1(argv[0]) + get2(argv[0]) + get3(argv[0]) + get4(argv[0]) +
             get5(argv[0]) + get6(argv[0]) + get7(argv[0]) + get8(argv[0]) +
             get9(argv[0]) + get10(argv[0]);

  return int(x);
}