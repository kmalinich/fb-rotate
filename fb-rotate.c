// fb-rotate.c
//
// Compile with:
// gcc -w -o fb-rotate fb-rotate.c -framework IOKit -framework ApplicationServices

#include <getopt.h>
#include <IOKit/graphics/IOGraphicsLib.h>
#include <ApplicationServices/ApplicationServices.h>

#define PROGNAME "fb-rotate"
#define MAX_DISPLAYS 16

// kIOFBSetTransform comes from <IOKit/graphics/IOGraphicsTypesPrivate.h>
// in the source for the IOGraphics family


enum {
	kIOFBSetTransform = 0x00000400,
};


void usage(void) {
	fprintf(
		stderr,
		"Usage:\n"
		"  %s -i                                     Show display info\n"
		"  %s -l                                     List displays\n"
		"  %s -d <display ID> -m                     Set <display ID> as main display\n"
		"  %s -d <display ID> -r <0|90|180|270|1>    Rotate <display ID>\n"
		"\n"
		"Display ID shortcuts:\n"
		"  -1 : Internal monitor\n"
		"   0 : Main monitor\n"
		"   1 : First non-internal monitor\n"
		"\n"
		"Rotation shortcuts:\n"
		"  -r 1 signfies 90 if currently not rotated; otherwise 0 (i.e. toggle)\n",
		PROGNAME, PROGNAME, PROGNAME, PROGNAME
	);

	exit(1);
}

void listDisplays(void) {
	CGDisplayErr      dErr;
	CGDisplayCount    displayCount, i;
	CGDirectDisplayID mainDisplay;
	CGDisplayCount    maxDisplays = MAX_DISPLAYS;
	CGDirectDisplayID onlineDisplays[MAX_DISPLAYS];

	mainDisplay = CGMainDisplayID();

	dErr = CGGetOnlineDisplayList(maxDisplays, onlineDisplays, &displayCount);

	if (dErr != kCGErrorSuccess) {
		fprintf(stderr, "CGGetOnlineDisplayList: error %d.\n", dErr);
		exit(1);
	}

	printf("|------------------------------------------------------|\n");
	printf("| ID         | resX | resY | Active | Main  | Internal |\n");
	printf("|------------------------------------------------------|\n");

	for (i = 0; i < displayCount; i++) {
		CGDirectDisplayID dID = onlineDisplays[i];

		printf(
			"| 0x%08x | %4d | %4d | %6s | %5s | %8s |\n",
			dID,
			(int) CGDisplayPixelsWide(dID),
			(int) CGDisplayPixelsHigh(dID),
			CGDisplayIsActive(dID)  ? "Yes" : "No",
			(dID == mainDisplay)    ? "Yes" : "No",
			CGDisplayIsBuiltin(dID) ? "Yes" : "No"
		);

		printf("|------------------------------------------------------|\n");
	}

	exit(0);
}

void infoDisplays(void) {
	CGDisplayErr      dErr;
	CGDisplayCount    displayCount, i;
	CGDirectDisplayID mainDisplay;
	CGDisplayCount    maxDisplays = MAX_DISPLAYS;
	CGDirectDisplayID onlineDisplays[MAX_DISPLAYS];

	CGEventRef ourEvent = CGEventCreate(NULL);
	CGPoint    ourLoc   = CGEventGetLocation(ourEvent);

	CFRelease(ourEvent);

	mainDisplay = CGMainDisplayID();

	dErr = CGGetOnlineDisplayList(maxDisplays, onlineDisplays, &displayCount);

	if (dErr != kCGErrorSuccess) {
		fprintf(stderr, "CGGetOnlineDisplayList: error %d.\n", dErr);
		exit(1);
	}

	printf(" --- ------------    ------ ------    ------- ------- ------- -------    -------    -------- ------- ---------- \n");
	printf("| # | ID         |  | resX | resY |  | bX0   | bY0   | bX1   | bY1   |  | Angle |  | Active | Main  | Internal |\n");
	printf("|---|------------|  |------|------|  |-------|-------|-------|-------|  |-------|  |--------|-------|----------|\n");

	for (i = 0; i < displayCount; i++) {
		CGDirectDisplayID dID = onlineDisplays[i];

		printf(
			"| %d | 0x%08x |  | %-4d | %-4d |  | %-5.0f | %-5.0f | %-5.0f | %-5.0f |  | %-5.0f |  | %-6s | %-5s | %-8s |\n",
			CGDisplayUnitNumber(dID),
			dID,
			(int) CGDisplayPixelsWide(dID),
			(int) CGDisplayPixelsHigh(dID),
			CGRectGetMinX(CGDisplayBounds(dID)),
			CGRectGetMinY(CGDisplayBounds(dID)),
			CGRectGetMaxX(CGDisplayBounds(dID)),
			CGRectGetMaxY(CGDisplayBounds(dID)),
			CGDisplayRotation(dID),
			CGDisplayIsActive(dID)  ? "Yes" : "No",
			(dID == mainDisplay)    ? "Yes" : "No",
			CGDisplayIsBuiltin(dID) ? "Yes" : "No"
		);

		if (i == (displayCount - 1)) {
			printf(" --- ------------    ------ ------    ------- ------- ------- -------    -------    -------- ------- ---------- \n");
		}
		else {
			printf("|---|------------|  |------|------|  |-------|-------|-------|-------|  |-------|  |--------|-------|----------|\n");
		}
	}

	printf("\nCursor position : %1.1f, %1.1f\n", (float)ourLoc.x, (float)ourLoc.y);

	exit(0);
}

