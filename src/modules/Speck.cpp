#include <string.h>
#include <complex.h>
#include <math.h>
#include <float.h>
#include "../LindenbergResearch.hpp"
#include "../LRModel.hpp"
#include "../dsp/kiss_fft/kiss_fft.h"
/*
 * FFT SCOPE, BASED ON SCOPE
 * Todo: precise f0 estimate
 */

#define BUFFER_SIZE 8192
#define FFT_POINTS BUFFER_SIZE
#define FFT_POINTS_NYQ FFT_POINTS/2+1
#define DIR_FFT 0
#define INV_FFT 1
#define FOFFS_RANGE 1.0
#define ZOOM_RANGE 8.0

using namespace rack;
using namespace lrt;


float cabsf_LG(kiss_fft_cpx v) {
    return sqrtf((v.r * v.r + v.i * v.i));
}


void HannWindow(float *w, int size) {
    if (size <= 0) return;

    if (w == NULL) {
        w = (float *) malloc(size * sizeof(float));
    }

    for (int i = 0; i < size; i++) {
        w[i] = 0.5 * (1 - cos(2 * M_PI * i / (size - 1))); // maybe double is better?
    }
}


struct Speck : LRModule {
    enum ParamIds {
        SCALE_1_PARAM,
        POS_1_PARAM,
        SCALE_2_PARAM,
        POS_2_PARAM,
        ZOOM_PARAM, // was ZOOM
        LINLOG_PARAM, // was MODE
        FOFFS_PARAM, // was TRIG
        ONOFF_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        INPUT_1,
        INPUT_2,
        NUM_INPUTS
    };
    enum OutputIds {
        OUTPUT_1,
        OUTPUT_2,
        NUM_OUTPUTS
    };
    enum LightsIds {
        LIGHTS_0_LIN,
        LIGHTS_1_LOG,
        LIGHTS_2_ON,
        NUM_LIGHTS,
    };

    float buffer1[BUFFER_SIZE] = {};
    float buffer2[BUFFER_SIZE] = {};
    float FFT1[FFT_POINTS_NYQ] = {};
    float FFT2[FFT_POINTS_NYQ] = {};
    int bufferIndex = 0;
    float frameIndex = 0;

    bool forceOff = false;
    bool linLog = false; // lin = 0, log = 1
    bool powered = false;
    kiss_fft_cfg cfg_for_FFT, cfg_for_IFFT;
    float HannW[BUFFER_SIZE];


    Speck() : LRModule(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {
        cfg_for_FFT = kiss_fft_alloc(FFT_POINTS, DIR_FFT, 0, 0);
        cfg_for_IFFT = kiss_fft_alloc(FFT_POINTS, INV_FFT, 0, 0);
        HannWindow(&HannW[0], BUFFER_SIZE);
    }


    ~Speck();
    void process(const ProcessArgs &args) override;


    json_t *dataToJson() override {
        json_t *rootJ = LRModule::dataToJson();

        json_object_set_new(rootJ, "linLog", json_integer((int) linLog));
        return rootJ;
    }


    void dataFromJson(json_t *rootJ) override {
        LRModule::dataFromJson(rootJ);

        json_t *sumJ = json_object_get(rootJ, "linLog");
        if (sumJ)
            linLog = json_integer_value(sumJ);
    }


    void reset() override {
        linLog = false;
        powered = false;
    }
};


Speck::~Speck() {
    free(cfg_for_FFT);
    free(cfg_for_IFFT);
}


void Speck::process(const ProcessArgs &args) {
    int n;
    kiss_fft_cpx cBufIn[FFT_POINTS], cBufOut[FFT_POINTS];

    forceOff = params[ONOFF_PARAM].getValue() == 1;


    if (inputs[INPUT_1].isConnected() || inputs[INPUT_2].isConnected()) {
        if (!powered && !forceOff) {
            powered = true;
        } else if (powered && forceOff) {
            powered = false;
        }
    } else {
        powered = false;
        forceOff = false;
    }

    lights[LIGHTS_2_ON].value = powered;

    linLog = params[LINLOG_PARAM].getValue() != 0.0;

    lights[LIGHTS_0_LIN].value = linLog ? 0.f : 1.f;
    lights[LIGHTS_1_LOG].value = linLog ? 1.f : 0.f;

    // pass through
    if (outputs[OUTPUT_1].isConnected()) {
        outputs[OUTPUT_1].setVoltage((inputs[INPUT_1].getVoltage()));
    }
    if (outputs[OUTPUT_2].isConnected()) {
        outputs[OUTPUT_2].setVoltage((inputs[INPUT_2].getVoltage()));
    }

    /* power off */
    if (!powered) return;

    //float deltaTime = powf(2.0, -14.0); // this could be the NFFT in the future (if rounded to nearest 2^N)
    //int frameCount = (int)ceilf(deltaTime * args.sampleRate);
    int frameCount = 1;

    if (bufferIndex < BUFFER_SIZE) {
        if (++frameIndex > frameCount) {
            frameIndex = 0;
            buffer1[bufferIndex] = (inputs[INPUT_1].getVoltage());
            buffer2[bufferIndex] = (inputs[INPUT_2].getVoltage());
            bufferIndex++;
        }
    } else {
        for (n = 0; n < FFT_POINTS; n++) {
            cBufIn[n].r = HannW[n] * buffer1[n];
            cBufIn[n].i = 0.0;
        }

        kiss_fft(cfg_for_FFT, cBufIn, cBufOut);

        for (n = 0; n < FFT_POINTS_NYQ; n++) {
            FFT1[n] = logf(cabsf_LG(cBufOut[n]));
        }

        for (n = 0; n < FFT_POINTS; n++) {
            cBufIn[n].r = HannW[n] * buffer2[n];
            cBufIn[n].i = 0.0;
        }

        kiss_fft(cfg_for_FFT, cBufIn, cBufOut);

        for (n = 0; n < FFT_POINTS_NYQ; n++) {
            FFT2[n] = logf(cabsf_LG(cBufOut[n]));
        }

        bufferIndex = 0;
        frameIndex = 0; // reset all. remove for future overlaps
    }
    /*
    // Reset buffer
    if (bufferIndex >= BUFFER_SIZE) {
        bufferIndex = 0; frameIndex = 0; return;
    }
    */
}


struct SpeckDisplay : TransparentWidget {
    Speck *module;
    int frame = 0;
    std::shared_ptr<Font> font;


