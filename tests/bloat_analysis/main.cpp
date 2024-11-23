

double get1(char const *input);
double get2(char const *input);
double get3(char const *input);
double get4(char const *input);
double get5(char const *input);
double get6(char const *input);
double get7(char const *input);
double get8(char const *input);
double get9(char const *input);
double get10(char const *input);

int main(int arg, char **argv) {
  double x = get1(argv[0]) + get2(argv[0]) + get3(argv[0]) + get4(argv[0]) +
             get5(argv[0]) + get6(argv[0]) + get7(argv[0]) + get8(argv[0]) +
             get9(argv[0]) + get10(argv[0]);

  return int(x);
}