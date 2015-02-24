#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <locale.h>

#include <X11/Xlib.h>

char *tzwarsaw = "Europe/Warsaw";

static Display *dpy;

char *
smprintf(char *fmt, ...) {
    va_list fmtargs;
    char *ret;
    int len;

    va_start(fmtargs, fmt);
    len = vsnprintf(NULL, 0, fmt, fmtargs);
    va_end(fmtargs);

    ret = malloc(++len);
    if (ret == NULL) {
        perror("malloc");
        exit(1);
    }

    va_start(fmtargs, fmt);
    vsnprintf(ret, len, fmt, fmtargs);
    va_end(fmtargs);

    return ret;
}

void
settz(char *tzname) {
    setenv("TZ", tzname, 1);
}

char *
mktimes(char *fmt, char *tzname) {
    char buf[129];
    time_t tim;
    struct tm *timtm;

    memset(buf, 0, sizeof(buf));
    settz(tzname);
    tim = time(NULL);
    timtm = localtime(&tim);
    if (timtm == NULL) {
        perror("localtime");
        exit(1);
    }

    if (!strftime(buf, sizeof(buf) - 1, fmt, timtm)) {
        fprintf(stderr, "strftime == 0\n");
        exit(1);
    }

    return smprintf("%s", buf);
}

void
setstatus(char *str) {
    XStoreName(dpy, DefaultRootWindow(dpy), str);
    XSync(dpy, False);
}

char *
loadavg(void) {
    double avgs[3];

    if (getloadavg(avgs, 3) < 0) {
        perror("getloadavg");
        exit(1);
    }

    return smprintf("%.2f %.2f %.2f", avgs[0], avgs[1], avgs[2]);
}

char *
getcpuutil(void) {
    long double a[4], b[4], loadavg;
    FILE *fp;
    fp = fopen("/proc/stat", "r");
    fscanf(fp, "%*s %Lf %Lf %Lf %Lf", &a[0], &a[1], &a[2], &a[3]);
    fclose(fp);
    sleep(1);
    fp = fopen("/proc/stat", "r");
    fscanf(fp, "%*s %Lf %Lf %Lf %Lf", &b[0], &b[1], &b[2], &b[3]);
    fclose(fp);
    loadavg = ((b[0] + b[1] + b[2]) - (a[0] + a[1] + a[2])) / ((b[0] + b[1] + b[2] + b[3]) - (a[0] + a[1] + a[2] + a[3])) * 100;
    return smprintf("%.0Lf", loadavg);
}

char *
getcputemp(void) {
    int temperature;
    FILE *fp;

    fp = fopen("/sys/devices/platform/coretemp.0/hwmon/hwmon0/temp1_input", "r");
    fscanf(fp, "%d", &temperature);
    fclose(fp);

    return smprintf("%d", temperature / 1000);
}

char *
getmemusage(void) {
    FILE *fp;
    fp = fopen("/proc/meminfo", "r");
    char label[128];
    char unit[10];
    int value;

    int total = 0;
    int used = 0;

    int done = 0;
    while (done != 15) {
        fscanf(fp, "%s %d %s", label, &value, unit);

        if (strncmp(label, "MemTotal:", 128) == 0) {
            total = value;
            used += value;
            done |= 1;
        } else if (strncmp(label, "MemFree:", 128) == 0) {
            used -= value;
            done |= 2;
        } else if (strncmp(label, "Buffers:", 128) == 0) {
            used -= value;
            done |= 4;
        } else if (strncmp(label, "Cached:", 128) == 0) {
            used -= value;
            done |= 8;
/*        } else if (strncmp(label, "SwapTotal:", 128) == 0) {
            total += value;
            done |= 16;
        } else if (strncmp(label, "SwapFree:", 128) == 0) {
            used -= value;
            done |= 32;
        } else if (strncmp(label, "SwapCached:", 128) == 0) {
            used -= value;
            done |= 64;*/
        }
    }
    fclose(fp);
    return smprintf("%d", used * 100 / total);
}

char *
getswapusage(void) {
    FILE *fp;
    fp = fopen("/proc/meminfo", "r");
    char label[128];
    int value;

    int total = 0;
    int used = 0;

    int done = 0;
    while (done != 7) {
        fscanf(fp, "%s %d %*s", label, &value);

        if (strncmp(label, "SwapTotal:", 128) == 0) {
            total = value;
            used += value;
            done |= 1;
        } else if (strncmp(label, "SwapFree:", 128) == 0) {
            used -= value;
            done |= 2;
        } else if (strncmp(label, "SwapCached:", 128) == 0) {
            used -= value;
            done |= 4;
        }
    }
    fclose(fp);
    return smprintf("%d", used * 100 / total);
}

int
main(void) {
    char *status;
    char *avgs;
    char *cpumhz;
    char *tmwarsaw;
    char *temperature;
    char *memusage;
    char *swapusage;

    if (!(dpy = XOpenDisplay(NULL))) {
        fprintf(stderr, "dwmstatus: cannot open display.\n");
        return 1;
    }

    setlocale(LC_TIME, "pl_PL.UTF-8");

    for (; ; sleep(10)) {
        avgs = loadavg();
        cpumhz = getcpuutil();
        tmwarsaw = mktimes("%a %H:%M", tzwarsaw);
        temperature = getcputemp();
        memusage = getmemusage();
        swapusage = getswapusage();

        status = smprintf("LOAD %s | MEM %s%% SWAP %s%% | CPU %s%% temp: %s°C | %s",
                avgs, memusage, swapusage, cpumhz, temperature, tmwarsaw);
        setstatus(status);
        free(avgs);
        free(cpumhz);
        free(tmwarsaw);
        free(temperature);
        free(memusage);
        free(swapusage);
        free(status);
    }

    XCloseDisplay(dpy);

    return 0;
}