    struct Stats {
        float f0, peakx, peaky;


        void calculate(float *values) {
            f0 = 0.0;
            peakx = 0.0;
            peaky = 0.0;
            for (int i = 0; i < FFT_POINTS_NYQ; i++) {
                float v = values[i];
                if (v > peaky) {
                    peaky = v;
                    peakx = i;
                }
            }

            peakx = args.sampleRate / 2.0f * ((peakx) / (float) (FFT_POINTS_NYQ));
            f0 = peakx;
        }
    };


    Stats stats1, stats2;


    SpeckDisplay() {
        font = APP->window->loadFont(asset::plugin(pluginInstance, "res/ibm-plex-mono/IBMPlexMono-Medium.ttf"));
    }


#define LOG_LOWER_FREQ 10.0 // lowest freq we are going to show in log mode


    float drawWaveform(NVGcontext *args
    .vg,
    float *values,
    float gain,
    float offset,
    float fzoom,
    float foffs,
    bool linLog
    ) {
        int xpos;
        float nyq = args.sampleRate / 2.0;
        float logMax = log10(nyq);
        float semilogx[FFT_POINTS_NYQ];
        float vgrid[100];
        float negOffs;
        int maxj = 0;
        Vec p;
        nvgSave(args.vg);
        Rect b = Rect(Vec(0, 15), box.size.minus(Vec(0, 15 * 2)));
        nvgScissor(args.vg, b.pos.x, b.pos.y, b.size.x, b.size.y);
        nvgBeginPath(args.vg);
        // Draw maximum display left to right
        int lwp = 0; // lowest point to show
        int spacing = (nyq / FFT_POINTS_NYQ);
        for (lwp = 0; lwp < FFT_POINTS_NYQ; lwp++) {
            if (lwp * spacing > LOG_LOWER_FREQ) break;
        }

        // create the semilogx axis
        if (linLog) {
            vgrid[0] = log10(LOG_LOWER_FREQ);
            vgrid[0] = (vgrid[0] * b.size.x / logMax);
            int j = 1;
            // create lin grid values
            for (int f = 100; f < 1000; f += 100) {
                vgrid[j++] = f;
            }
            for (int f = 1000; f < nyq; f += 1000) {
                vgrid[j++] = f;
            }
            maxj = j;
            for (int i = 0; i < maxj; i++) {
                vgrid[i] = log10((float) (vgrid[i]));
                vgrid[i] = (vgrid[i] * ((float) (b.size.x) + vgrid[0]) / logMax);
            }

            semilogx[lwp] = log10((float) (lwp) * nyq / (float) FFT_POINTS_NYQ);
            semilogx[lwp] = (semilogx[lwp] * b.size.x / logMax); // apply the range of the box
            for (int i = lwp + 1; i < FFT_POINTS_NYQ; i++) {
                semilogx[i] = log10((float) (i) * nyq / (float) FFT_POINTS_NYQ);
                semilogx[i] = (b.size.x + semilogx[lwp] + 60) * semilogx[i] / logMax; // apply the range of the box
            }

            float residual = semilogx[FFT_POINTS_NYQ - 1] - (semilogx[FFT_POINTS_NYQ - 1] / fzoom); // excluded from plot
            negOffs = -(0.8 * foffs / FOFFS_RANGE) * residual;
/*
			for (int i = 0; i < FFT_POINTS_NYQ; i++) {
				semilogx[i] = negOffs + semilogx[i]; // apply the range of the box TODO togliere?
			}
			for (int j = 0; j < maxj; j++) {
				vgrid[j] += negOffs;
			}
*/
            for (int i = lwp; i < FFT_POINTS_NYQ; i++) {
                float value = values[i] * gain + offset;

                p = Vec(b.pos.x + fzoom * (((semilogx[i]) - semilogx[lwp]) + negOffs), b.pos.y + b.size.y * (1 - value) / 2);

                if (i <= lwp)
                    nvgMoveTo(args.vg, p.x, p.y);
                else
                    nvgLineTo(args.vg, p.x, p.y);
            }

        } else {
            int zoomPoints = floor((float) (FFT_POINTS_NYQ) / (fzoom < 1.0 ? 1.0 : fzoom));
            int fstart = floor(foffs * ((float) (FFT_POINTS_NYQ) - (float) (zoomPoints)));

            for (int i = 0; i < zoomPoints; i++) {
                float value = values[i + fstart] * gain + offset;
                xpos = i;
                p = Vec(b.pos.x + xpos * b.size.x / (zoomPoints/*FFT_POINTS_NYQ*/- 1), b.pos.y + b.size.y * (1 - value) / 2);

                if (i == 0)
                    nvgMoveTo(args.vg, p.x, p.y);
                else
                    nvgLineTo(args.vg, p.x, p.y);
            }
        }

        //printf("xpos %d, bsize %f, zoomPts %d, bpos %f, x %f\n", xpos, b.size.x, zoomPoints, b.pos.x, p.x);
        nvgLineCap(args.vg, NVG_ROUND);
        nvgMiterLimit(args.vg, 2.0);
        nvgStrokeWidth(args.vg, 1.75);
        nvgGlobalCompositeOperation(args.vg, NVG_LIGHTER);
        nvgStroke(args.vg);
        nvgResetScissor(args.vg);
        nvgRestore(args.vg);

        if (linLog) {

            // UP TO 1k
            for (int j = 0; j < maxj; j++) {

                Vec p = Vec(b.pos.x + fzoom * (vgrid[j] - vgrid[0] + negOffs), box.size.y);
                nvgStrokeColor(args.vg, nvgRGBA(0xff, 0xff, 0xff, 0x10));
                {
                    nvgBeginPath(args.vg);
                    nvgMoveTo(args.vg, p.x, p.y);
                    nvgLineTo(args.vg, p.x, 0);
                    nvgClosePath(args.vg);
                }
                nvgStroke(args.vg);
            }
        }

        return negOffs;
    }


#define VERT_GRID_DIST 20
#define HORZ_GRID_DIST 20


