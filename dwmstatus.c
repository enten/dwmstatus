#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <locale.h>
#include <stdint.h>

#include <sys/statvfs.h>

#include <X11/Xlib.h>
#include <alsa/asoundlib.h>

#define BATT_NOW        "/sys/class/power_supply/BAT0/energy_now"
#define BATT_FULL       "/sys/class/power_supply/BAT0/energy_full"
#define BATT_STATUS     "/sys/class/power_supply/BAT0/status"

#define MB 1048576
#define GB 1073741824

char *tzwarsaw = "Europe/Paris";

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

    fp = fopen("/sys/devices/platform/coretemp.0/hwmon/hwmon2/temp1_input", "r");
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

char *
getbattery(){
    long lnum1, lnum2 = 0;
    char *status = malloc(sizeof(char)*12);
    char s = '?';
    FILE *fp = NULL;
    if ((fp = fopen(BATT_NOW, "r"))) {
        fscanf(fp, "%ld\n", &lnum1);
        fclose(fp);
        fp = fopen(BATT_FULL, "r");
        fscanf(fp, "%ld\n", &lnum2);
        fclose(fp);
        fp = fopen(BATT_STATUS, "r");
        fscanf(fp, "%s\n", status);
        fclose(fp);
        if (strcmp(status,"Charging") == 0)
            s = '+';
        if (strcmp(status,"Discharging") == 0)
            s = '-';
        if (strcmp(status,"Full") == 0)
            s = '=';
        return smprintf("%c%ld", s,(lnum1/(lnum2/100)));
    }
    else return smprintf("");
}

char *getdatetime() {
    time_t current;
    char buf[18];

    time(&current);
    strftime(buf, 14, "%m/%d %H:%M", localtime(&current));

    return smprintf("%s",buf);
}

char *get_volume()
{
    snd_mixer_t *handle;
    snd_mixer_elem_t *elem;
    snd_mixer_selem_id_t *s_elem;

    snd_mixer_open(&handle, 0);
    snd_mixer_attach(handle, "default");
    snd_mixer_selem_register(handle, NULL, NULL);
    snd_mixer_load(handle);
    snd_mixer_selem_id_malloc(&s_elem);
    snd_mixer_selem_id_set_name(s_elem, "Master");

    elem = snd_mixer_find_selem(handle, s_elem);

    if (NULL == elem)
    {
        snd_mixer_selem_id_free(s_elem);
        snd_mixer_close(handle);

        exit(EXIT_FAILURE);
    }

    long int vol, max, min, percent;

    snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
    snd_mixer_selem_get_playback_volume(elem, 0, &vol);

    percent = (vol * 100) / max;

    snd_mixer_selem_id_free(s_elem);
    snd_mixer_close(handle);

    return smprintf("%ld", percent);
}

char *get_ssd()
{
    uintmax_t percent = 0;

    struct statvfs ssd;
    statvfs(getenv("HOME"), &ssd);

    percent = ((ssd.f_blocks - ssd.f_bfree) * ssd.f_bsize) / GB;

    return smprintf("%ld", percent);
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
    char *ssdusage;
    char *datetime = NULL;
    char *volume = NULL;

    if (!(dpy = XOpenDisplay(NULL))) {
        fprintf(stderr, "dwmstatus: cannot open display.\n");
        return 1;
    }

    setlocale(LC_TIME, "fr_FR.UTF-8");

    for (; ; sleep(10)) {
        //avgs = loadavg();
	    avgs = getbattery();
        cpumhz = getcpuutil();
        //tmwarsaw = mktimes("%a %H:%M", tzwarsaw);
	//tmwarsaw = *getdatetime();
        datetime = getdatetime();
        //temperature = getcputemp();
        memusage = getmemusage();
        //swapusage = getswapusage();
        ssdusage = get_ssd();
        volume = get_volume();

        status = smprintf("\033[34;01BAT %s%% | CPU %s%% MEM %s%% SSD %s%% | VOL %s%% | %s",
                avgs, cpumhz, memusage, ssdusage, volume, datetime);

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

