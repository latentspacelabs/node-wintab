#include <node.h>
#include <v8.h>
#include <Windows.h>
#include <iostream>

#include "WINTAB.H"

#include "Utils.h"
char* gpszProgramName = "node_wintab";

#include "MSGPACK.H"

#define PACKETDATA (PK_X | PK_Y | PK_NORMAL_PRESSURE | PK_STATUS | PK_ORIENTATION)
#define PACKETMODE PK_BUTTONS
#include "PKTDEF.H"

using namespace v8;
using namespace std;

int pen_x = -1;
int pen_y = -1;
int pen_pressure = -1;
int pressure_min = -1;
int pressure_max = -1;
bool is_eraser = FALSE;
int azimuth = 0;
int altitude = 0;
int twist = 0;
int azimuth_min = -1;
int azimuth_max = -1;
int altitude_min = -1;
int altitude_max = -1;

void get_pressure(const FunctionCallbackInfo<Value>& info) {
	if (pen_pressure < 0) {
		info.GetReturnValue().SetNull();
	} else {
		info.GetReturnValue().Set(pen_pressure);
	}
}

void get_pen_x(const FunctionCallbackInfo<Value>& info) {
	info.GetReturnValue().Set(pen_x);
}

void get_pen_y(const FunctionCallbackInfo<Value>& info) {
	info.GetReturnValue().Set(pen_y);
}

void get_azimuth(const FunctionCallbackInfo<Value>& info) {
	info.GetReturnValue().Set(azimuth);
}

void get_altitude(const FunctionCallbackInfo<Value>& info) {
	info.GetReturnValue().Set(altitude);
}

void get_twist(const FunctionCallbackInfo<Value>& info) {
	info.GetReturnValue().Set(twist);
}

void get_azimuth_min(const FunctionCallbackInfo<Value>& info) {
	info.GetReturnValue().Set(azimuth_min);
}

void get_azimuth_max(const FunctionCallbackInfo<Value>& info) {
	info.GetReturnValue().Set(azimuth_max);
}

void get_altitude_min(const FunctionCallbackInfo<Value>& info) {
	info.GetReturnValue().Set(altitude_min);
}

void get_altitude_max(const FunctionCallbackInfo<Value>& info) {
	info.GetReturnValue().Set(altitude_max);
}

void get_pressure_min(const FunctionCallbackInfo<Value>& info) {
	if (pressure_min < 0) {
		info.GetReturnValue().SetNull();
	}
	else {
		info.GetReturnValue().Set(pressure_min);
	}
}

void get_pressure_max(const FunctionCallbackInfo<Value>& info) {
    if (pressure_max < 0) {
		info.GetReturnValue().SetNull();
	}
	else {
		info.GetReturnValue().Set(pressure_max);
	}
}

void check_eraser(const FunctionCallbackInfo<Value>& info) {
	info.GetReturnValue().Set(is_eraser);
}

HINSTANCE hinst;
WNDCLASS wc;
HWND hwnd;

HCTX hctx;
LOGCONTEXT lc = {0};

bool overlapped = FALSE;

void peek_message(const FunctionCallbackInfo<Value>& info) {
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
	info.GetReturnValue().SetUndefined();
}

void check_overlapped(const FunctionCallbackInfo<Value>& info) {
    bool tmp = overlapped;
    overlapped = FALSE;
	info.GetReturnValue().Set(tmp);
}

void enable_context(const FunctionCallbackInfo<Value>& info) {
    if (hctx) {
        gpWTEnable(hctx, TRUE);
        gpWTOverlap(hctx, TRUE);
    }
	info.GetReturnValue().SetUndefined();
}

HCTX initTablet(HWND hwnd) {
    AXIS TabletX = {0};
    AXIS TabletY = {0};
    AXIS Pressure = {0};
    lc.lcOptions |= CXO_SYSTEM;
    if (gpWTInfoA(WTI_DEFSYSCTX, 0, &lc) != sizeof(LOGCONTEXT))
        return (HCTX) NULL;
    if (!(lc.lcOptions & CXO_SYSTEM))
        return (HCTX) NULL;
    wsprintf(lc.lcName, "node_wintab_%x", hinst);
    lc.lcPktData = PACKETDATA;
    lc.lcOptions |= CXO_MESSAGES;
    lc.lcPktMode = PACKETMODE;
    lc.lcMoveMask = PACKETDATA;
    lc.lcBtnUpMask = lc.lcBtnDnMask;
    if (gpWTInfoA(WTI_DEVICES, DVC_X, &TabletX) != sizeof(AXIS))
        return (HCTX) NULL;
    if (gpWTInfoA(WTI_DEVICES, DVC_Y, &TabletY) != sizeof(AXIS))
        return (HCTX) NULL;
    if (gpWTInfoA(WTI_DEVICES, DVC_NPRESSURE, &Pressure) != sizeof(AXIS))
        return (HCTX) NULL;
    lc.lcInOrgX = 0;
    lc.lcInOrgY = 0;
    lc.lcInExtX = TabletX.axMax;
    lc.lcInExtY = TabletY.axMax;
    lc.lcOutOrgX = 0;
    lc.lcOutOrgY = 0;
    lc.lcOutExtX = GetSystemMetrics(SM_CXSCREEN);
    lc.lcOutExtY = -GetSystemMetrics(SM_CYSCREEN);
    lc.lcSysOrgX = 0;
    lc.lcSysOrgY = 0;
    lc.lcSysExtX = GetSystemMetrics(SM_CXSCREEN);
    lc.lcSysExtY = GetSystemMetrics(SM_CYSCREEN);

    pressure_min = (int) Pressure.axMin;
    pressure_max = (int) Pressure.axMax;

    struct tagAXIS tiltOrient[3];
    BOOL tiltSupport = FALSE;
    tiltSupport = gpWTInfoA(WTI_DEVICES, DVC_ORIENTATION, &tiltOrient);
    if (tiltSupport )
    {
        // Does the tablet support azimuth and altitude
        if (tiltOrient[0].axResolution && tiltOrient[1].axResolution)
        {
            // Get resolution
            azimuth_min = tiltOrient[0].axMin;
            azimuth_max = tiltOrient[0].axMax;
            altitude_min = tiltOrient[1].axMin;
            altitude_max = tiltOrient[1].axMax;
        }
        else
        {
            tiltSupport = FALSE;
        }
    }

    return gpWTOpenA(hwnd, &lc, TRUE);
}