void setMainDisplay(CGDirectDisplayID targetDisplay) {
	int deltaX, deltaY, flag;

	CGDisplayErr       dErr;
	CGDisplayCount     displayCount, i;
	CGDirectDisplayID  mainDisplay;
	CGDisplayCount     maxDisplays = MAX_DISPLAYS;
	CGDirectDisplayID  onlineDisplays[MAX_DISPLAYS];
	CGDisplayConfigRef config;

	mainDisplay = CGMainDisplayID();

	if (mainDisplay == targetDisplay) {
		exit(0);
	}

	dErr = CGGetOnlineDisplayList(maxDisplays, onlineDisplays, &displayCount);
	if (dErr != kCGErrorSuccess) {
		fprintf(stderr, "CGGetOnlineDisplayList: error %d.\n", dErr);
		exit(1);
	}

	flag = 0;

	for (i = 0; i < displayCount; i++) {
		CGDirectDisplayID dID = onlineDisplays[i];
		if (dID == targetDisplay) flag = 1;
	}

	if (flag == 0) {
		fprintf(stderr, "No such display ID: 0x%-10x.\n", targetDisplay);
		exit(1);
	}

	deltaX = -CGRectGetMinX(CGDisplayBounds(targetDisplay));
	deltaY = -CGRectGetMinY(CGDisplayBounds(targetDisplay));

	CGBeginDisplayConfiguration(&config);

	for (i = 0; i < displayCount; i++) {
		CGDirectDisplayID dID = onlineDisplays[i];

		CGConfigureDisplayOrigin(config, dID, CGRectGetMinX(CGDisplayBounds(dID)) + deltaX, CGRectGetMinY(CGDisplayBounds(dID)) + deltaY);
	}

	CGCompleteDisplayConfiguration(config, kCGConfigureForSession);

	exit(0);
}


CGDirectDisplayID InternalID(void) {
	// Returns the ID of the internal monitor
	CGDisplayErr      dErr;
	CGDisplayCount    displayCount, i;
	CGDisplayCount    maxDisplays = MAX_DISPLAYS;
	CGDirectDisplayID onlineDisplays[MAX_DISPLAYS];
	CGDirectDisplayID fallbackID = 0;

	dErr = CGGetOnlineDisplayList(maxDisplays, onlineDisplays, &displayCount);

	if (dErr != kCGErrorSuccess) {
		fprintf(stderr, "CGGetOnlineDisplayList: error %d.\n", dErr);
		exit(1);
	}

	for (i = 0; i < displayCount; i++) {
		CGDirectDisplayID dID = onlineDisplays[i];

		if ((CGDisplayIsBuiltin(dID))) return dID;
	}

	return fallbackID;
}


