/*
 * Yabar - A modern and lightweight status bar for X window managers.
 *
 * Copyright (c) 2016, George Badawi
 * See LICENSE for more information.
 *
 */

#include "yabar.h"

void ya_int_date(ya_block_t * blk);
void ya_int_uptime(ya_block_t * blk);
void ya_int_memory(ya_block_t * blk);
void ya_int_thermal(ya_block_t *blk);
void ya_int_brightness(ya_block_t *blk);
void ya_int_bandwidth(ya_block_t *blk);
void ya_int_cpu(ya_block_t *blk);
void ya_int_diskio(ya_block_t *blk);
void ya_int_network(ya_block_t *blk);

struct reserved_blk ya_reserved_blks[YA_INTERNAL_LEN] = { 
	{"YA_INT_DATE", ya_int_date},
	{"YA_INT_UPTIME", ya_int_uptime},
	{"YA_INT_THERMAL", ya_int_thermal},
	{"YA_INT_BRIGHTNESS", ya_int_brightness},
	{"YA_INT_BANDWIDTH", ya_int_bandwidth},
	{"YA_INT_MEMORY", ya_int_memory},
	{"YA_INT_CPU", ya_int_cpu},
	{"YA_INT_DISKIO", ya_int_diskio},
	{"YA_INT_NETWORK", ya_int_network}
}; 

//#define YA_INTERNAL

#ifdef YA_INTERNAL

inline void ya_setup_prefix_suffix(ya_block_t *blk, size_t * prflen, size_t *suflen, char **startstr) {
	if(blk->internal->prefix) {
		*prflen = strlen(blk->internal->prefix);
		if(*prflen) {
			strcpy(blk->buf, blk->internal->prefix);
			*startstr += *prflen;
		}
	}
	if(blk->internal->suffix) {
		*suflen = strlen(blk->internal->suffix);
	}
}


#include <time.h>
void ya_int_date(ya_block_t * blk) {
	time_t rawtime;
	struct tm * ya_tm;
	char *startstr = blk->buf;
	size_t prflen=0,suflen=0;
	ya_setup_prefix_suffix(blk, &prflen, &suflen, &startstr);
	while(1) {
		time(&rawtime);
		ya_tm = localtime(&rawtime);
		strftime(startstr, 100, blk->internal->option[0], ya_tm);
		if(suflen)
			strcat(blk->buf, blk->internal->suffix);
		ya_draw_pango_text(blk);
		sleep(blk->sleep);
	}
}

#include <sys/sysinfo.h>

void ya_int_uptime(ya_block_t *blk) {
	struct sysinfo ya_sysinfo;
	long hr, tmp;
	uint8_t min;
	char *startstr = blk->buf;
	size_t prflen=0,suflen=0;
	ya_setup_prefix_suffix(blk, &prflen, &suflen, &startstr);
	while(1) {
		sysinfo(&ya_sysinfo);
		tmp = ya_sysinfo.uptime;
		hr = tmp/3600;
		tmp %= 3600;
		min = tmp/60;
		//tmp %= 60;
		sprintf(startstr, "%ld:%02d", hr, min);
		if(suflen)
			strcat(blk->buf, blk->internal->suffix);
		ya_draw_pango_text(blk);
		sleep(blk->sleep);
	}
}


