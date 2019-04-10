/*                                                                     *\
**       __   ___  ______                                              **
**      / /  / _ \/_  __/                                              **
**     / /__/ , _/ / /    Lindenberg                                   **
**    /____/_/|_| /_/  Research Tec.                                   **
**                                                                     **
**                                                                     **
**	  https://github.com/lindenbergresearch/LRTRack	                   **
**    heapdump@icloud.com                                              **
**		                                                               **
**    Sound Modules for VCV Rack                                       **
**    Copyright 2017/2018 by Patrick Lindenberg / LRT                  **
**                                                                     **
**    For Redistribution and use in source and binary forms,           **
**    with or without modification please see LICENSE.                 **
**                                                                     **
\*                                                                     */

#include "../LindenbergResearch.hpp"
#include "../LRModel.hpp"
#include "../dsp/Type35Filter.hpp"


using namespace rack;
using namespace lrt;

using dsp::Type35Filter;


struct Type35 : LRModule {
    enum ParamIds {
        FREQ1_PARAM,
        PEAK1_PARAM,
        FREQ2_PARAM,
        PEAK2_PARAM,
        DRIVE_PARAM,
        CUTOFF1_CV_PARAM,
        PEAK1_CV_PARAM,
        CUTOFF2_CV_PARAM,
        PEAK2_CV_PARAM,
        DRIVE_CV_PARAM,
        MODE_SWITCH_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        FILTER_INPUT,
        CUTOFF1_CV_INPUT,
        PEAK1_CV_INPUT,
        CUTOFF2_CV_INPUT,
        PEAK2_CV_INPUT,
        DRIVE_CV_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    LRKnob *frqKnobLP, *peakKnobLP, *frqKnobHP, *peakKnobHP, *driveKnob;
    Type35Filter *lpf = new Type35Filter(engineGetSampleRate(), Type35Filter::LPF);
    Type35Filter *hpf = new Type35Filter(engineGetSampleRate(), Type35Filter::HPF);


    Type35() : LRModule(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}


