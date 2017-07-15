#include <vitasdk.h>
#include <taihen.h>
#include "renderer.h"

#define HOOKS_NUM 5 // Number of hooked functions
#define MODES_NUM 8 // Number of available modes

// Prototypes for sceTouch*2 funcs
int sceTouchRead2(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs);
int sceTouchPeek2(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs);

// Macros for available modes
enum{
	DEFAULT        = 0,
	FRONT_ONLY     = 1,
	REAR_ONLY      = 2,
	SWAPPED        = 3,
	FRONT_OFF      = 4,
	REAR_OFF       = 5,
	FRONT_OUT_ONLY = 6,
	REAR_OUT_ONLY  = 7
};

// Textual names for available modes
static char* modes[MODES_NUM] = {"Default", "Frontpad -> Rearpad", "Rearpad -> Frontpad", "Inverted pads", "Frontpad disabled", "Rearpad disabled", "Both -> Frontpad", "Both -> Rearpad"};

static uint8_t current_hook = 0;
static SceUID hooks[HOOKS_NUM];
static tai_hook_ref_t refs[HOOKS_NUM];
static uint8_t mode = DEFAULT;
static SceCtrlData pad;
static uint32_t oldpad;
static uint32_t mode_timer = 0;
static SceTouchData t;
static uint8_t not_fake = 1;

void hookFunction(uint32_t nid, const void *func){
	hooks[current_hook] = taiHookFunctionImport(&refs[current_hook],TAI_MAIN_MODULE,TAI_ANY_LIBRARY,nid,func);
	current_hook++;
}

int sceTouchRead_patched(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs){
	int ret = -1;
	switch (mode){
		case FRONT_ONLY:
			if (port == SCE_TOUCH_PORT_BACK){
				ret = TAI_CONTINUE(int, refs[0], SCE_TOUCH_PORT_FRONT, pData, nBufs);
			}
			break;
		case REAR_ONLY:
			if (port == SCE_TOUCH_PORT_FRONT){
				ret = TAI_CONTINUE(int, refs[0], SCE_TOUCH_PORT_BACK, pData, nBufs);
			}
			break;
		case SWAPPED:
			ret = TAI_CONTINUE(int, refs[0], (port + 1) % 2, pData, nBufs);
			break;
		case FRONT_OFF:
			if (port == SCE_TOUCH_PORT_FRONT){
				ret = TAI_CONTINUE(int, refs[0], port, pData, nBufs);
				pData->reportNum = 0;
			}
			break;
		case REAR_OFF:
			if (port == SCE_TOUCH_PORT_BACK){
				ret = TAI_CONTINUE(int, refs[0], port, pData, nBufs);
				pData->reportNum = 0;
			}
			break;
		case FRONT_OUT_ONLY:
			switch (port){
				case SCE_TOUCH_PORT_BACK:
					ret = TAI_CONTINUE(int, refs[0], port, pData, nBufs);
					if (not_fake) pData->reportNum = 0;
					break;
				case SCE_TOUCH_PORT_FRONT:
					ret = TAI_CONTINUE(int, refs[0], port, pData, nBufs);
					not_fake = 0;
					sceTouchRead(SCE_TOUCH_PORT_BACK, &t, nBufs);
					not_fake = 1;
					if (t.reportNum > 0){
						memcpy(&pData->report[pData->reportNum], &t.report[0], (SCE_TOUCH_MAX_REPORT - pData->reportNum) * sizeof(SceTouchReport));
						pData->reportNum += t.reportNum;
						if (pData->reportNum > SCE_TOUCH_MAX_REPORT) pData->reportNum = SCE_TOUCH_MAX_REPORT;
					}
					break;
			}
			break;
		case REAR_OUT_ONLY:
			switch (port){
				case SCE_TOUCH_PORT_FRONT:
					ret = TAI_CONTINUE(int, refs[0], port, pData, nBufs);
					if (not_fake) pData->reportNum = 0;
					break;
				case SCE_TOUCH_PORT_BACK:
					ret = TAI_CONTINUE(int, refs[0], port, pData, nBufs);
					not_fake = 0;
					sceTouchRead(SCE_TOUCH_PORT_FRONT, &t, nBufs);
					not_fake = 1;
					if (t.reportNum > 0){
						memcpy(&pData->report[pData->reportNum], &t.report[0], (SCE_TOUCH_MAX_REPORT - pData->reportNum) * sizeof(SceTouchReport));
						pData->reportNum += t.reportNum;
						if (pData->reportNum > SCE_TOUCH_MAX_REPORT) pData->reportNum = SCE_TOUCH_MAX_REPORT;
					}
					break;
			}
			break;
		default:
			ret = TAI_CONTINUE(int, refs[0], port, pData, nBufs);
			break;
	}
	return ret;
}