    void drawGrid(NVGcontext *args
    .vg,
    float fzoom,
    float foffs,
    bool linLog,
    float negOffs
    ) {
        Rect b = Rect(Vec(0, 15), box.size.minus(Vec(0, 15 * 2)));
        nvgScissor(args.vg, b.pos.x, b.pos.y, b.size.x, b.size.y);
        float nyq = args.sampleRate / 2.0;
        float range = nyq / (fzoom < 1.0 ? 1.0 : fzoom);
        float fstart = foffs * (nyq - range);
        int first = ceil(fstart / 1000) * 1000;
        float diff = first - fstart;

        // VERT LINES
        if (linLog == 0) {
            for (int f = first; f < first + range; f += 1000) {
                float v = ((f - first + diff) / range) * box.size.x;
                Vec p = Vec(v, box.size.y);
                nvgStrokeColor(args.vg, nvgRGBA(0xff, 0xff, 0xff, 0x10));
                {
                    nvgBeginPath(args.vg);
                    nvgMoveTo(args.vg, p.x, p.y);
                    nvgLineTo(args.vg, p.x, 0);
                    nvgClosePath(args.vg);
                }
                nvgStroke(args.vg);
            }
        } else { ; } // is done in the drawWaveform for convenience

        // HORZ LINES
        for (int h = 0; h < box.size.y; h += HORZ_GRID_DIST) {
            Vec p = Vec(box.size.x, h);
            nvgStrokeColor(args.vg, nvgRGBA(0xff, 0xff, 0xff, 0x10));
            {
                nvgBeginPath(args.vg);
                nvgMoveTo(args.vg, p.x, p.y);
                nvgLineTo(args.vg, 0, p.y);
                nvgClosePath(args.vg);
            }
            nvgStroke(args.vg);
        }
    }


/*	void drawTrig(NVGcontext *args.vg, float value, float gain, float offset) {
		Rect b = Rect(Vec(0, 15), box.size.minus(Vec(0, 15*2)));
		nvgScissor(args.vg, b.pos.x, b.pos.y, b.size.x, b.size.y);

		value = value * gain + offset;
		Vec p = Vec(box.size.x, b.pos.y + b.size.y * (1 - value) / 2);

		// Draw line
		nvgStrokeColor(args.vg, nvgRGBA(0xff, 0xff, 0xff, 0x10));
		{
			nvgBeginPath(args.vg);
			nvgMoveTo(args.vg, p.x - 13, p.y);
			nvgLineTo(args.vg, 0, p.y);
			nvgClosePath(args.vg);
		}
		nvgStroke(args.vg);

		// Draw indicator
		nvgFillColor(args.vg, nvgRGBA(0xff, 0xff, 0xff, 0x60));
		{
			nvgBeginPath(args.vg);
			nvgMoveTo(args.vg, p.x - 2, p.y - 4);
			nvgLineTo(args.vg, p.x - 9, p.y - 4);
			nvgLineTo(args.vg, p.x - 13, p.y);
			nvgLineTo(args.vg, p.x - 9, p.y + 4);
			nvgLineTo(args.vg, p.x - 2, p.y + 4);
			nvgClosePath(args.vg);
		}
		nvgFill(args.vg);

		nvgFontSize(args.vg, 8);
		nvgFontFaceId(args.vg, font->handle);
		nvgFillColor(args.vg, nvgRGBA(0x1e, 0x28, 0x2b, 0xff));
		nvgText(args.vg, p.x - 8, p.y + 3, "T", NULL);
		nvgResetScissor(args.vg);
	}*/