    void step() override {
        // compute all cv values
        float frq1cv = inputs[CUTOFF1_CV_INPUT].value * 0.1f * quadraticBipolar(params[CUTOFF1_CV_PARAM].value);
        float peak1cv = inputs[PEAK1_CV_INPUT].value * 0.1f * quadraticBipolar(params[PEAK1_CV_PARAM].value);

        float frq2cv = inputs[CUTOFF2_CV_INPUT].value * 0.1f * quadraticBipolar(params[CUTOFF2_CV_PARAM].value);
        float peak2cv = inputs[PEAK2_CV_INPUT].value * 0.1f * quadraticBipolar(params[PEAK2_CV_PARAM].value);

        float drivecv = inputs[DRIVE_CV_INPUT].value * 0.1f * quadraticBipolar(params[DRIVE_CV_PARAM].value);

        // set vc parameter and knob values
        lpf->fc = params[FREQ1_PARAM].value + frq1cv;
        lpf->peak = params[PEAK1_PARAM].value + peak1cv;
        hpf->fc = params[FREQ2_PARAM].value + frq2cv;
        hpf->peak = params[PEAK2_PARAM].value + peak2cv;

        lpf->sat = params[DRIVE_PARAM].value + drivecv;
        hpf->sat = params[DRIVE_PARAM].value + drivecv;


        if (frqKnobLP != nullptr && frqKnobHP != nullptr && peakKnobLP != nullptr && peakKnobHP != nullptr && driveKnob != nullptr) {
            frqKnobLP->setIndicatorActive(inputs[CUTOFF1_CV_INPUT].active);
            peakKnobLP->setIndicatorActive(inputs[PEAK1_CV_INPUT].active);
            frqKnobHP->setIndicatorActive(inputs[CUTOFF2_CV_INPUT].active);
            peakKnobHP->setIndicatorActive(inputs[PEAK2_CV_INPUT].active);
            driveKnob->setIndicatorActive(inputs[DRIVE_CV_INPUT].active);

            frqKnobLP->setIndicatorValue(params[FREQ1_PARAM].value + frq1cv);
            peakKnobLP->setIndicatorValue(params[PEAK1_PARAM].value + peak1cv);
            frqKnobHP->setIndicatorValue(params[FREQ2_PARAM].value + frq2cv);
            peakKnobHP->setIndicatorValue(params[PEAK2_PARAM].value + peak2cv);
            driveKnob->setIndicatorValue(params[DRIVE_PARAM].value + drivecv);
        }

        if (params[MODE_SWITCH_PARAM].value == 1) {
            hpf->in = inputs[FILTER_INPUT].value;
            hpf->invalidate();
            hpf->process2();

            // cascade
            lpf->in = hpf->out;//inputs[FILTER_INPUT].value;
            lpf->invalidate();
            lpf->process2();

            outputs[OUTPUT].value = lpf->out;
        } else if (params[MODE_SWITCH_PARAM].value == 2) {
            lpf->in = inputs[FILTER_INPUT].value;
            lpf->invalidate();
            lpf->process2();

            outputs[OUTPUT].value = lpf->out;
        } else if (params[MODE_SWITCH_PARAM].value == 3) {
            lpf->in = inputs[FILTER_INPUT].value;
            lpf->invalidate();
            lpf->process2();

            // cascade
            hpf->in = inputs[FILTER_INPUT].value;//inputs[FILTER_INPUT].value;
            hpf->invalidate();
            hpf->process2();

            outputs[OUTPUT].value = hpf->out + lpf->out;
        } else if (params[MODE_SWITCH_PARAM].value == 4) {
            hpf->in = inputs[FILTER_INPUT].value;//inputs[FILTER_INPUT].value;
            hpf->invalidate();
            hpf->process2();

            outputs[OUTPUT].value = hpf->out;
        } else if (params[MODE_SWITCH_PARAM].value == 5) {
            lpf->in = inputs[FILTER_INPUT].value;
            lpf->invalidate();
            lpf->process2();

            // cascade
            hpf->in = lpf->out;//inputs[FILTER_INPUT].value;
            hpf->invalidate();
            hpf->process2();

            outputs[OUTPUT].value = hpf->out;
        }


    }