CGDirectDisplayID nonInternalID(void) {
	// Returns the ID of the first active monitor that is not internal or 0 if only one monitor
	CGDisplayErr      dErr;
	CGDisplayCount    displayCount, i;
	CGDisplayCount    maxDisplays = MAX_DISPLAYS;
	CGDirectDisplayID onlineDisplays[MAX_DISPLAYS];
	CGDirectDisplayID fallbackID = 0;

	dErr = CGGetOnlineDisplayList(maxDisplays, onlineDisplays, &displayCount);

	if (dErr != kCGErrorSuccess) {
		fprintf(stderr, "CGGetOnlineDisplayList: error %d.\n", dErr);
		exit(1);
	}

	for (i = 0; i < displayCount; i++) {
		CGDirectDisplayID dID = onlineDisplays[i];

		if (!(CGDisplayIsBuiltin(dID)) && (CGDisplayIsActive(dID))) {
			return dID;
		}
	}

	return fallbackID;
}



CGDirectDisplayID cgIDfromU32(uint32_t preId) {
	CGDisplayErr      dErr;
	CGDisplayCount    displayCount, i;
	CGDisplayCount    maxDisplays = MAX_DISPLAYS;
	CGDirectDisplayID onlineDisplays[MAX_DISPLAYS];
	CGDirectDisplayID postId = preId;

	dErr = CGGetOnlineDisplayList(maxDisplays, onlineDisplays, &displayCount);

	if (dErr != kCGErrorSuccess) {
		fprintf(stderr, "CGGetOnlineDisplayList: error %d.\n", dErr);
		exit(1);
	}

	for (i = 0; i < displayCount; i++) {
		CGDirectDisplayID dID = onlineDisplays[i];

		if ((dID == preId) || (dID == postId) || (onlineDisplays[i] == preId) || (onlineDisplays[i] == postId)) {
			return dID;
		}
	}

	fprintf(stderr, " Could not find a matching id in onlineDisplays!\n");
	exit(1);
}

IOOptionBits angle2options(long angle) {
	static IOOptionBits anglebits[] = {
		(kIOFBSetTransform | (kIOScaleRotate0)   << 16),
		(kIOFBSetTransform | (kIOScaleRotate90)  << 16),
		(kIOFBSetTransform | (kIOScaleRotate180) << 16),
		(kIOFBSetTransform | (kIOScaleRotate270) << 16)
	};

	// Map arbitrary angles to a rotation reset
	if ((angle % 90) != 0) return anglebits[0];

	return anglebits[(angle / 90) % 4];
}

int main(int argc, char **argv) {
	int  i;
	long angle = 0;
	long currentRotation = 0;

	io_service_t      service;
	CGDisplayErr      dErr;
	CGDirectDisplayID targetDisplay = 0;
	IOOptionBits      options;

	while ((i = getopt(argc, argv, "d:limr:")) != -1) {
		switch (i) {
			case 'd' :
				targetDisplay = (CGDirectDisplayID)strtoul(optarg, NULL, 16);

				if (targetDisplay == -1) targetDisplay = InternalID();
				if (targetDisplay ==  0) targetDisplay = CGMainDisplayID();

				if (targetDisplay == 1) {
					targetDisplay = nonInternalID();

					if (targetDisplay == 0) {
						fprintf(stderr, "Could not find an active monitor besides the internal one.\n");
						exit(1);
					}
				}
				break;

			case 'i' : infoDisplays();                   break;
			case 'l' : listDisplays();                   break;
			case 'm' : setMainDisplay(targetDisplay);    break;
			case 'r' : angle = strtol(optarg, NULL, 10); break;

			default : break;
		}
	}

	if (targetDisplay == 0) usage();

	if (angle == 1) {
		currentRotation = CGDisplayRotation(targetDisplay);

		if (currentRotation == 0) {
			angle = 90;
		}
		else {
			angle = 0;
		}
	}

	options = angle2options(angle);

	// Get the I/O Kit service port of the target display
	// Since the port is owned by the graphics system, we should not destroy it

	// In Yosemite it seems important to have a call to CGGetOnlineDisplayList() before calling
	// CGDisplayIOServicePort() or the later replacements CGDisplayVendorNumber() etc.
	// otherwise this program can hang.
	CGDirectDisplayID td2 = cgIDfromU32(targetDisplay);
	service = CGDisplayIOServicePort(td2);

	// We will get an error if the target display doesn't support the
	// kIOFBSetTransform option for IOServiceRequestProbe()
	dErr = IOServiceRequestProbe(service, options);

	if (dErr != kCGErrorSuccess) {
		fprintf(stderr, "IOServiceRequestProbe: error %d\n", dErr);
		exit(1);
	}

	exit(0);
}