void ya_int_thermal(ya_block_t *blk) {
	FILE *tfile;
	int temp, wrntemp, crttemp;
	uint32_t oldbg, oldfg;
	uint32_t wrnbg, wrnfg; //warning colors
	uint32_t crtbg, crtfg; //critical colors
	char *startstr = blk->buf;
	size_t prflen=0,suflen=0;
	ya_setup_prefix_suffix(blk, &prflen, &suflen, &startstr);
	if(blk->attr & BLKA_BGCOLOR)
		oldbg = blk->bgcolor;
	else
		oldbg = blk->bar->bgcolor;
	oldfg = blk->fgcolor;
	char fpath[128];
	snprintf(fpath, 128, "/sys/class/thermal/%s/temp", blk->internal->option[0]);

	if((blk->internal->option[1]==NULL) ||
			(sscanf(blk->internal->option[1], "%d %u %u", &crttemp, &crtfg, &crtbg)!=3)) {
		crttemp = 70;
		crtbg = 0xFFED303C;
		crtfg = blk->fgcolor;
	}
	if((blk->internal->option[2]==NULL) ||
			(sscanf(blk->internal->option[2], "%d %u %u", &wrntemp, &wrnfg, &wrnbg)!=3)) {
		wrntemp = 58;
		wrnbg = 0xFFF4A345;
		wrnfg = blk->fgcolor;
	}
	tfile = fopen(fpath, "r");
	if (tfile == NULL) {
		fprintf(stderr, "Error opening file %s\n", fpath);
		strncpy(blk->buf, "BLOCK ERROR!", strlen("BLOCK ERROR!"));
		ya_draw_pango_text(blk);
		pthread_detach(blk->thread);
		pthread_exit(NULL);
	}
	fclose(tfile);
	while (1) {
		tfile = fopen(fpath, "r");
		fscanf(tfile, "%d", &temp);
		temp/=1000;

		if(temp > crttemp) {
			xcb_change_gc(ya.c, blk->gc, XCB_GC_FOREGROUND, (const uint32_t[]){crtbg});
			blk->fgcolor = crtfg;
		}
		else if (temp > wrntemp) {
			xcb_change_gc(ya.c, blk->gc, XCB_GC_FOREGROUND, (const uint32_t[]){wrnbg});
			blk->fgcolor = wrnfg;
		}
		else {
			xcb_change_gc(ya.c, blk->gc, XCB_GC_FOREGROUND, (const uint32_t[]){oldbg});
			blk->fgcolor = oldfg;
		}

		sprintf(startstr, "%d", temp);
		if(suflen)
			strcat(blk->buf, blk->internal->suffix);
		ya_draw_pango_text(blk);
		fclose(tfile);
		sleep(blk->sleep);
	}

}

void ya_int_brightness(ya_block_t *blk) {
	int bright;
	char *startstr = blk->buf;
	size_t prflen=0,suflen=0;
	ya_setup_prefix_suffix(blk, &prflen, &suflen, &startstr);
	FILE *tfile;
	char fpath[128];
	snprintf(fpath, 128, "/sys/class/backlight/%s/brightness", blk->internal->option[0]);
	tfile = fopen(fpath, "r");
	if (tfile == NULL) {
		fprintf(stderr, "Error opening file %s\n", fpath);
		strncpy(blk->buf, "BLOCK ERROR!", strlen("BLOCK ERROR!"));
		ya_draw_pango_text(blk);
		pthread_detach(blk->thread);
		pthread_exit(NULL);
	}
	while(1) {
		tfile = fopen(fpath, "r");
		fscanf(tfile, "%d", &bright);
		sprintf(startstr, "%d", bright);
		if(suflen)
			strcat(blk->buf, blk->internal->suffix);
		ya_draw_pango_text(blk);
		fclose(tfile);
		sleep(blk->sleep);
	}
}

