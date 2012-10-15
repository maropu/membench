/*-----------------------------------------------------------------------------
 *  cachebench.cpp - A micro-benchmark for cache penalties
 *
 *  Coding-Style: google-styleguide
 *      https://code.google.com/p/google-styleguide/
 *
 *  Copyright 2012 Takeshi Yamamuro <yamamuro.takeshi_at_lab.ntt.co.jp>
 *-----------------------------------------------------------------------------
 */

#include <vector>

#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>

namespace {

class BenchmarkTimer {
 public:
  BenchmarkTimer()
    : base_(get_time()) {}
  ~BenchmarkTimer() throw() {}

  double elapsed() const {
    return get_time() - base_;
  }

  void reset() {
    base_ = get_time();
  }

 private:
  double  base_;

  static double get_time() {
    struct timeval  tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + static_cast<double>(tv.tv_usec*1e-6);
  }
};

struct pchain_t {struct pchain_t *next;};

void run_cachebench(uint64_t n, struct pchain_t *chains)
  __attribute__((optimize(0)));

struct pchain_t *cachebench_init(int r,
                                 int stride, char **mem) {
  *mem = new char[r];
  struct pchain_t *p = (struct pchain_t *)*mem;
  for (int i = 1; i < r / stride; i++) {
    p->next = (struct pchain_t *)((*mem) + i * stride);
    p = p->next;
  }

  /* Link the tail to the head */
  p->next = (struct pchain_t *)*mem;
  return p->next;
}

void run_cachebench(uint64_t n, struct pchain_t *chains) {
  for (uint64_t i = 0; i < n; i++)
    chains = chains->next;
}

void destroy_chains(char *mem) {
  delete[] mem;
}

} /* namespace: */

int main(int argc, char **argv) {
  const uint64_t nloop = 1000000;
  const int range_min = 16;
  const int range_max = 27;
  const int strides[] = {16, 64, 256, 1024, 4096};

  /* Show a header */
  fprintf(stdout,"Show Cache Penalty Benchmarks(ns):\n");
  fprintf(stdout, "Stride\t| Memory Range");
  for (int i = range_min; i < range_max; i++)
    fprintf(stdout, "\t2^%dB", i);
  fprintf(stdout, "\n");
  fprintf(stdout,"========================\n");

  for (int i = 0; i < sizeof(strides) / sizeof(strides[0]); i++) {
    std::vector<double> tv;

    /* Range from 64KiB to 64MiB */
    for (int j = range_min; j < range_max; j++) {
      char *mem = NULL;
      struct pchain_t *p = cachebench_init(1U << j, strides[i], &mem);

      /* Do benchmarking */
      BenchmarkTimer t;
      run_cachebench(nloop, p);
      tv.push_back(t.elapsed());

      destroy_chains(mem);
    }

    fprintf(stdout, "%dB\t|\t", strides[i]);
    for (int j = 0; j < tv.size(); j++)
      fprintf(stdout, "\t%4.2lf", (tv[j] / nloop) * 1000000000);
    fprintf(stdout, "\n");
  }

  return 0;
}