    void drawStats(NVGcontext *args
    .vg,
    Vec pos,
    const char *title, Stats
    *stats) {
        nvgFontSize(args.vg, 12);
        nvgFontFaceId(args.vg, font->handle);
        nvgTextLetterSpacing(args.vg, 0);

        nvgFillColor(args.vg, nvgRGBA(0xff, 0xff, 0xff, 0xff));
        nvgText(args.vg, pos.x + 5, pos.y + 10, title, NULL);

        nvgFillColor(args.vg, nvgRGBA(0x0f, 0xff, 0xff, 0xA8));
        char text[128];
        snprintf(text, sizeof(text), /*"F0 %5.2f  */"PeakX %5.1f  PeakY % 5.1f", /*stats->f0, */stats->peakx, stats->peaky);
        //printf("%s\n", text);
        nvgText(args.vg, pos.x + 27, pos.y + 10, text, NULL);
    }


    void draw(const DrawArgs &args) override {
        float gain1 = powf(2.0, roundf(module->params[Speck::SCALE_1_PARAM].getValue())) / 12.0;
        float gain2 = powf(2.0, roundf(module->params[Speck::SCALE_2_PARAM].getValue())) / 12.0;
        float pos1 = module->params[Speck::POS_1_PARAM].getValue();
        float pos2 = module->params[Speck::POS_2_PARAM].getValue();
        float zoom = module->params[Speck::ZOOM_PARAM].getValue();
        float freqOffs = module->params[Speck::FOFFS_PARAM].getValue();
        float negOffs;

        // Draw waveforms
        // Y
        if (module->inputs[Speck::INPUT_2].isConnected()) {
            nvgStrokeColor(args.vg, nvgRGBAf(0.9f, 0.3f, 0.1, 1.f));
            //drawWaveform(args.vg, module->buffer2, gain2, pos2);
            negOffs = drawWaveform(args.vg, module->FFT2, gain2, pos2, zoom, freqOffs, module->linLog);
        }

        // X
        if (module->inputs[Speck::INPUT_1].isConnected()) {
            nvgStrokeColor(args.vg, nvgRGBAf(0.f, 0.3f, 0.8, 1.f));
            //drawWaveform(args.vg, module->buffer1, gain1, pos1);
            negOffs = drawWaveform(args.vg, module->FFT1, gain1, pos1, zoom, freqOffs, module->linLog);
        }
        //drawTrig(args.vg, module->params[Speck::TRIG_PARAM], gain1, pos1);

        // Calculate and draw stats
        if (++frame >= 4) {
            frame = 0;
            stats1.calculate(module->FFT1);
            stats2.calculate(module->FFT2);
        }

        drawStats(args.vg, Vec(0, 4), "IN1", &stats1);
        drawStats(args.vg, Vec(0, box.size.y - 15), "IN2", &stats2);
        drawGrid(args.vg, zoom, freqOffs, module->linLog, negOffs);
    }
};


struct SpeckWidget : LRModuleWidget {
    SpeckWidget(Speck *module);
};


SpeckWidget::SpeckWidget(Speck *module) : LRModuleWidget(module) {
    panel->addSVGVariant(LRGestaltType::DARK, APP->window->loadSvg(asset::plugin(pluginInstance, "res/panels/SpeckAnalyzer.svg")));
    // panel->addSVGVariant(APP->window->loadSvg(asset::plugin(plugin, "res/panels/SpeckAnalyzer.svg")));
    // panel->addSVGVariant(APP->window->loadSvg(asset::plugin(plugin, "res/panels/SpeckAnalyzer.svg")));

    noVariants = true;
    panel->init();
    addChild(panel);
    box.size = panel->box.size;


    addChild(createWidget<ScrewLight>(Vec(15, 1)));
    addChild(createWidget<ScrewLight>(Vec(300 - 30, 1)));
    addChild(createWidget<ScrewLight>(Vec(15, 364)));
    addChild(createWidget<ScrewLight>(Vec(300 - 30, 364)));
    addChild(createWidget<ScrewLight>(Vec(box.size.x - 30, 1)));
    addChild(createWidget<ScrewLight>(Vec(box.size.x - 30, 366)));

    SpeckDisplay *display = new SpeckDisplay();
    display->module = module;
    display->box.pos = Vec(300, 0);
    display->box.size = Vec(box.size.x - 300, 380);
    addChild(display);

    addParam(ParamcreateWidget<LRSwitch>(Vec(33, 340), module, Speck::ONOFF_PARAM, 0.0, 1.0, 0.0));


    addParam(ParamcreateWidget<LRAlternateMiddleLight>(Vec(118 - 50, 124), module, Speck::SCALE_1_PARAM, -10.0f, 20.0, -1.0f));
    addParam(ParamcreateWidget<LRAlternateMiddleLight>(Vec(118 - 50, 177), module, Speck::POS_1_PARAM, -1.0f, 1.0, 0.0));

    addParam(ParamcreateWidget<LRAlternateMiddleLight>(Vec(197 - 50, 124), module, Speck::SCALE_2_PARAM, -10.0f, 20.0, -1.0f));
    addParam(ParamcreateWidget<LRAlternateMiddleLight>(Vec(197 - 50, 177), module, Speck::POS_2_PARAM, -1.0f, 1.0, 0.0));

    addParam(ParamcreateWidget<LRAlternateMiddleLight>(Vec(253 - 50, 124), module, Speck::ZOOM_PARAM, 1.0, ZOOM_RANGE, 1.0));

    addParam(ParamcreateWidget<LRAlternateMiddleLight>(Vec(253 - 50, 177), module, Speck::FOFFS_PARAM, 0.0, FOFFS_RANGE, 0.0));

    addParam(ParamcreateWidget<LRSwitch>(Vec(258, 244), module, Speck::LINLOG_PARAM, 0.0, 1.0, 0.0));


    addInput(createInput<LRIOPortBLight>(Vec(12, 240), module, Speck::INPUT_1));
    addInput(createInput<LRIOPortBLight>(Vec(59, 240), module, Speck::INPUT_2));

    addOutput(createOutput<LRIOPortBLight>(Vec(9, 306), module, Speck::OUTPUT_1));
    addOutput(createOutput<LRIOPortBLight>(Vec(56, 306), module, Speck::OUTPUT_2));

    addChild(createLight<MediumLight<GreenLight>>(Vec(286, 230), module, Speck::LIGHTS_0_LIN));
    addChild(createLight<MediumLight<GreenLight>>(Vec(286, 280), module, Speck::LIGHTS_1_LOG));
    addChild(createLight<MediumLight<GreenLight>>(Vec(265, 30), module, Speck::LIGHTS_2_ON));

}


Model *modelSpeck = createModel<Speck, SpeckWidget>("Lindenberg Research", "Speck", "Spectrum Analyzer", VISUAL_TAG, UTILITY_TAG);