void ya_int_bandwidth(ya_block_t * blk) {
	unsigned long rx, tx, orx, otx;
	unsigned int rxrate, txrate; 
	FILE *rxfile, *txfile;
	char rxpath[128];
	char txpath[128];
	char rxc, txc;
	char *startstr = blk->buf;
	size_t prflen=0,suflen=0;
	char dnstr[20], upstr[20];
	ya_setup_prefix_suffix(blk, &prflen, &suflen, &startstr);
	snprintf(rxpath, 128, "/sys/class/net/%s/statistics/rx_bytes", blk->internal->option[0]);
	snprintf(txpath, 128, "/sys/class/net/%s/statistics/tx_bytes", blk->internal->option[0]);
	if(blk->internal->option[1]) {
		sscanf(blk->internal->option[1], "%s %s", dnstr, upstr);
	}
	rxfile = fopen(rxpath, "r");
	txfile = fopen(txpath, "r");
	if (rxfile == NULL || txfile == NULL) {
		fprintf(stderr, "Error opening file %s or %s\n", rxpath, txpath);
		strncpy(blk->buf, "BLOCK ERROR!", strlen("BLOCK ERROR!"));
		ya_draw_pango_text(blk);
		pthread_detach(blk->thread);
		pthread_exit(NULL);
	}
	else {
		fscanf(rxfile, "%lu", &orx);
		fscanf(txfile, "%lu", &otx);
	}
	fclose(rxfile);
	fclose(txfile);
	while(1) {
		txc = rxc = 'K';
		rxfile = fopen(rxpath, "r");
		txfile = fopen(txpath, "r");

		fscanf(rxfile, "%lu", &rx);
		fscanf(txfile, "%lu", &tx);

		rxrate = (rx - orx)/((blk->sleep)*1024);
		txrate = (tx - otx)/((blk->sleep)*1024);

		if(rxrate > 1024) {
			rxrate/=1024;
			rxc = 'M';
		}
		if(txrate > 1024) {
			txrate/=1024;
			txc = 'M';
		}


		orx = rx;
		otx = tx;

		sprintf(startstr, "%s%u%c %s%u%c", dnstr, rxrate, rxc, upstr, txrate, txc);
		if(suflen)
			strcat(blk->buf, blk->internal->suffix);
		ya_draw_pango_text(blk);
		fclose(rxfile);
		fclose(txfile);
		sleep(blk->sleep);
	}
	
}

void ya_int_memory(ya_block_t *blk) {
	unsigned long total, free, cached, buffered;
	float used;
	FILE *tfile;
	char tline[50];
	char unit;
	char *startstr = blk->buf;
	size_t prflen=0,suflen=0;
	ya_setup_prefix_suffix(blk, &prflen, &suflen, &startstr);
	tfile = fopen("/proc/meminfo", "r");
	if (tfile == NULL) {
		fprintf(stderr, "Error opening file %s\n", "/proc/meminfo");
		strncpy(blk->buf, "BLOCK ERROR!", strlen("BLOCK ERROR!"));
		ya_draw_pango_text(blk);
		pthread_exit(NULL);
	}
	fclose(tfile);
	while(1) {
		tfile = fopen("/proc/meminfo", "r");
		while(fgets(tline, 50, tfile) != NULL) {
			sscanf(tline, "MemTotal: %lu kB\n", &total);
			sscanf(tline, "MemFree: %lu kB\n", &free);
			sscanf(tline, "Buffers: %lu kB\n", &buffered);
			sscanf(tline, "Cached: %lu kB\n", &cached);
		}
		used = (float)(total - free - buffered - cached)/1024.0;
		unit = 'M';
		if(((int)used)>1024) {
			used = used/1024.0;
			unit = 'G';
		}
		sprintf(startstr, "%.1f%c", used, unit);
		if(suflen)
			strcat(blk->buf, blk->internal->suffix);
		ya_draw_pango_text(blk);
		fclose(tfile);
		sleep(blk->sleep);
	}
}


void ya_int_cpu(ya_block_t *blk) {
	FILE *tfile;
	long double old[4], cur[4], ya_avg=0.0;
	char *startstr = blk->buf;
	size_t prflen=0,suflen=0;
	char cpustr[20];
	ya_setup_prefix_suffix(blk, &prflen, &suflen, &startstr);
	tfile = fopen("/proc/stat", "r");
	if (tfile == NULL) {
		fprintf(stderr, "Error opening file %s\n", "/proc/stat");
		strncpy(blk->buf, "BLOCK ERROR!", strlen("BLOCK ERROR!"));
		ya_draw_pango_text(blk);
		pthread_exit(NULL);
	}
	else {
		fscanf(tfile,"%s %Lf %Lf %Lf %Lf",cpustr, &old[0],&old[1],&old[2],&old[3]);
	}
	fclose(tfile);
	while(1) {
		tfile = fopen("/proc/stat", "r");
		fscanf(tfile,"%s %Lf %Lf %Lf %Lf", cpustr, &cur[0],&cur[1],&cur[2],&cur[3]);
		ya_avg = ((cur[0]+cur[1]+cur[2]) - (old[0]+old[1]+old[2])) / ((cur[0]+cur[1]+cur[2]+cur[3]) - (old[0]+old[1]+old[2]+old[3]));
		for(int i=0; i<4;i++)
			old[i]=cur[i];
		ya_avg *= 100.0;
		sprintf(startstr, "%.1Lf", ya_avg);
		if(suflen)
			strcat(blk->buf, blk->internal->suffix);
		ya_draw_pango_text(blk);
		fclose(tfile);
		sleep(blk->sleep);
	}

}


