using namespace std;

extern int pflag, aflag, vflag, opt, port;
extern bool ctrlc_flag;
extern char* directory;

void process_cml(int argc, char *argv[]) {
  while ((opt = getopt(argc, argv, "p:av")) != -1) {
    switch(opt) {
      case 'p':
        pflag = 1;
        try {
          port = stoi(optarg);
        } catch (invalid_argument &ia) {
          cerr << "Invalid argument: please enter an integer." << endl;
          exit(-1);
        }
        break;
      case 'a':
        aflag = 1;
        break;
      case 'v':
        vflag = 1;
        break;
      default:
        abort();
    }
  }
  if (aflag) {
    cerr << "Author: Ying Tsai / yingt" << endl;
    exit(-1);
  }
  int count = 0;
  if (optind < argc) {
    directory = argv[optind];
  } else {
    cout << "Please enter a directory." << endl;
    exit(-1);
  }
}