int sceTouchRead2_patched(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs){
	int ret = -1;
	switch (mode){
		case FRONT_ONLY:
			if (port == SCE_TOUCH_PORT_BACK){
				ret = TAI_CONTINUE(int, refs[1], SCE_TOUCH_PORT_FRONT, pData, nBufs);
			}
			break;
		case REAR_ONLY:
			if (port == SCE_TOUCH_PORT_FRONT){
				ret = TAI_CONTINUE(int, refs[1], SCE_TOUCH_PORT_BACK, pData, nBufs);
			}
			break;
		case SWAPPED:
			ret = TAI_CONTINUE(int, refs[1], (port + 1) % 2, pData, nBufs);
			break;
		case FRONT_OFF:
			if (port == SCE_TOUCH_PORT_FRONT){
				ret = TAI_CONTINUE(int, refs[1], port, pData, nBufs);
				pData->reportNum = 0;
			}
			break;
		case REAR_OFF:
			if (port == SCE_TOUCH_PORT_BACK){
				ret = TAI_CONTINUE(int, refs[1], port, pData, nBufs);
				pData->reportNum = 0;
			}
			break;
		case FRONT_OUT_ONLY:
			switch (port){
				case SCE_TOUCH_PORT_BACK:
					ret = TAI_CONTINUE(int, refs[1], port, pData, nBufs);
					if (not_fake) pData->reportNum = 0;
					break;
				case SCE_TOUCH_PORT_FRONT:
					ret = TAI_CONTINUE(int, refs[1], port, pData, nBufs);
					not_fake = 0;
					sceTouchRead2(SCE_TOUCH_PORT_BACK, &t, nBufs);
					not_fake = 1;
					if (t.reportNum > 0){
						memcpy(&pData->report[pData->reportNum], &t.report[0], (SCE_TOUCH_MAX_REPORT - pData->reportNum) * sizeof(SceTouchReport));
						pData->reportNum += t.reportNum;
						if (pData->reportNum > SCE_TOUCH_MAX_REPORT) pData->reportNum = SCE_TOUCH_MAX_REPORT;
					}
					break;
			}
			break;
		case REAR_OUT_ONLY:
			switch (port){
				case SCE_TOUCH_PORT_FRONT:
					ret = TAI_CONTINUE(int, refs[1], port, pData, nBufs);
					if (not_fake) pData->reportNum = 0;
					break;
				case SCE_TOUCH_PORT_BACK:
					ret = TAI_CONTINUE(int, refs[1], port, pData, nBufs);
					not_fake = 0;
					sceTouchRead2(SCE_TOUCH_PORT_FRONT, &t, nBufs);
					not_fake = 1;
					if (t.reportNum > 0){
						memcpy(&pData->report[pData->reportNum], &t.report[0], (SCE_TOUCH_MAX_REPORT - pData->reportNum) * sizeof(SceTouchReport));
						pData->reportNum += t.reportNum;
						if (pData->reportNum > SCE_TOUCH_MAX_REPORT) pData->reportNum = SCE_TOUCH_MAX_REPORT;
					}
					break;
			}
			break;
		default:
			ret = TAI_CONTINUE(int, refs[1], port, pData, nBufs);
			break;
	}
	return ret;
}

