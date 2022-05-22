#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  if (argc <= 1) {
    fprintf(2, "sleep requires at least one arg(time to sleep in tick)");
  }
  int time_to_sleep_in_tick = atoi(argv[1]);
  sleep(time_to_sleep_in_tick);
  exit(0);
}
