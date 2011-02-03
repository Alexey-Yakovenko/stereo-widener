/*
    Stereo widener plugin for DeaDBeeF
    Copyright (C) 2010 Steven McDonald <steven.mcdonald@libremail.me>
    See COPYING file for modification and redistribution conditions.
*/

#include <deadbeef/deadbeef.h>
#include <stdlib.h>
#include <string.h>

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

// Simply transforming samples causes centre-panned
// instruments to sound quiet and distant. We correct
// for this by assigning weights to the effect on the
// mid and side channels.
#define MIDWEIGHT 0.2
#define SIDEWEIGHT 1.0

static DB_dsp_t plugin;
DB_functions_t *deadbeef;

/*
static short enabled = 0;
static float width = 1;
*/

float lsample, rsample, mid, side;

enum {
    SW_PARAM_WIDTH = 0,
    SW_PARAM_COUNT
};

typedef struct {
    ddb_dsp_context_t ctx;

    float width;
    float midamp, sideamp;
} ddb_sw_t;

/*
static void
ddb_sw_reset (void);
*/

void
recalc_amp (ddb_sw_t *sw) {
    float midamp = 1 - (((sw->width * MIDWEIGHT) + 1) / 2);
    float sideamp = ((sw->width * SIDEWEIGHT) + 1) / 2;

    // apply corrective gain (prevents clipping)
    float gain = midamp > sideamp ? 0.5 / midamp : 0.5 / sideamp;
    gain = gain > 1 ? 1 : gain;
    sw->midamp = gain * midamp;
    sw->sideamp = gain * sideamp;
    fprintf (stderr, "recalc_amp: %f, %f=n", sw->midamp, sw->sideamp);

    return;
}

/*
static int
stereo_widener_on_configchanged (DB_event_t *ev, uintptr_t data) {
    int e = deadbeef->conf_get_int ("stereo_widener.enable", 0);
    if (e != enabled) {
        if (e) {
            stereo_widener_reset ();
        }
        enabled = e;
    }

    float w = deadbeef->conf_get_float ("stereo_widener.width", 0) / 100;
    if (w > 1) { w = 1; deadbeef->conf_set_float ("stereo_widener.width", 100); }
    if (w < -1) { w = -1; deadbeef->conf_set_float ("stereo_widener.width", -100); }
    if (w != width) {
        width = w;
        recalc_amp ();
    }
    return 0;
}
*/

ddb_dsp_context_t*
ddb_sw_open (void) {
    ddb_sw_t *sw = malloc (sizeof (ddb_sw_t));
    DDB_INIT_DSP_CONTEXT (sw, ddb_sw_t, &plugin);

    sw->width = 0; // this may or may not need to be set, not sure
    return (ddb_dsp_context_t *)sw;
}

void
ddb_sw_close (ddb_dsp_context_t *_sw) {
    ddb_sw_t *sw = (ddb_sw_t*)_sw;
    free (sw);
}

int
ddb_sw_process (ddb_dsp_context_t *_sw, float *samples, int frames, int maxframes, ddb_waveformat_t *fmt, float *ratio)  {
    ddb_sw_t *sw = (ddb_sw_t*)_sw;

    fprintf (stderr, "ddb_sw_process: almost processing... %f, %f, %f\n", sw->width, sw->midamp, sw->sideamp);
    if (fmt->channels != 2 || sw->width == 0) return frames;
    fprintf (stderr, "ddb_sw_process: processing... %f, %f, %f\n", sw->width, sw->midamp, sw->sideamp);
    float mid, side;
    for (unsigned i = 0; i < frames; i++) {
        mid = sw->midamp * (samples[i*2] + samples[(i*2)+1]);
        side = sw->sideamp * (samples[i*2] - samples[(i*2)+1]);
        samples[i*2] = mid + side;
        samples[i*2 + 1] = mid - side;
    }
    return frames;
}

void
ddb_sw_reset (ddb_dsp_context_t *ctx) {
    return;
}

/*
static void
stereo_widener_enable (int e) {
    if (e != enabled) {
        deadbeef->conf_set_int ("stereo_widener.enable", e);
        if (e && !enabled) {
            stereo_widener_reset ();
        }
        enabled = e;
    }
    return;
}

static int
stereo_widener_enabled (void) {
    return enabled;
}
*/

int
ddb_sw_num_params (void) {
    return SW_PARAM_COUNT;
}

const char *
ddb_sw_get_param_name (int p) {
    switch (p) {
    case SW_PARAM_WIDTH:
        return "Stereo width";
        break;
    default:
        fprintf (stderr, "ddb_sw_get_param_name: invalid param index (%d)\n", p);
    }
}

void
ddb_sw_set_param (ddb_dsp_context_t *_sw, int p, const char *val) {
    ddb_sw_t *sw = (ddb_sw_t*)_sw;
    float w;

    switch (p) {
    case SW_PARAM_WIDTH:
        w = atof (val) / 100;
        if (w == 20) return;
        fprintf (stderr, "ddb_sw_set_param: %f\n", w);
        if (w > 1) w = 1;
        else if (w < -1) w = -1;
        if (w != sw->width) {
            sw->width = w;
            recalc_amp (sw);
        }
        break;
    default:
        fprintf (stderr, "ddb_sw_set_param: invalid param index (%d)\n", p);
    }
}

void
ddb_sw_get_param (ddb_dsp_context_t *_sw, int p, char *val, int sz) {
    ddb_sw_t *sw = (ddb_sw_t*)_sw;

    switch (p) {
    case SW_PARAM_WIDTH:
        snprintf (val, sz, "%d", sw->width);
        break;
    default:
        fprintf (stderr, "ddb_sw_get_param: invalid param index (%d)\n", p);
    }
}

static const char ddb_sw_dialog[] =
    "property \"Effect intensity\" hscale[-100,100,1] 0 0;\n"
;

static DB_dsp_t plugin = {
    .plugin.api_vmajor = DB_API_VERSION_MAJOR,
    .plugin.api_vminor = DB_API_VERSION_MINOR,
    .plugin.type = DB_PLUGIN_DSP,
    .plugin.id = "stereo_widener",
    .plugin.name = "Stereo widener",
    .plugin.descr = "Stereo widener plugin",
    .plugin.author = "Steven McDonald",
    .plugin.email = "steven.mcdonald@libremail.me",
    .plugin.website = "http://gitorious.org/deadbeef-sm-plugins/pages/Home",
//    .plugin.start = stereo_widener_plugin_start,
//    .plugin.stop = stereo_widener_plugin_stop,
    .num_params = ddb_sw_num_params,
    .get_param_name = ddb_sw_get_param_name,
    .set_param = ddb_sw_set_param,
    .get_param = ddb_sw_get_param,
    .configdialog = ddb_sw_dialog,
    .open = ddb_sw_open,
    .close = ddb_sw_close,
    .process = ddb_sw_process,
    .reset = ddb_sw_reset,
//    .enable = ddb_sw_enable,
//    .enabled = ddb_sw_enabled,
};

DB_plugin_t *
stereo_widener_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