int sceTouchPeek_patched(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs){
	int ret = -1;
	switch (mode){
		case FRONT_ONLY:
			if (port == SCE_TOUCH_PORT_BACK){
				ret = TAI_CONTINUE(int, refs[2], SCE_TOUCH_PORT_FRONT, pData, nBufs);
			}
			break;
		case REAR_ONLY:
			if (port == SCE_TOUCH_PORT_FRONT){
				ret = TAI_CONTINUE(int, refs[2], SCE_TOUCH_PORT_BACK, pData, nBufs);
			}
			break;
		case SWAPPED:
			ret = TAI_CONTINUE(int, refs[2], (port + 1) % 2, pData, nBufs);
			break;
		case FRONT_OFF:
			if (port == SCE_TOUCH_PORT_FRONT){
				ret = TAI_CONTINUE(int, refs[2], port, pData, nBufs);
				pData->reportNum = 0;
			}
			break;
		case REAR_OFF:
			if (port == SCE_TOUCH_PORT_BACK){
				ret = TAI_CONTINUE(int, refs[2], port, pData, nBufs);
				pData->reportNum = 0;
			}
			break;
		case FRONT_OUT_ONLY:
			switch (port){
				case SCE_TOUCH_PORT_BACK:
					ret = TAI_CONTINUE(int, refs[2], port, pData, nBufs);
					if (not_fake) pData->reportNum = 0;
					break;
				case SCE_TOUCH_PORT_FRONT:
					ret = TAI_CONTINUE(int, refs[2], port, pData, nBufs);
					not_fake = 0;
					sceTouchPeek(SCE_TOUCH_PORT_BACK, &t, nBufs);
					not_fake = 1;
					if (t.reportNum > 0){
						memcpy(&pData->report[pData->reportNum], &t.report[0], (SCE_TOUCH_MAX_REPORT - pData->reportNum) * sizeof(SceTouchReport));
						pData->reportNum += t.reportNum;
						if (pData->reportNum > SCE_TOUCH_MAX_REPORT) pData->reportNum = SCE_TOUCH_MAX_REPORT;
					}
					break;
			}
			break;
		case REAR_OUT_ONLY:
			switch (port){
				case SCE_TOUCH_PORT_FRONT:
					ret = TAI_CONTINUE(int, refs[2], port, pData, nBufs);
					if (not_fake) pData->reportNum = 0;
					break;
				case SCE_TOUCH_PORT_BACK:
					ret = TAI_CONTINUE(int, refs[2], port, pData, nBufs);
					not_fake = 0;
					sceTouchPeek(SCE_TOUCH_PORT_FRONT, &t, nBufs);
					not_fake = 1;
					if (t.reportNum > 0){
						memcpy(&pData->report[pData->reportNum], &t.report[0], (SCE_TOUCH_MAX_REPORT - pData->reportNum) * sizeof(SceTouchReport));
						pData->reportNum += t.reportNum;
						if (pData->reportNum > SCE_TOUCH_MAX_REPORT) pData->reportNum = SCE_TOUCH_MAX_REPORT;
					}
					break;
			}
			break;
		default:
			ret = TAI_CONTINUE(int, refs[2], port, pData, nBufs);
			break;
	}
	return ret;
}

