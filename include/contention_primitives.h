#ifndef CONTENTION_PRIMITIVES_H_
#define CONTENTION_PRIMITIVES_H_

struct contention_arch {
    const char *name;
    unsigned long num_ports;
    const char **port_names;

    void (**contenders)(unsigned long *, unsigned long);
};

// Skylake
extern void contend_p0_skylake(unsigned long *out_buf, unsigned long num_entries);

extern void contend_p1_skylake(unsigned long *out_buf, unsigned long num_entries);

extern void contend_p5_skylake(unsigned long *out_buf, unsigned long num_entries);

extern void contend_p06_skylake(unsigned long *out_buf, unsigned long num_entries);

const char *skylake_ports[] = {"p0", "p1", "p5", "p06"};

static void (* skylake_contenders[])(unsigned long *, unsigned long) = {
        contend_p0_skylake, contend_p1_skylake, contend_p5_skylake, contend_p06_skylake
};

const static struct contention_arch skylake = {
        "Skylake",
        4,
        skylake_ports,
        skylake_contenders,
};

// Alder Lake P
extern void contend_p0_alder_lake_p(unsigned long *out_buf, unsigned long num_entries);

extern void contend_p1_alder_lake_p(unsigned long *out_buf, unsigned long num_entries);

extern void contend_p5_alder_lake_p(unsigned long *out_buf, unsigned long num_entries);

extern void contend_p06_alder_lake_p(unsigned long *out_buf, unsigned long num_entries);

const char *alder_lake_p_ports[] = {"p0", "p1", "p5", "p06"};

static void (* alder_lake_p_contenders[])(unsigned long *, unsigned long) = {
        contend_p0_alder_lake_p, contend_p1_alder_lake_p, contend_p5_alder_lake_p, contend_p06_alder_lake_p
};

const static struct contention_arch alder_lake_p = {
        "AlderLakeP",
        4,
        alder_lake_p_ports,
        alder_lake_p_contenders,
};

// Zen3
extern void contend_div_zen3(unsigned long *out_buf, unsigned long num_entries);

extern void contend_alu1_zen3(unsigned long *out_buf, unsigned long num_entries);

extern void contend_alu03_zen3(unsigned long *out_buf, unsigned long num_entries);

extern void contend_alu12_zen3(unsigned long *out_buf, unsigned long num_entries);

extern void contend_fp1_zen3(unsigned long *out_buf, unsigned long num_entries);

extern void contend_fp3_zen3(unsigned long *out_buf, unsigned long num_entries);

extern void contend_fp01_zen3(unsigned long *out_buf, unsigned long num_entries);

extern void contend_fp12_zen3(unsigned long *out_buf, unsigned long num_entries);

extern void contend_fp45_zen3(unsigned long *out_buf, unsigned long num_entries);

const char *zen3_ports[] = {"div", "alu1", "alu03", "alu12", "fp1", "fp3", "fp01", "fp12", "fp45"};

static void (* zen3_contenders[])(unsigned long *, unsigned long) = {
        contend_div_zen3, contend_alu1_zen3, contend_alu03_zen3,
        contend_alu12_zen3, contend_fp1_zen3, contend_fp3_zen3, contend_fp01_zen3, contend_fp12_zen3, contend_fp45_zen3
};

const static struct contention_arch zen3 = {
        "Zen3",
        9,
        zen3_ports,
        zen3_contenders,
};

const static struct contention_arch* architectures[] = {
        &skylake, &alder_lake_p, &zen3
};


#endif