void ya_int_diskio(ya_block_t *blk) {
	unsigned long tdo[11], tdc[11];
	unsigned long drd=0, dwr=0;
	char crd, cwr;
	FILE *tfile;
	char tpath[100];
	char *startstr = blk->buf;
	size_t prflen=0,suflen=0;
	char dnstr[20], upstr[20];
	ya_setup_prefix_suffix(blk, &prflen, &suflen, &startstr);
	if(blk->internal->option[1]) {
		sscanf(blk->internal->option[1], "%s %s", dnstr, upstr);
	}
	snprintf(tpath, 100, "/sys/class/block/%s/stat", blk->internal->option[0]);
	tfile = fopen(tpath, "r");
	if (tfile == NULL) {
		fprintf(stderr, "Error opening file %s\n", "/proc/stat");
		strncpy(blk->buf, "BLOCK ERROR!", strlen("BLOCK ERROR!"));
		ya_draw_pango_text(blk);
		pthread_detach(blk->thread);
		pthread_exit(NULL);
	}
	else {
		fscanf(tfile,"%lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu", &tdo[0], &tdo[1], &tdo[2], &tdo[3], &tdo[4], &tdo[5], &tdo[6], &tdo[7], &tdo[8], &tdo[9], &tdo[10]);
	}
	fclose(tfile);
	while(1) {
		tfile = fopen(tpath, "r");
		fscanf(tfile,"%lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu", &tdc[0], &tdc[1], &tdc[2], &tdc[3], &tdc[4], &tdc[5], &tdc[6], &tdc[7], &tdc[8], &tdc[9], &tdc[10]);
		drd = (unsigned long)(((float)(tdc[2] - tdo[2])*0.5)/((float)(blk->sleep)));
		dwr = (unsigned long)(((float)(tdc[6] - tdo[6])*0.5)/((float)(blk->sleep)));
		crd = cwr = 'K';
		if(drd >1024) {
			drd /= 1024;
			crd = 'M';
		}
		if(dwr >1024) {
			dwr /= 1024;
			cwr = 'M';
		}
		sprintf(startstr, "%s%lu%c %s%lu%c", dnstr, drd, crd, upstr, dwr, cwr);
		for(int i=0; i<11;i++)
			tdo[i] = tdc[i];
		if(suflen)
			strcat(blk->buf, blk->internal->suffix);

		ya_draw_pango_text(blk);
		fclose(tfile);
		sleep(blk->sleep);
	}

}

#define _GNU_SOURCE
#include <sys/socket.h>
#include <ifaddrs.h>
#include <linux/if_link.h>
#include <arpa/inet.h>
#include <netdb.h>

void ya_int_network(ya_block_t *blk) {
	pthread_exit(NULL);
	/*
	struct ifaddrs *ifaddr, *ifa;
	int family, s, n;
	char host[1025];
	if(getifaddrs(&ifaddr) == -1) {
		fprintf(stderr, "error in getifaddrs\n");
		pthread_exit(NULL);
	}
	for(ifa = ifaddr; ifa; ifa = ifa->ifa_next, n++) {
		if(ifa == NULL)
			continue;
		family = ifa->ifa_addr->sa_family;
		//printf("%s\n", ifa->ifa_name);
		if (family == AF_INET || family == AF_INET6) { 
			s = getnameinfo(ifa->ifa_addr, 
					(family == AF_INET) ? sizeof(struct sockaddr_in) :
					sizeof(struct sockaddr_in6), host, 1025,
					NULL, 0, NI_NUMERICHOST);
			printf("%s %s\n", ifa->ifa_name, host);
		}
	}
	while(1) {

		ya_draw_pango_text(blk);
		sleep(blk->sleep);
		memset(blk->buf, '\0', 12);
	}
	*/
}
#endif //YA_INTERNAL