int sceTouchPeek2_patched(SceUInt32 port, SceTouchData *pData, SceUInt32 nBufs){
	int ret = -1;
	switch (mode){
		case FRONT_ONLY:
			if (port == SCE_TOUCH_PORT_BACK){
				ret = TAI_CONTINUE(int, refs[3], SCE_TOUCH_PORT_FRONT, pData, nBufs);
			}
			break;
		case REAR_ONLY:
			if (port == SCE_TOUCH_PORT_FRONT){
				ret = TAI_CONTINUE(int, refs[3], SCE_TOUCH_PORT_BACK, pData, nBufs);
			}
			break;
		case SWAPPED:
			ret = TAI_CONTINUE(int, refs[3], (port + 1) % 2, pData, nBufs);
			break;
		case FRONT_OFF:
			if (port == SCE_TOUCH_PORT_FRONT){
				ret = TAI_CONTINUE(int, refs[3], port, pData, nBufs);
				pData->reportNum = 0;
			}
			break;
		case REAR_OFF:
			if (port == SCE_TOUCH_PORT_BACK){
				ret = TAI_CONTINUE(int, refs[3], port, pData, nBufs);
				pData->reportNum = 0;
			}
			break;
		case FRONT_OUT_ONLY:
			switch (port){
				case SCE_TOUCH_PORT_BACK:
					ret = TAI_CONTINUE(int, refs[3], port, pData, nBufs);
					if (not_fake) pData->reportNum = 0;
					break;
				case SCE_TOUCH_PORT_FRONT:
					ret = TAI_CONTINUE(int, refs[3], port, pData, nBufs);
					not_fake = 0;
					sceTouchPeek2(SCE_TOUCH_PORT_BACK, &t, nBufs);
					not_fake = 1;
					if (t.reportNum > 0){
						memcpy(&pData->report[pData->reportNum], &t.report[0], (SCE_TOUCH_MAX_REPORT - pData->reportNum) * sizeof(SceTouchReport));
						pData->reportNum += t.reportNum;
						if (pData->reportNum > SCE_TOUCH_MAX_REPORT) pData->reportNum = SCE_TOUCH_MAX_REPORT;
					}
					break;
			}
			break;
		case REAR_OUT_ONLY:
			switch (port){
				case SCE_TOUCH_PORT_FRONT:
					ret = TAI_CONTINUE(int, refs[3], port, pData, nBufs);
					if (not_fake) pData->reportNum = 0;
					break;
				case SCE_TOUCH_PORT_BACK:
					ret = TAI_CONTINUE(int, refs[3], port, pData, nBufs);
					not_fake = 0;
					sceTouchPeek2(SCE_TOUCH_PORT_FRONT, &t, nBufs);
					not_fake = 1;
					if (t.reportNum > 0){
						memcpy(&pData->report[pData->reportNum], &t.report[0], (SCE_TOUCH_MAX_REPORT - pData->reportNum) * sizeof(SceTouchReport));
						pData->reportNum += t.reportNum;
						if (pData->reportNum > SCE_TOUCH_MAX_REPORT) pData->reportNum = SCE_TOUCH_MAX_REPORT;
					}
					break;
			}
			break;
		default:
			ret = TAI_CONTINUE(int, refs[3], port, pData, nBufs);
			break;
	}
	return ret;
}

int sceDisplaySetFrameBuf_patched(const SceDisplayFrameBuf *pParam, int sync) {
	sceCtrlPeekBufferPositive(0, &pad, 1);
	updateFramebuf(pParam);
	if (((pad.buttons & SCE_CTRL_CROSS) && (pad.buttons & SCE_CTRL_TRIANGLE) && (pad.buttons & SCE_CTRL_START)) && (!((oldpad & SCE_CTRL_CROSS) && (oldpad & SCE_CTRL_TRIANGLE) && (oldpad & SCE_CTRL_START)))){
		mode = (mode + 1) % MODES_NUM;
		mode_timer = 100;
		setTextColor(0x00FFFFFF);
	}
	oldpad = pad.buttons;
	
	if (mode_timer > 0){
		drawStringF(5, 5, "Touch Inverter mode: %s", modes[mode]);
		mode_timer--;
	}
	
	return TAI_CONTINUE(int, refs[4], pParam, sync);
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args) {
	
	// Hooking touch and display functions
	hookFunction(0x169A1D58, sceTouchRead_patched);
	hookFunction(0x39401BEA, sceTouchRead2_patched);
	hookFunction(0xFF082DF0, sceTouchPeek_patched);
	hookFunction(0x3AD3D0A1, sceTouchPeek2_patched);
	hookFunction(0x7A410B64, sceDisplaySetFrameBuf_patched);
	
	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {

	// Freeing hooks
	while (current_hook-- > 0){
		taiHookRelease(hooks[current_hook], refs[current_hook]);
	}

	return SCE_KERNEL_STOP_SUCCESS;
	
}