    void onSampleRateChange() override {
        Module::onSampleRateChange();
        lpf->setSamplerate(engineGetSampleRate());
        hpf->setSamplerate(engineGetSampleRate());
    }
};


/**
 * @brief Blank Panel with Logo
 */
struct Type35Widget : LRModuleWidget {
    Type35Widget(Type35 *module);
};


Type35Widget::Type35Widget(Type35 *module) : LRModuleWidget(module) {
    panel->addSVGVariant(LRGestalt::DARK, SVG::load(assetPlugin(plugin, "res/panels/Type35VCF.svg")));
    panel->addSVGVariant(LRGestalt::LIGHT, SVG::load(assetPlugin(plugin, "res/panels/Type35VCF.svg")));
    panel->addSVGVariant(LRGestalt::AGED, SVG::load(assetPlugin(plugin, "res/panels/Type35VCF.svg")));

    panel->init();
    addChild(panel);

    box.size = panel->box.size;

    // ***** SCREWS **********
    addChild(Widget::create<ScrewLight>(Vec(15, 1)));
    addChild(Widget::create<ScrewLight>(Vec(box.size.x - 30, 1)));
    addChild(Widget::create<ScrewLight>(Vec(15, 366)));
    addChild(Widget::create<ScrewLight>(Vec(box.size.x - 30, 366)));
    // ***** SCREWS **********

    // ***** MAIN KNOBS ******
    module->frqKnobLP = LRKnob::create<LRBigKnob>(Vec(32.9, 68.6), module, Type35::FREQ1_PARAM, 0.f, 1.f, 1.f);
    module->peakKnobLP = LRKnob::create<LRMiddleKnob>(Vec(39.9, 174.1), module, Type35::PEAK1_PARAM, 0.f, 1.f, 0.f);

    module->frqKnobHP = LRKnob::create<LRBigKnob>(Vec(196.2, 68.6), module, Type35::FREQ2_PARAM, 0.f, 1.f, 0.f);
    module->peakKnobHP = LRKnob::create<LRMiddleKnob>(Vec(203.1, 174.1), module, Type35::PEAK2_PARAM, 0.f, 1.f, 0.f);

    module->driveKnob = LRKnob::create<LRMiddleKnob>(Vec(122, 149.2), module, Type35::DRIVE_PARAM, 1.f, 2.5, 1.0f);

    module->frqKnobLP->setIndicatorColors(nvgRGBAf(0.9f, 0.9f, 0.9f, 1.0f));
    module->peakKnobLP->setIndicatorColors(nvgRGBAf(0.9f, 0.9f, 0.9f, 1.0f));

    module->frqKnobHP->setIndicatorColors(nvgRGBAf(0.9f, 0.9f, 0.9f, 1.0f));
    module->peakKnobHP->setIndicatorColors(nvgRGBAf(0.9f, 0.9f, 0.9f, 1.0f));

    module->driveKnob->setIndicatorColors(nvgRGBAf(0.9f, 0.9f, 0.9f, 1.0f));

    addParam(module->frqKnobLP);
    addParam(module->peakKnobLP);

    addParam(module->frqKnobHP);
    addParam(module->peakKnobHP);

    addParam(module->driveKnob);

    addParam(ParamWidget::create<LRSmallKnob>(Vec(36.5 - 7.5, 269.4), module, Type35::CUTOFF1_CV_PARAM, -1.f, 1.0f, 0.f));
    addParam(ParamWidget::create<LRSmallKnob>(Vec(78.5 - 7.5, 269.4), module, Type35::PEAK1_CV_PARAM, -1.f, 1.0f, 0.f));

    addParam(ParamWidget::create<LRSmallKnob>(Vec(197.5 - 7.5, 269.4), module, Type35::CUTOFF2_CV_PARAM, -1.f, 1.0f, 0.f));
    addParam(ParamWidget::create<LRSmallKnob>(Vec(239.5 - 7.5, 269.4), module, Type35::PEAK2_CV_PARAM, -1.f, 1.0f, 0.f));

    addParam(ParamWidget::create<LRSmallKnob>(Vec(130.7, 269.4), module, Type35::DRIVE_CV_PARAM, -1.f, 1.0f, 0.f));

    addParam(ParamWidget::create<LRSmallToggleKnob>(Vec(130.7, 88.6), module, Type35::MODE_SWITCH_PARAM, 1.f, 5.0f, 1.f));


    addInput(Port::create<LRIOPortCV>(Vec(34.4 - 7.5, 312), Port::INPUT, module, Type35::CUTOFF1_CV_INPUT));
    addInput(Port::create<LRIOPortCV>(Vec(76.4 - 7.5, 312), Port::INPUT, module, Type35::PEAK1_CV_INPUT));

    addInput(Port::create<LRIOPortCV>(Vec(195.4 - 7.5, 312), Port::INPUT, module, Type35::CUTOFF2_CV_INPUT));
    addInput(Port::create<LRIOPortCV>(Vec(237.4 - 7.5, 312), Port::INPUT, module, Type35::PEAK2_CV_INPUT));

    addInput(Port::create<LRIOPortCV>(Vec(136.4 - 7.5, 229), Port::INPUT, module, Type35::DRIVE_CV_INPUT));

    // ***** INPUTS **********
    addInput(Port::create<LRIOPortAudio>(Vec(118 - 8, 312), Port::INPUT, module, Type35::FILTER_INPUT));
    // ***** INPUTS **********

    // ***** OUTPUTS *********
    addOutput(Port::create<LRIOPortAudio>(Vec(156 - 8, 312), Port::OUTPUT, module, Type35::OUTPUT));
    // ***** OUTPUTS *********

    // addParam(ParamWidget::create<LRSwitch>(Vec(135, 55), module, Type35::MODE_SWITCH_PARAM, 0, 1, 0));
}


Model *modelType35 = Model::create<Type35, Type35Widget>("Lindenberg Research", "TYPE35 VCF", "Sallen-Key Type 35 Dual Filter", FILTER_TAG);
