/*-----------------------------------------------------------------------------
 *  tlbbench.cpp - A micro-benchmark for TLB penalties
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

const int CACHELINE_SZ = 64;

struct pchain_t {struct pchain_t *next;};

void run_tlbbench(uint64_t n, struct pchain_t *chains)
  __attribute__((optimize(0)));

struct pchain_t *tlbbench_init(int nslot,
                               int stride, char **mem) {
  *mem = new char[nslot * (stride + CACHELINE_SZ)];
  struct pchain_t *p = (struct pchain_t *)*mem;
  for (int i = 1; i < nslot; i++) {
    /* Slide gaps by CACHELINE_SZ */
    p->next = (struct pchain_t *)((*mem) + i * (stride + CACHELINE_SZ));
    p = p->next;
  }

  /* Link the tail to the head */
  p->next = (struct pchain_t *)*mem;
  return p->next;
}

void run_tlbbench(uint64_t n, struct pchain_t *chains) {
  for (uint64_t i = 0; i < n; i++)
    chains = chains->next;
}

void destroy_chains(char *mem) {
  delete[] mem;
}

} /* namespace: */

int main(int argc, char **argv) {
  const uint64_t nloop = 10000000;
  const int slide_min = 2;
  const int slide_max = 16;
  const int slots[] = {512, 1024, 2048, 4096, 8192};

  /* Show a header */
  fprintf(stdout,"Show TLB Penalty Benchmarks(ns):\n");
  fprintf(stdout, "Slot\t| Slide\t");
  for (int i = slide_min; i < slide_max; i++)
    fprintf(stdout, "\t2^%dB", i);
  fprintf(stdout, "\n");
  fprintf(stdout,"========================\n");

  for (int i = 0; i < sizeof(slots) / sizeof(slots[0]); i++) {
    std::vector<double> tv;

    /* Slide from 4B to 64KiB */
    for (int j = slide_min; j < slide_max; j++) {
      char *mem = NULL;
      struct pchain_t *p = tlbbench_init(slots[i], 1U << j, &mem);

      /* Do benchmarking */
      BenchmarkTimer t;
      run_tlbbench(nloop, p);
      tv.push_back(t.elapsed());

      destroy_chains(mem);
    }

    fprintf(stdout, "%d\t|\t", slots[i]);
    for (int j = 0; j < tv.size(); j++)
      fprintf(stdout, "\t%4.2lf", (tv[j] / nloop) * 1000000000);
    fprintf(stdout, "\n");
  }

  return 0;
}