LRESULT msgLoop(HWND hwnd, unsigned msg, WPARAM wp, LPARAM lp) {
    PACKET pkt;
    switch (msg) {
    case WM_CREATE:
        hctx = initTablet(hwnd);
        break;
    case WM_NCCREATE:
        break;
    case WT_PACKET:
        if (gpWTPacket((HCTX) lp, wp, &pkt)) {
            pen_x = (int) pkt.pkX;
            pen_y = (int) pkt.pkY;
            pen_pressure = (int) pkt.pkNormalPressure;
            is_eraser = (bool) (pkt.pkStatus & TPS_INVERT);
			azimuth = (int) pkt.pkOrientation.orAzimuth;
            altitude = (int) pkt.pkOrientation.orAltitude;
            twist = (int) pkt.pkOrientation.orTwist;
        }
        break;
    case WT_CTXOVERLAP:
        overlapped = TRUE;
        break;
    case WT_PROXIMITY:
        pen_x = -1;
        pen_y = -1;
        pen_pressure = -1;
        is_eraser = FALSE;
        azimuth = -1;
        altitude = -1;
        twist = -1;
        break;
    default:
        return (LRESULT) 0L;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

void init(Local<Object> exports) {
    hinst = (HINSTANCE) GetModuleHandle(NULL);
    wc.style = 0;
    wc.lpfnWndProc = (WNDPROC) msgLoop;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hinst;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH) (COLOR_APPWORKSPACE + 1);
    wc.lpszMenuName =  "wintabMenu";
    wc.lpszClassName = "wintabClass";
    RegisterClass(&wc);
    LoadWintab();
    hwnd = CreateWindow(
        "wintabClass",
        "wintabWindow",
        WS_OVERLAPPED, CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT, (HWND) NULL,
        (HMENU) NULL, hinst, (LPVOID) NULL
    );
	Isolate* isolate = Isolate::GetCurrent();
	v8::Local<v8::Context> context = exports->CreationContext();
    exports->Set(context, String::NewFromUtf8(isolate, "pressure"), FunctionTemplate::New(isolate, get_pressure)->GetFunction(context).ToLocalChecked());
    exports->Set(context, String::NewFromUtf8(isolate, "x"), FunctionTemplate::New(isolate, get_pen_x)->GetFunction(context).ToLocalChecked());
    exports->Set(context, String::NewFromUtf8(isolate, "y"), FunctionTemplate::New(isolate, get_pen_y)->GetFunction(context).ToLocalChecked());
    exports->Set(context, String::NewFromUtf8(isolate, "azimuth"), FunctionTemplate::New(isolate, get_azimuth)->GetFunction(context).ToLocalChecked());
    exports->Set(context, String::NewFromUtf8(isolate, "altitude"), FunctionTemplate::New(isolate, get_altitude)->GetFunction(context).ToLocalChecked());
    exports->Set(context, String::NewFromUtf8(isolate, "twist"), FunctionTemplate::New(isolate, get_twist)->GetFunction(context).ToLocalChecked());
    exports->Set(context, String::NewFromUtf8(isolate, "azimuthMin"), FunctionTemplate::New(isolate, get_azimuth_min)->GetFunction(context).ToLocalChecked());
    exports->Set(context, String::NewFromUtf8(isolate, "azimuthMax"), FunctionTemplate::New(isolate, get_azimuth_max)->GetFunction(context).ToLocalChecked());
    exports->Set(context, String::NewFromUtf8(isolate, "altitudeMin"), FunctionTemplate::New(isolate, get_altitude_min)->GetFunction(context).ToLocalChecked());
    exports->Set(context, String::NewFromUtf8(isolate, "altitudeMax"), FunctionTemplate::New(isolate, get_altitude_max)->GetFunction(context).ToLocalChecked());
    exports->Set(context, String::NewFromUtf8(isolate, "minPressure"), FunctionTemplate::New(isolate, get_pressure_min)->GetFunction(context).ToLocalChecked());
    exports->Set(context, String::NewFromUtf8(isolate, "maxPressure"), FunctionTemplate::New(isolate, get_pressure_max)->GetFunction(context).ToLocalChecked());
    exports->Set(context, String::NewFromUtf8(isolate, "isEraser"), FunctionTemplate::New(isolate, check_eraser)->GetFunction(context).ToLocalChecked());
    exports->Set(context, String::NewFromUtf8(isolate, "peekMessage"), FunctionTemplate::New(isolate, peek_message)->GetFunction(context).ToLocalChecked());
    exports->Set(context, String::NewFromUtf8(isolate, "checkOverlapped"), FunctionTemplate::New(isolate, check_overlapped)->GetFunction(context).ToLocalChecked());
    exports->Set(context, String::NewFromUtf8(isolate, "enableContext"), FunctionTemplate::New(isolate, enable_context)->GetFunction(context).ToLocalChecked());
}

NODE_MODULE(wintab, init)

// Wintab Docs: http://www.wacomeng.com/windows/docs/Wintab_v140.htm
